#ifdef PETSC_RCS_HEADER
static char vcid[] = "$Id: dlinegw.c,v 1.16 1998/04/27 14:40:31 curfman Exp bsmith $";
#endif
/*
       Provides the calling sequences for all the basic Draw routines.
*/
#include "src/draw/drawimpl.h"  /*I "draw.h" I*/

#undef __FUNC__  
#define __FUNC__ "DrawLineGetWidth" 
/*@
   DrawLineGetWidth - Gets the line width for future draws.  The width is
   relative to the user coordinates of the window; 0.0 denotes the natural
   width; 1.0 denotes the interior viewport. 

   Not collective

   Input Parameter:
.  draw - the drawing context

   Output Parameter:
.  width - the width in user coordinates

   Notes:
   Not currently implemented.

.keywords:  draw, line, get, width

.seealso:  DrawLineSetWidth()
@*/
int DrawLineGetWidth(Draw draw,double *width)
{
  int ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,DRAW_COOKIE);
  if (PetscTypeCompare(draw->type_name,DRAW_NULL)) PetscFunctionReturn(0);
  if (!draw->ops->linegetwidth) SETERRQ(PETSC_ERR_SUP,1,0);
  ierr = (*draw->ops->linegetwidth)(draw,width);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

