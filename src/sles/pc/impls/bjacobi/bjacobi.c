#ifndef lint
static char vcid[] = "$Id: bjacobi.c,v 1.44 1995/09/06 03:05:07 bsmith Exp bsmith $";
#endif
/*
   Defines a block Jacobi preconditioner.

   This is far from object oriented. We include the matrix 
  details in the code. This is because otherwise we would have to 
  include knowledge of SLES in the matrix code. In other words,
  it is a loop of objects, not a tree, so inheritence is a bad joke.
*/
#include "src/mat/matimpl.h"
#include "pcimpl.h"
#include "bjacobi.h"
#include "pinclude/pviewer.h"

extern int PCSetUp_BJacobiMPIAIJ(PC);
extern int PCSetUp_BJacobiMPIRow(PC);
extern int PCSetUp_BJacobiMPIBDiag(PC);

static int PCSetUp_BJacobi(PC pc)
{
  Mat        mat = pc->mat, pmat = pc->pmat;
  PC_BJacobi *jac = (PC_BJacobi *) pc->data;

  if (pmat->type == MATMPIAIJ) {
    if (!jac->use_true_local || mat->type == MATMPIAIJ)
      return PCSetUp_BJacobiMPIAIJ(pc);
  } else if (pmat->type == MATMPIROW) {
    if (!jac->use_true_local || mat->type == MATMPIROW)
      return PCSetUp_BJacobiMPIRow(pc);
  } else if (pmat->type == MATMPIBDIAG) {
    if (!jac->use_true_local || mat->type == MATMPIBDIAG)
      return PCSetUp_BJacobiMPIBDiag(pc);
  } 
  SETERRQ(1,"PCSetUp_BJacobi:Cannot use block Jacobi on this matrix type\n");
}

/* Default destroy, if it has never been setup */
static int PCDestroy_BJacobi(PetscObject obj)
{
  PC         pc = (PC) obj;
  PC_BJacobi *jac = (PC_BJacobi *) pc->data;
  PETSCFREE(jac);
  return 0;
}

static int PCSetFromOptions_BJacobi(PC pc)
{
  int        blocks;

  if (OptionsGetInt(pc->prefix,"-pc_bjacobi_blocks",&blocks)) {
    PCBJacobiSetBlocks(pc,blocks);
  }
  if (OptionsHasName(pc->prefix,"-pc_bjacobi_truelocal")) {
    PCBJacobiSetUseTrueLocal(pc);
  }
  return 0;
}

/*@
   PCBJacobiSetUseTrueLocal - Sets a flag to indicate that the block 
   problem is associated with the linear system matrix instead of the
   default (where it is associated with the preconditioning matrix).

   Input Parameters:
.  pc - the preconditioner context

   Options Database Key:
$  -pc_bjacobi_truelocal

   Note:
   For the common case in which the preconditioning and linear 
   system matrices are identical, this routine is unnecessary.

.keywords:  block, Jacobi, set, true, local, flag

.seealso: PCSetOperators(), PCBJacobiSetBlocks()
@*/
int PCBJacobiSetUseTrueLocal(PC pc)
{
  PC_BJacobi   *jac;
  PETSCVALIDHEADERSPECIFIC(pc,PC_COOKIE);
  if (pc->type != PCBJACOBI) return 0;
  jac = (PC_BJacobi *) pc->data;
  jac->use_true_local = 1;
  return 0;
}
/*@C
   PCBJacobiGetSubSLES - Gets the local SLES contexts for all blocks on
   this processor.
   
   Input Parameter:
.  pc - the preconditioner context

   Output Parameters:
.  n_local - the number of blocks on this processor
.  first_local - the global number of the first block on this processor
.  sles - the array of SLES contexts

   Note:  
   Currently only 1 block per processor is supported.
   
   You must call SLESSetUp() before caling PCBJacobiGetSubSLES().

.keywords:  block, Jacobi, get, sub, SLES, context

.seealso: PCBJacobiGetSubSLES()
@*/
int PCBJacobiGetSubSLES(PC pc,int *n_local,int *first_local,SLES **sles)
{
  PC_BJacobi   *jac;
  PETSCVALIDHEADERSPECIFIC(pc,PC_COOKIE);
  if (pc->type != PCBJACOBI) return 0;
  if (!pc->setupcalled) SETERRQ(1,"PCBJacobiGetSubSLES:Must call SLESSetUp first");
  jac = (PC_BJacobi *) pc->data;
  *n_local = jac->n_local;
  *first_local = jac->first_local;
  *sles = jac->sles;
  jac->same_local_solves = 0; /* Assume that local solves are now different;
                                 not necessarily true though!  This flag is 
                                 used only for PCView_BJacobi */
  return 0;
}
  
static int PCPrintHelp_BJacobi(PC pc)
{
  char *p;
  if (pc->prefix) p = pc->prefix; else p = "-";
  MPIU_printf(pc->comm," Options for PCBJACOBI preconditioner:\n");
  MPIU_printf(pc->comm," %spc_bjacobi_blocks blks: blocks in preconditioner\n",p);
  MPIU_printf(pc->comm, " %spc_bjacobi_truelocal: use blocks from the local linear\
 system matrix \n      instead of the preconditioning matrix\n",p);
  MPIU_printf(pc->comm," %ssub : prefix to control options for individual blocks.\
 Add before the \n      usual KSP and PC option names (i.e., -sub_ksp_method\
 <meth>)\n",p);
  return 0;
}

static int PCView_BJacobi(PetscObject obj,Viewer viewer)
{
  PC               pc = (PC)obj;
  FILE             *fd;
  PC_BJacobi       *jac = (PC_BJacobi *) pc->data;
  int              mytid, ierr;
  ierr = ViewerFileGetPointer_Private(viewer,&fd); CHKERRQ(ierr);
  if (jac->use_true_local) 
    MPIU_fprintf(pc->comm,fd,
       "    Block Jacobi: using true local matrix, number of blocks = %d\n",
       jac->n);
  MPIU_fprintf(pc->comm,fd,"    Block Jacobi: number of blocks = %d\n",jac->n);
  MPI_Comm_rank(pc->comm,&mytid);
  if (jac->same_local_solves) {
    MPIU_fprintf(pc->comm,fd,
    "    Local solve is same for all blocks, in the following KSP and PC objects:\n");
    if (!mytid) {
      ierr = SLESView(jac->sles[0],STDOUT_VIEWER_SELF); CHKERRQ(ierr);
    }           /* now only 1 block per proc */
                /* This shouldn't really be STDOUT */
  } else {
    MPIU_fprintf(pc->comm,fd,
     "    Local solve info for each block is in the following KSP and PC objects:\n");
    MPIU_Seq_begin(pc->comm,1);
    fprintf(fd,"Proc %d: number of local blocks = %d, first local block number = %d\n",
    mytid,jac->n_local,jac->first_local);
    ierr = SLESView(jac->sles[0],STDOUT_VIEWER_SELF); CHKERRQ(ierr);
           /* now only 1 block per proc */
           /* This shouldn't really be STDOUT */
    fflush(fd);
    MPIU_Seq_end(pc->comm,1);
  }
  return 0;
}

int PCCreate_BJacobi(PC pc)
{
  int          mytid,numtid;
  PC_BJacobi   *jac = PETSCNEW(PC_BJacobi); CHKPTRQ(jac);

  MPI_Comm_rank(pc->comm,&mytid);
  MPI_Comm_size(pc->comm,&numtid);
  pc->apply              = 0;
  pc->setup              = PCSetUp_BJacobi;
  pc->destroy            = PCDestroy_BJacobi;
  pc->setfrom            = PCSetFromOptions_BJacobi;
  pc->printhelp          = PCPrintHelp_BJacobi;
  pc->view               = PCView_BJacobi;
  pc->type               = PCBJACOBI;
  pc->data               = (void *) jac;
  jac->n                 = numtid;
  jac->n_local           = 1;
  jac->first_local       = mytid;
  jac->sles              = 0;
  jac->use_true_local    = 0;
  jac->same_local_solves = 1;
  return 0;
}

/*@
   PCBJacobiSetBlocks - Sets the number of blocks for the block Jacobi
   preconditioner.

   Input Parameters:
.  pc - the preconditioner context
.  blocks - the number of blocks

   Options Database Key:
$  -pc_bjacobi_blocks  blocks

   Note:  
   Currently only 1 block per processor is supported.

.keywords:  set, number, Jacobi, blocks

.seealso: PCBJacobiSetUseTrueLocal()
@*/
int PCBJacobiSetBlocks(PC pc, int blocks)
{
  PC_BJacobi *jac = (PC_BJacobi *) pc->data; 
  PETSCVALIDHEADERSPECIFIC(pc,PC_COOKIE);
  if (pc->type != PCBJACOBI) return 0;
  jac->n = blocks;
  return 0;
}





