#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: dpoint.c,v 1.15 1998/04/27 14:40:31 curfman Exp bsmith $";
#endif
/*
       Provides the calling sequences for all the basic Draw routines.
*/
#include "src/draw/drawimpl.h"  /*I "draw.h" I*/

#undef __FUNC__  
#define __FUNC__ "DrawPoint" 
/*@
   DrawPoint - Draws a point onto a drawable.

   Not collective

   Input Parameters:
+  draw - the drawing context
.  xl,yl - the coordinates of the point
-  cl - the color of the point

.keywords:  draw, point
@*/
int DrawPoint(Draw draw,double xl,double yl,int cl)
{
  int ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,DRAW_COOKIE);
  if (PetscTypeCompare(draw->type_name,DRAW_NULL)) PetscFunctionReturn(0);
  ierr = (*draw->ops->point)(draw,xl,yl,cl);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

