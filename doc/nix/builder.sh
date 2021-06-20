
# setup environment
source $stdenv/setup
export PATH=$which/bin:$gnum4/bin:$autoconf/bin:$automake/bin:$libtool/bin:$pkgconf/bin:$PATH
export ACLOCAL_PATH=$pkgconf/share/aclocal:$libtool/share/aclocal

if test "x$debugEnable" = x1 ; then
echo "Enabling debug build"
CONFIG_ENABLE_DEBUG="--enable-debug"
CFLAGS_EXTRA="-O -g"
else
CFLAGS_EXTRA="-O2"
fi

if test "x$mpiSupport" = x1 ; then
echo "Enabling MPI support"
export PATH=$mpi/bin:$PATH
export CC=mpicc
CONFIG_ENABLE_MPI="--enable-mpi"
fi

# initialize source
cp -r $src ./fromgit
chmod -R u+w ./fromgit
(cd ./fromgit && ./bootstrap)

# configure and build
mkdir build && cd build
../fromgit/configure \
  CPPFLAGS="-I$zlibdev/include" \
  CFLAGS="$CFLAGS_EXTRA -Wall" \
  LDFLAGS="-L$zlib/lib" \
  $CONFIG_ENABLE_DEBUG $CONFIG_ENABLE_MPI --prefix=$out
make -j8 V=0
make -j8 V=0 install
