/*$Id: dm.c,v 1.4 2001/03/23 23:25:14 balay Exp bsmith $*/
 
#include "src/dm/da/daimpl.h"     /*I      "petscda.h"     I*/

/*
   Provides an interface for functionality needed by the DAMG routines.
   Currently this interface is supported by the DA and VecPack objects
  
   Note: this is actually no such thing as a DM object, rather it is 
   the common set of functions shared by DA and VecPack.

*/

#undef __FUNCT__  
#define __FUNCT__ "DMDestroy"
/*@C
    DMDestroy - Destroys a vector packer or DA.

    Collective on DM

    Input Parameter:
.   dm - the DM object to destroy

    Level: developer

.seealso DMView(), DMCreateGlobalVector(), DMGetInterpolation(), DMGetColoring()

@*/
int DMDestroy(DM dm)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*dm->bops->destroy)((PetscObject)dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMView"
/*@C
    DMView - Views a vector packer or DA.

    Collective on DM

    Input Parameter:
+   dm - the DM object to view
-   v - the viewer

    Level: developer

.seealso DMDestroy(), DMCreateGlobalVector(), DMGetInterpolation(), DMGetColoring()

@*/
int DMView(DM dm,PetscViewer v)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*dm->bops->view)((PetscObject)dm,v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMCreateGlobalVector"
/*@C
    DMCreateGlobalVector - Creates a global vector from a DA or VecPack object

    Collective on DM

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   vec - the global vector

    Level: developer

.seealso DMDestroy(), DMView(), DMGetInterpolation(), DMGetColoring()

@*/
int DMCreateGlobalVector(DM dm,Vec *vec)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*dm->ops->createglobalvector)(dm,vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMGetInterpolation"
/*@C
    DMGetInterpolation - Gets interpolation matrix between two DA or VecPack objects

    Collective on DM

    Input Parameter:
+   dm1 - the DM object
-   dm2 - the second, coarser DM object

    Output Parameter:
+  mat - the interpolation
-  vec - the scaling (optional)

    Level: developer

.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMGetColoring()

@*/
int DMGetInterpolation(DM dm1,DM dm2,Mat *mat,Vec *vec)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*dm1->ops->getinterpolation)(dm1,dm2,mat,vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMGetColoring"
/*@C
    DMGetColoring - Gets coloring and empty Jacobian for a DA or VecPack

    Collective on DM

    Input Parameter:
+   dm - the DM object
.   ctype - IS_COLORING_LOCAL or IS_COLORING_GLOBAL
-   mtype - MATMPIAIJ or MATMPIBAIJ

    Output Parameter:
+   coloring - the coloring (optional)
-   mat - the Jacobian (optional)

    Level: developer

.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMGetInterpolation()

@*/
int DMGetColoring(DM dm,ISColoringType ctype,MatType mtype,ISColoring *coloring,Mat *mat)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*dm->ops->getcoloring)(dm,ctype,mtype,coloring,mat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "DMRefine"
/*@C
    DMRefine - Refines a DA or VecPack object

    Collective on DM

    Input Parameter:
+   dm - the DM object
-   comm - the communicator to contain the new DM object (or PETSC_NULL)

    Output Parameter:
.   dmf - the refined DM

    Level: developer

.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMGetInterpolation()

@*/
int DMRefine(DM dm,MPI_Comm comm,DM *dmf)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*dm->ops->refine)(dm,comm,dmf);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

