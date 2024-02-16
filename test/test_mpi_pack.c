/*
  This file is part of the SC Library.
  The SC Library provides support for parallel scientific applications.

  Copyright (C) 2020 individual authors

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <sc.h>
#include <sc_mpi.h>

/** Test the mpi pack and unpack function
 * Construct multiple test messages, each with different size double arrays depending on their type
 * Pack, Unpack and Compare
*/

#define TEST_NUM_TYPES 3
const int           num_values[TEST_NUM_TYPES] = { 2, 5, 6 };

typedef struct message
{
  int8_t              type;
  double             *values;
}
test_message_t;

void
test_message_construct (test_message_t *message, int8_t type,
                        double startvalue)
{
  int                 ival;
  message->type = type;
  message->values = SC_ALLOC (double, num_values[type]);
  for (ival = 0; ival < num_values[type]; ival++) {
    message->values[ival] = startvalue * ival;
  }
}

void
test_message_destroy (test_message_t *message)
{
  SC_FREE (message->values);
}

int
test_message_equal (test_message_t *message1, test_message_t *message2)
{
  if (message1->type != message2->type) {
    return 0;
  }
  for (int ivalue = 0; ivalue < num_values[message1->type]; ivalue++) {
    if (message1->values[ivalue] != message2->values[ivalue]) {
      return 0;
    }
  }
  return 1;
}

/* Pack several instances of test messages into contiguous memory */
int
test_message_MPI_Pack (const test_message_t *messages, int incount,
                       void *outbuf, int outsize, int *position,
                       sc_MPI_Comm comm)
{
  int                 imessage;
  for (imessage = 0; imessage < incount; imessage++) {
    int                 mpiret;
    mpiret =
      sc_MPI_Pack (&(messages[imessage].type), 1, sc_MPI_INT8_T, outbuf,
                   outsize, position, comm);
    SC_CHECK_MPI (mpiret);
    mpiret = sc_MPI_Pack (messages[imessage].values,
                          num_values[messages[imessage].type], sc_MPI_DOUBLE,
                          outbuf, outsize, position, comm);
    SC_CHECK_MPI (mpiret);
  }
  return 0;
}

/* Unpack contiguous memory into several instances of the same datatype */
int
test_message_MPI_Unpack (const void *inbuf, int insize,
                         int *position, test_message_t *messages,
                         int outcount, sc_MPI_Comm comm)
{
  int                 imessage;
  for (imessage = 0; imessage < outcount; imessage++) {
    int                 mpiret;
    mpiret =
      sc_MPI_Unpack (inbuf, insize, position, &(messages[imessage].type), 1,
                     sc_MPI_INT8_T, comm);
    SC_CHECK_MPI (mpiret);

    messages[imessage].values =
      SC_ALLOC (double, num_values[messages[imessage].type]);
    mpiret =
      sc_MPI_Unpack (inbuf, insize, position, messages[imessage].values,
                     num_values[messages[imessage].type], sc_MPI_DOUBLE,
                     comm);
    SC_CHECK_MPI (mpiret);
  }
  return 0;
}

/* Determine how much space in bytes is needed to pack several test messages */
int
test_message_MPI_Pack_size (int incount, const test_message_t *messages,
                            sc_MPI_Comm comm, int *size)
{
  *size = 0;
  int                 imessage;
  for (imessage = 0; imessage < incount; imessage++) {
    int                 pack_size;
    int                 single_message_size = 0;
    int                 mpiret;

    mpiret = sc_MPI_Pack_size (1, sc_MPI_INT8_T, comm, &pack_size);
    SC_CHECK_MPI (mpiret);
    single_message_size += pack_size;

    mpiret = sc_MPI_Pack_size (1, sc_MPI_DOUBLE, comm, &pack_size);
    SC_CHECK_MPI (mpiret);
    single_message_size += num_values[messages[imessage].type] * pack_size;

    *size += single_message_size;
  }
  return 0;
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  sc_MPI_Comm         mpicomm;

  int                 num_test_messages = 5;
  int                 imessage;
  test_message_t     *messages;
  test_message_t     *unpacked_messages;

  char               *pack_buffer;
  int                 buffer_size = 0;
  int                 position = 0;

  /* standard initialization */
  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);
  mpicomm = sc_MPI_COMM_WORLD;
  sc_init (mpicomm, 1, 1, NULL, SC_LP_DEFAULT);

  /* allocate and construct test messages */
  messages = SC_ALLOC (test_message_t, num_test_messages);
  for (imessage = 0; imessage < num_test_messages; imessage++) {
    test_message_construct (messages + imessage, imessage % TEST_NUM_TYPES,
                            imessage + 1.0);
  }

  /* get message size, pack, unpack and compare */
  test_message_MPI_Pack_size (num_test_messages, messages, mpicomm,
                              &buffer_size);

  pack_buffer = SC_ALLOC (char, buffer_size);
  test_message_MPI_Pack (messages, num_test_messages, pack_buffer,
                         buffer_size, &position, mpicomm);
  SC_CHECK_ABORT (position == buffer_size, "message not of full size");

  unpacked_messages = SC_ALLOC (test_message_t, num_test_messages);
  position = 0;
  test_message_MPI_Unpack (pack_buffer, buffer_size, &position,
                           unpacked_messages, num_test_messages, mpicomm);
  SC_CHECK_ABORT (position == buffer_size, "message not of full size");

  for (imessage = 0; imessage < num_test_messages; imessage++) {
    SC_CHECK_ABORT (test_message_equal
                    (messages + imessage, unpacked_messages + imessage),
                    "message do not equal");
  }

  /* free up memory */
  SC_FREE (pack_buffer);

  for (imessage = 0; imessage < num_test_messages; imessage++) {
    test_message_destroy (messages + imessage);
    test_message_destroy (unpacked_messages + imessage);
  }
  SC_FREE (messages);
  SC_FREE (unpacked_messages);

  /* clean up and exit */
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return EXIT_SUCCESS;
}
