#ifndef lint
static char vcid[] = "$Id: umtr.c,v 1.15 1995/09/06 14:31:53 curfman Exp bsmith $";
#endif

#include <math.h>
#include "umtr.h"
#include "kspimpl.h"
#include "qcg.h"
#include "pinclude/pviewer.h"

/*
    NLM_NTR1 - Implements Newton's Method with a trust region approach 
    for solving unconstrained minimization problems.  

    The basic algorithm is taken from MINPACK-2 (dstrn).

    NLM_NTR1 computes a local minimizer of a twice differentiable function
    f  by applying a trust region variant of Newton's method.  At each stage 
    of the algorithm, we us the prconditioned conjugate gradient method to
    determine an approximate minimizer of the quadratic equation

	     q(s) = g^T * s + .5 * s^T * H * s

    subject to the Euclidean norm trust region constraint

	     || L^T * s || <= delta,

    where delta is the trust region radius.  

    Note:
    The method NLM_NTR1 currently uses only ICCG (Incomplete Cholesky
    Conjugate Gradient) to find an approximate minimizer of the 
    resulting quadratic.  Eventually, we will generalize the solver.
*/
static int SNESSolve_UMTR(SNES snes,int *outits)
{
  SNES_UMTR    *neP = (SNES_UMTR *) snes->data;
  int          maxits, i, history_len, nlconv, ierr, qits, newton;
  double       *gnorm, xnorm, max_val, *history, ftrial, delta;
  double       zero = 0.0, *f, two = 2.0, four = 4.0;
  Scalar       one = 1.0;
  Vec          X, G, Y, S, Xtrial;
  MatStructure flg = ALLMAT_DIFFERENT_NONZERO_PATTERN;
  SLES         sles;
  KSP          ksp;
  KSP_QCG      *qcgP;

  nlconv        = 0;
  history	= snes->conv_hist;       /* convergence history */
  history_len	= snes->conv_hist_len;   /* convergence history length */
  maxits	= snes->max_its;         /* maximum number of iterations */
  X		= snes->vec_sol; 	 /* solution vector */
  G		= snes->vec_func;	 /* gradient vector */
  S		= snes->work[0]; 	 /* work vectors */
  Xtrial	= snes->work[1]; 
  Y		= snes->work[2]; 
  delta	        = neP->delta;           /* trust region radius */
  f		= &(snes->fc);		/* function to minimize */
  gnorm		= &(snes->norm);	/* gradient norm */

  ierr = SNESComputeInitialGuess(snes,X); CHKERRQ(ierr);/* X <- X_0 */
  ierr = VecNorm(X,&xnorm); CHKERRQ(ierr);              /* xnorm = || X || */
  ierr = SNESComputeMinimizationFunction(snes,X,f); CHKERRQ(ierr); /* f(X) */
  ierr = SNESComputeGradient(snes,X,G); CHKERRQ(ierr);  /* G(X) <- gradient */
  ierr = VecNorm(G,gnorm); CHKERRQ(ierr);               /* gnorm = || G || */
  if (history && history_len > 0) history[0] = *gnorm;
  if (snes->monitor)
    {(*snes->monitor)(snes,0,*gnorm,snes->monP); CHKERRQ(ierr);}

  ierr = SNESGetSLES(snes,&sles); CHKERRQ(ierr);
  ierr = SLESGetKSP(sles,&ksp); CHKERRQ(ierr);
  ierr = KSPSetMethod(ksp,KSPQCG); CHKERRQ(ierr);
  PLogInfo((PetscObject)snes,"setting KSPMethod = KSPQCG\n");
  qcgP = (KSP_QCG *) ksp->MethodPrivate;

  for ( i=0; i<maxits && !nlconv; i++ ) {
    snes->iter = i+1;
    newton = 0;
    neP->success = 0;
    snes->nfailures = 0;
    ierr = SNESComputeHessian(snes,X,&snes->jacobian,&snes->jacobian_pre,
                                            &flg); CHKERRQ(ierr);
    ierr = SLESSetOperators(snes->sles,snes->jacobian,snes->jacobian_pre,flg);
    CHKERRQ(ierr);

    if (i == 0) {			/* Initialize delta */
      if (delta <= 0) {
        if (xnorm > zero) delta = neP->factor1*xnorm;
        else delta = neP->delta0;
        ierr = MatNorm(snes->jacobian,NORM_1,&max_val);
        if (ierr == PETSC_ERR_SUP) {
          PLogInfo((PetscObject)snes,
            "Initial delta computed without matrix norm information.");
        } else {
          CHKERRQ(ierr);
          if (PETSCABS(max_val) < 1.e-14) 
            SETERRQ(1,"SNESSolve_UMTR:Hessian norm is too small");
          delta = PETSCMAX(delta,*gnorm/max_val);
        }
      } else { 
        delta = neP->delta0;
      }
    }
    do {
      /* Minimize the quadratic to compute the step s */
      qcgP->delta = delta;
      ierr = SLESSolve(snes->sles,G,S,&qits); CHKERRQ(ierr);
      if (qits < 0) SETERRQ(1,"SNESSolve_UMTR:Failure in SLESSolve");
      if (qcgP->info == 3) newton=1;	            /* truncated Newton step */
      PLogInfo((PetscObject)snes,"%d: ltsnrm=%g, delta=%g, q=%g, qits=%d\n", 
        i, qcgP->ltsnrm, delta, qcgP->quadratic, qits );

      ierr = VecWAXPY(&one,X,S,Xtrial); CHKERRQ(ierr); /* Xtrial <- X + S */
      ierr = VecNorm(Xtrial,&xnorm); CHKERRQ(ierr);
                           		               /* ftrial = f(Xtrial) */
      ierr = SNESComputeMinimizationFunction(snes,Xtrial,&ftrial); CHKERRQ(ierr);

      /* Compute the function reduction and the step size */
      neP->actred = *f - ftrial;
      neP->prered = -qcgP->quadratic;

      /* Adjust delta for the first Newton step */
      if ((i == 0) && (newton)) delta = PETSCMIN(delta,qcgP->ltsnrm);

      if (neP->actred < neP->eta1 * neP->prered) {  /* Unsuccessful step */

         PLogInfo((PetscObject)snes,"Rejecting step\n");
         snes->nfailures += 1;

         /* If iterate is Newton step, reduce delta to current step length */
         if (newton) {
           delta = qcgP->ltsnrm;
           newton = 0;
         }
         delta /= four; 

      } else {          /* Successful iteration; adjust trust radius */

        neP->success = 1;
        PLogInfo((PetscObject)snes,"Accepting step\n");
        if (newton) {
           delta = sqrt(qcgP->ltsnrm*delta);
           if (neP->actred < neP->eta2 * neP->prered) delta /= two;
        } else if (neP->actred < neP->eta2 * neP->prered)
           delta /= delta;
        else if ((neP->actred >= neP->eta3 * neP->prered) && 
           (neP->actred < neP->eta4 * neP->prered))
           delta *= two;
        else if (neP->actred >= neP->eta4 * neP->prered)
           delta *= four;
        else neP->sflag = 1;
      }

      neP->delta = delta;
      if ((*snes->converged)(snes,xnorm,*gnorm,ftrial,snes->cnvP)) nlconv = 1;
    } while (!neP->success && !nlconv);

    /* Question:  If (!neP->success && break), then last step was rejected, 
       but convergence was detected.  Should this update really happen? */
    *f = ftrial;
    ierr = VecCopy(Xtrial,X); CHKERRQ(ierr);
    snes->vec_sol_always = X;
    /* Note:  At last iteration, the gradient evaluation is unnecessary */
    ierr = SNESComputeGradient(snes,X,G); CHKERRQ(ierr);
    ierr = VecNorm(G,gnorm); CHKERRQ(ierr);
    if (history && history_len > i+1) history[i+1] = *gnorm;
    snes->vec_func_always = G;

    if (snes->monitor) 
     {ierr = (*snes->monitor)(snes,i+1,*gnorm,snes->monP); CHKERRQ(ierr);}
  }
  /* Verify solution is in corect location */
  if (X != snes->vec_sol) {
    ierr = VecCopy(X,snes->vec_sol); CHKERRQ(ierr);
    snes->vec_sol_always = snes->vec_sol;
    snes->vec_func_always = snes->vec_func; 
  }
  if (i == maxits) {
    PLogInfo((PetscObject)snes,
      "Maximum number of iterations has been reached: %d\n",maxits);
    i--;
  }
  *outits = i+1;
  return 0;
}
/*------------------------------------------------------------*/
static int SNESSetUp_UMTR(SNES snes)
{
  int ierr;
  snes->nwork = 4;
  ierr = VecGetVecs(snes->vec_sol,snes->nwork,&snes->work); CHKERRQ(ierr);
  PLogObjectParents(snes,snes->nwork,snes->work);
  snes->vec_sol_update_always = snes->work[3];
  return 0;
}
/*------------------------------------------------------------*/
static int SNESDestroy_UMTR(PetscObject obj )
{
  SNES snes = (SNES) obj;
  int  ierr;
  ierr = VecFreeVecs(snes->work,snes->nwork); CHKERRQ(ierr);
  PETSCFREE(snes->data);
  return 0;
}
/*------------------------------------------------------------*/
/*@ 
   SNESConverged_UMTR - Default test for monitoring the 
   convergence of the SNESSolve_UMTR() routine. 

   Input Parameters:
.  snes - the SNES context
.  xnorm - 2-norm of current iterate
.  gnorm - 2-norm of current gradient
.  f - objective function value

   Returns:
$  1  if  ( f < fmin ),
$  2  if  ( abs(ared) <= rtol*abs(f) && 
$           pred <= rtol*abs(f) ),
$  3  if  ( delta <= deltatol*xnorm ),
$ -1  if  ( nfuncs > maxfunc ),
$ -2  if  ( abs(ared) <= epsmch && pred <= epsmch ),
$  0  otherwise,

   where
$    ared     - actual reduction
$    delta    - trust region paramenter
$    deltatol - trust region size tolerance,
$    epsmch   - machine epsilon
$    fmin     - lower bound on function value,
$               set with SNESSetMinFunctionTolerance()
$    nfunc    - number of function evaluations
$    maxfunc  - maximum number of function evaluations, 
$               set with SNESSetMaxFunctionEvaluations()
$    pred     - predicted reduction
$    rtol     - relative function tolerance, 
$               set with SNESSetRelativeTolerance()
@*/
int SNESConverged_UMTR(SNES snes,double xnorm,double gnorm,double f,
                       void *dummy)
{
  SNES_UMTR *neP = (SNES_UMTR *) snes->data;
  double    rtol = snes->rtol, delta = neP->delta;
  double    ared = neP->actred, pred = neP->prered;
  double    epsmch = 1.0e-14;   /* This must be fixed */

  if (snes->method_class != SNES_UNCONSTRAINED_MINIMIZATION) SETERRQ(1,
   "SNESConverged_UMTR:For SNES_UNCONSTRAINED_MINIMIZATION only");

  /* Test for successful convergence */
  if ((!neP->success || neP->sflag) && (delta <= snes->deltatol * xnorm)) {
    neP->sflag = 0;
    PLogInfo((PetscObject)snes,
      "SNES: Trust region param satisfies tolerance: %g <= %g * %g\n",
      delta,snes->deltatol,xnorm);  
    return 3;
  }
  if ((fabs(ared) <= fabs(f) * rtol) && (pred) <= rtol*fabs(f)) {
    PLogInfo((PetscObject)snes,
      "SNES: Actual (%g) and predicted (%g) reductions < %g * %g\n",
      fabs(ared),pred,rtol,fabs(f));
    return 2;
  }
  if (f < snes->fmin) {
    PLogInfo((PetscObject)snes,
      "SNES: function value (%g) < f_{minimum} (%g)\n",f,snes->fmin);
    return 1;
  }
  /* Test for termination and stringent tolerances. (failure and stop) */
  if ( (fabs(ared) <= epsmch) && pred <= epsmch ) {
    PLogInfo((PetscObject)snes,
      "SNES: Actual (%g) and predicted (%g) reductions < epsmch (%g)\n",
      fabs(ared),pred,epsmch);
    return -2;
  }
  if (snes->nfuncs > snes->max_funcs) {
    PLogInfo((PetscObject)snes,
      "SNES: Exceeded maximum number of function evaluations: %d > %d\n",
      snes->nfuncs, snes->max_funcs );
    return -1;
  }
  return 0;
}
/*------------------------------------------------------------*/
static int SNESSetFromOptions_UMTR(SNES snes)
{
  SNES_UMTR *ctx = (SNES_UMTR *)snes->data;
  double  tmp;
  if (OptionsGetDouble(snes->prefix,"-eta1",&tmp)) {ctx->eta1 = tmp;}
  if (OptionsGetDouble(snes->prefix,"-eta2",&tmp)) {ctx->eta2 = tmp;}
  if (OptionsGetDouble(snes->prefix,"-eta3",&tmp)) {ctx->eta3 = tmp;}
  if (OptionsGetDouble(snes->prefix,"-eta4",&tmp)) {ctx->eta4 = tmp;}
  if (OptionsGetDouble(snes->prefix,"-delta0",&tmp)) {ctx->delta0 = tmp;}
  if (OptionsGetDouble(snes->prefix,"-factor1",&tmp)) {ctx->factor1 = tmp;}
  return 0;
}
/*------------------------------------------------------------*/
static int SNESPrintHelp_UMTR(SNES snes)
{
  SNES_UMTR *ctx = (SNES_UMTR *)snes->data;
  char      *p;
  if (snes->prefix) p = snes->prefix; else p = "-";
  MPIU_printf(snes->comm," method umtr (unconstrained minimization):\n");
  MPIU_printf(snes->comm,"   %ssnes_trust_region_eta1 eta1 (default %g)\n",p,ctx->eta1);
  MPIU_printf(snes->comm,"   %ssnes_trust_region_eta2 eta2 (default %g)\n",p,ctx->eta2);
  MPIU_printf(snes->comm,"   %ssnes_trust_region_eta3 eta3 (default %g)\n",p,ctx->eta3);
  MPIU_printf(snes->comm,"   %ssnes_trust_region_eta4 eta4 (default %g)\n",p,ctx->eta4);
  MPIU_printf(snes->comm,"   %ssnes_trust_region_delta0 delta0 (default %g)\n",p,ctx->delta0);
  MPIU_printf(snes->comm,"   %ssnes_trust_region_factor1 factor1 (default %g)\n",p,ctx->factor1);
  MPIU_printf(snes->comm,
    "   delta0, factor1: used to initialize trust region parameter\n");
  MPIU_printf(snes->comm,
    "   eta2, eta3, eta4: used to compute trust region parameter\n");
  MPIU_printf(snes->comm,
    "   eta1: step is unsuccessful if actred < eta1 * prered, where\n"); 
  MPIU_printf(snes->comm,
    "         pred = predicted reduction, actred = actual reduction\n");
  return 0;
}
/*------------------------------------------------------------*/
static int SNESView_UMTR(PetscObject obj,Viewer viewer)
{
  SNES      snes = (SNES)obj;
  SNES_UMTR *tr = (SNES_UMTR *)snes->data;
  FILE      *fd;
  int       ierr;

  ierr = ViewerFileGetPointer_Private(viewer,&fd); CHKERRQ(ierr);
  MPIU_fprintf(snes->comm,fd,"    eta1=%g, eta1=%g, eta3=%g, eta4=%g\n",
    tr->eta1,tr->eta2,tr->eta3,tr->eta4);
  MPIU_fprintf(snes->comm,fd,
    "    delta0=%g, factor1=%g\n",tr->delta0,tr->factor1);
  return 0;
}
/*------------------------------------------------------------*/
int SNESCreate_UMTR(SNES snes)
{
  SNES_UMTR *neP;

  if (snes->method_class != SNES_UNCONSTRAINED_MINIMIZATION) SETERRQ(1,
   "SNESCreate_UMTR:For SNES_UNCONSTRAINED_MINIMIZATION only");
  snes->type 		= SNES_UM_NTR;
  snes->setup		= SNESSetUp_UMTR;
  snes->solve		= SNESSolve_UMTR;
  snes->destroy		= SNESDestroy_UMTR;
  snes->converged	= SNESConverged_UMTR;
  snes->monitor         = SNESDefaultMonitor;
  snes->printhelp       = SNESPrintHelp_UMTR;
  snes->setfromoptions  = SNESSetFromOptions_UMTR;
  snes->view            = SNESView_UMTR;

  neP			= PETSCNEW(SNES_UMTR); CHKPTRQ(neP);
  PLogObjectMemory(snes,sizeof(SNES_UMTR));
  snes->data	        = (void *) neP;
  neP->delta0		= 1.0e-6;
  neP->delta 		= 0.0;
  neP->eta1		= 1.0e-4;
  neP->eta2		= 0.25;
  neP->eta3		= 0.50;
  neP->eta4		= 0.90;
  neP->factor1		= 1.0e-8;
  neP->actred		= 0.0;
  neP->prered		= 0.0;
  neP->success		= 0;
  neP->sflag		= 0;
  return 0;
}
