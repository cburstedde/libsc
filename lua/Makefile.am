
# This file is part of the SC Library
# Makefile.am in lua
# included non-recursively from toplevel directory
#
# This Makefile.am is part of the SC Library and NOT part of lua.
# All other files in this directory are unchanged as in lua 5.1.4.

EXTRA_DIST += \
        lua/COPYRIGHT lua/HISTORY lua/INSTALL lua/Makefile lua/README \
        lua/src/lua.c lua/src/luac.c lua/src/print.c \
        lua/doc lua/etc lua/test

if SC_PROVIDE_LUA

lua_internal_headers = \
        lua/src/lapi.h lua/src/lauxlib.h lua/src/lcode.h lua/src/ldebug.h \
        lua/src/ldo.h lua/src/lfunc.h lua/src/lgc.h lua/src/llex.h \
        lua/src/llimits.h lua/src/lmem.h lua/src/lobject.h lua/src/lopcodes.h \
        lua/src/lparser.h lua/src/lstate.h lua/src/lstring.h lua/src/ltable.h \
        lua/src/ltm.h lua/src/luaconf.h lua/src/lua.h lua/src/lualib.h \
        lua/src/lundump.h lua/src/lvm.h lua/src/lzio.h
lua_compiled_sources = \
        lua/src/lapi.c lua/src/lauxlib.c lua/src/lbaselib.c lua/src/lcode.c \
        lua/src/ldblib.c lua/src/ldebug.c lua/src/ldo.c lua/src/ldump.c \
        lua/src/lfunc.c lua/src/lgc.c lua/src/linit.c lua/src/liolib.c \
        lua/src/llex.c lua/src/lmathlib.c lua/src/lmem.c lua/src/loadlib.c \
        lua/src/lobject.c lua/src/lopcodes.c lua/src/loslib.c \
        lua/src/lparser.c lua/src/lstate.c lua/src/lstring.c \
        lua/src/lstrlib.c lua/src/ltable.c lua/src/ltablib.c lua/src/ltm.c \
        lua/src/lundump.c lua/src/lvm.c lua/src/lzio.c

libsc_internal_headers += $(lua_internal_headers)
libsc_compiled_sources += $(lua_compiled_sources)
LIBSC_CPPFLAGS += -DLUA_USE_LINUX

endif
