/*$Id: ex1.c,v 1.11 2001/03/23 23:21:30 balay Exp bsmith $*/

static char help[] = "Tests repeated VecSetType().\n\n";

#include "petscvec.h"
#include "petscsys.h"

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  int           ierr,n = 5;
  Scalar        one = 1.0,two = 2.0;
  Vec           x,y;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr); 

  /* create vector */
  ierr = VecCreate(PETSC_COMM_SELF,n,PETSC_DECIDE,&x);CHKERRQ(ierr);
  ierr = VecSetType(x,"mpi");CHKERRQ(ierr);
  ierr = VecSetType(x,"seq");CHKERRQ(ierr);
  ierr = VecDuplicate(x,&y);CHKERRQ(ierr);
  ierr = VecSetType(x,"mpi");CHKERRQ(ierr);

  ierr = VecSet(&one,x);CHKERRQ(ierr);
  ierr = VecSet(&two,y);CHKERRQ(ierr);

  ierr = VecDestroy(x);CHKERRQ(ierr);
  ierr = VecDestroy(y);CHKERRQ(ierr);

  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;
}
 
