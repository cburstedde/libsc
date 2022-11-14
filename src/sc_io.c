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
#include <sc_puff.h>
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
sc_io_sink_complete (sc_io_sink_t * sink, size_t *bytes_in, size_t *bytes_out)
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
                   size_t bytes_avail, size_t *bytes_out)
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
                       size_t *bytes_in, size_t *bytes_out)
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
                          size_t bytes_avail, size_t *bytes_out)
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
sc_io_have_zlib (void)
{
#ifndef SC_HAVE_ZLIB
  return 0;
#else
  return 1;
#endif
}

/* byte count for one line of data must be a multiple of 3 */
#define SC_IO_DBC 54
#if SC_IO_DBC % 3 != 0
#error "SC_IO_DBC must be a multiple of 3"
#endif

/* byte count for one line of base64 encoded data and newline */
#define SC_IO_LBC (SC_IO_DBC / 3 * 4)
#define SC_IO_LBD (SC_IO_LBC + 1)
#define SC_IO_LBE (SC_IO_LBC + 2)

/* see RFC 1950 and RFC 1950 for the uncompressed zlib format */
#ifndef SC_HAVE_ZLIB
#define SC_IO_NONCOMP_BLOCK 65531       /**< +5 byte header = 64k */
#define SC_IO_ADLER32_PRIME 65521       /**< defined by RFC 1950 */

static void
sc_io_adler32_init (uint32_t *adler)
{
  SC_ASSERT (adler != NULL);
  *adler = 1;
}

static void
sc_io_adler32_update (uint32_t *adler, const char *buffer, size_t length)
{
  size_t              iz;
  int16_t             cn;
  uint32_t            s1, s2;

  SC_ASSERT (adler != NULL);
  s1 = (*adler) & 0xFFFFU;
  s2 = (*adler) >> 16;

  cn = 0;
  for (iz = 0; iz < length; ++iz) {
    unsigned char       uc = (unsigned char) buffer[iz];
    if (cn == 5000) {
      s1 = s1 % SC_IO_ADLER32_PRIME;
      s2 = s2 % SC_IO_ADLER32_PRIME;
      cn = 0;
    }
    s1 += uc;
    s2 += s1;
    ++cn;
  }
  *adler = s2 % SC_IO_ADLER32_PRIME;
  *adler = (*adler << 16) + (s1 % SC_IO_ADLER32_PRIME);
}

static size_t
sc_io_noncompress_bound (size_t length)
{
  size_t              num_blocks =
    (length + (SC_IO_NONCOMP_BLOCK - 1)) / SC_IO_NONCOMP_BLOCK;

  return 2 + 5 * SC_MAX (num_blocks, 1) + length + 4;
}

static void
sc_io_noncompress (char *dest, size_t dest_size,
                   const char *src, size_t src_size)
{
  uint16_t            bsize, nsize;
  uint32_t            adler;

  /* write zlib format header */
  SC_ASSERT (dest_size >= 2);
  dest[0] = (7 << 4) + 8;
  dest[1] = 1;
  dest += 2;
  dest_size -= 2;

  /* prepare checksum */
  sc_io_adler32_init (&adler);

  /* write individual non-compressed blocks */
  do {
    /* write block header */
    SC_ASSERT (dest_size >= 5);
    if (src_size > SC_IO_NONCOMP_BLOCK) {
      /* not the final block */
      bsize = SC_IO_NONCOMP_BLOCK;
      dest[0] = 0;
    }
    else {
      /* last block */
      bsize = src_size;
      dest[0] = 1;
    }
    nsize = ~bsize;
    dest[1] = (char) (bsize & 0xFF);
    dest[2] = (char) (bsize >> 8);
    dest[3] = (char) (nsize & 0xFF);
    dest[4] = (char) (nsize >> 8);
    dest += 5;
    dest_size -= 5;

    /* copy data */
    SC_ASSERT (dest_size >= bsize);
    SC_ASSERT (src_size >= bsize);
    memcpy (dest, src, bsize);
    dest += bsize;
    dest_size -= bsize;

    /* extend adler32 checksum */
    sc_io_adler32_update (&adler, src, bsize);
    src += bsize;
    src_size -= bsize;
  }
  while (src_size > 0);

  /* write adler32 checksum */
  SC_ASSERT (src_size == 0);
  SC_ASSERT (dest_size == 4);
  dest[0] = (char) (adler >> 24);
  dest[1] = (char) ((adler >> 16) & 0xFF);
  dest[2] = (char) ((adler >> 8) & 0xFF);
  dest[3] = (char) (adler & 0xFF);
}

static int
sc_io_nonuncompress (char *dest, size_t dest_size,
                     const char *src, size_t src_size)
{
  int                 final_block;
  uint32_t            adler;
  unsigned char       uca, ucb;
#ifndef SC_PUFF_INCLUDED
  uint16_t            bsize, nsize;
#else
  unsigned long       destlen, sourcelen;
#endif

  /* check zlib format header */
  if (src_size < 2) {
    SC_LERROR ("uncompress header short\n");
    return -1;
  }
  uca = (unsigned char) src[0];
  if ((uca & 0x8F) != 8) {
    SC_LERROR ("uncompress method unsupported\n");
    return -1;
  }
  ucb = (unsigned char) src[1];
  if (((((unsigned) uca) << 8) + ucb) % 31) {
    SC_LERROR ("uncompress header not conforming\n");
    return -1;
  }
  if (ucb & 0x20) {
    SC_LERROR ("uncompress cannot handle dictionary\n");
    return -1;
  }
  src += 2;
  src_size -= 2;

  /* prepare checksum */
  sc_io_adler32_init (&adler);

  /* go through zlib blocks */
  do {
    /* verify minimum required size */
    if (src_size < 5) {
      SC_LERROR ("uncompress block header short\n");
      return -1;
    }
#ifdef SC_PUFF_INCLUDED

    /* use the builtin puff fallback to decompress deflate data */
    destlen = (unsigned long) dest_size;
    sourcelen = (unsigned long) src_size - 4;
    if (sc_puff ((unsigned char *) dest, &destlen,
                 (const unsigned char *) src, &sourcelen)) {
      SC_LERROR ("uncompress by puff failed\n");
      return -1;
    }
    if (destlen != (unsigned long) dest_size ||
        sourcelen != (unsigned long) src_size - 4) {
      SC_LERROR ("uncompress by puff mismatch\n");
      return -1;
    }

    /* extend adler32 checksum */
    sc_io_adler32_update (&adler, dest, dest_size);
    src += sourcelen;
    src_size = 4;
    dest += destlen;
    dest_size = 0;
    final_block = 1;
#else

    /* examine block header */
    uca = (unsigned char) src[0];
    if (uca > 1) {
      SC_LERROR ("uncompress block header unsupported\n");
      return -1;
    }
    final_block = (uca == 1);
    uca = (unsigned char) src[1];
    ucb = (unsigned char) src[2];
    bsize = (ucb << 8) + uca;
    uca = (unsigned char) src[3];
    ucb = (unsigned char) src[4];
    nsize = (ucb << 8) + uca;
    if ((final_block && bsize < dest_size) || bsize + nsize != 65535) {
      SC_LERROR ("uncompress block header invalid\n");
      return -1;
    }
    src += 5;
    src_size -= 5;

    /* copy data */
    if (bsize > dest_size || bsize > src_size) {
      SC_LERROR ("uncompress content overflow\n");
      return -1;
    }
    memcpy (dest, src, bsize);
    src += bsize;
    src_size -= bsize;

    /* extend adler32 checksum */
    sc_io_adler32_update (&adler, dest, bsize);
    dest += bsize;
    dest_size -= bsize;
#endif
  }
  while (!final_block);
  if (src_size != 4 || dest_size != 0) {
    SC_LERROR ("uncompress content error\n");
    return -1;
  }

  /* verify adler32 checksum */
  if (src[0] != (char) (adler >> 24) ||
      src[1] != (char) ((adler >> 16) & 0xFF) ||
      src[2] != (char) ((adler >> 8) & 0xFF) ||
      src[3] != (char) (adler & 0xFF)) {
    SC_LERROR ("uncompress checksum error\n");
    return -1;
  }

  /* task accomplished */
  return 0;
}

#endif /* !SC_HAVE_ZLIB */

#define SC_IO_ENCODE_INFO_LEN 9

void
sc_io_encode (sc_array_t *data, sc_array_t *out)
{
  sc_io_encode_zlib (data, out, Z_BEST_COMPRESSION);
}

void
sc_io_encode_zlib (sc_array_t *data, sc_array_t *out,
                   int zlib_compression_level)
{
  int                 i;
  size_t              input_size;
#ifndef SC_HAVE_ZLIB
  size_t              input_compress_bound;
#else
  int                 zrv;
  uLong               input_compress_bound;
#endif
  char               *ipos, *opos;
  char                base_out[2 * SC_IO_LBC];
  size_t              base64_lines;
  size_t              encoded_size;
  size_t              zlin, irem;
#ifdef SC_ENABLE_DEBUG
  size_t              ocnt;
#endif
  unsigned char       original_size[SC_IO_ENCODE_INFO_LEN];
  sc_array_t          compressed;
  base64_encodestate  bstate;

  SC_ASSERT (data != NULL);
  if (out == NULL) {
    /* in-place operation on string */
    SC_ASSERT (SC_ARRAY_IS_OWNER (data));
    SC_ASSERT (data->elem_size == 1);
  }
  else {
    /* data is placed in output string */
    SC_ASSERT (SC_ARRAY_IS_OWNER (out));
    SC_ASSERT (out->elem_size == 1);
  }
#ifdef SC_HAVE_ZLIB
  SC_ASSERT (zlib_compression_level == Z_DEFAULT_COMPRESSION ||
             (zlib_compression_level >= 0 && zlib_compression_level <= 9));
#endif

  /* save original size to output */
  input_size = data->elem_count * data->elem_size;
  for (i = 0; i < 8; ++i) {
    /* enforce big endian byte order for original size */
    original_size[i] = (input_size >> ((7 - i) * 8)) & 0xFF;
  }
  original_size[SC_IO_ENCODE_INFO_LEN - 1] = 'z';

  /* zlib compress input */
#ifndef SC_HAVE_ZLIB
  input_compress_bound = sc_io_noncompress_bound (input_size);
#else
  input_compress_bound = compressBound ((uLong) input_size);
#endif /* SC_HAVE_ZLIB */
  sc_array_init_count (&compressed, 1,
                       SC_IO_ENCODE_INFO_LEN + input_compress_bound);
  memcpy (compressed.array, original_size, SC_IO_ENCODE_INFO_LEN);
#ifndef SC_HAVE_ZLIB
  sc_io_noncompress (compressed.array + SC_IO_ENCODE_INFO_LEN,
                     input_compress_bound, data->array, input_size);
#else
  zrv = compress2 ((Bytef *) compressed.array + SC_IO_ENCODE_INFO_LEN,
                   &input_compress_bound, (Bytef *) data->array,
                   (uLong) input_size, zlib_compression_level);
  SC_CHECK_ABORT (zrv == Z_OK, "Error on zlib compression");
#endif /* SC_HAVE_ZLIB */

  /* prepare output array */
  if (out == NULL) {
    out = data;
  }
  SC_ASSERT (out->elem_size == 1);
  input_size = (size_t) (SC_IO_ENCODE_INFO_LEN + input_compress_bound);
  base64_lines = (input_size + SC_IO_DBC - 1) / SC_IO_DBC;
  encoded_size = 4 * ((input_size + 2) / 3) + base64_lines + 1;
  sc_array_resize (out, encoded_size);

  /* run base64 encoder */
  base64_init_encodestate (&bstate);
  ipos = compressed.array;
  irem = input_size;
  opos = out->array;
#ifdef SC_ENABLE_DEBUG
  ocnt = 0;
#endif
  SC_ASSERT (ocnt + 1 <= encoded_size);
  opos[0] = '\0';
  for (zlin = 0; zlin < base64_lines; ++zlin) {
    size_t              lein = SC_MIN (irem, SC_IO_DBC);
    size_t              lout =
      base64_encode_block (ipos, lein, base_out, &bstate);

    SC_ASSERT (lein > 0);
    if (zlin < base64_lines - 1) {
      /* not the final line */
      SC_ASSERT (irem > SC_IO_DBC);
      SC_ASSERT (lout == SC_IO_LBC);
      SC_ASSERT (ocnt + SC_IO_LBE <= encoded_size);
      memcpy (opos, base_out, SC_IO_LBC);
      opos[SC_IO_LBC] = '\n';
      opos[SC_IO_LBD] = '\0';
      opos += SC_IO_LBD;
#ifdef SC_ENABLE_DEBUG
      ocnt += SC_IO_LBD;
#endif
      ipos += SC_IO_DBC;
      irem -= SC_IO_DBC;
    }
    else {
      /* the final line */
      SC_ASSERT (irem <= SC_IO_DBC);
      SC_ASSERT (lout <= SC_IO_LBC);
      SC_ASSERT (ocnt + lout <= encoded_size);
      memcpy (opos, base_out, lout);
      opos += lout;
#ifdef SC_ENABLE_DEBUG
      ocnt += lout;
#endif
      lout = base64_encode_blockend (base_out, &bstate);
      SC_ASSERT (lout <= 4);
      SC_ASSERT (ocnt + lout <= encoded_size);
      memcpy (opos, base_out, lout);
      opos += lout;
#ifdef SC_ENABLE_DEBUG
      ocnt += lout;
#endif
      SC_ASSERT (ocnt + 2 <= encoded_size);
      opos[0] = '\n';
      opos[1] = '\0';
      opos = NULL;
#ifdef SC_ENABLE_DEBUG
      ocnt += 2;
#endif
      SC_ASSERT (ocnt == encoded_size);
      ipos = NULL;
      irem = 0;
    }
  }

  /* free temporary memory */
  sc_array_reset (&compressed);
}

int
sc_io_decode_info (sc_array_t *data,
                   size_t *original_size, char *format_char)
{
  int                 i;
  size_t              osize;
  char                dec[12];
  base64_decodestate  bstate;

  SC_ASSERT (SC_IO_ENCODE_INFO_LEN == 9);
  SC_ASSERT (data != NULL);
  SC_ASSERT (data->elem_size == 1);
  if (data->elem_count < 12) {
    SC_LERROR ("sc_io_decode_info requires >= 12 bytes of input\n");
    return -1;
  }

  /* decode first 12 characters of encoded data */
  memset (dec, 0, 12);
  base64_init_decodestate (&bstate);
  osize = base64_decode_block (data->array, 12, dec, &bstate);
  if (osize != 9) {
    SC_LERROR ("sc_io_decode_info base 64 error\n");
    return -1;
  }

#if 0
  /* verify first byte for zlib format */
  if (dec[8] != 'z') {
    SC_LERROR ("sc_io_decode_info data format error\n");
    return -1;
  }
#endif

  /* decode original length of data */
  if (original_size != NULL) {
    unsigned char       uc;
    osize = 0;
    for (i = 0; i < 8; ++i) {
      /* read original byte order in big endian */
      uc = (unsigned char) dec[i];
      osize |= ((size_t) uc) << ((7 - i) * 8);
    }
    *original_size = osize;
  }

  /* return format character */
  if (format_char != NULL) {
    *format_char = dec[8];
  }

  /* success! */
  return 0;
}

int
sc_io_decode (sc_array_t *data, sc_array_t *out, size_t max_original_size)
{
  int                 i;
  int                 zrv;
  int                 retval = -1;
  char               *ipos, *opos;
  char                base_out[SC_IO_LBC];
  size_t              compressed_size;
  size_t              base64_lines;
  size_t              encoded_size;
  size_t              current_size;
  size_t              zlin, irem;
  size_t              ocnt;
#ifdef SC_HAVE_ZLIB
  uLong               uncompsize;
#endif
  sc_array_t          compressed;
  base64_decodestate  bstate;

  /* examine input data */
  SC_ASSERT (data != NULL);
  SC_ASSERT (data->elem_size == 1);
  encoded_size = data->elem_count;
  if (encoded_size == 0 ||
      *(char *) sc_array_index (data, encoded_size - 1) != '\0') {
    SC_LERROR ("input not NUL-terminated\n");
    return -1;
  }

  /* decode line by line from base 64 */
  base64_init_decodestate (&bstate);
  base64_lines = (encoded_size - 1 + SC_IO_LBC) / SC_IO_LBD;
  compressed_size = base64_lines * SC_IO_DBC;
  ipos = data->array;
  SC_ASSERT (encoded_size >= base64_lines + 1);
  irem = encoded_size - 1 - base64_lines;
  sc_array_init_count (&compressed, 1, compressed_size);
  opos = compressed.array;
  ocnt = 0;
  for (zlin = 0; zlin < base64_lines; ++zlin) {
    size_t              lein = SC_MIN (irem, SC_IO_LBC);
    size_t              lout =
      base64_decode_block (ipos, lein, base_out, &bstate);

    SC_ASSERT (lein > 0);
    if (lout == 0) {
      SC_LERROR ("base 64 decode short\n");
      goto decode_error;
    }
    if (ipos[lein] != '\n') {
      SC_LERROR ("base 64 missing newline\n");
      goto decode_error;
    }
    if (zlin < base64_lines - 1) {
      SC_ASSERT (lein == SC_IO_LBC);
      if (lout != SC_IO_DBC) {
        SC_LERROR ("base 64 decode mismatch\n");
        goto decode_error;
      }
      memcpy (opos, base_out, SC_IO_DBC);
      ipos += SC_IO_LBD;
      SC_ASSERT (irem >= SC_IO_LBC);
      irem -= SC_IO_LBC;
      opos += SC_IO_DBC;
      ocnt += SC_IO_DBC;
    }
    else {
      SC_ASSERT (lein <= SC_IO_LBC);
      SC_ASSERT (lout <= SC_IO_DBC);
      memcpy (opos, base_out, lout);
      ipos += lein + 1;
      SC_ASSERT (irem >= lein);
      irem -= lein;
      opos += lout;
      ocnt += lout;
    }
  }
  SC_ASSERT (irem == 0);
  SC_ASSERT (ocnt <= compressed_size);
  SC_ASSERT (ipos + 1 == data->array + encoded_size);
  if (ocnt < SC_IO_ENCODE_INFO_LEN) {
    SC_LERRORF ("base 64 decodes to less than %d bytes\n",
                SC_IO_ENCODE_INFO_LEN);
    goto decode_error;
  }
  if (compressed.array[SC_IO_ENCODE_INFO_LEN - 1] != 'z') {
    SC_LERROR ("encoded format character mismatch\n");
    goto decode_error;
  }

  /* determine length of uncompressed data */
  encoded_size = 0;
  for (i = 0; i < 8; ++i) {
    /* enforce big endian byte order for original size */
    unsigned char       uc = (unsigned char) compressed.array[i];
    encoded_size |= ((size_t) uc) << ((7 - i) * 8);
  }
  if (out == NULL) {
    /* allow for in-place operation */
    out = data;
  }
  if (encoded_size % out->elem_size != 0) {
    SC_LERROR ("encoded size not commensurable with output array\n");
    goto decode_error;
  }
  if (max_original_size > 0 && encoded_size > max_original_size) {
    SC_LERRORF ("encoded size %llu larger than specified maximum %llu\n",
                (unsigned long long) encoded_size,
                (unsigned long long) max_original_size);
    goto decode_error;
  }
  if (!SC_ARRAY_IS_OWNER (out) &&
      encoded_size > (current_size = out->elem_count * out->elem_size)) {
    SC_LERRORF ("encoded size %llu larger than byte size of view %llu\n",
                (unsigned long long) encoded_size,
                (unsigned long long) current_size);
    goto decode_error;
  }
  sc_array_resize (out, encoded_size / out->elem_size);

  /* decompress decoded data */
#ifndef SC_HAVE_ZLIB
  zrv = sc_io_nonuncompress (out->array, encoded_size,
                             compressed.array + SC_IO_ENCODE_INFO_LEN,
                             ocnt - SC_IO_ENCODE_INFO_LEN);
  if (zrv) {
    SC_LERROR ("Please consider configuring the build"
               " such that zlib is found.\n");
    goto decode_error;
  }
#else
  uncompsize = (uLong) encoded_size;
  zrv = uncompress ((Bytef *) out->array, &uncompsize,
                    (Bytef *) (compressed.array + SC_IO_ENCODE_INFO_LEN),
                    ocnt - SC_IO_ENCODE_INFO_LEN);
  if (zrv != Z_OK) {
    SC_LERROR ("zlib uncompress error\n");
    goto decode_error;
  }
  if (uncompsize != (uLong) encoded_size) {
    SC_LERROR ("zlib uncompress short\n");
    goto decode_error;
  }
#endif /* SC_HAVE_ZLIB */

  /* exit cleanly */
  retval = 0;
decode_error:
  sc_array_reset (&compressed);
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
#else /* no MPI */
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
