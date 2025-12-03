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

#include <string.h>
#include <math.h>

#include <sc_camera.h>

/* math utility functions needed in this file */

static void         sc_camera_mat_to_q (const sc_camera_mat3x3_t A,
                                        sc_camera_vec4_t q);

static inline void  sc_camera_cross_prod (const sc_camera_vec3_t a,
                                          const sc_camera_vec3_t b,
                                          sc_camera_vec3_t out);

/* out = a - b*/
static inline void  sc_camera_vec3_minus (const sc_camera_vec3_t a,
                                          const sc_camera_vec3_t b,
                                          sc_camera_vec3_t out);

static inline sc_camera_coords_t sc_camera_vec3_norm (const sc_camera_vec3_t
                                                      x);

/* scalar out = alpha * x */
static inline void  sc_camera_vec3_scale (sc_camera_coords_t alpha,
                                          const sc_camera_vec3_t x,
                                          sc_camera_vec3_t out);

static void         sc_camera_q_mult (const sc_camera_vec4_t q1,
                                      const sc_camera_vec4_t q2,
                                      sc_camera_vec4_t out);

static void         sc_camera_mat4_transpose (sc_camera_mat4x4_t mat);

typedef void        (*sc_camera_mat_mul_t) (const sc_camera_mat4x4_t,
                                            const sc_camera_coords_t *,
                                            sc_camera_coords_t *);

static void         sc_camera_apply_mat (sc_camera_mat_mul_t funct,
                                         const sc_camera_mat4x4_t mat,
                                         sc_array_t * points_in,
                                         sc_array_t * points_out);

static void         sc_camera_mat4_mul_v3_to_v3 (const sc_camera_mat4x4_t mat,
                                                 const sc_camera_vec3_t in,
                                                 sc_camera_vec3_t out);

static void         sc_camera_mat4_mul_v3_to_v4 (const sc_camera_mat4x4_t mat,
                                                 const sc_camera_vec3_t in,
                                                 sc_camera_vec4_t out);

static void         sc_camera_mat4_mul_v4_to_v4 (const sc_camera_mat4x4_t mat,
                                                 const sc_camera_vec4_t in,
                                                 sc_camera_vec4_t out);

/* out = A * B */
static void         sc_camera_mult_4x4_4x4 (const sc_camera_mat4x4_t A,
                                            const sc_camera_mat4x4_t B,
                                            sc_camera_mat4x4_t out);

/* function for updating the planes */
static void         sc_camera_update_planes (sc_camera_t * camera);

static sc_camera_coords_t sc_camera_signed_dist (const sc_camera_vec3_t point,
                                                 const sc_camera_vec4_t
                                                 plane);

static inline void sc_camera_normalize_vec4(sc_camera_vec4_t vec);

sc_camera_t        *
sc_camera_new (void)
{
  sc_camera_t        *camera = SC_ALLOC (sc_camera_t, 1);
  sc_camera_init (camera);

  return camera;
}

void
sc_camera_init (sc_camera_t * camera)
{
  SC_ASSERT (camera != NULL);

  camera->position[0] = 0.0;
  camera->position[1] = 0.0;
  camera->position[2] = 1.0;

  camera->rotation[0] = 0.0;
  camera->rotation[1] = 0.0;
  camera->rotation[2] = 0.0;
  camera->rotation[3] = 1.0;

  camera->FOV = 1.57079632679;

  camera->width = 1000;
  camera->height = 1000;

  camera->near_plane = 0.01;
  camera->far_plane = 100.0;

  sc_camera_update_planes (camera);
}

void
sc_camera_destroy (sc_camera_t * camera)
{
  SC_FREE (camera);
}

void
sc_camera_position (sc_camera_t * camera, const sc_camera_vec3_t position)
{
  SC_ASSERT (camera != NULL);
  SC_ASSERT (position != NULL);

  (void) memcpy (camera->position, position, sizeof (camera->position));

  sc_camera_update_planes (camera);
}

/* because the camera should rotate right handed we have to rotate the points
  the other way around */
void
sc_camera_yaw (sc_camera_t * camera, double angle)
{
  sc_camera_vec4_t    q;

  SC_ASSERT (camera != NULL);

  q[0] = 0.0;
  q[1] = -sin (angle / 2.0);
  q[2] = 0.0;
  q[3] = cos (angle / 2.0);

  sc_camera_q_mult (q, camera->rotation, camera->rotation);

  sc_camera_normalize_vec4(camera->rotation);

  sc_camera_update_planes (camera);
}

void
sc_camera_pitch (sc_camera_t * camera, double angle)
{
  sc_camera_vec4_t    q;

  SC_ASSERT (camera != NULL);

  q[0] = -sin (angle / 2.0);
  q[1] = 0.0;
  q[2] = 0.0;
  q[3] = cos (angle / 2.0);

  sc_camera_q_mult (q, camera->rotation, camera->rotation);

  sc_camera_normalize_vec4(camera->rotation);

  sc_camera_update_planes (camera);
}

void
sc_camera_roll (sc_camera_t * camera, double angle)
{
  sc_camera_vec4_t    q;

  SC_ASSERT (camera != NULL);

  q[0] = 0.0;
  q[1] = 0.0;
  q[2] = -sin (angle / 2.0);
  q[3] = cos (angle / 2.0);

  sc_camera_q_mult (q, camera->rotation, camera->rotation);

  sc_camera_normalize_vec4(camera->rotation);

  sc_camera_update_planes (camera);
}

void
sc_camera_walk (sc_camera_t * camera, double distance)
{
  const sc_camera_coords_t *nearn =
   camera->frustum_planes[SC_CAMERA_PLANE_NEAR];

  /* the normal to the near plane is facing towards the camera */
  camera->position[0] += distance * nearn[0];
  camera->position[1] += distance * nearn[1];
  camera->position[2] += distance * nearn[2];

  /* technically, only the distances need to be recomputed */
  sc_camera_update_planes (camera);
}

void
sc_camera_fov (sc_camera_t * camera, double angle)
{
  SC_ASSERT (camera != NULL);
  /* Tangens at Pi/2 behaves like 1/(Pi/2 - x).*/
  SC_ASSERT (angle > 0. && angle < M_PI);

  camera->FOV = angle;

  sc_camera_update_planes (camera);
}

void
sc_camera_aspect_ratio (sc_camera_t * camera, int width, int height)
{
  SC_ASSERT (camera != NULL);
  SC_ASSERT (width > 0 && height > 0);

  camera->width = width;
  camera->height = height;

  sc_camera_update_planes (camera);
}

void
sc_camera_clipping_dist (sc_camera_t * camera, sc_camera_coords_t near_plane,
                         sc_camera_coords_t far_plane)
{
  SC_ASSERT (camera != NULL);
  SC_ASSERT (near_plane > 0.);
  SC_ASSERT (far_plane > near_plane);

  camera->near_plane = near_plane;
  camera->far_plane = far_plane;

  sc_camera_update_planes (camera);
}

void
sc_camera_look_at (sc_camera_t * camera, const sc_camera_vec3_t eye,
                   const sc_camera_vec3_t center, const sc_camera_vec3_t up)
{
  sc_camera_vec3_t    z_new, x_new, y_new;
  sc_camera_coords_t  z_new_norm, x_new_norm;
  sc_camera_mat3x3_t  rotation;
  sc_camera_vec4_t    q;

  SC_ASSERT (camera != NULL);
  SC_ASSERT (eye != NULL && center != NULL && up != NULL);

  (void) memcpy (camera->position, eye, sizeof (camera->position));

  sc_camera_vec3_minus (eye, center, z_new);
  z_new_norm = sc_camera_vec3_norm (z_new);
  SC_ASSERT (z_new_norm > 0.);
  sc_camera_vec3_scale (1.0 / z_new_norm, z_new, z_new);

  sc_camera_cross_prod (up, z_new, x_new);
  x_new_norm = sc_camera_vec3_norm (x_new);
  SC_ASSERT (x_new_norm > 0.);
  sc_camera_vec3_scale (1.0 / x_new_norm, x_new, x_new);

  sc_camera_cross_prod (z_new, x_new, y_new);

  rotation[0] = x_new[0];
  rotation[1] = y_new[0];
  rotation[2] = z_new[0];
  rotation[3] = x_new[1];
  rotation[4] = y_new[1];
  rotation[5] = z_new[1];
  rotation[6] = x_new[2];
  rotation[7] = y_new[2];
  rotation[8] = z_new[2];

  sc_camera_mat_to_q (rotation, q);

  (void) memcpy (camera->rotation, q, sizeof (camera->rotation));

  sc_camera_update_planes (camera);
}

void
sc_camera_get_view_mat (const sc_camera_t * camera, sc_camera_mat4x4_t view_matrix)
{
  sc_camera_coords_t  xx, yy, zz, wx, wy, wz, xy, xz, yz;

  SC_ASSERT (camera != NULL);
  SC_ASSERT (view_matrix != NULL);

  /* calculate rotation matrix from the quaternion and write it in upper left block */
  xx = camera->rotation[0] * camera->rotation[0];
  yy = camera->rotation[1] * camera->rotation[1];
  zz = camera->rotation[2] * camera->rotation[2];
  wx = camera->rotation[3] * camera->rotation[0];
  wy = camera->rotation[3] * camera->rotation[1];
  wz = camera->rotation[3] * camera->rotation[2];
  xy = camera->rotation[0] * camera->rotation[1];
  xz = camera->rotation[0] * camera->rotation[2];
  yz = camera->rotation[1] * camera->rotation[2];

  view_matrix[0] = 1.0 - 2.0 * (yy + zz);
  view_matrix[1] = 2.0 * (xy + wz);
  view_matrix[2] = 2.0 * (xz - wy);
  view_matrix[3] = 0.0;

  view_matrix[4] = 2.0 * (xy - wz);
  view_matrix[5] = 1.0 - 2.0 * (xx + zz);
  view_matrix[6] = 2.0 * (yz + wx);
  view_matrix[7] = 0.0;

  view_matrix[8] = 2.0 * (xz + wy);
  view_matrix[9] = 2.0 * (yz - wx);
  view_matrix[10] = 1.0 - 2.0 * (xx + yy);
  view_matrix[11] = 0.0;

  view_matrix[12] = -view_matrix[0] * camera->position[0] -
    view_matrix[4] * camera->position[1] -
    view_matrix[8] * camera->position[2];
  view_matrix[13] =
    -view_matrix[1] * camera->position[0] -
    view_matrix[5] * camera->position[1] -
    view_matrix[9] * camera->position[2];
  view_matrix[14] =
    -view_matrix[2] * camera->position[0] -
    view_matrix[6] * camera->position[1] -
    view_matrix[10] * camera->position[2];
  view_matrix[15] = 1.0;
}

void
sc_camera_view_transform (const sc_camera_t * camera, sc_array_t * points_in,
                          sc_array_t * points_out)
{
  sc_camera_mat4x4_t  transformation;

  SC_ASSERT (points_in != NULL && points_out != NULL);
  SC_ASSERT (points_in->elem_size == sizeof (sc_camera_vec3_t));
  SC_ASSERT (points_out->elem_size == sizeof (sc_camera_vec3_t));

  sc_camera_get_view_mat (camera, transformation);

  sc_camera_apply_mat (sc_camera_mat4_mul_v3_to_v3, transformation, points_in,
                       points_out);
}

void
sc_camera_get_projection_mat (const sc_camera_t * camera,
                              sc_camera_mat4x4_t proj_matrix)
{
  /* the factor 2 * camera->near_plane could be reduced */
  sc_camera_coords_t  s_x, s_y, s_z;

  SC_ASSERT (camera != NULL);
  SC_ASSERT (proj_matrix != NULL);

  s_x = 2.0 * camera->near_plane * tan (camera->FOV / 2.0);
  s_y = s_x * ((sc_camera_coords_t) camera->height /
               (sc_camera_coords_t) camera->width);
  s_z = camera->far_plane - camera->near_plane;

  proj_matrix[0] = 2.0 * camera->near_plane / s_x;
  proj_matrix[1] = 0.0;
  proj_matrix[2] = 0.0;
  proj_matrix[3] = 0.0;

  proj_matrix[4] = 0.0;
  proj_matrix[5] = 2.0 * camera->near_plane / s_y;
  proj_matrix[6] = 0.0;
  proj_matrix[7] = 0.0;

  proj_matrix[8] = 0.0;
  proj_matrix[9] = 0.0;
  proj_matrix[10] = -(camera->near_plane + camera->far_plane) / s_z;
  proj_matrix[11] = -1.0;

  proj_matrix[12] = 0.0;
  proj_matrix[13] = 0.0;
  proj_matrix[14] = -(2.0 * camera->near_plane * camera->far_plane) / s_z;
  proj_matrix[15] = 0.0;
}

void
sc_camera_projection_transform (const sc_camera_t * camera, sc_array_t * points_in,
                                sc_array_t * points_out)
{
  sc_camera_mat4x4_t  transformation;

  SC_ASSERT (points_in != NULL && points_out != NULL);
  SC_ASSERT (points_in->elem_size == sizeof (sc_camera_vec3_t));
  SC_ASSERT (points_out->elem_size == sizeof (sc_camera_vec4_t));

  sc_camera_get_projection_mat (camera, transformation);

  sc_camera_apply_mat (sc_camera_mat4_mul_v3_to_v4, transformation, points_in,
                       points_out);
}

void
sc_camera_get_frustum (const sc_camera_t * camera, sc_array_t * planes)
{
  sc_array_t *arr_view;

  SC_ASSERT (camera != NULL);
  SC_ASSERT (planes != NULL);

  /* const -> not const */
  arr_view = sc_array_new_data ((void *) camera->frustum_planes,
                      sizeof (sc_camera_vec4_t), 6);

  sc_array_copy (planes, arr_view);
  sc_array_destroy (arr_view);
}

void
sc_camera_clipping_pre (const sc_camera_t * camera, sc_array_t * points,
                        sc_array_t * indices)
{
  size_t              i, j;
  sc_camera_coords_t *point;
  size_t             *elem;
  int                is_inside;

  SC_ASSERT (camera != NULL);
  SC_ASSERT (points != NULL);
  SC_ASSERT (points->elem_size == sizeof (sc_camera_vec3_t));
  SC_ASSERT (indices != NULL);
  SC_ASSERT (indices->elem_size == sizeof (size_t));

  sc_array_reset (indices);

  for (i = 0; i < points->elem_count; ++i) {
    is_inside = 1;
    point = (sc_camera_coords_t *) sc_array_index (points, i);

    for (j = 0; j < 6; ++j) {

      if (sc_camera_signed_dist (point, camera->frustum_planes[j]) > 0.) {
        is_inside = 0;
        break;
      }
    }

    if (is_inside) {
      elem = (size_t *) sc_array_push (indices);
      *elem = i;
    }
  }
}

void
sc_camera_frustum_dist (const sc_camera_t * camera, const sc_camera_vec3_t point,
                        sc_camera_coords_t distances[6])
{
  size_t              i;

  SC_ASSERT (camera != NULL);
  SC_ASSERT (point != NULL);
  SC_ASSERT (distances != NULL);

  for (i = 0; i < 6; ++i) {
    distances[i] = sc_camera_signed_dist (point, camera->frustum_planes[i]);
  }
}

void sc_camera_clipping_post(sc_array_t *points, sc_array_t *indices)
{
  size_t i, *index;
  sc_camera_coords_t *point;
  int is_inside;

  SC_ASSERT(points != NULL);
  SC_ASSERT(points->elem_size == sizeof(sc_camera_vec4_t));
  SC_ASSERT(indices != NULL);
  SC_ASSERT(indices->elem_size == sizeof(size_t));

  sc_array_reset(indices);

  for (i = 0; i < points->elem_count; ++i)
  {
    point = (sc_camera_coords_t *) sc_array_index(points, i);

    SC_ASSERT (point[0] != 0. || point[1] != 0. || point[2] != 0. || point[3] != 0.);

    is_inside = point[0] >= -point[3] && point[0] <= point[3] &&
                point[1] >= -point[3] && point[1] <= point[3] &&
                point[2] >= -point[3] && point[2] <= point[3];

    if (is_inside)
    {
      index = (size_t *) sc_array_push(indices);
      *index = i;
    }
  }
}

void sc_camera_perspective_division(sc_array_t *points_in, sc_array_t *points_out)
{
  size_t i;
  sc_camera_coords_t *point_in, *point_out;

  SC_ASSERT(points_in != NULL);
  SC_ASSERT(points_in->elem_size == sizeof(sc_camera_vec4_t));
  SC_ASSERT(points_out != NULL);
  SC_ASSERT(points_out->elem_size == sizeof(sc_camera_vec3_t));

  sc_array_resize(points_out, points_in->elem_count);

  for (i = 0; i < points_in->elem_count; ++i)
  {
    point_in = (sc_camera_coords_t *) sc_array_index(points_in, i);
    point_out = (sc_camera_coords_t *) sc_array_index(points_out, i);

    SC_ASSERT (point_in[3] != 0.);

    point_out[0] = point_in[0] / point_in[3];
    point_out[1] = point_in[1] / point_in[3];
    point_out[2] = point_in[2] / point_in[3];
  }
}

/** The mathematics are for example described here:
 * https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion */
static void
sc_camera_mat_to_q (const sc_camera_mat3x3_t A, sc_camera_vec4_t q)
{
  sc_camera_coords_t  t = A[0 + 3 * 0] + A[1 + 3 * 1] + A[2 + 3 * 2];
  sc_camera_coords_t  r, s;

  if (t >= 0) {
    r = sqrt (1 + t);
    s = 1.0 / (2.0 * r);
    q[3] = r / 2.0;
    q[0] = (A[2 + 3 * 1] - A[1 + 3 * 2]) * s;
    q[1] = (A[0 + 3 * 2] - A[2 + 3 * 0]) * s;
    q[2] = (A[1 + 3 * 0] - A[0 + 3 * 1]) * s;
  }
  else {
    if (A[0 + 3 * 0] >= A[1 + 3 * 1] && A[0 + 3 * 0] >= A[2 + 3 * 2]) {
      r = sqrt (1 + A[0 + 3 * 0] - A[1 + 3 * 1] - A[2 + 3 * 2]);
      s = 1.0 / (2.0 * r);
      q[3] = (A[2 + 3 * 1] - A[1 + 3 * 2]) * s;
      q[0] = r / 2.0;
      q[1] = (A[0 + 3 * 1] + A[1 + 3 * 0]) * s;
      q[2] = (A[2 + 3 * 0] + A[0 + 3 * 2]) * s;
    }
    else if (A[1 + 3 * 1] >= A[2 + 3 * 2]) {
      r = sqrt (1 - A[0 + 3 * 0] + A[1 + 3 * 1] - A[2 + 3 * 2]);
      s = 1.0 / (2.0 * r);
      q[3] = (A[0 + 3 * 2] - A[2 + 3 * 0]) * s;
      q[0] = (A[1 + 3 * 0] + A[0 + 3 * 1]) * s;
      q[1] = r / 2.0;
      q[2] = (A[2 + 3 * 1] + A[1 + 3 * 2]) * s;
    }
    else {
      r = sqrt (1 - A[0 + 3 * 0] - A[1 + 3 * 1] + A[2 + 3 * 2]);
      s = 1.0 / (2.0 * r);
      q[3] = (A[1 + 3 * 0] - A[0 + 3 * 1]) * s;
      q[0] = (A[2 + 3 * 0] + A[0 + 3 * 2]) * s;
      q[1] = (A[2 + 3 * 1] + A[1 + 3 * 2]) * s;
      q[2] = r / 2.0;
    }
  }
}

static inline void
sc_camera_cross_prod (const sc_camera_vec3_t a,
                      const sc_camera_vec3_t b, sc_camera_vec3_t out)
{
  sc_camera_coords_t  s1, s2, s3;
  s1 = a[1] * b[2] - a[2] * b[1];
  s2 = a[2] * b[0] - a[0] * b[2];
  s3 = a[0] * b[1] - a[1] * b[0];

  out[0] = s1;
  out[1] = s2;
  out[2] = s3;
}

/* out = a - b*/
static inline void
sc_camera_vec3_minus (const sc_camera_vec3_t a,
                      const sc_camera_vec3_t b, sc_camera_vec3_t out)
{
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

static inline       sc_camera_coords_t
sc_camera_vec3_norm (const sc_camera_vec3_t x)
{
  return sqrt (x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

 /* scalar out = alpha * x */
static inline void
sc_camera_vec3_scale (sc_camera_coords_t alpha,
                      const sc_camera_vec3_t x, sc_camera_vec3_t out)
{
  out[0] = alpha * x[0];
  out[1] = alpha * x[1];
  out[2] = alpha * x[2];
}

static void
sc_camera_q_mult (const sc_camera_vec4_t q1, const sc_camera_vec4_t q2,
                  sc_camera_vec4_t out)
{
  sc_camera_coords_t  x1 = q1[0], y1 = q1[1], z1 = q1[2], w1 = q1[3];
  sc_camera_coords_t  x2 = q2[0], y2 = q2[1], z2 = q2[2], w2 = q2[3];

  out[0] = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
  out[1] = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2;
  out[2] = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2;
  out[3] = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
}

static void
sc_camera_mat4_transpose (sc_camera_mat4x4_t mat)
{
  size_t              i, j;
  sc_camera_coords_t  temp;

  for (i = 1; i < 4; ++i) {
    for (j = 0; j < i; ++j) {
      temp = mat[i + 4 * j];
      mat[i + 4 * j] = mat[j + 4 * i];
      mat[j + 4 * i] = temp;
    }
  }
}

/**
 * - The current design requires defining multiple specialized matrix
 *   multiplication functions.
 * - This approach basically just applies a function to an array with an extra
 *   matrix argument and could be generalized using void pointers, but the
 *   current implementation should suffice.
 */
static void
sc_camera_apply_mat (sc_camera_mat_mul_t funct,
                     const sc_camera_mat4x4_t mat,
                     sc_array_t * points_in, sc_array_t * points_out)
{
  size_t              i;
  sc_camera_coords_t *in, *out;

  sc_array_resize (points_out, points_in->elem_count);

  for (i = 0; i < points_in->elem_count; ++i) {
    in = (sc_camera_coords_t *) sc_array_index (points_in, i);
    out = (sc_camera_coords_t *) sc_array_index (points_out, i);

    funct (mat, in, out);
  }
}

static void
sc_camera_mat4_mul_v3_to_v3 (const sc_camera_mat4x4_t mat,
                             const sc_camera_vec3_t in, sc_camera_vec3_t out)
{
  sc_camera_vec4_t    x = { in[0], in[1], in[2], 1.0 };
  size_t              i, j;

  for (i = 0; i < 3; ++i) {
    out[i] = 0.;

    for (j = 0; j < 4; ++j) {
      out[i] += mat[i + j * 4] * x[j];
    }
  }
}

static void
sc_camera_mat4_mul_v3_to_v4 (const sc_camera_mat4x4_t mat,
                             const sc_camera_vec3_t in, sc_camera_vec4_t out)
{
  sc_camera_vec4_t    x = { in[0], in[1], in[2], 1.0 };
  size_t              i, j;

  for (i = 0; i < 4; ++i) {
    out[i] = 0.;

    for (j = 0; j < 4; ++j) {
      out[i] += mat[i + j * 4] * x[j];
    }
  }
}

static void
sc_camera_mat4_mul_v4_to_v4 (const sc_camera_mat4x4_t mat,
                             const sc_camera_vec4_t in, sc_camera_vec4_t out)
{
  sc_camera_vec4_t    x = { in[0], in[1], in[2], in[3] };
  size_t              i, j;

  for (i = 0; i < 4; ++i) {
    out[i] = 0.;

    for (j = 0; j < 4; ++j) {
      out[i] += mat[i + j * 4] * x[j];
    }
  }
}

static void
sc_camera_mult_4x4_4x4 (const sc_camera_mat4x4_t A,
                        const sc_camera_mat4x4_t B, sc_camera_mat4x4_t out)
{
  sc_camera_mat4x4_t  product;
  size_t              i, j, k;
  /* i index of rows of product */
  for (i = 0; i < 4; ++i) {
    /* j index of columns of product */
    for (j = 0; j < 4; ++j) {
      product[i + j * 4] = 0.0;
      for (k = 0; k < 4; ++k) {
        product[i + 4 * j] += A[i + 4 * k] * B[k + 4 * j];
      }
    }
  }

  (void) memcpy (out, product, 16 * sizeof (sc_camera_coords_t));
}

static void
sc_camera_update_planes (sc_camera_t * camera)
{
  sc_camera_mat4x4_t  view, projection, transform;
  size_t              i;
  sc_camera_coords_t  inorm;

  sc_camera_get_view_mat (camera, view);
  sc_camera_get_projection_mat (camera, projection);
  sc_camera_mult_4x4_4x4 (projection, view, transform);

  sc_camera_mat4_transpose (transform);

  /*
     near = (0, 0, -1, -1)
     far = (0, 0, 1, -1)
     left = (-1, 0, 0, -1)
     right = (1, 0, 0, -1)
     up = (0, 1, 0, -1)
     down = (0, -1, 0, -1)
   */

  camera->frustum_planes[SC_CAMERA_PLANE_NEAR][0] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_NEAR][1] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_NEAR][2] = -1.;
  camera->frustum_planes[SC_CAMERA_PLANE_NEAR][3] = -1.;

  camera->frustum_planes[SC_CAMERA_PLANE_FAR][0] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_FAR][1] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_FAR][2] = 1.;
  camera->frustum_planes[SC_CAMERA_PLANE_FAR][3] = -1.;

  camera->frustum_planes[SC_CAMERA_PLANE_LEFT][0] = -1.;
  camera->frustum_planes[SC_CAMERA_PLANE_LEFT][1] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_LEFT][2] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_LEFT][3] = -1.;

  camera->frustum_planes[SC_CAMERA_PLANE_RIGHT][0] = 1.;
  camera->frustum_planes[SC_CAMERA_PLANE_RIGHT][1] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_RIGHT][2] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_RIGHT][3] = -1.;

  camera->frustum_planes[SC_CAMERA_PLANE_TOP][0] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_TOP][1] = 1.;
  camera->frustum_planes[SC_CAMERA_PLANE_TOP][2] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_TOP][3] = -1.;

  camera->frustum_planes[SC_CAMERA_PLANE_BOTTOM][0] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_BOTTOM][1] = -1.;
  camera->frustum_planes[SC_CAMERA_PLANE_BOTTOM][2] = 0.;
  camera->frustum_planes[SC_CAMERA_PLANE_BOTTOM][3] = -1.;

  for (i = 0; i < 6; ++i) {
    sc_camera_mat4_mul_v4_to_v4 (transform, camera->frustum_planes[i],
                                 camera->frustum_planes[i]);

    /*
       norm is bigger
       min (1/tan(field of view/2), 1)
       and smaller
       4 * max(1/tan(field of view/2), 1) + (2*(n + f + n*f) / (f - n))^2

       => no numerical issues if field of view and near and far are reasonable
     */
    inorm = 1. / sc_camera_vec3_norm (camera->frustum_planes[i]);
    camera->frustum_planes[i][0] *= inorm;
    camera->frustum_planes[i][1] *= inorm;
    camera->frustum_planes[i][2] *= inorm;
    camera->frustum_planes[i][3] *= inorm;
  }
}

static              sc_camera_coords_t
sc_camera_signed_dist (const sc_camera_vec3_t point,
                       const sc_camera_vec4_t plane)
{
  return point[0] * plane[0] + point[1] * plane[1] +
    point[2] * plane[2] + plane[3];
}


static inline void sc_camera_normalize_vec4(sc_camera_vec4_t vec)
{
    sc_camera_coords_t x, y, z, w;
    sc_camera_coords_t norm;
    sc_camera_coords_t inv;

    x = vec[0];
    y = vec[1];
    z = vec[2];
    w = vec[3];

    norm = (sc_camera_coords_t)sqrt(x * x + y * y + z * z + w * w);
    SC_ASSERT(norm > 0);

    inv = 1.0 / norm;

    vec[0] = x * inv;
    vec[1] = y * inv;
    vec[2] = z * inv;
    vec[3] = w * inv;
}
