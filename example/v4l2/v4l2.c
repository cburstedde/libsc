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
}
v4l2_global_t;

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

static void
v4l2_loop (v4l2_global_t * g)
{
  int                 retval;
  double              tnow;

  SC_ASSERT (g != NULL);
  SC_ASSERT (g->vd != NULL);
  SC_ASSERT (g->wbuf != NULL || g->wsiz == 0);

  /* simulation parameters */
  g->t = 0;
  g->tfinal = 10.;
  g->omega = 1.;
  g->radius = .45;
  g->yfactor = sqrt (2.);
  g->center[0] = g->center[1] = 0.;
  g->xy[0] = g->center[0] + g->radius * cos (g->omega * g->t);
  g->xy[1] = g->center[1] + g->radius * sin (g->omega * g->yfactor * g->t);

  /* simulation loop */
  g->tlast = sc_MPI_Wtime ();
  while (g->t < g->tfinal) {
    retval = sc_v4l2_device_select (g->vd, 10 * 1000);
    SC_CHECK_ABORT (retval >= 0, "Failed to select on device");

    tnow = sc_MPI_Wtime ();
    g->t += tnow - g->tlast;

    g->xy[0] = g->center[0] + g->radius * cos (g->omega * g->t);
    g->xy[1] = g->center[1] + g->radius * sin (g->omega * g->yfactor * g->t);

    g->tlast = tnow;
  }
}

static void
v4l2_run (v4l2_global_t * g, const char *devname)
{
  int                 retval;
  const char         *outstring;

  SC_ASSERT (g != NULL);
  memset (g, 0, sizeof (*g));

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
  v4l2_global_t       sg, *g = &sg;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_DEFAULT);

  if (argc >= 2) {
    v4l2_run (g, argv[1]);
  }

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
