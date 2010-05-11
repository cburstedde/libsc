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

#include <sc_blas.h>

const char          sc_transchar[] = { 'N', 'T', 'C' };
const char          sc_antitranschar[] = { 'T', 'N', '?' };
const char          sc_uplochar[] = { 'U', 'L', '?' };
const char          sc_cmachchar[] =
  { 'E', 'S', 'B', 'P', 'N', 'R', 'M', 'U', 'L', 'O' };

#ifndef SC_BLAS

int
sc_blas_nonimplemented ()
{
  SC_ABORT ("BLAS not compiled in this configuration");
  return 0;
}

#endif
