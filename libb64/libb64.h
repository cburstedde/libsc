
/*
 * adapted from libb64 by CB
 */

/* #define SC_BASE64_WRAP */

#include <stdlib.h>

/*
cdecode.h - c header for a base64 decoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#ifndef BASE64_CDECODE_H
#define BASE64_CDECODE_H

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

typedef enum
{
  step_a, step_b, step_c, step_d
}
base64_decodestep;

typedef struct
{
  base64_decodestep   step;
  char                plainchar;
}
base64_decodestate;

void                base64_init_decodestate (base64_decodestate * state_in);
int                 base64_decode_value (char value_in);
size_t              base64_decode_block (const char *code_in,
                                         size_t length_in,
                                         char *plaintext_out,
                                         base64_decodestate * state_in);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* BASE64_CDECODE_H */

/*
cencode.h - c header for a base64 encoding algorithm

This is part of the libb64 project, and has been placed in the public domain.
For details, see http://sourceforge.net/projects/libb64
*/

#ifndef BASE64_CENCODE_H
#define BASE64_CENCODE_H

#ifdef __cplusplus
extern              "C"
{
#if 0
}
#endif
#endif

typedef enum
{
  step_A, step_B, step_C
}
base64_encodestep;

typedef struct
{
  base64_encodestep   step;
  char                result;
  int                 stepcount;
}
base64_encodestate;

void                base64_init_encodestate (base64_encodestate * state_in);
char                base64_encode_value (char value_in);
size_t              base64_encode_block (const char *plaintext_in,
                                         size_t length_in, char *code_out,
                                         base64_encodestate * state_in);
size_t              base64_encode_blockend (char *code_out,
                                            base64_encodestate * state_in);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* BASE64_CENCODE_H */
