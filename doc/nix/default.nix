{
  stdenv, fetchgit,
  which, gnum4, autoconf, automake, libtool, pkgconf,
  debugEnable ? true, mpiSupport ? true,
  mpich ? null, zlib
}:

assert mpiSupport -> mpich != null;

stdenv.mkDerivation {
  name = "p4est-sc-prev3-develop-1ae814e3";
  builder = ./builder.sh;
  src = fetchgit {
    name = "p4est-sc.git";
    url = "https://github.com/cburstedde/libsc.git";
    rev = "1ae814e3fb1cc5456652e0d77550386842cb9bfb";
    sha256 = "14vm0b162jh8399pgpsikbwq4z5lkrw9vfzy3drqykw09n6nc53z";
  };
  inherit which gnum4 autoconf automake libtool pkgconf zlib;
  inherit debugEnable mpiSupport;
  mpi = if mpiSupport then mpich else null;
  zlibdev = zlib.dev;
}
