/* puff.h
  Copyright (C) 2002-2013 Mark Adler, all rights reserved
  version 2.3, 21 Jan 2013

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler    madler@alumni.caltech.edu
 */

/* CB: we acknowledge that the origin of this software is the zlib library.
 *     This version is altered by renaming the puff function to sc_puff
 *     and moving the NIL #define into the .c file,
 *     as well as adding protection against multiple
 *     inclusion, an extern "C", and white space.
 */

#ifndef SC_PUFF_H
#define SC_PUFF_H

/* every libsc file includes at least sc_config.h */
#include <sc_config.h>

/* convenient define for code to fallback even further without puff */
#define SC_PUFF_INCLUDED

#ifdef __cplusplus
extern "C"
#endif

/*
 * See sc_puff.c for purpose and usage.
 */
int sc_puff (unsigned char *dest,           /* pointer to destination pointer */
             unsigned long *destlen,        /* amount of output space */
             const unsigned char *source,   /* pointer to source data pointer */
             unsigned long *sourcelen);     /* amount of input available */

#endif /* !SC_PUFF_H */
