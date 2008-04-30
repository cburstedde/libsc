/*
  This file is part of sc.
  sc is a C library to support parallel scientific applications.

  Copyright (C) 2008 Carsten Burstedde, Lucas Wilcox.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* config.h comes first in every compilation unit */
#include <config.h>
#include <sc.h>

/* header files safe to use via gnulib */
#include <getopt.h>
#include <obstack.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*@unused@*/

static void
test_printf (void)
{
  int64_t             i64 = 0;
  long                l = 0;
  size_t              s = 0;
  ssize_t             ss = 0;

  printf ("%lld %ld %zu %zd\n", (long long) i64, l, s, ss);
}

/* EOF sc.c */
