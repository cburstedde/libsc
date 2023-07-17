include(CheckCSourceCompiles)
function(check_mpiio)

  set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)

  # We cannot use check_include_file here as <mpi.h> needs to be
  # included before <mpi-ext.h>, and check_include_file doesn't
  # support this.
  check_c_source_compiles(
    "
        #include <mpi.h>
        int main() {
          int mpiret;
          MPI_File fh;
          MPI_Init ((int *) 0, (char ***) 0);
          mpiret =
            MPI_File_open (MPI_COMM_WORLD, \"filename\",
                           MPI_MODE_WRONLY | MPI_MODE_APPEND,
                           MPI_INFO_NULL, &fh);
          if (mpiret == MPI_ERR_FILE ||
              mpiret == MPI_ERR_NOT_SAME ||
              mpiret == MPI_ERR_AMODE ||
              mpiret == MPI_ERR_UNSUPPORTED_DATAREP ||
              mpiret == MPI_ERR_UNSUPPORTED_OPERATION ||
              mpiret == MPI_ERR_NO_SUCH_FILE ||
              mpiret == MPI_ERR_FILE_EXISTS ||
              mpiret == MPI_ERR_BAD_FILE ||
              mpiret == MPI_ERR_ACCESS ||
              mpiret == MPI_ERR_NO_SPACE ||
              mpiret == MPI_ERR_QUOTA ||
              mpiret == MPI_ERR_READ_ONLY ||
              mpiret == MPI_ERR_FILE_IN_USE ||
              mpiret == MPI_ERR_DUP_DATAREP ||
              mpiret == MPI_ERR_CONVERSION ||
              mpiret == MPI_ERR_IO) {
            mpiret = MPI_SUCCESS;
           }
          MPI_File_close (&fh);
          MPI_Finalize ();
          return 0;
        }
    "
    SC_ENABLE_MPIIO)

endfunction()

check_mpiio()
