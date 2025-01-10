include(CheckCSourceCompiles)
function(check_mpitype MPI_TYPE)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          int size;
          MPI_Init ((int *) 0, (char ***) 0);
          /* check if ${MPI_TYPE} is defined */
          MPI_Type_size (${MPI_TYPE}, &size);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_HAVE_${MPI_TYPE})

endfunction()

check_mpitype(MPI_UNSIGNED_LONG_LONG)
check_mpitype(MPI_SIGNED_CHAR)
check_mpitype(MPI_INT8_T)
