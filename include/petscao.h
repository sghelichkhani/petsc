/* $Id: petscao.h,v 1.25 2001/01/15 21:48:40 bsmith Exp bsmith $ */

/* 
  An application ordering is mapping between an application-centric
  ordering (the ordering that is "natural" for the application) and 
  the parallel ordering that PETSc uses.
*/
#if !defined(__PETSCAO_H)
#define __PETSCAO_H
#include "petscis.h"
#include "petscmat.h"

typedef enum {AO_BASIC=0,AO_ADVANCED=1} AOType;

#define AO_COOKIE PETSC_COOKIE+20

/*S
     AO - Abstract PETSc object that manages mapping between different global numbering

   Level: intermediate

  Concepts: global numbering

.seealso:  AOCreateBasic(), AOCreateBasicIS(), AOPetscToApplication(), AOView()
S*/
typedef struct _p_AO* AO;

EXTERN int AOCreateBasic(MPI_Comm,int,int*,int*,AO*);
EXTERN int AOCreateBasicIS(IS,IS,AO*);

EXTERN int AOPetscToApplication(AO,int,int*);
EXTERN int AOApplicationToPetsc(AO,int,int*);
EXTERN int AOPetscToApplicationIS(AO,IS);
EXTERN int AOApplicationToPetscIS(AO,IS);

EXTERN int AODestroy(AO);
EXTERN int AOView(AO,PetscViewer);

/* ----------------------------------------------------*/

typedef enum {AODATA_BASIC=0,AODATA_ADVANCED=1} AODataType;

#define AODATA_COOKIE PETSC_COOKIE+24

/*S
     AOData - Abstract PETSc object that manages complex parallel data structures intended to 
         hold grid information, etc

   Level: advanced

.seealso:  AODataCreateBasic()
S*/
typedef struct _p_AOData* AOData;

EXTERN int AODataCreateBasic(MPI_Comm,AOData *);
EXTERN int AODataView(AOData,PetscViewer);
EXTERN int AODataDestroy(AOData);
EXTERN int AODataLoadBasic(PetscViewer,AOData *);
EXTERN int AODataGetInfo(AOData,int*,char ***);

EXTERN int AODataKeyAdd(AOData,char*,int,int);
EXTERN int AODataKeyRemove(AOData,char*);

EXTERN int AODataKeySetLocalToGlobalMapping(AOData,char*,ISLocalToGlobalMapping);
EXTERN int AODataKeyGetLocalToGlobalMapping(AOData,char*,ISLocalToGlobalMapping*);
EXTERN int AODataKeyRemap(AOData,char *,AO);

EXTERN int AODataKeyExists(AOData,char*,PetscTruth*);
EXTERN int AODataKeyGetInfo(AOData,char *,int *,int*,int*,char***);
EXTERN int AODataKeyGetOwnershipRange(AOData,char *,int *,int*);

EXTERN int AODataKeyGetNeighbors(AOData,char *,int,int*,IS *);
EXTERN int AODataKeyGetNeighborsIS(AOData,char *,IS,IS *);
EXTERN int AODataKeyGetAdjacency(AOData,char *,Mat*);

EXTERN int AODataKeyGetActive(AOData,char*,char*,int,int *,int,IS*);
EXTERN int AODataKeyGetActiveIS(AOData,char*,char*,IS,int,IS*);
EXTERN int AODataKeyGetActiveLocal(AOData,char*,char*,int,int *,int,IS*);
EXTERN int AODataKeyGetActiveLocalIS(AOData,char*,char*,IS,int,IS*);

EXTERN int AODataKeyPartition(AOData,char *);

EXTERN int AODataSegmentAdd(AOData,char*,char *,int,int,int *,void *,PetscDataType);
EXTERN int AODataSegmentRemove(AOData,char *,char *);
EXTERN int AODataSegmentAddIS(AOData,char*,char *,int,IS,void *,PetscDataType);

EXTERN int AODataSegmentExists(AOData,char*,char*,PetscTruth*);
EXTERN int AODataSegmentGetInfo(AOData,char *,char *,int *,PetscDataType*);

EXTERN int AODataSegmentGet(AOData,char *,char *,int,int*,void **);
EXTERN int AODataSegmentRestore(AOData,char *,char *,int,int*,void **);
EXTERN int AODataSegmentGetIS(AOData,char *,char *,IS,void **);
EXTERN int AODataSegmentRestoreIS(AOData,char *,char *,IS,void **);

EXTERN int AODataSegmentGetLocal(AOData,char *,char *,int,int*,void **);
EXTERN int AODataSegmentRestoreLocal(AOData,char *,char *,int,int*,void **);
EXTERN int AODataSegmentGetLocalIS(AOData,char *,char *,IS,void **);
EXTERN int AODataSegmentRestoreLocalIS(AOData,char *,char *,IS,void **);

EXTERN int AODataSegmentGetReduced(AOData,char *,char *,int,int*,IS *);
EXTERN int AODataSegmentGetReducedIS(AOData,char *,char *,IS,IS *);
EXTERN int AODataSegmentGetExtrema(AOData,char*,char*,void *,void *);

EXTERN int AODataSegmentPartition(AOData,char *,char *);

EXTERN int AODataPartitionAndSetupLocal(AOData,char*,char*,IS*,IS*,ISLocalToGlobalMapping*);
EXTERN int AODataAliasAdd(AOData,char *,char *);

   
typedef struct _p_AOData2dGrid *AOData2dGrid;
EXTERN int AOData2dGridAddNode(AOData2dGrid, double, double, int *);
EXTERN int AOData2dGridInput(AOData2dGrid,PetscDraw);
EXTERN int AOData2dGridFlipCells(AOData2dGrid);
EXTERN int AOData2dGridComputeNeighbors(AOData2dGrid);
EXTERN int AOData2dGridComputeVertexBoundary(AOData2dGrid);
EXTERN int AOData2dGridDraw(AOData2dGrid,PetscDraw);
EXTERN int AOData2dGridDestroy(AOData2dGrid);
EXTERN int AOData2dGridCreate(AOData2dGrid*);
EXTERN int AOData2dGridToAOData(AOData2dGrid,AOData*);

#endif


