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

#ifndef SC_BUFSIZE
#define SC_BUFSIZE BUFSIZ
#endif

struct sc_v4l2_device
{
  int                 fd;
  int                 supports_output;
  struct v4l2_capability capability;
  struct v4l2_output  output;
  struct v4l2_format  format;
  struct v4l2_pix_format *pix;
  char                devname[SC_BUFSIZE];
  char                devstring[SC_BUFSIZE];
  char                capstring[SC_BUFSIZE];
  char                outstring[SC_BUFSIZE];
};

static int
querycap (sc_v4l2_device_t * vd)
{
  int                 retval;
  __u32               caps;

  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);

  /* query device capabilities */
  if ((retval = ioctl (vd->fd, VIDIOC_QUERYCAP, &vd->capability)) != 0) {
    return retval;
  }

  /* summarize driver and device information */
  snprintf (vd->devstring, SC_BUFSIZE, "Driver: %s Device: %s Bus: %s",
            vd->capability.driver, vd->capability.card,
            vd->capability.bus_info);

  /* summarize selected capabilities */
  if (!(vd->capability.capabilities & V4L2_CAP_DEVICE_CAPS)) {
    caps = vd->capability.capabilities;
  }
  else {
    caps = vd->capability.device_caps;
  }
  vd->supports_output = caps & V4L2_CAP_VIDEO_OUTPUT;
  snprintf (vd->capstring, SC_BUFSIZE, "Output: %d RW: %d Stream: %d MC: %d",
            vd->supports_output ? 1 : 0, caps & V4L2_CAP_READWRITE ? 1 : 0,
            caps & V4L2_CAP_STREAMING ? 1 : 0, caps & V4L2_CAP_IO_MC ? 1 : 0);

  /* go through outputs and identify the one to use */
  if (vd->supports_output) {
    vd->supports_output = 0;
    for (vd->output.index = 0;; ++vd->output.index) {
      if ((retval = ioctl (vd->fd, VIDIOC_ENUMOUTPUT, &vd->output)) != 0) {
        /* the loop is over on ioctl error */
        break;
      }
      if (vd->output.type == V4L2_OUTPUT_TYPE_ANALOG) {
        /* successfully found a fitting output */
        vd->supports_output = 1;
        break;
      }
    }
  }
  if (!vd->supports_output) {
    snprintf (vd->outstring, SC_BUFSIZE, "%s",
              "Output not supported as desired");
  }
  else {
    snprintf (vd->outstring, SC_BUFSIZE,
              "Output index: %d Name: %s Std: %08x", vd->output.index,
              vd->output.name, (__u32) vd->output.std);
  }

  return 0;
}

sc_v4l2_device_t   *
sc_v4l2_device_open (const char *devname)
{
  int                 retval;
  sc_v4l2_device_t   *vd;

  SC_ASSERT (devname != NULL);

  if ((vd = SC_ALLOC (struct sc_v4l2_device, 1)) == NULL) {
    return NULL;
  }
  memset (vd, 0, sizeof (*vd));
  snprintf (vd->devname, SC_BUFSIZE, "%s", devname);

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
sc_v4l2_device_devstring (const sc_v4l2_device_t * vd)
{
  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);
  return vd->devstring;
}

const char         *
sc_v4l2_device_capstring (const sc_v4l2_device_t * vd)
{
  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);
  return vd->capstring;
}

const char         *
sc_v4l2_device_outstring (const sc_v4l2_device_t * vd)
{
  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);
  return vd->supports_output ? vd->outstring : NULL;
}

int
sc_v4l2_device_format (sc_v4l2_device_t * vd,
                       unsigned int *width, unsigned int *height,
                       unsigned int *bytesperline)
{
  int                 retval;
  int                 output_index;

  SC_ASSERT (vd != NULL);
  SC_ASSERT (vd->fd >= 0);
  SC_ASSERT (vd->supports_output);

  SC_ASSERT (width != NULL);
  SC_ASSERT (height != NULL);
  SC_ASSERT (bytesperline != NULL);

  /* select video output */
  if ((retval = ioctl (vd->fd, VIDIOC_G_OUTPUT, &output_index)) != 0) {
    return retval;
  }
  if (output_index != (int) vd->output.index) {
    output_index = vd->output.index;
    if ((retval = ioctl (vd->fd, VIDIOC_S_OUTPUT, &output_index)) != 0) {
      return retval;
    }
  }

  /* query current format */
  vd->format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  if ((retval = ioctl (vd->fd, VIDIOC_G_FMT, &vd->format)) != 0) {
    return retval;
  }
  vd->pix = &vd->format.fmt.pix;

#if 0
  /* hack pre info */
  fprintf (stderr, "Image %ux%u Pixelformat %08x Field %08x\n",
           vd->pix->width, vd->pix->height,
           vd->pix->pixelformat, vd->pix->field);
  fprintf (stderr, "Bytesperline %u Size %u Colorspace %08x\n",
           vd->pix->bytesperline, vd->pix->sizeimage, vd->pix->colorspace);
#endif

  /* set desired values */
  vd->pix->width = *width;
  vd->pix->height = *height;
  vd->pix->pixelformat = V4L2_PIX_FMT_RGB565;
  vd->pix->field = V4L2_FIELD_NONE;
  vd->pix->bytesperline = 2 * vd->pix->width;
  vd->pix->sizeimage = vd->pix->bytesperline * vd->pix->height;
  vd->pix->colorspace = V4L2_COLORSPACE_SRGB;
  vd->pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
  vd->pix->quantization = V4L2_QUANTIZATION_DEFAULT;
  vd->pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;

  /* set desired format */
  if ((retval = ioctl (vd->fd, VIDIOC_S_FMT, &vd->format)) != 0) {
    return retval;
  }
  if (vd->pix->pixelformat != V4L2_PIX_FMT_RGB565 ||
      vd->pix->colorspace != V4L2_COLORSPACE_SRGB ||
      vd->pix->field != V4L2_FIELD_NONE) {
    return -1;
  }

#if 0
  /* hack post info */
  fprintf (stderr, "Image %ux%u Pixelformat %08x Field %08x\n",
           vd->pix->width, vd->pix->height,
           vd->pix->pixelformat, vd->pix->field);
  fprintf (stderr, "Bytesperline %u Size %u Colorspace %08x\n",
           vd->pix->bytesperline, vd->pix->sizeimage, vd->pix->colorspace);
#endif

  /* report back negotiated format */
  *width = vd->pix->width;
  *height = vd->pix->height;
  *bytesperline = vd->pix->bytesperline;

  return 0;
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
