#ifndef lint
static char vcid[] = "$Id: aijfact.c,v 1.60 1996/03/23 20:42:31 bsmith Exp bsmith $";
#endif

#include "aij.h"
/*
    Factorization code for AIJ format. 
*/

int MatLUFactorSymbolic_SeqAIJ(Mat A,IS isrow,IS iscol,double f,Mat *B)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data, *b;
  IS         isicol;
  int        *r,*ic, ierr, i, n = a->m, *ai = a->i, *aj = a->j;
  int        *ainew,*ajnew, jmax,*fill, *ajtmp, nz,shift = a->indexshift;
  int        *idnew, idx, row,m,fm, nnz, nzi,len, realloc = 0,nzbd,*im;
 
  if (n != a->n) SETERRQ(1,"MatLUFactorSymbolic_SeqAIJ:Must be square");
  if (!isrow) SETERRQ(1,"MatLUFactorSymbolic_SeqAIJ:Must have row permutation");
  if (!iscol) SETERRQ(1,"MatLUFactorSymbolic_SeqAIJ:Must have col. permutation");

  ierr = ISInvertPermutation(iscol,&isicol); CHKERRQ(ierr);
  ISGetIndices(isrow,&r); ISGetIndices(isicol,&ic);

  /* get new row pointers */
  ainew = (int *) PetscMalloc( (n+1)*sizeof(int) ); CHKPTRQ(ainew);
  ainew[0] = -shift;
  /* don't know how many column pointers are needed so estimate */
  jmax = (int) (f*ai[n]+(!shift));
  ajnew = (int *) PetscMalloc( (jmax)*sizeof(int) ); CHKPTRQ(ajnew);
  /* fill is a linked list of nonzeros in active row */
  fill = (int *) PetscMalloc( (2*n+1)*sizeof(int)); CHKPTRQ(fill);
  im = fill + n + 1;
  /* idnew is location of diagonal in factor */
  idnew = (int *) PetscMalloc( (n+1)*sizeof(int)); CHKPTRQ(idnew);
  idnew[0] = -shift;

  for ( i=0; i<n; i++ ) {
    /* first copy previous fill into linked list */
    nnz     = nz    = ai[r[i]+1] - ai[r[i]];
    ajtmp   = aj + ai[r[i]] + shift;
    fill[n] = n;
    while (nz--) {
      fm  = n;
      idx = ic[*ajtmp++ + shift];
      do {
        m  = fm;
        fm = fill[m];
      } while (fm < idx);
      fill[m]   = idx;
      fill[idx] = fm;
    }
    row = fill[n];
    while ( row < i ) {
      ajtmp = ajnew + idnew[row] + (!shift);
      nzbd  = 1 + idnew[row] - ainew[row];
      nz    = im[row] - nzbd;
      fm    = row;
      while (nz-- > 0) {
        idx = *ajtmp++ + shift;
        nzbd++;
        if (idx == i) im[row] = nzbd;
        do {
          m  = fm;
          fm = fill[m];
        } while (fm < idx);
        if (fm != idx) {
          fill[m]   = idx;
          fill[idx] = fm;
          fm        = idx;
          nnz++;
        }
      }
      row = fill[row];
    }
    /* copy new filled row into permanent storage */
    ainew[i+1] = ainew[i] + nnz;
    if (ainew[i+1] > jmax) {
      /* allocate a longer ajnew */
      int maxadd;
      maxadd = (int) ((f*(ai[n]+(!shift))*(n-i+5))/n);
      if (maxadd < nnz) maxadd = (n-i)*(nnz+1);
      jmax += maxadd;
      ajtmp = (int *) PetscMalloc( jmax*sizeof(int) );CHKPTRQ(ajtmp);
      PetscMemcpy(ajtmp,ajnew,(ainew[i]+shift)*sizeof(int));
      PetscFree(ajnew);
      ajnew = ajtmp;
      realloc++; /* count how many times we realloc */
    }
    ajtmp = ajnew + ainew[i] + shift;
    fm    = fill[n];
    nzi   = 0;
    im[i] = nnz;
    while (nnz--) {
      if (fm < i) nzi++;
      *ajtmp++ = fm - shift;
      fm       = fill[fm];
    }
    idnew[i] = ainew[i] + nzi;
  }

  PLogInfo(A,
    "Info:MatLUFactorSymbolic_SeqAIJ:Reallocs %d Fill ratio:given %g needed %g\n",
                             realloc,f,((double)ainew[n])/((double)ai[i]));

  ierr = ISRestoreIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISRestoreIndices(isicol,&ic); CHKERRQ(ierr);

  PetscFree(fill);

  /* put together the new matrix */
  ierr = MatCreateSeqAIJ(A->comm,n,n,0,PETSC_NULL,B); CHKERRQ(ierr);
  PLogObjectParent(*B,isicol); 
  ierr = ISDestroy(isicol); CHKERRQ(ierr);
  b = (Mat_SeqAIJ *) (*B)->data;
  PetscFree(b->imax);
  b->singlemalloc = 0;
  len             = (ainew[n] + shift)*sizeof(Scalar);
  /* the next line frees the default space generated by the Create() */
  PetscFree(b->a); PetscFree(b->ilen);
  b->a          = (Scalar *) PetscMalloc( len ); CHKPTRQ(b->a);
  b->j          = ajnew;
  b->i          = ainew;
  b->diag       = idnew;
  b->ilen       = 0;
  b->imax       = 0;
  b->row        = isrow;
  b->col        = iscol;
  b->solve_work = (Scalar *) PetscMalloc( n*sizeof(Scalar)); 
  CHKPTRQ(b->solve_work);
  /* In b structure:  Free imax, ilen, old a, old j.  
     Allocate idnew, solve_work, new a, new j */
  PLogObjectMemory(*B,(ainew[n]+shift-n)*(sizeof(int)+sizeof(Scalar)));
  b->maxnz = b->nz = ainew[n] + shift;

  return 0; 
}
/* ----------------------------------------------------------- */
int Mat_AIJ_CheckInode(Mat);

int MatLUFactorNumeric_SeqAIJ(Mat A,Mat *B)
{
  Mat        C = *B;
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data, *b = (Mat_SeqAIJ *)C->data;
  IS         iscol = b->col, isrow = b->row, isicol;
  int        *r,*ic, ierr, i, j, n = a->m, *ai = b->i, *aj = b->j;
  int        *ajtmpold, *ajtmp, nz, row, *ics, shift = a->indexshift;
  int        *diag_offset = b->diag,diag,k;
  int        preserve_row_sums = (int) a->ilu_preserve_row_sums;
  Scalar     *rtmp,*v, *pc, multiplier,sum,inner_sum,*rowsums = 0;
  double     ssum; 
  /* These declarations are for optimizations.  They reduce the number of
     memory references that are made by locally storing information; the
     word "register" used here with pointers can be viewed as "private" or 
     "known only to me"
   */
  register Scalar *pv, *rtmps,*u_values;
  register int    *pj;

  ierr  = ISInvertPermutation(iscol,&isicol); CHKERRQ(ierr);
  PLogObjectParent(*B,isicol);
  ierr  = ISGetIndices(isrow,&r); CHKERRQ(ierr);
  ierr  = ISGetIndices(isicol,&ic); CHKERRQ(ierr);
  rtmp  = (Scalar *) PetscMalloc( (n+1)*sizeof(Scalar) ); CHKPTRQ(rtmp);
  rtmps = rtmp + shift; ics = ic + shift;

  /* precalcuate row sums */
  if (preserve_row_sums) {
    rowsums = (Scalar *) PetscMalloc( n*sizeof(Scalar) ); CHKPTRQ(rowsums);
    for ( i=0; i<n; i++ ) {
      nz  = a->i[r[i]+1] - a->i[r[i]];
      v   = a->a + a->i[r[i]] + shift;
      sum = 0.0;
      for ( j=0; j<nz; j++ ) sum += v[j];
      rowsums[i] = sum;
    }
  }

  for ( i=0; i<n; i++ ) {
    nz    = ai[i+1] - ai[i];
    ajtmp = aj + ai[i] + shift;
    for  ( j=0; j<nz; j++ ) rtmps[ajtmp[j]] = 0.0;

    /* load in initial (unfactored row) */
    nz       = a->i[r[i]+1] - a->i[r[i]];
    ajtmpold = a->j + a->i[r[i]] + shift;
    v        = a->a + a->i[r[i]] + shift;
    for ( j=0; j<nz; j++ ) rtmp[ics[ajtmpold[j]]] =  v[j];

    row = *ajtmp++ + shift;
    while (row < i) {
      pc = rtmp + row;
      if (*pc != 0.0) {
        pv         = b->a + diag_offset[row] + shift;
        pj         = b->j + diag_offset[row] + (!shift);
        multiplier = *pc / *pv++;
        *pc        = multiplier;
        nz         = ai[row+1] - diag_offset[row] - 1;
        for (j=0; j<nz; j++) rtmps[pj[j]] -= multiplier * pv[j];
        PLogFlops(2*nz);
      }
      row = *ajtmp++ + shift;
    }
    /* finished row so stick it into b->a */
    pv = b->a + ai[i] + shift;
    pj = b->j + ai[i] + shift;
    nz = ai[i+1] - ai[i];
    for ( j=0; j<nz; j++ ) {pv[j] = rtmps[pj[j]];}
    diag = diag_offset[i] - ai[i];
    /*
          Possibly adjust diagonal entry on current row to force
        LU matrix to have same row sum as initial matrix. 
    */
    if (preserve_row_sums) {
      pj  = b->j + ai[i] + shift;
      sum = rowsums[i];
      for ( j=0; j<diag; j++ ) {
        u_values  = b->a + diag_offset[pj[j]] + shift;
        nz        = ai[pj[j]+1] - diag_offset[pj[j]];
        inner_sum = 0.0;
        for ( k=0; k<nz; k++ ) {
          inner_sum += u_values[k];
        }
        sum -= pv[j]*inner_sum;

      }
      nz       = ai[i+1] - diag_offset[i] - 1;
      u_values = b->a + diag_offset[i] + 1 + shift;
      for ( k=0; k<nz; k++ ) {
        sum -= u_values[k];
      }
      ssum = PetscAbsScalar(sum/pv[diag]);
      if (ssum < 1000. && ssum > .001) pv[diag] = sum; 
    }
    /* check pivot entry for current row */
    if (pv[diag] == 0.0) {
      SETERRQ(1,"MatLUFactorNumeric_SeqAIJ:Zero pivot");
    }
  }

  /* invert diagonal entries for simplier triangular solves */
  for ( i=0; i<n; i++ ) {
    b->a[diag_offset[i]+shift] = 1.0/b->a[diag_offset[i]+shift];
  }

  if (preserve_row_sums) PetscFree(rowsums); 
  PetscFree(rtmp);
  ierr = ISRestoreIndices(isicol,&ic); CHKERRQ(ierr);
  ierr = ISRestoreIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISDestroy(isicol); CHKERRQ(ierr);
  C->factor = FACTOR_LU;
  ierr = Mat_AIJ_CheckInode(C); CHKERRQ(ierr);
  C->assembled = PETSC_TRUE;
  PLogFlops(b->n);
  return 0;
}
/* ----------------------------------------------------------- */
int MatLUFactor_SeqAIJ(Mat A,IS row,IS col,double f)
{
  Mat_SeqAIJ *mat = (Mat_SeqAIJ *) A->data;
  int        ierr;
  Mat        C;

  ierr = MatLUFactorSymbolic_SeqAIJ(A,row,col,f,&C); CHKERRQ(ierr);
  ierr = MatLUFactorNumeric_SeqAIJ(A,&C); CHKERRQ(ierr);

  /* free all the data structures from mat */
  PetscFree(mat->a); 
  if (!mat->singlemalloc) {PetscFree(mat->i); PetscFree(mat->j);}
  if (mat->diag) PetscFree(mat->diag);
  if (mat->ilen) PetscFree(mat->ilen);
  if (mat->imax) PetscFree(mat->imax);
  if (mat->solve_work) PetscFree(mat->solve_work);
  if (mat->inode.size) PetscFree(mat->inode.size);
  PetscFree(mat);

  PetscMemcpy(A,C,sizeof(struct _Mat));
  PetscHeaderDestroy(C);
  return 0;
}
/* ----------------------------------------------------------- */
int MatSolve_SeqAIJ(Mat A,Vec bb, Vec xx)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data;
  IS         iscol = a->col, isrow = a->row;
  int        *r,*c, ierr, i,  n = a->m, *vi, *ai = a->i, *aj = a->j;
  int        nz,shift = a->indexshift;
  Scalar     *x,*b,*tmp, *tmps, *aa = a->a, sum, *v;

  if (A->factor != FACTOR_LU) SETERRQ(1,"MatSolve_SeqAIJ:Not for unfactored matrix");

  ierr = VecGetArray(bb,&b); CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x); CHKERRQ(ierr);
  tmp  = a->solve_work;

  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);
  ierr = ISGetIndices(iscol,&c);CHKERRQ(ierr); c = c + (n-1);

  /* forward solve the lower triangular */
  tmp[0] = b[*r++];
  tmps   = tmp + shift;
  for ( i=1; i<n; i++ ) {
    v   = aa + ai[i] + shift;
    vi  = aj + ai[i] + shift;
    nz  = a->diag[i] - ai[i];
    sum = b[*r++];
    while (nz--) sum -= *v++ * tmps[*vi++];
    tmp[i] = sum;
  }

  /* backward solve the upper triangular */
  for ( i=n-1; i>=0; i-- ){
    v   = aa + a->diag[i] + (!shift);
    vi  = aj + a->diag[i] + (!shift);
    nz  = ai[i+1] - a->diag[i] - 1;
    sum = tmp[i];
    while (nz--) sum -= *v++ * tmps[*vi++];
    x[*c--] = tmp[i] = sum*aa[a->diag[i]+shift];
  }

  ierr = ISRestoreIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISRestoreIndices(iscol,&c); CHKERRQ(ierr);
  PLogFlops(2*a->nz - a->n);
  return 0;
}
int MatSolveAdd_SeqAIJ(Mat A,Vec bb, Vec yy, Vec xx)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data;
  IS         iscol = a->col, isrow = a->row;
  int        *r,*c, ierr, i,  n = a->m, *vi, *ai = a->i, *aj = a->j;
  int        nz, shift = a->indexshift;
  Scalar     *x,*b,*tmp, *aa = a->a, sum, *v;

  if (A->factor != FACTOR_LU) SETERRQ(1,"MatSolveAdd_SeqAIJ:Not for unfactored matrix");
  if (yy != xx) {ierr = VecCopy(yy,xx); CHKERRQ(ierr);}

  ierr = VecGetArray(bb,&b); CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x); CHKERRQ(ierr);
  tmp  = a->solve_work;

  ierr = ISGetIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISGetIndices(iscol,&c); CHKERRQ(ierr); c = c + (n-1);

  /* forward solve the lower triangular */
  tmp[0] = b[*r++];
  for ( i=1; i<n; i++ ) {
    v   = aa + ai[i] + shift;
    vi  = aj + ai[i] + shift;
    nz  = a->diag[i] - ai[i];
    sum = b[*r++];
    while (nz--) sum -= *v++ * tmp[*vi++ + shift];
    tmp[i] = sum;
  }

  /* backward solve the upper triangular */
  for ( i=n-1; i>=0; i-- ){
    v   = aa + a->diag[i] + (!shift);
    vi  = aj + a->diag[i] + (!shift);
    nz  = ai[i+1] - a->diag[i] - 1;
    sum = tmp[i];
    while (nz--) sum -= *v++ * tmp[*vi++ + shift];
    tmp[i] = sum*aa[a->diag[i]+shift];
    x[*c--] += tmp[i];
  }

  ierr = ISRestoreIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISRestoreIndices(iscol,&c); CHKERRQ(ierr);
  PLogFlops(2*a->nz);

  return 0;
}
/* -------------------------------------------------------------------*/
int MatSolveTrans_SeqAIJ(Mat A,Vec bb, Vec xx)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data;
  IS         iscol = a->col, isrow = a->row, invisrow,inviscol;
  int        *r,*c, ierr, i, n = a->m, *vi, *ai = a->i, *aj = a->j;
  int        nz,shift = a->indexshift;
  Scalar     *x,*b,*tmp, *aa = a->a, *v;

  if (A->factor != FACTOR_LU)  SETERRQ(1,"MatSolveTrans_SeqAIJ:Not unfactored matrix");
  ierr = VecGetArray(bb,&b); CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x); CHKERRQ(ierr);
  tmp  = a->solve_work;

  /* invert the permutations */
  ierr = ISInvertPermutation(isrow,&invisrow); CHKERRQ(ierr);
  ierr = ISInvertPermutation(iscol,&inviscol); CHKERRQ(ierr);

  ierr = ISGetIndices(invisrow,&r); CHKERRQ(ierr);
  ierr = ISGetIndices(inviscol,&c); CHKERRQ(ierr);

  /* copy the b into temp work space according to permutation */
  for ( i=0; i<n; i++ ) tmp[c[i]] = b[i];

  /* forward solve the U^T */
  for ( i=0; i<n; i++ ) {
    v   = aa + a->diag[i] + shift;
    vi  = aj + a->diag[i] + (!shift);
    nz  = ai[i+1] - a->diag[i] - 1;
    tmp[i] *= *v++;
    while (nz--) {
      tmp[*vi++ + shift] -= (*v++)*tmp[i];
    }
  }

  /* backward solve the L^T */
  for ( i=n-1; i>=0; i-- ){
    v   = aa + a->diag[i] - 1 + shift;
    vi  = aj + a->diag[i] - 1 + shift;
    nz  = a->diag[i] - ai[i];
    while (nz--) {
      tmp[*vi-- + shift] -= (*v--)*tmp[i];
    }
  }

  /* copy tmp into x according to permutation */
  for ( i=0; i<n; i++ ) x[r[i]] = tmp[i];

  ierr = ISRestoreIndices(invisrow,&r); CHKERRQ(ierr);
  ierr = ISRestoreIndices(inviscol,&c); CHKERRQ(ierr);
  ierr = ISDestroy(invisrow); CHKERRQ(ierr);
  ierr = ISDestroy(inviscol); CHKERRQ(ierr);

  PLogFlops(2*a->nz-a->n);
  return 0;
}

int MatSolveTransAdd_SeqAIJ(Mat A,Vec bb, Vec zz,Vec xx)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data;
  IS         iscol = a->col, isrow = a->row, invisrow,inviscol;
  int        *r,*c, ierr, i, n = a->m, *vi, *ai = a->i, *aj = a->j;
  int        nz,shift = a->indexshift;
  Scalar     *x,*b,*tmp, *aa = a->a, *v;

  if (A->factor != FACTOR_LU)SETERRQ(1,"MatSolveTransAdd_SeqAIJ:Not unfactored matrix");
  if (zz != xx) VecCopy(zz,xx);

  ierr = VecGetArray(bb,&b); CHKERRQ(ierr);
  ierr = VecGetArray(xx,&x); CHKERRQ(ierr);
  tmp = a->solve_work;

  /* invert the permutations */
  ierr = ISInvertPermutation(isrow,&invisrow); CHKERRQ(ierr);
  ierr = ISInvertPermutation(iscol,&inviscol); CHKERRQ(ierr);
  ierr = ISGetIndices(invisrow,&r); CHKERRQ(ierr);
  ierr = ISGetIndices(inviscol,&c); CHKERRQ(ierr);

  /* copy the b into temp work space according to permutation */
  for ( i=0; i<n; i++ ) tmp[c[i]] = b[i];

  /* forward solve the U^T */
  for ( i=0; i<n; i++ ) {
    v   = aa + a->diag[i] + shift;
    vi  = aj + a->diag[i] + (!shift);
    nz  = ai[i+1] - a->diag[i] - 1;
    tmp[i] *= *v++;
    while (nz--) {
      tmp[*vi++ + shift] -= (*v++)*tmp[i];
    }
  }

  /* backward solve the L^T */
  for ( i=n-1; i>=0; i-- ){
    v   = aa + a->diag[i] - 1 + shift;
    vi  = aj + a->diag[i] - 1 + shift;
    nz  = a->diag[i] - ai[i];
    while (nz--) {
      tmp[*vi-- + shift] -= (*v--)*tmp[i];
    }
  }

  /* copy tmp into x according to permutation */
  for ( i=0; i<n; i++ ) x[r[i]] += tmp[i];

  ierr = ISRestoreIndices(invisrow,&r); CHKERRQ(ierr);
  ierr = ISRestoreIndices(inviscol,&c); CHKERRQ(ierr);
  ierr = ISDestroy(invisrow); CHKERRQ(ierr);
  ierr = ISDestroy(inviscol); CHKERRQ(ierr);

  PLogFlops(2*a->nz);
  return 0;
}
/* ----------------------------------------------------------------*/

int MatILUFactorSymbolic_SeqAIJ(Mat A,IS isrow,IS iscol,double f,int levels,Mat *fact)
{
  Mat_SeqAIJ *a = (Mat_SeqAIJ *) A->data, *b;
  IS         isicol;
  int        *r,*ic, ierr, prow, n = a->m, *ai = a->i, *aj = a->j;
  int        *ainew,*ajnew, jmax,*fill, *xi, nz, *im,*ajfill,*flev;
  int        *dloc, idx, row,m,fm, nzf, nzi,len,  realloc = 0;
  int        incrlev,nnz,i,shift = a->indexshift;
  PetscTruth col_identity, row_identity;
 
  if (n != a->n) SETERRQ(1,"MatILUFactorSymbolic_SeqAIJ:Matrix must be square");
  if (!isrow) SETERRQ(1,"MatILUFactorSymbolic_SeqAIJ:Must have row permutation");
  if (!iscol) SETERRQ(1,"MatILUFactorSymbolic_SeqAIJ:Must have column permutation");

  /* special case that simply copies fill pattern */
  ISIdentity(isrow,&row_identity); ISIdentity(iscol,&col_identity);
  if (levels == 0 && row_identity && col_identity) {
    ierr = MatConvertSameType_SeqAIJ(A,fact,DO_NOT_COPY_VALUES); CHKERRQ(ierr);
    (*fact)->factor = FACTOR_LU;
    b               = (Mat_SeqAIJ *) (*fact)->data;
    if (!b->diag) {
      ierr = MatMarkDiag_SeqAIJ(*fact); CHKERRQ(ierr);
    }
    b->row          = isrow;
    b->col          = iscol;
    b->solve_work = (Scalar *) PetscMalloc((b->m+1)*sizeof(Scalar));CHKPTRQ(b->solve_work);
    return 0;
  }

  ierr = ISInvertPermutation(iscol,&isicol); CHKERRQ(ierr);
  ierr = ISGetIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISGetIndices(isicol,&ic); CHKERRQ(ierr);

  /* get new row pointers */
  ainew = (int *) PetscMalloc( (n+1)*sizeof(int) ); CHKPTRQ(ainew);
  ainew[0] = -shift;
  /* don't know how many column pointers are needed so estimate */
  jmax = (int) (f*(ai[n]+!shift));
  ajnew = (int *) PetscMalloc( (jmax)*sizeof(int) ); CHKPTRQ(ajnew);
  /* ajfill is level of fill for each fill entry */
  ajfill = (int *) PetscMalloc( (jmax)*sizeof(int) ); CHKPTRQ(ajfill);
  /* fill is a linked list of nonzeros in active row */
  fill = (int *) PetscMalloc( (n+1)*sizeof(int)); CHKPTRQ(fill);
  /* im is level for each filled value */
  im = (int *) PetscMalloc( (n+1)*sizeof(int)); CHKPTRQ(im);
  /* dloc is location of diagonal in factor */
  dloc = (int *) PetscMalloc( (n+1)*sizeof(int)); CHKPTRQ(dloc);
  dloc[0]  = 0;
  for ( prow=0; prow<n; prow++ ) {
    /* first copy previous fill into linked list */
    nzf     = nz  = ai[r[prow]+1] - ai[r[prow]];
    xi      = aj + ai[r[prow]] + shift;
    fill[n] = n;
    while (nz--) {
      fm  = n;
      idx = ic[*xi++ + shift];
      do {
        m  = fm;
        fm = fill[m];
      } while (fm < idx);
      fill[m]   = idx;
      fill[idx] = fm;
      im[idx]   = 0;
    }
    nzi = 0;
    row = fill[n];
    while ( row < prow ) {
      incrlev = im[row] + 1;
      nz      = dloc[row];
      xi      = ajnew  + ainew[row] + shift + nz;
      flev    = ajfill + ainew[row] + shift + nz + 1;
      nnz     = ainew[row+1] - ainew[row] - nz - 1;
      if (*xi++ + shift != row) {
        SETERRQ(1,"MatILUFactorSymbolic_SeqAIJ:zero pivot");
      }
      fm      = row;
      while (nnz-- > 0) {
        idx = *xi++ + shift;
        if (*flev + incrlev > levels) {
          flev++;
          continue;
        }
        do {
          m  = fm;
          fm = fill[m];
        } while (fm < idx);
        if (fm != idx) {
          im[idx]   = *flev + incrlev;
          fill[m]   = idx;
          fill[idx] = fm;
          fm        = idx;
          nzf++;
        }
        else {
          if (im[idx] > *flev + incrlev) im[idx] = *flev+incrlev;
        }
        flev++;
      }
      row = fill[row];
      nzi++;
    }
    /* copy new filled row into permanent storage */
    ainew[prow+1] = ainew[prow] + nzf;
    if (ainew[prow+1] > jmax-shift) {
      /* allocate a longer ajnew */
      int maxadd;
      maxadd = (int) ((f*(ai[n]+!shift)*(n-prow+5))/n);
      if (maxadd < nzf) maxadd = (n-prow)*(nzf+1);
      jmax += maxadd;
      xi = (int *) PetscMalloc( jmax*sizeof(int) );CHKPTRQ(xi);
      PetscMemcpy(xi,ajnew,(ainew[prow]+shift)*sizeof(int));
      PetscFree(ajnew);
      ajnew = xi;
      /* allocate a longer ajfill */
      xi = (int *) PetscMalloc( jmax*sizeof(int) );CHKPTRQ(xi);
      PetscMemcpy(xi,ajfill,(ainew[prow]+shift)*sizeof(int));
      PetscFree(ajfill);
      ajfill = xi;
      realloc++;
    }
    xi          = ajnew + ainew[prow] + shift;
    flev        = ajfill + ainew[prow] + shift;
    dloc[prow]  = nzi;
    fm          = fill[n];
    while (nzf--) {
      *xi++   = fm - shift;
      *flev++ = im[fm];
      fm      = fill[fm];
    }
  }
  PetscFree(ajfill); 
  ierr = ISRestoreIndices(isrow,&r); CHKERRQ(ierr);
  ierr = ISRestoreIndices(isicol,&ic); CHKERRQ(ierr);
  ierr = ISDestroy(isicol); CHKERRQ(ierr);
  PetscFree(fill); PetscFree(im);

  PLogInfo(A,
    "Info:MatILUFactorSymbolic_SeqAIJ:Realloc %d Fill ratio:given %g needed %g\n",
                             realloc,f,((double)ainew[n])/((double)ai[prow]));

  /* put together the new matrix */
  ierr = MatCreateSeqAIJ(A->comm,n,n,0,PETSC_NULL,fact); CHKERRQ(ierr);
  b = (Mat_SeqAIJ *) (*fact)->data;
  PetscFree(b->imax);
  b->singlemalloc = 0;
  len = (ainew[n] + shift)*sizeof(Scalar);
  /* the next line frees the default space generated by the Create() */
  PetscFree(b->a); PetscFree(b->ilen);
  b->a          = (Scalar *) PetscMalloc( len ); CHKPTRQ(b->a);
  b->j          = ajnew;
  b->i          = ainew;
  for ( i=0; i<n; i++ ) dloc[i] += ainew[i];
  b->diag       = dloc;
  b->ilen       = 0;
  b->imax       = 0;
  b->row        = isrow;
  b->col        = iscol;
  b->solve_work = (Scalar *) PetscMalloc( (n+1)*sizeof(Scalar)); 
  CHKPTRQ(b->solve_work);
  /* In b structure:  Free imax, ilen, old a, old j.  
     Allocate dloc, solve_work, new a, new j */
  PLogObjectMemory(*fact,(ainew[n]+shift-n) * (sizeof(int)+sizeof(Scalar)));
  b->maxnz          = b->nz = ainew[n] + shift;
  (*fact)->factor   = FACTOR_LU;
  return 0; 
}




