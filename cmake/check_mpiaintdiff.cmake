include(CheckCSourceCompiles)
function(check_mpiaintdiff)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          MPI_Aint a, b, res;
          a = 42;
          b = 12;
          MPI_Init ((int *) 0, (char ***) 0);
          res = MPI_Aint_diff (a, b);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_HAVE_AINT_DIFF)

endfunction()

check_mpiaintdiff()
