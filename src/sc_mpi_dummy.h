/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_MPI_DUMMY_H
#define SC_MPI_DUMMY_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

#define MPI_SUCCESS 0
#define MPI_COMM_NULL   ((MPI_Comm) 0x04000000)
#define MPI_COMM_WORLD  ((MPI_Comm) 0x44000000)
#define MPI_COMM_SELF   ((MPI_Comm) 0x44000001)

typedef int         MPI_Comm;

int                 MPI_Init (int *, char ***);
int                 MPI_Finalize (void);

int                 MPI_Comm_size (MPI_Comm, int *);
int                 MPI_Comm_rank (MPI_Comm, int *);

int                 MPI_Barrier (MPI_Comm);

double              MPI_Wtime (void);

int                 MPI_Abort (MPI_Comm, int)
  __attribute__ ((noreturn));

#endif /* !SC_MPI_DUMMY_H */
