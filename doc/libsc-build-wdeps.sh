#! /bin/bash

# Build and install the latest develop branch including zlib and jansson.
# We first download and install a recent zlib to a local directory, then do
# the same for jansson.  We set a build environment to point to these two.
# Then we clone and install the current develop branch of libsc using them.

# This results in three installation directories that any higher
# level software package may be compiled and linked against.
# The options are similar to those used in this script.
# In particular, the -rpath option may turn out useful.

# set installation root to local subdirectory
PREFIX="$PWD/local"

# download jansson
JVER=2.14
JSHA=5798d010e41cf8d76b66236cfb2f2543c8d082181d16bc3085ab49538d4b9929

# feel free to make changes to the libsc configure line.
# CONFIG="--enable-mpi --disable-shared"
# CONFIG="--enable-mpi --enable-debug"
CONFIG="--enable-mpi"

# exit on error
bdie () {
        echo "Error building $@"
        exit 1
}

# download, build and install zlib
ZTAR="zlib.tar.gz"
wget -N "https://www.zlib.net/current/$ZTAR"            && \
ZDIR=`tar -tzf "$ZTAR" | head -1 | perl -p -e 's/.*(zlib-[\d\.]+).*/\1/'` && \
tar -xvzf "$ZTAR"                                       && \
cd $ZDIR                                                && \
./configure --prefix="$PREFIX/zlib"                     && \
make -j install                                         && \
cd ..                                                   && \
rm -r "$ZDIR" "$ZTAR"                              || bdie "zlib"

# download, build and install jansson
JTAR="jansson-$JVER.tar.gz"
JRELEASE="https://github.com/akheron/jansson/releases"
JDOWNLOAD="download/v$JVER/$JTAR"
wget -N "$JRELEASE/$JDOWNLOAD"                          && \
test `sha256sum "$JTAR" | cut -d ' ' -f1` = "$JSHA"     && \
tar -xvzf "$JTAR"                                       && \
cd "jansson-$JVER"                                      && \
./configure --prefix="$PREFIX/jansson"                  && \
make -j install V=0                                     && \
cd ..                                                   && \
rm -r "jansson-$JVER" "$JTAR"                           || bdie "jansson"

# provide environment that links to installed zlib and jansson
export CPPFLAGS="$CPPFLAGS -I$PREFIX/zlib/include -I$PREFIX/jansson/include"
export CFLAGS="$CFLAGS -O2 -g -Wall \
  -Wl,-rpath=$PREFIX/zlib/lib -Wl,-rpath=$PREFIX/jansson/lib"
export LDFLAGS="$LDFLAGS -L$PREFIX/zlib/lib -L$PREFIX/jansson/lib"

# clone, build and install libsc
rm -rf libsc
git clone --depth 1 https://github.com/cburstedde/libsc.git -b develop  && \
cd libsc                                                && \
./bootstrap                                             && \
mkdir build                                             && \
cd build                                                && \
../configure $CONFIG --prefix="$PREFIX/libsc"           && \
make -j install V=0                                     && \
cd ../../                                               && \
rm -rf libsc/.git                                       && \
rm -r libsc                                             || bdie "libsc"
