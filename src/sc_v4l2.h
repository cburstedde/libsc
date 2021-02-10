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

/** \file sc_v4l2.h
 *
 * Support for video for linux version 2.
 */

#ifndef SC_V4L2_H
#define SC_V4L2_H

#include <sc.h>

SC_EXTERN_C_BEGIN;

typedef struct sc_v4l2_device sc_v4l2_device_t;

/** Open a video device by special file name.
 * \param [in] devname      Special file name such as `/dev/video8`.
 * \return                  Valid pointer on success, NULL on error.
 */
sc_v4l2_device_t   *sc_v4l2_device_open (const char *devname);

/** Close a video device.
 * \return          0 on sucess, -1 otherwise.
 */
int                 sc_v4l2_device_close (sc_v4l2_device_t * vd);

SC_EXTERN_C_END;

#endif /* SC_V4L2_H */
