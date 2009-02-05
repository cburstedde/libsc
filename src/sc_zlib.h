/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2008,2009 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_ZLIB_H
#define SC_ZLIB_H

#ifndef SC_H
#error "sc.h should be included before this header file"
#endif

#ifdef SC_PROVIDE_ZLIB
#ifdef ZLIB_H
#error "zlib.h is included.  Include sc.h first or use --without-zlib".
#endif
#include "sc_builtin/zlib.h"
#else
#include <zlib.h>
#endif

#endif /* !SC_ZLIB_H */
