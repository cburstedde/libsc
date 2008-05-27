/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2007,2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

#include <sc_blas.h>

typedef enum sc_jobz
{
  SC_EIGVALS_ONLY,
  SC_EIGVALS_AND_EIGVECS,
  SC_JOBZ_ANCHOR
}
sc_jobz_t;

extern const char   sc_jobzchar[];

#ifndef SC_F77_FUNC
#define SC_F77_FUNC(small,caps) small ## _
#endif

#define LAPACK_SSTEV  SC_F77_FUNC(sstev,SSTEV)

void                LAPACK_SSTEV (const char * jobz,
                                  const sc_bint_t * n,
                                  const double * d,
                                  const double * e,
                                  const double * z,
                                  const sc_bint_t * ldz,
                                  const double * work,
                                  const sc_bint_t * info);

#endif /* !SC_LAPACK_H */
