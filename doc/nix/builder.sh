
# setup environment
source $stdenv/setup
export PATH=$which/bin:$gnum4/bin:$autoconf/bin:$automake/bin:$libtool/bin:$pkgconf/bin:$mpich/bin:$PATH
export ACLOCAL_PATH=$pkgconf/share/aclocal:$libtool/share/aclocal
export CC=mpicc

# initialize source
cp -r $src ./fromgit
chmod -R u+w ./fromgit
(cd ./fromgit && ./bootstrap)

# configure and build
mkdir build && cd build
../fromgit/configure \
  CPPFLAGS="-I$zlibdev/include" \
  CFLAGS="-O -g -Wall" \
  LDFLAGS="-L$zlib/lib" \
  --enable-debug --enable-mpi --prefix=$out
make -j8 V=0
make -j8 V=0 install
