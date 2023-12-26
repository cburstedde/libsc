#! /bin/bash

# Build and install the latest develop branch including zlib and jansson.
# We first download and install a recent zlib to a local directory, then do
# the same for libjansson.  We set a build environment to point to these two.
# Then we clone and install the current develop branch of libsc using them.

# This results in three installation directories that any higher
# level software package may be compiled and linked against.
# The options are similar to those used in this script.

# set installation root to local subdirectory
PREFIX="$PWD/local"

# download zlib
ZVER=1.3
ZSHA=ff0ba4c292013dbc27530b3a81e1f9a813cd39de01ca5e0f8bf355702efa593e

# download jansson
JVER=2.14

# feel free to make changes to the libsc and p4est configure line.
CONFIG="--enable-mpi"

# exit on error
bdie () {
        echo "Error building $@"
        exit 1
}

# download, build and install zlib
ZTAR="zlib-$ZVER.tar.gz"
wget -N "https://www.zlib.net/$ZTAR"                    && \
test `sha256sum "$ZTAR" | cut -d ' ' -f1` = "$ZSHA"     && \
tar -xvzf "$ZTAR"                                       && \
cd "zlib-$ZVER"                                         && \
./configure --prefix="$PREFIX/zlib"                     && \
make -j install                                         && \
cd ..                                                   && \
rm -r "zlib-$ZVER" "$ZTAR"                              || bdie "zlib"

# download, build and install jansson
JRELEASE="https://github.com/akheron/jansson/releases/"
JDOWNLOAD="download/v$JVER/jansson-$JVER.tar.gz"
wget -N "$JRELEASE$JDOWNLOAD"                           && \
tar -xvzf "jansson-$JVER.tar.gz"                        && \
cd "jansson-$JVER"                                      && \
./configure --prefix="$PREFIX/jansson"                  && \
make -j install V=0                                     && \
cd ..                                                   && \
rm -r "jansson-$JVER" "jansson-$JVER.tar.gz"            || bdie "jansson"

# provide environment that links to installed zlib and jansson
export CPPFLAGS="-I$PREFIX/zlib/include -I$PREFIX/jansson/include"
export CFLAGS="-O2 -g -Wall \
  -Wl,-rpath=$PREFIX/zlib/lib -Wl,-rpath=$PREFIX/jansson/lib"
export LDFLAGS="-L$PREFIX/zlib/lib -L$PREFIX/jansson/lib"

# clone, build and install libsc
rm -rf libsc
git clone https://github.com/cburstedde/libsc.git -b develop    && \
cd libsc                                                && \
./bootstrap                                             && \
mkdir build                                             && \
cd build                                                && \
../configure $CONFIG --prefix="$PREFIX/libsc"           && \
make -j install V=0                                     && \
cd ../../                                               && \
rm -rf libsc/.git                                       && \
rm -r libsc                                             || bdie "libsc"
