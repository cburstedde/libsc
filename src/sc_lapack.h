/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007-2009 Carsten Burstedde, Lucas Wilcox.

  The SC Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with the SC Library.  If not, see <http://www.gnu.org/licenses/>.
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
#define LAPACK_ILAENV   (int)  sc_lapack_nonimplemented

int                 sc_lapack_nonimplemented ();

#endif

SC_EXTERN_C_END;

#endif /* !SC_LAPACK_H */
