/*$Id: ex2.c,v 1.13 2001/03/23 23:25:13 balay Exp bsmith $*/

static char help[] = "Tests DAGlobalToNaturalAllCreate() using contour plotting for 2d DAs.\n\n";

#include "petscda.h"
#include "petscsys.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  int            i,j,rank,M = 10,N = 8,m = PETSC_DECIDE,n = PETSC_DECIDE,ierr;
  PetscTruth     flg;
  DA             da;
  PetscViewer    viewer;
  Vec            localall,global;
  Scalar         value,*vlocal;
  DAPeriodicType ptype = DA_NONPERIODIC;
  DAStencilType  stype = DA_STENCIL_BOX;
  VecScatter     tolocalall,fromlocalall;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr); 
  ierr = PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",300,0,300,300,&viewer);CHKERRQ(ierr);

  /* Read options */
  ierr = PetscOptionsGetInt(PETSC_NULL,"-M",&M,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-N",&N,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-m",&m,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(PETSC_NULL,"-star_stencil",&flg);CHKERRQ(ierr);
  if (flg) stype = DA_STENCIL_STAR;

  /* Create distributed array and get vectors */
  ierr = DACreate2d(PETSC_COMM_WORLD,ptype,stype,
                    M,N,m,n,1,1,PETSC_NULL,PETSC_NULL,&da);CHKERRQ(ierr);
  ierr = DACreateGlobalVector(da,&global);CHKERRQ(ierr);
  ierr = VecCreateSeq(PETSC_COMM_SELF,M*N,&localall);CHKERRQ(ierr);

  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  value = 5.0*rank;
  ierr = VecSet(&value,global);CHKERRQ(ierr);

  ierr = VecView(global,viewer);CHKERRQ(ierr);

  /*
     Create Scatter from global DA parallel vector to local vector that
   contains all entries
  */
  ierr = DAGlobalToNaturalAllCreate(da,&tolocalall);CHKERRQ(ierr);
  ierr = DANaturalAllToGlobalCreate(da,&fromlocalall);CHKERRQ(ierr);

  ierr = VecScatterBegin(global,localall,INSERT_VALUES,SCATTER_FORWARD,tolocalall);CHKERRQ(ierr);
  ierr = VecScatterEnd(global,localall,INSERT_VALUES,SCATTER_FORWARD,tolocalall);CHKERRQ(ierr);

  ierr = VecGetArray(localall,&vlocal);CHKERRQ(ierr);
  for (j=0; j<N; j++) {
    for (i=0; i<M; i++) {
      *vlocal++ += i + j*M;
    }
  }
  ierr = VecRestoreArray(localall,&vlocal);CHKERRQ(ierr);

  /* scatter back to global vector */
  ierr = VecScatterBegin(localall,global,INSERT_VALUES,SCATTER_FORWARD,fromlocalall);CHKERRQ(ierr);
  ierr = VecScatterEnd(localall,global,INSERT_VALUES,SCATTER_FORWARD,fromlocalall);CHKERRQ(ierr);

  ierr = VecView(global,viewer);CHKERRQ(ierr);

  /* Free memory */
  ierr = VecScatterDestroy(tolocalall);CHKERRQ(ierr);
  ierr = VecScatterDestroy(fromlocalall);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(viewer);CHKERRQ(ierr);
  ierr = VecDestroy(localall);CHKERRQ(ierr);
  ierr = VecDestroy(global);CHKERRQ(ierr);
  ierr = DADestroy(da);CHKERRQ(ierr);
  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 
