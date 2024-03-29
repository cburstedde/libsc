
/*
 * adapted from libb64 by CB
 */

/*
cencoder.c - c source to a base64 encoding algorithm implementation

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#include "libb64.h"

#ifdef SC_BASE64_WRAP
const int           CHARS_PER_LINE = 72;
#endif

static inline char
base64_encode_value (char value_in)
{
  static const char  *encoding =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  return value_in > 63 ? '=' : encoding[(int) value_in];
}

void
base64_init_encodestate (base64_encodestate * state_in)
{
  state_in->step = step_A;
  state_in->result = 0;
  state_in->stepcount = 0;
}

size_t
base64_encode_block (const char *plaintext_in, size_t length_in,
                     char *code_out, base64_encodestate * state_in)
{
  /*@unused@ */
  const char         *plainchar = plaintext_in;
  /*@unused@ */
  const char         *const plaintextend = plaintext_in + length_in;
  char               *codechar = code_out;
  char                result;
  /*@unused@ */
  char                fragment;

  result = state_in->result;

  switch (state_in->step) {

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

    while (1) {
  case step_A:
      if (plainchar == plaintextend) {
        state_in->result = result;
        state_in->step = step_A;
        return (size_t) (codechar - code_out);
      }
      fragment = *plainchar++;
      result = (char) ((fragment & 0x0fc) >> 2);
      *codechar++ = base64_encode_value (result);
      result = (char) ((fragment & 0x003) << 4);
  case step_B:
      if (plainchar == plaintextend) {
        state_in->result = result;
        state_in->step = step_B;
        return (size_t) (codechar - code_out);
      }
      fragment = *plainchar++;
      result = (char) (result | ((fragment & 0x0f0) >> 4));
      *codechar++ = base64_encode_value (result);
      result = (char) ((fragment & 0x00f) << 2);
  case step_C:
      if (plainchar == plaintextend) {
        state_in->result = result;
        state_in->step = step_C;
        return (size_t) (codechar - code_out);
      }
      fragment = *plainchar++;
      result = (char) (result | ((fragment & 0x0c0) >> 6));
      *codechar++ = base64_encode_value (result);
      result = (char) ((fragment & 0x03f) >> 0);
      *codechar++ = base64_encode_value (result);

      ++(state_in->stepcount);
      /* CB: disable wrapping by default */
#ifdef SC_BASE64_WRAP
      if (state_in->stepcount == CHARS_PER_LINE / 4) {
        *codechar++ = '\n';
        state_in->stepcount = 0;
      }
#endif
    }
  }
  /* control should not reach here */
  return (size_t) (codechar - code_out);
}

size_t
base64_encode_blockend (char *code_out, base64_encodestate * state_in)
{
  char               *codechar = code_out;

  switch (state_in->step) {
  case step_B:
    *codechar++ = base64_encode_value (state_in->result);
    *codechar++ = '=';
    *codechar++ = '=';
    break;
  case step_C:
    *codechar++ = base64_encode_value (state_in->result);
    *codechar++ = '=';
    break;
  case step_A:
    break;
  }
  /* CB: remove final newline by default */
#ifdef SC_BASE64_WRAP
  *codechar++ = '\n';
#endif

  return (size_t) (codechar - code_out);
}
