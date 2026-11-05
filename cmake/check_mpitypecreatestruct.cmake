include(CheckCSourceCompiles)
function(check_mpitypecreatestruct)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main(void) {
          int block_lens[1];
          MPI_Aint displacements[1];
          MPI_Datatype types[1], custom;
          block_lens[0] = 1;
          displacements[0] = 0;
          types[0] = MPI_BYTE;
          MPI_Init ((int *) 0, (char ***) 0);
          MPI_Type_create_struct (1, block_lens, displacements, types,
                                  &custom);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_HAVE_TYPE_CREATE_STRUCT)

endfunction()

check_mpitypecreatestruct()
