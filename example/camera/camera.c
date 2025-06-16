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

static void print_vec3(sc_camera_vec3_t vec)
{
  size_t i;

  for (i = 0; i < 3; ++i)
  {
    printf("%lf ", vec[i]);
  }
  printf("\n");
}

int
main(int argc, char **argv)
{
  sc_camera_t *camera;
  size_t num_points = 4;
  sc_rand_state_t seed = 0;
  size_t i, j;
  sc_array_t *points_in, *points_out;
  sc_camera_coords_t *p_in, *p_out;
  sc_camera_vec3_t eye = {2.0, 1.0, 3.0};
  sc_camera_vec3_t center = {0.0, 0.0, 0.0};
  sc_camera_vec3_t up = {0.0, 1.0, 0.0};

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

  for (size_t i = 0; i < num_points; ++i)
  {
    p_in = (sc_camera_coords_t *) sc_array_index(points_in, i);
    printf("IN: ");
    print_vec3(p_in);
    p_out = (sc_camera_coords_t *) sc_array_index(points_out, i);
    printf("OUT: ");
    print_vec3(p_out);
  }

  sc_camera_destroy(camera);

  return 0;
}
