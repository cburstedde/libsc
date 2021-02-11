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

#ifdef SC_HAVE_SIGNAL_H
#include <signal.h>
#endif

typedef struct v4l2_global
{
  unsigned            width, height;
  unsigned            bytesperline;
  unsigned            sizeimage;
  sc_v4l2_device_t   *vd;
  size_t              wsiz;
  char               *wbuf;

  double              t;
  double              tlast;
  double              tfinal;
  double              omega;
  double              radius;
  double              yfactor;
  double              center[2];
  double              xy[2];

  unsigned long       num_frames;
}
v4l2_global_t;

typedef void        (*v4l2_sig_t) (int);

static volatile int caught_sigint = 0;

static void
v4l2_sigint_handler (int sig)
{
  caught_sigint = 1;
}

static int
v4l2_prepare (v4l2_global_t * g)
{
  int                 retval;

  retval = sc_v4l2_device_format (g->vd, &g->width, &g->height,
                                  &g->bytesperline, &g->sizeimage);
  SC_CHECK_ABORT (!retval, "Failed to configure device format");

  fprintf (stderr, "Negotiated %ux%u with %u bytes per line %u size\n",
           g->width, g->height, g->bytesperline, g->sizeimage);

  SC_CHECK_ABORT (sc_v4l2_device_is_readwrite (g->vd),
                  "Device does not support read/write I/O");

  g->wsiz = g->sizeimage;
  g->wbuf = SC_ALLOC (char, g->wsiz);

  return 0;
}

static void
v4l2_postpare (v4l2_global_t * g)
{
  SC_ASSERT (g != NULL);
  SC_ASSERT (g->vd != NULL);

  SC_FREE (g->wbuf);
}

#if 1

static              uint16_t
pack_rgb565 (unsigned char r, unsigned char g, unsigned char b)
{
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

#else

static              uint16_t
pack_rgba555 (unsigned char r, unsigned char g, unsigned char b,
              unsigned char a)
{
  return ((g & 0x18) << 11) | ((b >> 3) << 9) | (a ? 0x100 : 0x00) |
    (r & ((1 << 8) - (1 << 3))) | (g >> 5);
}

#endif

static void
paint_image (v4l2_global_t * g)
{
  unsigned            i, j;
  unsigned            whmin;
  unsigned            di, dj;
  unsigned            bi, bj;
  uint16_t           *upix;
  uint16_t            ubg, ufg;
  double              pxy[2];
  double              invm;
  double              dx, dy, r2, weight;

  SC_ASSERT (g != NULL);
  SC_ASSERT (g->vd != NULL);
  SC_ASSERT (g->wbuf != NULL || g->wsiz == 0);

  whmin = SC_MIN (g->width, g->height);
  if (whmin == 0) {
    return;
  }
  invm = 1. / whmin;

  /* color values */
#if 0
  ubg = pack_rgba555 (0xE0, 0xF0, 0xE8, 0);
#else
  ubg = pack_rgb565 (0xE0, 0xF0, 0xE8);
#endif

  /* if higher than wide, fill top set of blank lines */
  bj = dj = (g->height - whmin) / 2;
  for (j = 0; j < bj; ++j) {
    upix = (uint16_t *) (g->wbuf + j * g->bytesperline);
    for (i = 0; i < g->width; ++i) {
      *upix++ = ubg;
    }
  }

  /* center square of image */
  bj += whmin;
  for (; j < bj; ++j) {
    upix = (uint16_t *) (g->wbuf + j * g->bytesperline);
    bi = di = (g->width - whmin) / 2;
    for (i = 0; i < bi; ++i) {
      *upix++ = ubg;
    }
    bi += whmin;
    pxy[1] = (whmin - 1 - (j - dj) + .5) * invm;
    dy = pxy[1] - g->xy[1];
    dy *= dy;
    for (; i < bi; ++i) {
      pxy[0] = ((i - di) + .5) * invm;
      dx = pxy[0] - g->xy[0];
      dx *= dx;
      r2 = dy + dx;
      weight = 1. - r2 / SC_SQR (.08);
      weight = SC_MAX (weight, 0.);
      ufg = pack_rgb565 (0xE0, 0xF0 - (unsigned char) (0x70 * weight),
                         0xE8 + (unsigned char) (0x17 * weight));
      *upix++ = ufg;
    }
    bi = g->width;
    for (; i < bi; ++i) {
      *upix++ = ubg;
    }
  }

  /* if higher than wide, fill bottom set of blank lines */
  bj = g->height;
  for (; j < bj; ++j) {
    upix = (uint16_t *) (g->wbuf + j * g->bytesperline);
    for (i = 0; i < g->width; ++i) {
      *upix++ = ubg;
    }
  }
}

static void
v4l2_loop (v4l2_global_t * g)
{
  int                 retval;
  double              tnow;
  v4l2_sig_t          system_sigint_handler;

  SC_ASSERT (g != NULL);
  SC_ASSERT (g->vd != NULL);
  SC_ASSERT (g->wbuf != NULL || g->wsiz == 0);
  SC_ASSERT (g->tfinal >= 0.);

  /* simulation parameters */
  g->t = 0;
  g->omega = 1.;
  g->radius = .4;
  g->yfactor = sqrt (2.);
  g->center[0] = g->center[1] = .5;
  g->xy[0] = g->center[0] + g->radius * cos (g->omega * g->t);
  g->xy[1] = g->center[1] + g->radius * sin (g->omega * g->yfactor * g->t);

  /* catch signals */
  system_sigint_handler = signal (SIGINT, v4l2_sigint_handler);
  SC_CHECK_ABORT (system_sigint_handler != SIG_ERR, "Catching SIGINT");

  /* simulation loop */
  g->num_frames = 0;
  g->tlast = sc_MPI_Wtime ();
  while (!caught_sigint && g->t < g->tfinal) {
    retval = sc_v4l2_device_select (g->vd, 10 * 1000);
    SC_CHECK_ABORT (retval >= 0, "Failed to select on device");

    tnow = sc_MPI_Wtime ();
    g->t += tnow - g->tlast;

    g->xy[0] = g->center[0] + g->radius * cos (g->omega * g->t);
    g->xy[1] = g->center[1] + g->radius * sin (g->omega * g->yfactor * g->t);

    paint_image (g);
    retval = sc_v4l2_device_write (g->vd, g->wbuf);
    SC_CHECK_ABORT (retval == 0, "Failed to write to device");

    ++g->num_frames;
    g->tlast = tnow;
  }

  /* restore signals */
  system_sigint_handler = signal (SIGINT, system_sigint_handler);
  SC_CHECK_ABORT (system_sigint_handler != SIG_ERR, "Restoring SIGINT");
  caught_sigint = 0;

  fprintf (stderr, "Written %lu frames to time %g\n", g->num_frames, g->t);
}

static void
v4l2_run (v4l2_global_t * g, const char *devname, double finaltime)
{
  int                 retval;
  const char         *outstring;

  SC_ASSERT (g != NULL);
  memset (g, 0, sizeof (*g));

  g->tfinal = finaltime <= 0. ? 1e100 : finaltime;
  g->width = 640;
  g->height = 480;

  g->vd = sc_v4l2_device_open (devname);
  SC_CHECK_ABORTF (g->vd != NULL, "Failed to open device %s", devname);

  fprintf (stderr, "%s\n", sc_v4l2_device_devstring (g->vd));
  fprintf (stderr, "%s\n", sc_v4l2_device_capstring (g->vd));
  if ((outstring = sc_v4l2_device_outstring (g->vd)) != NULL) {
    fprintf (stderr, "%s\n", outstring);
    retval = v4l2_prepare (g);
    SC_CHECK_ABORTF (!retval, "Failed to output to device %s", devname);
  }

  v4l2_loop (g);

  v4l2_postpare (g);

  retval = sc_v4l2_device_close (g->vd);
  SC_CHECK_ABORTF (!retval, "Failed to close device %s", devname);
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  double              finaltime;
  v4l2_global_t       sg, *g = &sg;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 0, 1, NULL, SC_LP_DEFAULT);

  finaltime = 10.;
  if (argc >= 3) {
    finaltime = strtod (argv[2], NULL);
  }
  if (argc >= 2) {
    v4l2_run (g, argv[1], finaltime);
  }

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
