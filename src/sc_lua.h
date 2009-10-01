/*
  This file is part of the SC Library.
  The SC library provides support for parallel scientific applications.

  Copyright (C) 2009 Carsten Burstedde, Lucas Wilcox.

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

#ifndef SC_LUA_H
#define SC_LUA_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

#define LUA_USE_LINUX

#ifdef SC_PROVIDE_LUA
#ifdef lua_h
#error "lua.h is included.  Include sc.h first or use --without-lua".
#endif
#include "sc_builtin/lua.h"
#include "sc_builtin/lualib.h"
#include "sc_builtin/lauxlib.h"
#else
#ifdef SC_HAVE_LUA_H
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif
#ifdef SC_HAVE_LUA5_1_LUA_H
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>
#endif
#endif

SC_EXTERN_C_END;

#endif /* !SC_LUA_H */
