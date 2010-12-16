/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef SC_LAPACK_H
#define SC_LAPACK_H

#include <sc_blas.h>

SC_EXTERN_C_BEGIN;

typedef enum sc_jobz
{
  SC_EIGVALS_ONLY,
  SC_EIGVALS_AND_EIGVECS,
  SC_JOBZ_ANCHOR
}
sc_jobz_t;

extern const char   sc_jobzchar[];

#ifdef SC_LAPACK

#ifndef SC_F77_FUNC
#define SC_F77_FUNC(small,CAPS) small ## _
#endif

#define LAPACK_DGELS   SC_F77_FUNC(dgels,DGELS)
#define LAPACK_DGETRF  SC_F77_FUNC(dgetrf,DGETRF)
#define LAPACK_DGETRS  SC_F77_FUNC(dgetrs,DGETRS)
#define LAPACK_DSTEV   SC_F77_FUNC(dstev,DSTEV)
#define LAPACK_DTRSM   SC_F77_FUNC(dtrsm,DTRSM)
#define LAPACK_DLAIC1  SC_F77_FUNC(dlaic1,DLAIC1)
#define LAPACK_ILAENV  SC_F77_FUNC(ilaenv,ILAENV)

void                LAPACK_DGELS (const char *trans,
                                  const sc_bint_t * m, const sc_bint_t * n,
                                  const sc_bint_t * nrhs, double *a,
                                  const sc_bint_t * lda, double *b,
                                  const sc_bint_t * ldb, double *work,
                                  const sc_bint_t * lwork, sc_bint_t * info);
void                LAPACK_DGETRF (const sc_bint_t * m, const sc_bint_t * n,
                                   double *a, const sc_bint_t * lda,
                                   sc_bint_t * ipiv, sc_bint_t * info);

void                LAPACK_DGETRS (const char *trans, const sc_bint_t * n,
                                   const sc_bint_t * nrhs, const double *a,
                                   const sc_bint_t * lda,
                                   const sc_bint_t * ipiv, double *b,
                                   const sc_bint_t * ldx, sc_bint_t * info);

void                LAPACK_DSTEV (const char *jobz,
                                  const sc_bint_t * n,
                                  double *d,
                                  double *e,
                                  double *z,
                                  const sc_bint_t * ldz,
                                  double *work, sc_bint_t * info);

void                LAPACK_DTRSM (const char *side,
                                  const char *uplo,
                                  const char *transa,
                                  const char *diag,
                                  const sc_bint_t * m,
                                  const sc_bint_t * n,
                                  const double *alpha,
                                  const double *a,
                                  const sc_bint_t * lda,
                                  const double *b, const sc_bint_t * ldb);

void                LAPACK_DLAIC1 (const char *job,
                                   const int *j,
                                   const double *x,
                                   const double *sest,
                                   const double *w,
                                   const double *gamma,
                                   double *sestpr,
                                   double *s,
                                   double *c);

int                 LAPACK_ILAENV (const sc_bint_t * ispec,
                                   const char *name,
                                   const char *opts,
                                   const sc_bint_t * N1,
                                   const sc_bint_t * N2,
                                   const sc_bint_t * N3,
                                   const sc_bint_t * N4,
                                   sc_buint_t name_length,
                                   sc_buint_t opts_length);

#else /* !SC_LAPACK */

#define LAPACK_DGELS    (void) sc_lapack_nonimplemented
#define LAPACK_DGETRF   (void) sc_lapack_nonimplemented
#define LAPACK_DGETRS   (void) sc_lapack_nonimplemented
#define LAPACK_DSTEV    (void) sc_lapack_nonimplemented
#define LAPACK_DTRSM    (void) sc_lapack_nonimplemented
#define LAPACK_DLAIC1   (void) sc_lapack_nonimplemented
#define LAPACK_ILAENV   (int)  sc_lapack_nonimplemented

int                 sc_lapack_nonimplemented ();

#endif

SC_EXTERN_C_END;

#endif /* !SC_LAPACK_H */
