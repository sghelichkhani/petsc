/* $Id: petscda.h,v 1.65 2001/03/28 03:46:37 bsmith Exp bsmith $ */

/*
      Regular array object, for easy parallelism of simple grid 
   problems on regular distributed arrays.
*/
#if !defined(__PETSCDA_H)
#define __PETSCDA_H
#include "petscvec.h"
#include "petscao.h"

#define DA_COOKIE PETSC_COOKIE+14

/*S
     DA - Abstract PETSc object that manages distributed field data for a single structured grid

   Level: beginner

  Concepts: distributed array

.seealso:  DACreate1d(), DACreate2d(), DACreate3d(), DADestroy(), VecScatter
S*/
typedef struct _p_DA* DA;

/*E
    DAStencilType - Determines if the stencil extends only along the coordinate directions, or also
      to the northest, northwest etc

   Level: beginner

.seealso: DACreate1d(), DACreate2d(), DACreate3d(), DA
E*/
typedef enum { DA_STENCIL_STAR,DA_STENCIL_BOX } DAStencilType;

/*E
    DAPeriodicType - Is the domain periodic in one or more directions

   Level: beginner

.seealso: DACreate1d(), DACreate2d(), DACreate3d(), DA
E*/
typedef enum { DA_NONPERIODIC,DA_XPERIODIC,DA_YPERIODIC,DA_XYPERIODIC,
               DA_XYZPERIODIC,DA_XZPERIODIC,DA_YZPERIODIC,DA_ZPERIODIC} 
               DAPeriodicType;

#define DAXPeriodic(pt) ((pt)==DA_XPERIODIC||(pt)==DA_XYPERIODIC||(pt)==DA_XZPERIODIC||(pt)==DA_XYZPERIODIC)
#define DAYPeriodic(pt) ((pt)==DA_YPERIODIC||(pt)==DA_XYPERIODIC||(pt)==DA_YZPERIODIC||(pt)==DA_XYZPERIODIC)
#define DAZPeriodic(pt) ((pt)==DA_ZPERIODIC||(pt)==DA_XZPERIODIC||(pt)==DA_YZPERIODIC||(pt)==DA_XYZPERIODIC)

typedef enum { DA_X,DA_Y,DA_Z } DADirection;

EXTERN int   DACreate1d(MPI_Comm,DAPeriodicType,int,int,int,int*,DA *);
EXTERN int   DACreate2d(MPI_Comm,DAPeriodicType,DAStencilType,int,int,int,int,int,int,int*,int*,DA *);
EXTERN int   DACreate3d(MPI_Comm,DAPeriodicType,DAStencilType,
                        int,int,int,int,int,int,int,int,int *,int *,int *,DA *);
EXTERN int   DADestroy(DA);
EXTERN int   DAView(DA,PetscViewer);

EXTERN int   DAPrintHelp(DA);

EXTERN int   DAGlobalToLocalBegin(DA,Vec,InsertMode,Vec);
EXTERN int   DAGlobalToLocalEnd(DA,Vec,InsertMode,Vec);
EXTERN int   DAGlobalToNaturalBegin(DA,Vec,InsertMode,Vec);
EXTERN int   DAGlobalToNaturalEnd(DA,Vec,InsertMode,Vec);
EXTERN int   DANaturalToGlobalBegin(DA,Vec,InsertMode,Vec);
EXTERN int   DANaturalToGlobalEnd(DA,Vec,InsertMode,Vec);
EXTERN int   DALocalToLocalBegin(DA,Vec,InsertMode,Vec);
EXTERN int   DALocalToLocalEnd(DA,Vec,InsertMode,Vec);
EXTERN int   DALocalToGlobal(DA,Vec,InsertMode,Vec);
EXTERN int   DAGetOwnershipRange(DA,int **,int **,int **);
EXTERN int   DACreateGlobalVector(DA,Vec *);
EXTERN int   DACreateNaturalVector(DA,Vec *);
EXTERN int   DACreateLocalVector(DA,Vec *);
EXTERN int   DAGetLocalVector(DA,Vec *);
EXTERN int   DARestoreLocalVector(DA,Vec *);
EXTERN int   DAGetGlobalVector(DA,Vec *);
EXTERN int   DARestoreGlobalVector(DA,Vec *);
EXTERN int   DALoad(PetscViewer,int,int,int,DA *);
EXTERN int   DAGetCorners(DA,int*,int*,int*,int*,int*,int*);
EXTERN int   DAGetGhostCorners(DA,int*,int*,int*,int*,int*,int*);
EXTERN int   DAGetInfo(DA,int*,int*,int*,int*,int*,int*,int*,int*,int*,DAPeriodicType*,DAStencilType*);
EXTERN int   DAGetProcessorSubset(DA,DADirection,int,MPI_Comm*);
EXTERN int   DARefine(DA,MPI_Comm,DA*);

EXTERN int   DAGlobalToNaturalAllCreate(DA,VecScatter*);
EXTERN int   DANaturalAllToGlobalCreate(DA,VecScatter*);

EXTERN int   DAGetGlobalIndices(DA,int*,int**);
EXTERN int   DAGetISLocalToGlobalMapping(DA,ISLocalToGlobalMapping*);
EXTERN int   DAGetISLocalToGlobalMappingBlck(DA,ISLocalToGlobalMapping*);

EXTERN int   DAGetScatter(DA,VecScatter*,VecScatter*,VecScatter*);

EXTERN int   DAGetAO(DA,AO*);
EXTERN int   DASetCoordinates(DA,Vec); 
EXTERN int   DAGetCoordinates(DA,Vec *);
EXTERN int   DASetUniformCoordinates(DA,double,double,double,double,double,double);
EXTERN int   DASetFieldName(DA,int,const char[]);
EXTERN int   DAGetFieldName(DA,int,char **);

EXTERN int   DAVecGetArray(DA,Vec,void **);
EXTERN int   DAVecRestoreArray(DA,Vec,void **);

EXTERN int   DASplitComm2d(MPI_Comm,int,int,int,MPI_Comm*);

/*S
     DALocalInfo - C struct that contains information about a structured grid and a processors logical
              location in it.

   Level: beginner

  Concepts: distributed array

.seealso:  DACreate1d(), DACreate2d(), DACreate3d(), DADestroy(), DA, DAGetLocalInfo(), DAGetInfo()
S*/
typedef struct {
  int            dim,dof,sw;
  DAPeriodicType pt;
  DAStencilType  st;
  int            mx,my,mz;    /* global number of grid points in each direction */
  int            xs,ys,zs;    /* starting point of this processor */
  int            xm,ym,zm;    /* number of grid points on this processor */
} DALocalInfo;

EXTERN int DAGetLocalInfo(DA,DALocalInfo*);

#include "petscmat.h"
EXTERN int   DAGetColoring(DA,ISColoringType,MatType,ISColoring *,Mat *);
EXTERN int   DAGetInterpolation(DA,DA,Mat*,Vec*);

#include "petscpf.h"
EXTERN int DACreatePF(DA,PF*);

/*S
     VecPack - Abstract PETSc object that manages treating several distinct vectors as if they
        were one.   The VecPack routines allow one to manage a nonlinear solver that works on a
        vector that consists of several distinct parts. This is mostly used for LNKS solvers, 
        that is design optimization problems that are written as a nonlinear system

   Level: beginner

  Concepts: multi-component, LNKS solvers

.seealso:  VecPackCreate(), VecPackDestroy()
S*/
typedef struct _p_VecPack *VecPack;

EXTERN int VecPackCreate(MPI_Comm,VecPack*);
EXTERN int VecPackDestroy(VecPack);
EXTERN int VecPackAddArray(VecPack,int);
EXTERN int VecPackAddDA(VecPack,DA);
EXTERN int VecPackAddVecScatter(VecPack,VecScatter);
EXTERN int VecPackScatter(VecPack,Vec,...);
EXTERN int VecPackGather(VecPack,Vec,...);
EXTERN int VecPackGetAccess(VecPack,Vec,...);
EXTERN int VecPackRestoreAccess(VecPack,Vec,...);
EXTERN int VecPackGetLocalVectors(VecPack,...);
EXTERN int VecPackGetEntries(VecPack,...);
EXTERN int VecPackRestoreLocalVectors(VecPack,...);
EXTERN int VecPackCreateGlobalVector(VecPack,Vec*);
EXTERN int VecPackGetGlobalIndices(VecPack,...);
EXTERN int VecPackRefine(VecPack,MPI_Comm,VecPack*);
EXTERN int VecPackGetInterpolation(VecPack,VecPack,Mat*,Vec*);

#include "petscsnes.h"

/*S
     DM - Abstract PETSc object that manages an abstract grid object
          
   Level: intermediate

  Concepts: grids, grid refinement

   Notes: The DA object and the VecPack object are examples of DMs

.seealso:  VecPackCreate(), DA, VecPack
S*/
typedef struct _p_DM* DM;

EXTERN int DMView(DM,PetscViewer);
EXTERN int DMDestroy(DM);
EXTERN int DMCreateGlobalVector(DM,Vec*);
EXTERN int DMGetColoring(DM,ISColoringType,MatType,ISColoring*,Mat*);
EXTERN int DMGetInterpolation(DM,DM,Mat*,Vec*);
EXTERN int DMRefine(DM,MPI_Comm,DM*);
EXTERN int DMGetInterpolationScale(DM,DM,Mat,Vec*);

/*S
     DMMG -  Data structure to easily manage multi-level non-linear solvers on grids managed by DM
          
   Level: intermediate

  Concepts: multigrid, Newton-multigrid

.seealso:  VecPackCreate(), DA, VecPack, DM, DMMGCreate()
S*/
typedef struct _p_DMMG *DMMG;
struct _p_DMMG {
  DM         dm;                   /* grid information for this level */
  Vec        x,b,r;                /* global vectors used in multigrid preconditioner for this level*/
  Mat        J;                    /* matrix on this level */
  Mat        R;                    /* restriction to next coarser level (not defined on level 0) */
  int        nlevels;              /* number of levels above this one (total number of levels on level 0)*/
  MPI_Comm   comm;
  int        (*solve)(DMMG*,int);
  void       *user;         

  /* SLES only */
  SLES       sles;             
  int        (*rhs)(DMMG,Vec);

  /* SNES only */
  PetscTruth    matrixfree;
  Mat           B;
  Vec           Rscale;                /* scaling to restriction before computing Jacobian */
  int           (*computejacobian)(SNES,Vec,Mat*,Mat*,MatStructure*,void*);  
  int           (*computefunction)(SNES,Vec,Vec,void*);  
  int           (*computefunctionlocal)(Scalar**,Scalar**,DALocalInfo*,void*);  
  int           (*ad_computefunctionlocal)(Scalar**,Scalar**,DALocalInfo*,void*);  
  ISColoring    iscoloring;            /* used with AD or FD coloring for Jacobian */
  MatFDColoring fdcoloring;            /* only used with FD coloring for Jacobian */  
  SNES          snes;                  
  int           (*initialguess)(SNES,Vec,void*);
  Vec           work1,work2;
};

EXTERN int DMMGCreate(MPI_Comm,int,void*,DMMG**);
EXTERN int DMMGDestroy(DMMG*);
EXTERN int DMMGSetUp(DMMG*);
EXTERN int DMMGSetSLES(DMMG*,int (*)(DMMG,Vec),int (*)(DMMG,Mat));
EXTERN int DMMGSetSNES(DMMG*,int (*)(SNES,Vec,Vec,void*),int (*)(SNES,Vec,Mat*,Mat*,MatStructure*,void*));
EXTERN int DMMGSetSNESLocal(DMMG*,int(*)(Scalar**,Scalar**,DALocalInfo*,void*),int(*jacobian)(SNES,Vec,Mat*,Mat*,MatStructure*,void*));
EXTERN int DMMGSetInitialGuess(DMMG*,int (*)(SNES,Vec,void*));
EXTERN int DMMGView(DMMG*,PetscViewer);
EXTERN int DMMGSolve(DMMG*);
EXTERN int DMMGSetUseMatrixFree(DMMG*);
EXTERN int DMMGSetDM(DMMG*,DM);

#define DMMGGetb(ctx)    (ctx)[(ctx)[0]->nlevels-1]->b
#define DMMGGetr(ctx)    (ctx)[(ctx)[0]->nlevels-1]->r
#define DMMGGetx(ctx)    (ctx)[(ctx)[0]->nlevels-1]->x
#define DMMGGetJ(ctx)    (ctx)[(ctx)[0]->nlevels-1]->J
#define DMMGGetB(ctx)    (ctx)[(ctx)[0]->nlevels-1]->B
#define DMMGGetFine(ctx) (ctx)[(ctx)[0]->nlevels-1]
#define DMMGGetSLES(ctx) (ctx)[(ctx)[0]->nlevels-1]->sles
#define DMMGGetSNES(ctx) (ctx)[(ctx)[0]->nlevels-1]->snes
#define DMMGGetDA(ctx)   (DA)((ctx)[(ctx)[0]->nlevels-1]->dm)
#define DMMGGetVecPack(ctx)   (VecPack)((ctx)[(ctx)[0]->nlevels-1]->dm)
#define DMMGGetUser(ctx)      (VecPack)((ctx)[(ctx)[0]->nlevels-1]->user)
#define DMMGGetLevels(ctx)  (ctx)[0]->nlevels

#endif


