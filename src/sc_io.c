/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System

  The SC Library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The SC Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the SC Library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#include <sc_io.h>
#include <sc_zlib.h>
#include <libb64.h>

int
sc_vtk_write_binary (FILE * vtkfile, char *numeric_data, size_t byte_length)
{
  size_t              chunks, chunksize, remaining, writenow;
  size_t              code_length, base_length;
  uint32_t            int_header;
  char               *base_data;
  base64_encodestate  encode_state;

  /* VTK format used 32bit header info */
  SC_ASSERT (byte_length <= (size_t) UINT32_MAX);

  chunksize = (size_t) 1 << 15; /* 32768 */
  int_header = (uint32_t) byte_length;

  code_length = 2 * chunksize;
  base_data = SC_ALLOC (char, code_length);

  base64_init_encodestate (&encode_state);
  base_length =
    base64_encode_block ((char *) &int_header, sizeof (int_header), base_data,
                         &encode_state);
  base_data[base_length] = '\0';
  (void) fwrite (base_data, 1, base_length, vtkfile);

  chunks = 0;
  remaining = byte_length;
  while (remaining > 0) {
    writenow = SC_MIN (remaining, chunksize);
    base_length = base64_encode_block (numeric_data + chunks * chunksize,
                                       writenow, base_data, &encode_state);
    base_data[base_length] = '\0';
    SC_ASSERT (base_length < code_length);
    (void) fwrite (base_data, 1, base_length, vtkfile);
    remaining -= writenow;
    ++chunks;
  }

  base_length = base64_encode_blockend (base_data, &encode_state);
  (void) fwrite (base_data, 1, base_length, vtkfile);

  SC_FREE (base_data);
  if (ferror (vtkfile)) {
    return -1;
  }
  return 0;
}

int
sc_vtk_write_compressed (FILE * vtkfile, char *numeric_data,
                         size_t byte_length)
{
  int                 retval, fseek1, fseek2;
  size_t              iz;
  size_t              blocksize, lastsize;
  size_t              theblock, numregularblocks, numfullblocks;
  size_t              header_entries;
  size_t              code_length, base_length;
  long                header_pos, final_pos;
  char               *comp_data, *base_data;
  uint32_t           *compression_header;
  uLongf              comp_length;
  base64_encodestate  encode_state;

  /* compute block sizes */
  blocksize = (size_t) (1 << 15);       /* 32768 */
  lastsize = byte_length % blocksize;
  numregularblocks = byte_length / blocksize;
  numfullblocks = numregularblocks + (lastsize > 0 ? 1 : 0);

  /* allocate compression and base64 arrays */
  code_length = 2 * blocksize;
  comp_data = SC_ALLOC (char, code_length);
  base_data = SC_ALLOC (char, code_length);

  /* figure out the size of the header and write a dummy */
  header_entries = 3 + numfullblocks;
  compression_header = SC_ALLOC (uint32_t, header_entries);
  compression_header[0] = (uint32_t) numfullblocks;
  compression_header[1] = (uint32_t) blocksize;
  compression_header[2] = (uint32_t)
    (lastsize > 0 || byte_length == 0 ? lastsize : blocksize);
  for (iz = 3; iz < header_entries; ++iz) {
    compression_header[iz] = 0;
  }
  base64_init_encodestate (&encode_state);
  base_length = base64_encode_block ((char *) compression_header,
                                     sizeof (*compression_header) *
                                     header_entries, base_data,
                                     &encode_state);
  base_length +=
    base64_encode_blockend (base_data + base_length, &encode_state);
  base_data[base_length] = '\0';
  SC_ASSERT (base_length < code_length);
  header_pos = ftell (vtkfile);
  (void) fwrite (base_data, 1, base_length, vtkfile);

  /* write the regular data blocks */
  base64_init_encodestate (&encode_state);
  for (theblock = 0; theblock < numregularblocks; ++theblock) {
    comp_length = code_length;
    retval = compress2 ((Bytef *) comp_data, &comp_length,
                        (const Bytef *) (numeric_data + theblock * blocksize),
                        (uLong) blocksize, Z_BEST_COMPRESSION);
    SC_ASSERT (retval == Z_OK);
    compression_header[3 + theblock] = comp_length;
    base_length = base64_encode_block (comp_data, comp_length,
                                       base_data, &encode_state);
    base_data[base_length] = '\0';
    SC_ASSERT (base_length < code_length);
    (void) fwrite (base_data, 1, base_length, vtkfile);
  }

  /* write odd-sized last block if necessary */
  if (lastsize > 0) {
    comp_length = code_length;
    retval = compress2 ((Bytef *) comp_data, &comp_length,
                        (const Bytef *) (numeric_data + theblock * blocksize),
                        (uLong) lastsize, Z_BEST_COMPRESSION);
    SC_ASSERT (retval == Z_OK);
    compression_header[3 + theblock] = comp_length;
    base_length = base64_encode_block (comp_data, comp_length,
                                       base_data, &encode_state);
    base_data[base_length] = '\0';
    SC_ASSERT (base_length < code_length);
    (void) fwrite (base_data, 1, base_length, vtkfile);
  }

  /* write base64 end block */
  base_length = base64_encode_blockend (base_data, &encode_state);
  (void) fwrite (base_data, 1, base_length, vtkfile);

  /* seek back, write header block, seek forward */
  final_pos = ftell (vtkfile);
  base64_init_encodestate (&encode_state);
  base_length = base64_encode_block ((char *) compression_header,
                                     sizeof (*compression_header) *
                                     header_entries, base_data,
                                     &encode_state);
  base_length +=
    base64_encode_blockend (base_data + base_length, &encode_state);
  base_data[base_length] = '\0';
  SC_ASSERT (base_length < code_length);
  fseek1 = fseek (vtkfile, header_pos, SEEK_SET);
  (void) fwrite (base_data, 1, base_length, vtkfile);
  fseek2 = fseek (vtkfile, final_pos, SEEK_SET);

  /* clean up and return */
  SC_FREE (compression_header);
  SC_FREE (comp_data);
  SC_FREE (base_data);
  if (fseek1 != 0 || fseek2 != 0 || ferror (vtkfile)) {
    return -1;
  }
  return 0;
}

void
sc_fwrite (const void *ptr, size_t size, size_t nmemb, FILE * file,
           const char *errmsg)
{
  size_t              nwritten;

  nwritten = fwrite (ptr, size, nmemb, file);
  SC_CHECK_ABORT (nwritten == nmemb, errmsg);
}

void
sc_fread (void *ptr, size_t size, size_t nmemb, FILE * file,
          const char *errmsg)
{
  size_t              nread;

  nread = fread (ptr, size, nmemb, file);
  SC_CHECK_ABORT (nread == nmemb, errmsg);
}

#ifdef SC_MPIIO

void
sc_mpi_write (sc_MPI_File mpifile, const void *ptr, size_t zcount,
              sc_MPI_Datatype t, const char *errmsg)
{
#ifdef SC_DEBUG
  int                 icount;
#endif
  int                 mpiret;
  sc_MPI_Status       mpistatus;

  mpiret = sc_MPI_File_write (mpifile, (void *) ptr,
                           (int) zcount, t, &mpistatus);
  SC_CHECK_ABORT (mpiret == sc_MPI_SUCCESS, errmsg);

#ifdef SC_DEBUG
  sc_MPI_Get_count (&mpistatus, t, &icount);
  SC_CHECK_ABORT (icount == (int) zcount, errmsg);
#endif
}

#endif
