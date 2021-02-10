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

#ifdef SC_HAVE_LINUX_VIDEODEV2_H
#include <linux/videodev2.h>
#endif
#ifdef SC_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef SC_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef SC_HAVE_SYS_TYPES_H
#include <sys/types.h>
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
  struct v4l2_capability capability;
  char                devstring[BUFSIZ];
  char                capstring[BUFSIZ];
};

static int
querycap (sc_v4l2_device_t * vd)
{
  int                 retval;
  __u32               caps;

  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);

  if ((retval = ioctl (vd->fd, VIDIOC_QUERYCAP, &vd->capability)) != 0) {
    return retval;
  }

  snprintf (vd->devstring, BUFSIZ, "Driver: %s Device: %s Bus: %s",
            vd->capability.driver, vd->capability.card,
            vd->capability.bus_info);

  if (!(vd->capability.capabilities & V4L2_CAP_DEVICE_CAPS)) {
    caps = vd->capability.capabilities;
  }
  else {
    caps = vd->capability.device_caps;
  }
  snprintf (vd->capstring, BUFSIZ, "Output: %d RW: %d Stream: %d MC: %d",
            caps & V4L2_CAP_VIDEO_OUTPUT ? 1 : 0,
            caps & V4L2_CAP_READWRITE ? 1 : 0,
            caps & V4L2_CAP_STREAMING ? 1 : 0, caps & V4L2_CAP_IO_MC ? 1 : 0);

  return 0;
}

sc_v4l2_device_t   *
sc_v4l2_device_open (const char *devname)
{
  int                 retval;
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

  if ((retval = querycap (vd)) != 0) {
    close (vd->fd);
    SC_FREE (vd);
    return NULL;
  }

  return vd;
}

const char         *
sc_v4l2_device_devstring (sc_v4l2_device_t * vd)
{
  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);
  return vd->devstring;
}

const char         *
sc_v4l2_device_capstring (sc_v4l2_device_t * vd)
{
  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);
  return vd->capstring;
}

int
sc_v4l2_device_close (sc_v4l2_device_t * vd)
{
  int                 retval;

  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);

  if ((retval = close (vd->fd)) != 0) {
    return retval;
  }

  SC_FREE (vd);
  return 0;
}
