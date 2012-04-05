/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2012 Carsten Burstedde

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
#include <sc_options.h>

int
main (int argc, char **argv)
{
  int           mpiret, retval;
  int           first;
  const char *  filename;
  const char    input[] = "This is a string for sinking and sourcing.\n";
  sc_options_t *opt;
  sc_array_t   *buffer;
  sc_io_sink_t *sink;

  mpiret = MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  sc_init (MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  opt = sc_options_new (argv[0]);
  sc_options_add_string (opt, 'f', "filename", &filename, NULL,
                         "File to write");
  first = sc_options_parse (sc_package_id, SC_LP_INFO, opt, argc, argv);
  if (first < 0) {
    sc_options_print_usage (sc_package_id, SC_LP_INFO, opt, NULL);
    sc_abort_collective ("Usage error");
  }

  buffer = NULL;
  if (filename == NULL) {
    buffer = sc_array_new (sizeof (char));
    sink = sc_io_sink_new (SC_IO_SINK_BUFFER, SC_IO_MODE_WRITE,
                           SC_IO_ENCODE_NONE, buffer);
  }
  else {
    sink = sc_io_sink_new (SC_IO_SINK_FILENAME, SC_IO_MODE_WRITE,
                           SC_IO_ENCODE_NONE, filename);
  }
  SC_CHECK_ABORT (sink != NULL, "Sink create");

  retval = sc_io_sink_write (sink, input, strlen (input));
  SC_CHECK_ABORT (retval == 0, "Sink write");

  retval = sc_io_sink_destroy (sink);
  SC_CHECK_ABORT (retval == 0, "Sink destroy");

  if (filename == NULL) {
    sc_array_destroy (buffer);
  }

  sc_options_destroy (opt);
  sc_finalize ();

  return 0;
}
