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

#include <stdio.h>

#include <sc_camera.h>
#include <sc_random.h>

static void print_vec3(const char *label, sc_camera_vec3_t vec)
{
  SC_STATISTICSF("%s %lf %lf %lf\n", label, vec[0], vec[1], vec[2]);
}

int
main(int argc, char **argv)
{
  int mpiret;
  sc_camera_t *camera;
  size_t num_points = 10;
  sc_rand_state_t seed = 0;
  size_t i, j;
  sc_array_t *points_in, *points_out;
  sc_camera_coords_t *p_in, *p_out;
  sc_camera_vec3_t eye = {2.0, 1.0, 3.0};
  sc_camera_vec3_t center = {0.0, 0.0, 0.0};
  sc_camera_vec3_t up = {0.0, 1.0, 0.0};

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 0, 1, NULL, SC_LP_DEFAULT);

  points_in = sc_array_new_count(sizeof(sc_camera_coords_t) * 3, num_points);
  points_out = sc_array_new_count(sizeof(sc_camera_coords_t) * 3, num_points);

  camera = sc_camera_new();

  for (i = 0; i < num_points; ++i)
  {
    p_in = (sc_camera_coords_t *) sc_array_index(points_in, i);

    for (j = 0; j < 3; ++j)
    {
      p_in[j] = sc_rand(&seed);
    }
  }

  sc_camera_look_at(camera, eye, center, up);
  sc_camera_view_transform(camera, points_in, points_out);

  for (i = 0; i < num_points; ++i)
  {
    p_in = (sc_camera_coords_t *) sc_array_index(points_in, i);
    print_vec3("IN : ", p_in);
    p_out = (sc_camera_coords_t *) sc_array_index(points_out, i);
    print_vec3("OUT :", p_out);
  }

  sc_camera_destroy(camera);
  sc_array_destroy(points_in);
  sc_array_destroy(points_out);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
