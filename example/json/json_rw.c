
#include <sc3_mpi.h>
#ifdef SC_HAVE_JANSSON_H
#include <jansson.h>
#endif

static void
single_program (int *argc, char ***argv)
{
}

int
main (int argc, char **argv)
{
  sc3_MPI_Comm_t      mpicomm = SC3_MPI_COMM_WORLD;
  int                 mpirank;

  SC3X (sc3_MPI_Init (&argc, &argv));
  SC3X (sc3_MPI_Comm_rank (mpicomm, &mpirank));
  if (mpirank == 0) {
    single_program (&argc, &argv);
  }
  SC3X (sc3_MPI_Finalize ());
  return EXIT_SUCCESS;
}
