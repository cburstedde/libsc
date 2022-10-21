/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2010 The University of Texas System
  Additional copyright (C) 2011 individual authors

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
#include <libb64.h>
#ifdef SC_HAVE_ZLIB
#include <zlib.h>
#endif

#ifndef SC_ENABLE_MPIIO
#include <errno.h>
#endif

sc_io_sink_t       *
sc_io_sink_new (int iotype, int iomode, int ioencode, ...)
{
  sc_io_sink_t       *sink;
  va_list             ap;

  SC_ASSERT (0 <= iotype && iotype < SC_IO_TYPE_LAST);
  SC_ASSERT (0 <= iomode && iomode < SC_IO_MODE_LAST);
  SC_ASSERT (0 <= ioencode && ioencode < SC_IO_ENCODE_LAST);

  sink = SC_ALLOC_ZERO (sc_io_sink_t, 1);
  sink->iotype = (sc_io_type_t) iotype;
  sink->mode = (sc_io_mode_t) iomode;
  sink->encode = (sc_io_encode_t) ioencode;

  va_start (ap, ioencode);
  if (iotype == SC_IO_TYPE_BUFFER) {
    sink->buffer = va_arg (ap, sc_array_t *);
    if (sink->mode == SC_IO_MODE_WRITE) {
      sc_array_resize (sink->buffer, 0);
    }
  }
  else if (iotype == SC_IO_TYPE_FILENAME) {
    const char         *filename = va_arg (ap, const char *);

    sink->file = fopen (filename,
                        sink->mode == SC_IO_MODE_WRITE ? "wb" : "ab");
    if (sink->file == NULL) {
      SC_FREE (sink);
      return NULL;
    }
  }
  else if (iotype == SC_IO_TYPE_FILEFILE) {
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

  /* The error value SC_IO_ERROR_AGAIN is turned into FATAL */
  retval = sc_io_sink_complete (sink, NULL, NULL);
  if (sink->iotype == SC_IO_TYPE_FILENAME) {
    SC_ASSERT (sink->file != NULL);

    /* Attempt close even on complete error */
    retval = fclose (sink->file) || retval;
  }
  SC_FREE (sink);

  return retval ? SC_IO_ERROR_FATAL : SC_IO_ERROR_NONE;
}

int
sc_io_sink_write (sc_io_sink_t * sink, const void *data, size_t bytes_avail)
{
  size_t              bytes_out;

  bytes_out = 0;

  if (sink->iotype == SC_IO_TYPE_BUFFER) {
    size_t              elem_size, new_count;

    SC_ASSERT (sink->buffer != NULL);
    elem_size = sink->buffer->elem_size;
    new_count =
      (sink->buffer_bytes + bytes_avail + elem_size - 1) / elem_size;
    sc_array_resize (sink->buffer, new_count);
    /* For a view sufficient size is asserted only in debug mode. */
    if (new_count * elem_size > SC_ARRAY_BYTE_ALLOC (sink->buffer)) {
      return SC_IO_ERROR_FATAL;
    }

    memcpy (sink->buffer->array + sink->buffer_bytes, data, bytes_avail);
    sink->buffer_bytes += bytes_avail;
    bytes_out = bytes_avail;
  }
  else if (sink->iotype == SC_IO_TYPE_FILENAME ||
           sink->iotype == SC_IO_TYPE_FILEFILE) {
    SC_ASSERT (sink->file != NULL);
    bytes_out = fwrite (data, 1, bytes_avail, sink->file);
    if (bytes_out != bytes_avail) {
      return SC_IO_ERROR_FATAL;
    }
  }

  sink->bytes_in += bytes_avail;
  sink->bytes_out += bytes_out;

  return SC_IO_ERROR_NONE;
}

int
sc_io_sink_complete (sc_io_sink_t * sink, size_t * bytes_in,
                     size_t * bytes_out)
{
  int                 retval;

  retval = 0;
  if (sink->iotype == SC_IO_TYPE_BUFFER) {
    SC_ASSERT (sink->buffer != NULL);
    if (sink->buffer_bytes % sink->buffer->elem_size != 0) {
      return SC_IO_ERROR_AGAIN;
    }
  }
  else if (sink->iotype == SC_IO_TYPE_FILENAME ||
           sink->iotype == SC_IO_TYPE_FILEFILE) {
    SC_ASSERT (sink->file != NULL);
    retval = fflush (sink->file);
  }
  if (retval) {
    return SC_IO_ERROR_FATAL;
  }

  if (bytes_in != NULL) {
    *bytes_in = sink->bytes_in;
  }
  if (bytes_out != NULL) {
    *bytes_out = sink->bytes_out;
  }
  sink->bytes_in = sink->bytes_out = 0;

  return SC_IO_ERROR_NONE;
}

int
sc_io_sink_align (sc_io_sink_t * sink, size_t bytes_align)
{
  size_t              fill_bytes;
  char               *fill;
  int                 retval;

  fill_bytes = (bytes_align - sink->bytes_out % bytes_align) % bytes_align;
  fill = SC_ALLOC_ZERO (char, fill_bytes);
  retval = sc_io_sink_write (sink, fill, fill_bytes);
  SC_FREE (fill);

  return retval;
}

sc_io_source_t     *
sc_io_source_new (int iotype, int ioencode, ...)
{
  sc_io_source_t     *source;
  va_list             ap;

  SC_ASSERT (0 <= iotype && iotype < SC_IO_TYPE_LAST);
  SC_ASSERT (0 <= ioencode && ioencode < SC_IO_ENCODE_LAST);

  source = SC_ALLOC_ZERO (sc_io_source_t, 1);
  source->iotype = (sc_io_type_t) iotype;
  source->encode = (sc_io_encode_t) ioencode;

  va_start (ap, ioencode);
  if (iotype == SC_IO_TYPE_BUFFER) {
    source->buffer = va_arg (ap, sc_array_t *);
  }
  else if (iotype == SC_IO_TYPE_FILENAME) {
    const char         *filename = va_arg (ap, const char *);

    source->file = fopen (filename, "rb");
    if (source->file == NULL) {
      SC_FREE (source);
      return NULL;
    }
  }
  else if (iotype == SC_IO_TYPE_FILEFILE) {
    source->file = va_arg (ap, FILE *);
    if (ferror (source->file)) {
      SC_FREE (source);
      return NULL;
    }
  }
  else {
    SC_ABORT_NOT_REACHED ();
  }
  va_end (ap);

  return source;
}

int
sc_io_source_destroy (sc_io_source_t * source)
{
  int                 retval;

  /* complete reading */
  retval = sc_io_source_complete (source, NULL, NULL);

  /* destroy mirror */
  if (source->mirror != NULL) {
    retval = sc_io_sink_destroy (source->mirror) || retval;
    sc_array_destroy (source->mirror_buffer);
  }

  /* The error value SC_IO_ERROR_AGAIN is turned into FATAL */
  if (source->iotype == SC_IO_TYPE_FILENAME) {
    SC_ASSERT (source->file != NULL);

    /* Attempt close even on complete error */
    retval = fclose (source->file) || retval;
  }
  SC_FREE (source);

  return retval ? SC_IO_ERROR_FATAL : SC_IO_ERROR_NONE;
}

int
sc_io_source_read (sc_io_source_t * source, void *data,
                   size_t bytes_avail, size_t * bytes_out)
{
  int                 retval;
  size_t              bbytes_out;

  retval = 0;
  bbytes_out = 0;

  if (source->iotype == SC_IO_TYPE_BUFFER) {
    SC_ASSERT (source->buffer != NULL);
    bbytes_out = SC_ARRAY_BYTE_ALLOC (source->buffer);
    SC_ASSERT (bbytes_out >= source->buffer_bytes);
    bbytes_out -= source->buffer_bytes;
    bbytes_out = SC_MIN (bbytes_out, bytes_avail);

    if (data != NULL) {
      memcpy (data, source->buffer->array + source->buffer_bytes, bbytes_out);
    }
    source->buffer_bytes += bbytes_out;
  }
  else if (source->iotype == SC_IO_TYPE_FILENAME ||
           source->iotype == SC_IO_TYPE_FILEFILE) {
    SC_ASSERT (source->file != NULL);
    if (data != NULL) {
      bbytes_out = fread (data, 1, bytes_avail, source->file);
      if (bbytes_out < bytes_avail) {
        retval = !feof (source->file) || ferror (source->file);
      }
      if (retval == SC_IO_ERROR_NONE && source->mirror != NULL) {
        retval = sc_io_sink_write (source->mirror, data, bbytes_out);
      }
    }
    else {
      retval = fseek (source->file, (long) bytes_avail, SEEK_CUR);
      bbytes_out = bytes_avail;
    }
  }
  if (retval) {
    return SC_IO_ERROR_FATAL;
  }
  if (bytes_out == NULL && bbytes_out < bytes_avail) {
    return SC_IO_ERROR_FATAL;
  }

  if (bytes_out != NULL) {
    *bytes_out = bbytes_out;
  }
  source->bytes_in += bbytes_out;
  source->bytes_out += bbytes_out;

  return SC_IO_ERROR_NONE;
}

int
sc_io_source_complete (sc_io_source_t * source,
                       size_t * bytes_in, size_t * bytes_out)
{
  int                 retval = SC_IO_ERROR_NONE;

  if (source->iotype == SC_IO_TYPE_BUFFER) {
    SC_ASSERT (source->buffer != NULL);
    if (source->buffer_bytes % source->buffer->elem_size != 0) {
      return SC_IO_ERROR_AGAIN;
    }
  }
  else if (source->iotype == SC_IO_TYPE_FILENAME ||
           source->iotype == SC_IO_TYPE_FILEFILE) {
    if (source->mirror != NULL) {
      retval = sc_io_sink_complete (source->mirror, NULL, NULL);
    }
  }

  if (bytes_in != NULL) {
    *bytes_in = source->bytes_in;
  }
  if (bytes_out != NULL) {
    *bytes_out = source->bytes_out;
  }
  source->bytes_in = source->bytes_out = 0;

  return retval;
}

int
sc_io_source_align (sc_io_source_t * source, size_t bytes_align)
{
  size_t              fill_bytes;

  fill_bytes = (bytes_align - source->bytes_out % bytes_align) % bytes_align;

  return sc_io_source_read (source, NULL, fill_bytes, NULL);
}

int
sc_io_source_activate_mirror (sc_io_source_t * source)
{
  if (source->iotype == SC_IO_TYPE_BUFFER) {
    return SC_IO_ERROR_FATAL;
  }
  if (source->mirror != NULL) {
    return SC_IO_ERROR_FATAL;
  }

  source->mirror_buffer = sc_array_new (sizeof (char));
  source->mirror = sc_io_sink_new (SC_IO_TYPE_BUFFER, SC_IO_MODE_WRITE,
                                   SC_IO_ENCODE_NONE, source->mirror_buffer);

  return (source->mirror != NULL ? SC_IO_ERROR_NONE : SC_IO_ERROR_FATAL);
}

int
sc_io_source_read_mirror (sc_io_source_t * source, void *data,
                          size_t bytes_avail, size_t * bytes_out)
{
  sc_io_source_t     *mirror_src;
  int                 retval;

  if (source->mirror_buffer == NULL) {
    return SC_IO_ERROR_FATAL;
  }

  mirror_src = sc_io_source_new (SC_IO_TYPE_BUFFER, SC_IO_ENCODE_NONE,
                                 source->mirror_buffer);
  retval = (mirror_src != NULL ? SC_IO_ERROR_NONE : SC_IO_ERROR_FATAL);
  retval = retval || sc_io_source_read (mirror_src, data, bytes_avail,
                                        bytes_out);
  if (mirror_src != NULL) {
    retval = sc_io_source_destroy (mirror_src) || retval;
  }

  return retval;
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
#ifdef SC_HAVE_ZLIB
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
    SC_CHECK_ZLIB (retval);
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
    SC_CHECK_ZLIB (retval);
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
#else
  SC_ABORT ("Configure did not find a recent enough zlib.  Abort.\n");
#endif

  return 0;
}

FILE               *
sc_fopen (const char *filename, const char *mode, const char *errmsg)
{
  FILE               *fp;

  fp = fopen (filename, mode);
  SC_CHECK_ABORT (fp != NULL, errmsg);

  return fp;
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

void
sc_fflush_fsync_fclose (FILE * file)
{
  int                 retval;

  /* fflush is called anyway from fclose.
     fsync is fine, but fileno is not portable.
     Better to remove altogether. */
#if 0
  /* best attempt to flush file to disk */
  retval = fflush (file);
  SC_CHECK_ABORT (!retval, "file flush");
#ifdef SC_HAVE_FSYNC
  retval = fsync (fileno (file));
  SC_CHECK_ABORT (!retval, "file fsync");
#endif
#endif
  retval = fclose (file);
  SC_CHECK_ABORT (!retval, "file close");
}

/** Translate an I/O error into an appropriate MPI error class.
 * This function is strictly meant for file access functions.
 * If MPI I/O is not present, translate an errno set by stdio.
 * It is thus possible to substitute MPI I/O by fopen, fread, etc.
 * and to process the errors with this one function regardless.
 * \param [in] errorcode   Without MPI I/O: Translate errors from
 *                         fopen, fclose, fread, fwrite, fseek, ftell
 *                         into an appropriate MPI error class.
 *                         With MPI I/O: Turn error code into its class.
 * \param [out] errorclass Regardless of whether MPI I/O is enabled,
 *                         an MPI file related error from \ref sc_mpi.h.
 *                         It may be passed to \ref sc_MPI_Error_string.
 * \return                 sc_MPI_SUCCESS only on successful conversion.
 */
static int
sc_io_error_class (int errorcode, int *errorclass)
{
#ifdef SC_ENABLE_MPIIO
  return MPI_Error_class (errorcode, errorclass);
#else
  if (errorclass == NULL) {
    return sc_MPI_ERR_ARG;
  }

  /* We do not check for a certain range of error codes
   * since we do not know the last errno.
   */

  if (errorcode == 0) {
    /* This is the errno for no error or
     * possibly also the sc_MPI_SUCESS value.
     * In both cases, we want to set errorclass to
     * sc_MPI_SUCCESS.
     */
    *errorclass = sc_MPI_SUCCESS;
    return sc_MPI_SUCCESS;
  }

  switch (errorcode) {
  case sc_MPI_SUCCESS:
    *errorclass = sc_MPI_SUCCESS;
    break;

  case EBADF:
  case ESPIPE:
    *errorclass = sc_MPI_ERR_FILE;
    break;

  case EINVAL:
  case EOPNOTSUPP:
    *errorclass = sc_MPI_ERR_AMODE;
    break;

  case ENOENT:
    *errorclass = sc_MPI_ERR_NO_SUCH_FILE;
    break;

  case EEXIST:
    *errorclass = sc_MPI_ERR_FILE_EXISTS;
    break;

  case EFAULT:
  case EISDIR:
  case ELOOP:
  case ENAMETOOLONG:
  case ENODEV:
  case ENOTDIR:
    *errorclass = sc_MPI_ERR_BAD_FILE;
    break;

  case EACCES:
  case EPERM:
  case EROFS:
  case ETXTBSY:
    *errorclass = sc_MPI_ERR_ACCESS;
    break;

  case EFBIG:
  case ENOSPC:
  case EOVERFLOW:
    *errorclass = sc_MPI_ERR_NO_SPACE;
    break;

  case EMFILE:
  case ENFILE:
  case ENOMEM:
    *errorclass = sc_MPI_ERR_NO_MEM;
    break;

  case EAGAIN:
  case EDESTADDRREQ:
  case EINTR:
  case EIO:
  case ENXIO:
  case EPIPE:
    *errorclass = sc_MPI_ERR_IO;
    break;
  default:
    *errorclass = sc_MPI_ERR_UNKNOWN;
  }
  return sc_MPI_SUCCESS;
#endif
}

#ifndef SC_ENABLE_MPIIO
static void
sc_io_parse_nompiio_access_mode (sc_io_open_mode_t amode, char mode[4])
{
  /* parse access mode */
  switch (amode) {
  case SC_IO_READ:
    snprintf (mode, 3, "%s", "rb");
    break;
  case SC_IO_WRITE_CREATE:
    snprintf (mode, 3, "%s", "wb");
    break;
  case SC_IO_WRITE_APPEND:
    /* the file is opened in the corresponding write call */
#if 0
    snprintf (mode, 3, "%s", "rb");
#endif
    snprintf (mode, 1, "%s", "");
    break;
  default:
    SC_ABORT ("Invalid non MPI IO file access mode");
    break;
  }
}
#else
static void
sc_io_parse_mpiio_access_mode (sc_io_open_mode_t amode, int *mode)
{
  /* parse access mode */
  switch (amode) {
  case SC_IO_READ:
    *mode = sc_MPI_MODE_RDONLY;
    break;
  case SC_IO_WRITE_CREATE:
    *mode = sc_MPI_MODE_WRONLY | sc_MPI_MODE_CREATE;
    break;
  case SC_IO_WRITE_APPEND:
    *mode = sc_MPI_MODE_WRONLY | sc_MPI_MODE_APPEND;
    break;
  default:
    SC_ABORT ("Invalid MPI IO file access mode");
    break;
  }
}
#endif

int
sc_io_open (sc_MPI_Comm mpicomm, const char *filename,
            sc_io_open_mode_t amode, sc3_MPI_Info_t mpiinfo,
            sc_MPI_File * mpifile)
{
#ifdef SC_ENABLE_MPIIO
  int                 retval, mpiret, errcode, mode;

  sc_io_parse_mpiio_access_mode (amode, &mode);

  mpiret = MPI_File_open (mpicomm, filename, mode, mpiinfo, mpifile);
  retval = sc_io_error_class (mpiret, &errcode);
  SC_CHECK_MPI (retval);

  return errcode;
#elif defined (SC_ENABLE_MPI)
  {
    int                 rank, mpiret, retval, errcode;
    char                mode[4];

    /* allocate file struct */
    *mpifile = (sc_MPI_File) SC_ALLOC (struct no_mpiio_file, 1);

    /* serialize the I/O operations */
    /* active flag is set later in  */
    (*mpifile)->filename = filename;

    /* store the communicator */
    (*mpifile)->mpicomm = mpicomm;

    sc_io_parse_nompiio_access_mode (amode, mode);

    /* get my rank */
    mpiret = sc_MPI_Comm_rank (mpicomm, &rank);
    SC_CHECK_MPI (mpiret);
    if (rank == 0) {
      errno = 0;
      (*mpifile)->file = fopen (filename, mode);
      mpiret = errno;
    }
    else {
      (*mpifile)->file = sc_MPI_FILE_NULL;
      mpiret = sc_MPI_SUCCESS;
    }
    /* broadcast errno */
    sc_MPI_Bcast (&mpiret, 1, sc_MPI_INT, 0, mpicomm);

    retval = sc_io_error_class (mpiret, &errcode);
    SC_CHECK_MPI (retval);

    return errcode;
  }
#else
/* no MPI */
  {
    int                 retval, errcode;
    char                mode[4];

    /* allocate file struct */
    *mpifile = (sc_MPI_File) SC_ALLOC (struct no_mpiio_file, 1);

    (*mpifile)->filename = filename;

    sc_io_parse_nompiio_access_mode (amode, mode);
    errno = 0;
    (*mpifile)->file = fopen (filename, mode);

    retval = sc_io_error_class (errno, &errcode);
    SC_CHECK_MPI (retval);

    return errcode;
  }
#endif
}

#ifdef SC_ENABLE_MPIIO

void
sc_io_read (sc_MPI_File mpifile, void *ptr, size_t zcount,
            sc_MPI_Datatype t, const char *errmsg)
{
#ifdef SC_ENABLE_DEBUG
  int                 icount;
#endif
  int                 mpiret;
  sc_MPI_Status       mpistatus;

  mpiret = MPI_File_read (mpifile, ptr, (int) zcount, t, &mpistatus);
  SC_CHECK_ABORT (mpiret == sc_MPI_SUCCESS, errmsg);

#ifdef SC_ENABLE_DEBUG
  mpiret = sc_MPI_Get_count (&mpistatus, t, &icount);
  SC_CHECK_MPI (mpiret);
  SC_CHECK_ABORT (icount == (int) zcount, errmsg);
#endif
}

#endif

int
sc_io_read_at (sc_MPI_File mpifile, sc_MPI_Offset offset, void *ptr,
               int zcount, sc_MPI_Datatype t, int *ocount)
{
#ifndef SC_ENABLE_MPIIO
  int                 size;
#endif
  int                 mpiret, errcode, retval;

  *ocount = 0;

#ifdef SC_ENABLE_MPIIO
  sc_MPI_Status       mpistatus;

  mpiret = MPI_File_read_at (mpifile, offset, ptr, zcount, t, &mpistatus);
  if (mpiret == sc_MPI_SUCCESS) {
    mpiret = sc_MPI_Get_count (&mpistatus, t, ocount);
    SC_CHECK_MPI (mpiret);
    return sc_MPI_SUCCESS;
  }
  retval = sc_io_error_class (mpiret, &errcode);
  SC_CHECK_MPI (retval);
  return errcode;
#else
  mpiret = fseek (mpifile->file, offset, SEEK_SET);
  SC_CHECK_ABORT (mpiret == 0, "read_at: fseek failed");
  /* get the data size of the data type */
  mpiret = sc_MPI_Type_size (t, &size);
  SC_CHECK_ABORT (mpiret == sc_MPI_SUCCESS, "read_at: get type size failed");
  errno = 0;
  *ocount = (int) fread (ptr, (size_t) size, zcount, mpifile->file);
  retval = sc_io_error_class (errno, &errcode);
  SC_CHECK_MPI (retval);
  return errcode;
#endif
}

int
sc_io_read_at_all (sc_MPI_File mpifile, sc_MPI_Offset offset, void *ptr,
                   int zcount, sc_MPI_Datatype t, int *ocount)
{
#ifdef SC_ENABLE_MPI
  int                 mpiret, errcode;
#endif
#ifdef SC_ENABLE_MPIIO
  int                 retval;
  sc_MPI_Status       mpistatus;

  *ocount = 0;

  if (zcount == 0) {
    /* This should be not necessary according to the MPI standard but some
     * MPI impementations trigger valgrind warnigs due to uninitialized
     * MPI status members.
     */
    mpiret = sc_MPI_SUCCESS;

    retval = sc_io_error_class (mpiret, &errcode);
    SC_CHECK_MPI (retval);

    return errcode;
  }

  mpiret = MPI_File_read_at_all (mpifile, offset, ptr,
                                 (int) zcount, t, &mpistatus);
  if (mpiret == sc_MPI_SUCCESS) {
    mpiret = sc_MPI_Get_count (&mpistatus, t, ocount);
    SC_CHECK_MPI (mpiret);

    return sc_MPI_SUCCESS;
  }

  retval = sc_io_error_class (mpiret, &errcode);
  SC_CHECK_MPI (retval);

  return errcode;
#elif defined (SC_ENABLE_MPI)
  /* MPI but no MPI IO */
  {
    int                 mpisize, rank, count, size;
    int                 active, errval, retval;
    sc_MPI_Status       status;

    *ocount = 0;

    mpiret = sc_MPI_Comm_rank (mpifile->mpicomm, &rank);
    SC_CHECK_MPI (mpiret);
    mpiret = sc_MPI_Comm_size (mpifile->mpicomm, &mpisize);
    SC_CHECK_MPI (mpiret);

    /* initially only rank 0 writes to the disk */
    active = (rank == 0) ? -1 : 0;

    /* initialize potential return value */
    errval = sc_MPI_SUCCESS;

    if (rank != 0) {
      /* wait until the preceding process finished the I/O operation */
      /* receive */
      mpiret = sc_MPI_Recv (&active, 1, sc_MPI_INT,
                            rank - 1, sc_MPI_ANY_TAG,
                            mpifile->mpicomm, &status);
      SC_CHECK_MPI (mpiret);
      mpiret = sc_MPI_Get_count (&status, sc_MPI_INT, &count);
      SC_CHECK_MPI (mpiret);
      SC_CHECK_ABORT (count == 1, "MPI receive");
    }

    /* active == -1 means process is active */
    if (active == -1) {
      /* process 0 must not wait for other processes */
      if (rank != 0) {
        /* open the file */
        errno = 0;
        mpifile->file = fopen (mpifile->filename, "rb");
        errval = errno;
        if (errval != 0) {
          /* it occurred an error */
          SC_ASSERT (errval > 0);
          if (rank < mpisize - 1) {
            active = errval;
            mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                                  rank + 1, 1, mpifile->mpicomm);
            SC_CHECK_MPI (mpiret);
          }
          goto failure;
        }
      }

      /* file was opened successfully */
      /* get the data size */
      mpiret = MPI_Type_size (t, &size);
      SC_CHECK_ABORT (mpiret == 0, "read_at_all: get type size failed");
      mpiret = fseek (mpifile->file, offset, SEEK_SET);
      SC_CHECK_ABORT (mpiret == 0, "read_at_all: seek failed");
      /* read data */
      errno = 0;
      *ocount = (int) fread (ptr, (size_t) size, zcount, mpifile->file);
      errval = errno;
      /* the consecutive error codes fflush and fclose are not reported */
      SC_CHECK_ABORT (fflush (mpifile->file) == 0,
                      "read_at_all: fflush failed");
      SC_CHECK_ABORT (fclose (mpifile->file) == 0,
                      "read_at_all: fclose failed");
      if (errval != 0) {
        /* error during write call */
        if (rank < mpisize - 1) {
          active = errval;
          /* inform next rank about write error */
          mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                                rank + 1, 1, mpifile->mpicomm);
          SC_CHECK_MPI (mpiret);
        }
        goto failure;
      }
      else {
        /* only update active process if there are processes left */
        if (rank < mpisize - 1) {
          SC_ASSERT (active == -1);
          /* send active flag to the next processor */
          mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                                rank + 1, 1, mpifile->mpicomm);
          SC_CHECK_MPI (mpiret);
        }
      }
    }
    else if (active > 0) {
      /** fopen failed for the last process and active is the errno.
       * We propagate the errno to all subsequent processes.
       */
      if (rank < mpisize - 1) {
        mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                              rank + 1, 1, mpifile->mpicomm);
        SC_CHECK_MPI (mpiret);
      }
    }
    else {
      SC_ABORT_NOT_REACHED ();
    }

  failure:
    /* The processes have to wait here because they are not allowed to start
     * other I/O operations.
     */
    sc_MPI_Barrier (mpifile->mpicomm);

    /** The following code restores the open status of the file and
     * we assume that the user checked the return value of \ref sc_io_open.
     * Therefore, we assume that is possible that we can open the file on
     * process 0 again. However, the opening operation below could fail due
     * to changed exterior conditions given by the file system.
     * We abort in this case since we expect the user to check the return
     * values and having constant file system conditions during the
     * program execution.
     */
    if (rank == 0) {
      /* open the file on rank 0 to be ready for the next file_read call */
      mpifile->file = fopen (mpifile->filename, "rb");
      errval = errno;
      if (errval != 0) {
        /* it occurred an error */
        SC_ASSERT (errval > 0);
        SC_ABORT ("sc_mpi_read_at_all: rank 0 open failed");
      }
    }
    else {
      mpifile->file = sc_MPI_FILE_NULL;
    }

    /* last rank broadcasts the first error that appeared */
    sc_MPI_Bcast (&errval, 1, sc_MPI_INT, mpisize - 1, mpifile->mpicomm);

    retval = sc_io_error_class (errval, &errcode);
    SC_CHECK_MPI (retval);

    return errcode;
  }
#else
  /* There is no collective read without MPI. */
  return sc_io_read_at (mpifile, offset, ptr, zcount, t, ocount);
#endif
}

int
sc_io_read_all (sc_MPI_File mpifile, void *ptr, int zcount, sc_MPI_Datatype t,
                int *ocount)
{
  return sc_io_read_at_all (mpifile, 0, ptr, zcount, t, ocount);
}

#ifdef SC_ENABLE_MPIIO

void
sc_io_write (sc_MPI_File mpifile, const void *ptr, size_t zcount,
             sc_MPI_Datatype t, const char *errmsg)
{
#ifdef SC_ENABLE_DEBUG
  int                 icount;
#endif
  int                 mpiret;
  sc_MPI_Status       mpistatus;

  mpiret = MPI_File_write (mpifile, (void *) ptr,
                           (int) zcount, t, &mpistatus);
  SC_CHECK_ABORT (mpiret == sc_MPI_SUCCESS, errmsg);

#ifdef SC_ENABLE_DEBUG
  mpiret = sc_MPI_Get_count (&mpistatus, t, &icount);
  SC_CHECK_MPI (mpiret);
  SC_CHECK_ABORT (icount == (int) zcount, errmsg);
#endif
}

#endif

int
sc_io_write_at (sc_MPI_File mpifile, sc_MPI_Offset offset,
                const void *ptr, size_t zcount, sc_MPI_Datatype t,
                int *ocount)
{
  int                 retval, errcode;
#ifndef SC_ENABLE_MPIIO
  int                 size;
#endif
  int                 mpiret;
#ifdef SC_ENABLE_MPIIO
  sc_MPI_Status       mpistatus;

  *ocount = 0;

  mpiret = MPI_File_write_at (mpifile, offset, (void *) ptr,
                              (int) zcount, t, &mpistatus);
  if (mpiret == sc_MPI_SUCCESS) {
    mpiret = sc_MPI_Get_count (&mpistatus, t, ocount);
    SC_CHECK_MPI (mpiret);
    return sc_MPI_SUCCESS;
  }

  retval = sc_io_error_class (mpiret, &errcode);
  SC_CHECK_MPI (retval);

  return errcode;
#else
  *ocount = 0;

  /* This code is only legal on one process. */
  /* This works with and without MPI */
  mpiret = fseek (mpifile->file, offset, SEEK_SET);
  SC_CHECK_ABORT (mpiret == 0, "write_at: fseek failed");
  /* get the data size of the data type */
  mpiret = sc_MPI_Type_size (t, &size);
  SC_CHECK_ABORT (mpiret == sc_MPI_SUCCESS, "write_at: get type size failed");
  errno = 0;
  *ocount = (int) fwrite (ptr, (size_t) size, zcount, mpifile->file);
  mpiret = errno;
  SC_CHECK_ABORT (fflush (mpifile->file) == 0, "write_at: fflush failed");
  retval = sc_io_error_class (mpiret, &errcode);
  SC_CHECK_MPI (retval);
  return errcode;
#endif
}

int
sc_io_write_at_all (sc_MPI_File mpifile, sc_MPI_Offset offset,
                    const void *ptr, size_t zcount, sc_MPI_Datatype t,
                    int *ocount)
{
#ifdef SC_ENABLE_MPI
  int                 mpiret, errcode, retval;
#endif
#ifdef SC_ENABLE_MPIIO
  sc_MPI_Status       mpistatus;

  *ocount = 0;

  if (zcount == 0) {
    /* This should be not necessary according to the MPI standard but some
     * MPI impementations trigger valgrind warnigs due to uninitialized
     * MPI status members.
     */
    mpiret = sc_MPI_SUCCESS;

    retval = sc_io_error_class (mpiret, &errcode);
    SC_CHECK_MPI (retval);

    return errcode;
  }

  mpiret = MPI_File_write_at_all (mpifile, offset, (void *) ptr,
                                  (int) zcount, t, &mpistatus);
  if (mpiret == sc_MPI_SUCCESS) {
    mpiret = sc_MPI_Get_count (&mpistatus, t, ocount);
    SC_CHECK_MPI (mpiret);
    return sc_MPI_SUCCESS;
  }

  retval = sc_io_error_class (mpiret, &errcode);
  SC_CHECK_MPI (retval);

  return errcode;
#elif defined (SC_ENABLE_MPI)
  /* MPI but no MPI IO */
  /* offset is ignored and we use here the append mode.
   * This is the case since the C-standard open mode
   * "wb" would earse the existing file and create a
   * new empty one. Therefore, we need to open the
   * file in the mode "ab" but according to the
   * C-standard then fseek does not work.
   */
  {
    int                 mpisize, rank, count, size;
    int                 active, errval;
    sc_MPI_Status       status;

    *ocount = 0;

    mpiret = sc_MPI_Comm_rank (mpifile->mpicomm, &rank);
    SC_CHECK_MPI (mpiret);
    mpiret = sc_MPI_Comm_size (mpifile->mpicomm, &mpisize);
    SC_CHECK_MPI (mpiret);

    /* initially only rank 0 writes to the disk */
    active = (rank == 0) ? -1 : 0;

    /* initialize potential return value */
    errval = sc_MPI_SUCCESS;

    if (rank != 0) {
      /* wait until the preceding process finished the I/O operation */
      /* receive */
      mpiret = sc_MPI_Recv (&active, 1, sc_MPI_INT,
                            rank - 1, sc_MPI_ANY_TAG,
                            mpifile->mpicomm, &status);
      SC_CHECK_MPI (mpiret);
      mpiret = sc_MPI_Get_count (&status, sc_MPI_INT, &count);
      SC_CHECK_MPI (mpiret);
      SC_CHECK_ABORT (count == 1, "MPI receive");
    }

    /* active == -1 means process is active */
    if (active == -1) {
      /* process 0 must not wait for other processes */
      if (rank != 0) {
        /* open the file */
        errno = 0;
        mpifile->file = fopen (mpifile->filename, "ab");
        errval = errno;
        if (errval != 0) {
          /* it occurred an error */
          SC_ASSERT (errval > 0);
          if (rank < mpisize - 1) {
            active = errval;
            mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                                  rank + 1, 1, mpifile->mpicomm);
            SC_CHECK_MPI (mpiret);
          }
          goto failure;
        }
      }

      /* file was opened successfully */
      /* get the data size */
      mpiret = MPI_Type_size (t, &size);
      SC_CHECK_ABORT (mpiret == 0, "write_at_all: get type size failed");
      /* write data */
      errno = 0;
      *ocount = (int) fwrite (ptr, (size_t) size, zcount, mpifile->file);
      errval = errno;
      /* the consecutive error codes fflush and fclose are not reported */
      SC_CHECK_ABORT (fflush (mpifile->file) == 0,
                      "write_at_all: fflush failed");
      SC_CHECK_ABORT (fclose (mpifile->file) == 0,
                      "write_at_all: fclose failed");
      if (errval != 0) {
        /* error during write call */
        if (rank < mpisize - 1) {
          active = errval;
          /* inform next rank about write error */
          mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                                rank + 1, 1, mpifile->mpicomm);
          SC_CHECK_MPI (mpiret);
        }
        goto failure;
      }
      else {
        /* only update active process if there are processes left */
        if (rank < mpisize - 1) {
          SC_ASSERT (active == -1);
          /* send active flag to the next processor */
          mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                                rank + 1, 1, mpifile->mpicomm);
          SC_CHECK_MPI (mpiret);
        }
      }
    }
    else if (active > 0) {
      /** fopen failed for the last process and active is the errno.
       * We propagate the errno to all subsequent processes.
       */
      if (rank < mpisize - 1) {
        mpiret = sc_MPI_Send (&active, 1, sc_MPI_INT,
                              rank + 1, 1, mpifile->mpicomm);
        SC_CHECK_MPI (mpiret);
      }
    }
    else {
      SC_ABORT_NOT_REACHED ();
    }

  failure:
    /* The processes have to wait here because they are not allowed to start
     * other I/O operations.
     */
    sc_MPI_Barrier (mpifile->mpicomm);

    /** The following code restores the open status of the file and
     * we assume that the user checked the return value of \ref sc_io_open.
     * Therefore, we assume that is possible that we can open the file on
     * process 0 again. However, the opening operation below could fail due
     * to changed exterior conditions given by the file system.
     * We abort in this case since we expect the user to check the return
     * values and having constant file system conditions during the
     * program execution.
     */
    if (rank == 0) {
      /* open the file on rank 0 to be ready for the next file_write call */
      errno = 0;
      mpifile->file = fopen (mpifile->filename, "ab");
      errval = errno;
      if (errval != 0) {
        /* it occurred an error */
        SC_ASSERT (errval > 0);
        SC_ABORT ("sc_mpi_write_at_all: rank 0 open failed");
      }
    }
    else {
      mpifile->file = sc_MPI_FILE_NULL;
    }

    /* last rank broadcasts the first error that appeared */
    sc_MPI_Bcast (&errval, 1, sc_MPI_INT, mpisize - 1, mpifile->mpicomm);

    retval = sc_io_error_class (errval, &errcode);
    SC_CHECK_MPI (retval);

    return errcode;
  }
#else
  /* There is no collective write without MPI. */
  return sc_io_write_at (mpifile, offset, ptr, zcount, t, ocount);
#endif
}

int
sc_io_write_all (sc_MPI_File mpifile, const void *ptr, size_t zcount,
                 sc_MPI_Datatype t, int *ocount)
{
  return sc_io_write_at_all (mpifile, 0, ptr, zcount, t, ocount);
}

int
sc_io_close (sc_MPI_File * file)
{
  SC_ASSERT (file != NULL);

  int                 mpiret;
  int                 eclass;
#if defined (SC_ENABLE_MPI) && defined (SC_ENABLE_DEBUG) && !defined (SC_ENABLE_MPIIO)
  int                 rank;
#endif

#ifdef SC_ENABLE_MPIIO
  mpiret = MPI_File_close (file);
  mpiret = sc_io_error_class (mpiret, &eclass);
  SC_CHECK_MPI (mpiret);
#else
  if ((*file)->file != NULL) {
#ifdef SC_ENABLE_DEBUG
#ifdef SC_ENABLE_MPI
    /* by convention this can only happen on proc 0 */
    mpiret = sc_MPI_Comm_rank ((*file)->mpicomm, &rank);
    SC_CHECK_MPI (mpiret);
    SC_ASSERT (rank == 0);
#endif
#endif
    eclass = sc_MPI_SUCCESS;
    errno = 0;
    fclose ((*file)->file);
    mpiret = sc_io_error_class (errno, &eclass);
    SC_CHECK_MPI (mpiret);
  }
  else {
    eclass = sc_MPI_SUCCESS;
  }
  SC_FREE (*file);
#endif

  return eclass;
}
