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

sc_array_t * array_by_indices(sc_array_t *arr, sc_array_t *indices)
{
  size_t i;
  sc_array_t *out;
  size_t *index;
  void *elem;

  SC_ASSERT(arr != NULL);
  SC_ASSERT(indices != NULL);
  SC_ASSERT(indices->elem_size == sizeof(size_t));

  out = sc_array_new_count(arr->elem_size, indices->elem_count);

  for (i = 0; i < indices->elem_count; ++i)
  {
    index = (size_t *) sc_array_index(indices, i);
    SC_ASSERT(*index < arr->elem_count);

    elem = sc_array_index(arr, *index);
    memcpy(sc_array_index(out, i), elem, arr->elem_size);
  }

  return out;
}

int
main (int argc, char **argv)
{
  int                 mpiret;
  sc_camera_t        *camera;
  size_t              num_points = 10;
  sc_rand_state_t     seed = 0;
  size_t              i, j, point_index;
  sc_array_t         *points_world, *points_camera, *points_clipping, 
                     *points_inside_xyzw, *points_inside_xyz;
  sc_array_t         *indices_inside;
  sc_camera_coords_t *p;
  sc_camera_vec3_t    eye = { -0.5, 1.5, 0.5 };
  sc_camera_vec3_t    center = { 0.0, 0.6, 0.0 };
  sc_camera_vec3_t    up = { 0.0, 1.0, 0.0 };

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 0, 1, NULL, SC_LP_DEFAULT);

  points_world = sc_array_new_count (sizeof (sc_camera_vec3_t), num_points);
  points_camera = sc_array_new_count (sizeof (sc_camera_vec3_t), num_points);
  points_clipping =
    sc_array_new_count (sizeof (sc_camera_vec4_t), num_points);

  camera = sc_camera_new ();
  sc_camera_look_at (camera, eye, center, up);
  sc_camera_pitch (camera, -M_PI / 4.0);

  for (i = 0; i < num_points; ++i) {
    p = (sc_camera_coords_t *) sc_array_index (points_world, i);

    for (j = 0; j < 3; ++j) {
      p[j] = sc_rand (&seed);
    }
  }

  indices_inside = sc_array_new (sizeof (size_t));

  sc_camera_clipping_pre (camera, points_world, indices_inside);

  for (i = 0; i < indices_inside->elem_count; ++i) {
    point_index = *((size_t *) sc_array_index (indices_inside, i));

    SC_INFOF ("Point inside pre: %lu\n", (unsigned long) point_index);
  }

  sc_camera_view_transform (camera, points_world, points_camera);
  sc_camera_projection_transform (camera, points_camera, points_clipping);

  for (i = 0; i < num_points; ++i) {
    p = (sc_camera_coords_t *) sc_array_index (points_world, i);
    SC_INFOF ("World : %lf %lf %lf\n", p[0], p[1], p[2]);

    p = (sc_camera_coords_t *) sc_array_index (points_camera, i);
    SC_INFOF ("Camera : %lf %lf %lf\n", p[0], p[1], p[2]);

    p = (sc_camera_coords_t *) sc_array_index (points_clipping, i);
    SC_INFOF ("Clipping : %lf %lf %lf %lf\n", p[0], p[1], p[2], p[3]);
  }

  sc_camera_clipping_post (points_clipping, indices_inside);

  for (i = 0; i < indices_inside->elem_count; ++i) {
    point_index = *((size_t *) sc_array_index (indices_inside, i));

    SC_INFOF ("Point inside post: %lu\n", (unsigned long) point_index);
  }

  points_inside_xyzw = array_by_indices(points_clipping, indices_inside);

  points_inside_xyz = sc_array_new (sizeof (sc_camera_vec3_t));
  sc_camera_perspective_division(points_inside_xyzw, points_inside_xyz);

  for (i = 0; i < points_inside_xyz->elem_count; ++i) {
    p = (sc_camera_coords_t *) sc_array_index (points_inside_xyz, i);
    SC_INFOF ("Point after perspective division : %lf %lf %lf\n", p[0], p[1], p[2]);
  }

  sc_camera_destroy (camera);
  sc_array_destroy (points_world);
  sc_array_destroy (points_camera);
  sc_array_destroy (points_clipping);
  sc_array_destroy (points_inside_xyz);
  sc_array_destroy (points_inside_xyzw);
  sc_array_destroy (indices_inside);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);

  return 0;
}
