{
  stdenv, fetchgit, lib,
  which, gnum4, autoconf, automake, libtool, pkgconf,
  debugEnable ? true, mpiSupport ? true,
  mpich ? null, zlib
}:

assert mpiSupport -> mpich != null;

let
  dbg = if debugEnable then "-dbg" else "";
in
stdenv.mkDerivation {
  name = "p4est-sc-prev3-develop-1ae814e3${dbg}";

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

  meta = {
    branch = "prev3-develop";
    description = "Support parallel scientific applications";
    homepage = https://www.p4est.org/;
    licence = lib.licenses.lgpl21Plus;
    maintainers = [ "Carsten Burstedde <burstedde@ins.uni-bonn.de>" ];
  };
}
