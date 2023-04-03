
#include <sc.h>

int
main (int argc, char **argv)
{
  sc_init (sc_MPI_COMM_WORLD, 0, 0, NULL, SC_LP_DEFAULT);
  sc_finalize ();

  return EXIT_SUCCESS;
}
