/*$Id: viewreg.c,v 1.34 2001/03/23 23:20:05 balay Exp bsmith $*/

#include "src/sys/src/viewer/viewerimpl.h"  /*I "petsc.h" I*/  

PetscFList PetscViewerList              = 0;

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerCreate" 
/*@C
   PetscViewerCreate - Creates a viewing context

   Collective on MPI_Comm

   Input Parameter:
.  comm - MPI communicator

   Output Parameter:
.  inviewer - location to put the PetscViewer context

   Level: advanced

   Concepts: graphics^creating PetscViewer
   Concepts: file input/output^creating PetscViewer
   Concepts: sockets^creating PetscViewer

.seealso: PetscViewerDestroy(), PetscViewerSetType()

@*/
int PetscViewerCreate(MPI_Comm comm,PetscViewer *inviewer)
{
  PetscViewer viewer;

  PetscFunctionBegin;
  *inviewer = 0;
  PetscHeaderCreate(viewer,_p_PetscViewer,struct _PetscViewerOps,PETSC_VIEWER_COOKIE,-1,"PetscViewer",comm,PetscViewerDestroy,0);
  PetscLogObjectCreate(viewer);
  *inviewer           = viewer;
  viewer->type        = -1;
  viewer->data        = 0;
  PetscFunctionReturn(0);
}
 
#undef __FUNCT__  
#define __FUNCT__ "PetscViewerSetType" 
/*@C
   PetscViewerSetType - Builds PetscViewer for a particular implementation.

   Collective on PetscViewer

   Input Parameter:
+  PetscViewer      - the PetscViewer context
-  type        - for example, "ASCII"

   Options Database Command:
.  -draw_type  <type> - Sets the type; use -help for a list 
    of available methods (for instance, ascii)

   Level: advanced

   Notes:  
   See "include/petscviewer.h" for available methods (for instance,
   PETSC_VIEWER_SOCKET)

.seealso: PetscViewerCreate(), PetscViewerGetType()
@*/
int PetscViewerSetType(PetscViewer viewer,PetscViewerType type)
{
  int        ierr,(*r)(PetscViewer);
  PetscTruth match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE);
  PetscValidCharPointer(type);

  ierr = PetscTypeCompare((PetscObject)viewer,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  if (viewer->data) {
    /* destroy the old private PetscViewer context */
    ierr = (*viewer->ops->destroy)(viewer);CHKERRQ(ierr);
    viewer->data      = 0;
  }
  /* Get the function pointers for the graphics method requested */
  if (!PetscViewerList) SETERRQ(1,"No PetscViewer implementations registered");

  ierr =  PetscFListFind(viewer->comm,PetscViewerList,type,(void (**)()) &r);CHKERRQ(ierr);

  if (!r) SETERRQ1(1,"Unknown PetscViewer type given: %s",type);

  viewer->data        = 0;
  ierr = PetscMemzero(viewer->ops,sizeof(struct _PetscViewerOps));CHKERRQ(ierr);
  ierr = (*r)(viewer);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)viewer,type);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerRegisterDestroy" 
/*@C
   PetscViewerRegisterDestroy - Frees the list of PetscViewer methods that were
   registered by PetscViewerRegisterDynamic().

   Not Collective

   Level: developer

.seealso: PetscViewerRegisterDynamic(), PetscViewerRegisterAll()
@*/
int PetscViewerRegisterDestroy(void)
{
  int ierr;

  PetscFunctionBegin;
  if (PetscViewerList) {
    ierr = PetscFListDestroy(&PetscViewerList);CHKERRQ(ierr);
    PetscViewerList = 0;
  }
  PetscFunctionReturn(0);
}

/*MC
   PetscViewerRegisterDynamic - Adds a method to the Krylov subspace solver package.

   Synopsis:
   PetscViewerRegisterDynamic(char *name_solver,char *path,char *name_create,int (*routine_create)(PetscViewer))

   Not Collective

   Input Parameters:
+  name_solver - name of a new user-defined solver
.  path - path (either absolute or relative) the library containing this solver
.  name_create - name of routine to create method context
-  routine_create - routine to create method context

   Level: developer

   Notes:
   PetscViewerRegisterDynamic() may be called multiple times to add several user-defined solvers.

   If dynamic libraries are used, then the fourth input argument (routine_create)
   is ignored.

   Sample usage:
.vb
   PetscViewerRegisterDynamic("my_viewer_type",/home/username/my_lib/lib/libO/solaris/mylib.a,
               "MyViewerCreate",MyViewerCreate);
.ve

   Then, your solver can be chosen with the procedural interface via
$     PetscViewerSetType(ksp,"my_viewer_type")
   or at runtime via the option
$     -viewer_type my_viewer_type

  Concepts: registering^Viewers

.seealso: PetscViewerRegisterAll(), PetscViewerRegisterDestroy()
M*/

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerRegister" 
int PetscViewerRegister(char *sname,char *path,char *name,int (*function)(PetscViewer))
{
  int  ierr;
  char fullname[256];

  PetscFunctionBegin;
  ierr = PetscFListConcat(path,name,fullname);CHKERRQ(ierr);
  ierr = PetscFListAdd(&PetscViewerList,sname,fullname,(void (*)())function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "PetscViewerSetFromOptions" 
/*@C
   PetscViewerSetFromOptions - Sets the graphics type from the options database.
      Defaults to a PETSc X windows graphics.

   Collective on PetscViewer

   Input Parameter:
.     PetscViewer - the graphics context

   Level: intermediate

   Notes: 
    Must be called after PetscViewerCreate() before the PetscViewer is used.

  Concepts: PetscViewer^setting options

.seealso: PetscViewerCreate(), PetscViewerSetType()

@*/
int PetscViewerSetFromOptions(PetscViewer viewer)
{
  int        ierr;
  char       vtype[256];
  PetscTruth flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_COOKIE);

  if (!PetscViewerList) SETERRQ(1,"No PetscViewer implementations registered");
  ierr = PetscOptionsBegin(viewer->comm,viewer->prefix,"PetscViewer options","PetscViewer");CHKERRQ(ierr);
    ierr = PetscOptionsList("-viewer_type","Type of PetscViewer","None",PetscViewerList,(char *)(viewer->type_name?viewer->type_name:PETSC_VIEWER_ASCII),vtype,256,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscViewerSetType(viewer,vtype);CHKERRQ(ierr);
    }
    /* type has not been set? */
    if (!viewer->type_name) {
      ierr = PetscViewerSetType(viewer,PETSC_VIEWER_ASCII);CHKERRQ(ierr);
    }
    if (viewer->ops->setfromoptions) {
      ierr = (*viewer->ops->setfromoptions)(viewer);CHKERRQ(ierr);
    }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
