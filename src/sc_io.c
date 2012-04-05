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

sc_io_sink_t       *
sc_io_sink_new (sc_io_sink_type_t stype, sc_io_mode_t mode,
                sc_io_encode_t encode, ...)
{
  sc_io_sink_t       *sink;
  va_list             ap;

  SC_ASSERT (0 <= stype && stype < SC_IO_SINK_LAST);
  SC_ASSERT (0 <= mode && mode < SC_IO_MODE_LAST);
  SC_ASSERT (0 <= encode && encode < SC_IO_ENCODE_LAST);

  sink = SC_ALLOC_ZERO (sc_io_sink_t, 1);
  sink->stype = stype;
  sink->mode = mode;
  sink->encode = encode;

  va_start (ap, encode);
  if (stype == SC_IO_SINK_BUFFER) {
    sink->buffer = va_arg (ap, sc_array_t *);
    SC_ASSERT (SC_ARRAY_IS_OWNER (sink->buffer));
    if (sink->mode == SC_IO_MODE_WRITE) {
      sc_array_reset (sink->buffer);
    }
  }
  else if (stype == SC_IO_SINK_FILENAME) {
    const char         *filename = va_arg (ap, const char *);

    sink->file = fopen (filename,
                        sink->mode == SC_IO_MODE_WRITE ? "wb" : "ab");
    if (sink->file == NULL) {
      SC_FREE (sink);
      return NULL;
    }
  }
  else if (stype == SC_IO_SINK_FILEFILE) {
    sink->file = va_arg (ap, FILE *);
    if (ferror (sink->file)) {
      SC_FREE (sink);
      return NULL;
    }
  }
  else {
    SC_ABORT_NOT_REACHED ();
  }
  va_end (ap);

  return sink;
}

int
sc_io_sink_destroy (sc_io_sink_t * sink)
{
  int                 retval;

  retval = 0;
  if (sink->stype == SC_IO_SINK_FILENAME) {
    SC_ASSERT (sink->file != NULL);
    retval = fclose (sink->file);
  }
  else if (sink->stype == SC_IO_SINK_FILEFILE) {
    SC_ASSERT (sink->file != NULL);
    retval = fflush (sink->file);
  }
  SC_FREE (sink);

  return retval;
}

int
sc_io_sink_write (sc_io_sink_t * sink, const void *data, size_t bytes)
{
  size_t              nwritten;

  nwritten = bytes;
  if (sink->stype == SC_IO_SINK_BUFFER) {
    void               *start;

    SC_ASSERT (sink->buffer != NULL);
    start = sc_array_push_count (sink->buffer, bytes);
    memcpy (start, data, bytes);
  }
  else if (sink->stype == SC_IO_SINK_FILENAME ||
           sink->stype == SC_IO_SINK_FILEFILE) {
    SC_ASSERT (sink->file != NULL);
    nwritten = fwrite (data, 1, bytes, sink->file);
  }

  return nwritten != bytes;
}

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

  /* This value may be changed although this is not tested with VTK */
  chunksize = (size_t) 1 << 15; /* 32768 */
  int_header = (uint32_t) byte_length;

  /* Allocate sufficient memory for base64 encoder */
  code_length = 2 * SC_MAX (chunksize, sizeof (int_header));
  code_length = SC_MAX (code_length, 4) + 1;
  base_data = SC_ALLOC (char, code_length);

  base64_init_encodestate (&encode_state);
  base_length =
    base64_encode_block ((char *) &int_header, sizeof (int_header), base_data,
                         &encode_state);
  SC_ASSERT (base_length < code_length);
  base_data[base_length] = '\0';
  (void) fwrite (base_data, 1, base_length, vtkfile);

  chunks = 0;
  remaining = byte_length;
  while (remaining > 0) {
    writenow = SC_MIN (remaining, chunksize);
    base_length = base64_encode_block (numeric_data + chunks * chunksize,
                                       writenow, base_data, &encode_state);
    SC_ASSERT (base_length < code_length);
    base_data[base_length] = '\0';
    (void) fwrite (base_data, 1, base_length, vtkfile);
    remaining -= writenow;
    ++chunks;
  }

  base_length = base64_encode_blockend (base_data, &encode_state);
  SC_ASSERT (base_length < code_length);
  base_data[base_length] = '\0';
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
  size_t              header_entries, header_size;
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
  header_entries = 3 + numfullblocks;
  header_size = header_entries * sizeof (uint32_t);

  /* allocate compression and base64 arrays */
  code_length = 2 * SC_MAX (blocksize, header_size) + 4 + 1;
  comp_data = SC_ALLOC (char, code_length);
  base_data = SC_ALLOC (char, code_length);

  /* figure out the size of the header and write a dummy */
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
                                     header_size, base_data, &encode_state);
  base_length +=
    base64_encode_blockend (base_data + base_length, &encode_state);
  SC_ASSERT (base_length < code_length);
  base_data[base_length] = '\0';
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
    SC_ASSERT (base_length < code_length);
    base_data[base_length] = '\0';
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
    SC_ASSERT (base_length < code_length);
    base_data[base_length] = '\0';
    (void) fwrite (base_data, 1, base_length, vtkfile);
  }

  /* write base64 end block */
  base_length = base64_encode_blockend (base_data, &encode_state);
  SC_ASSERT (base_length < code_length);
  base_data[base_length] = '\0';
  (void) fwrite (base_data, 1, base_length, vtkfile);

  /* seek back, write header block, seek forward */
  final_pos = ftell (vtkfile);
  base64_init_encodestate (&encode_state);
  base_length = base64_encode_block ((char *) compression_header,
                                     header_size, base_data, &encode_state);
  base_length +=
    base64_encode_blockend (base_data + base_length, &encode_state);
  SC_ASSERT (base_length < code_length);
  base_data[base_length] = '\0';
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
sc_mpi_write (MPI_File mpifile, const void *ptr, size_t zcount,
              MPI_Datatype t, const char *errmsg)
{
#ifdef SC_DEBUG
  int                 icount;
#endif
  int                 mpiret;
  MPI_Status          mpistatus;

  mpiret = MPI_File_write (mpifile, (void *) ptr,
                           (int) zcount, t, &mpistatus);
  SC_CHECK_ABORT (mpiret == MPI_SUCCESS, errmsg);

#ifdef SC_DEBUG
  MPI_Get_count (&mpistatus, t, &icount);
  SC_CHECK_ABORT (icount == (int) zcount, errmsg);
#endif
}

#endif
