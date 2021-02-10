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

#include <sc_v4l2.h>

#ifdef SC_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef SC_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef SC_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef SC_HAVE_UNISTD_H
#include <unistd.h>
#endif

struct sc_v4l2_device
{
  int                 fd;
};

sc_v4l2_device_t   *
sc_v4l2_device_open (const char *devname)
{
  sc_v4l2_device_t   *vd;

  SC_ASSERT (devname != NULL);

  vd = SC_ALLOC (struct sc_v4l2_device, 1);
  if (vd == NULL) {
    return NULL;
  }

  vd->fd = open (devname, O_RDWR);
  if (vd->fd < 0) {
    SC_FREE (vd);
    return NULL;
  }

  return vd;
}

int
sc_v4l2_device_close (sc_v4l2_device_t * vd)
{
  int                 retval;

  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);

  retval = close (vd->fd);
  if (retval) {
    return retval;
  }

  SC_FREE (vd);
  return 0;
}
