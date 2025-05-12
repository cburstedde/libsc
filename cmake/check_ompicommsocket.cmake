include(CheckCSourceCompiles)
function(check_ompicommsocket)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  check_c_source_compiles(
    "
        #include <mpi.h>
        int main (void) {
          MPI_Comm subcomm;
          MPI_Init ((int *) 0, (char ***) 0);
          MPI_Comm_split_type(MPI_COMM_WORLD,OMPI_COMM_TYPE_SOCKET,0,MPI_INFO_NULL,&subcomm);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_ENABLE_OMPICOMMSOCKET)

endfunction()

check_ompicommsocket()