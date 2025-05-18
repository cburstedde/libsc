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

sc_camera_t        *
sc_camera_new ()
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

  camera->near = 0.01;
  camera->far = 100.0;
}

void
sc_camera_destroy (sc_camera_t * camera)
{
  SC_FREE (camera);
}

void sc_camera_position (sc_camera_t * camera, sc_camera_vec3_t position)
{
  SC_ASSERT (camera != NULL);

  memcpy(camera->position, position, sizeof(camera->position));
}

static void sc_camera_q_mult(const sc_camera_vec4_t q1, const sc_camera_vec4_t q2,
  sc_camera_vec4_t out)
{
  sc_camera_coords_t x1 = q1[0], y1 = q1[1], z1 = q1[2], w1 = q1[3]; // q1 = (i, j, k, real)
  sc_camera_coords_t x2 = q2[0], y2 = q2[1], z2 = q2[2], w2 = q2[3]; // q2 = (i, j, k, real)

  out[0] = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2; // i component
  out[1] = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2; // j component
  out[2] = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2; // k component
  out[3] = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2; // real part
}

void sc_camera_yaw (sc_camera_t *camera, double angle)
{
  SC_ASSERT (camera != NULL);

  sc_camera_vec4_t q;
  q[0] = 0.0;
  q[1] = sin(angle/2.0);
  q[2] = 0.0;
  q[3] = cos(angle/2.0);

  sc_camera_q_mult (q, camera->rotation, camera->rotation);
}

void sc_camera_pitch (sc_camera_t * camera, double angle)
{
  SC_ASSERT (camera != NULL);

  sc_camera_vec4_t q;
  q[0] = sin(angle/2.0);
  q[1] = 0.0;
  q[2] = 0.0;
  q[3] = cos(angle/2.0);

  sc_camera_q_mult (q, camera->rotation, camera->rotation);
}

void sc_camera_roll (sc_camera_t * camera, double angle)
{
  SC_ASSERT (camera != NULL);

  sc_camera_vec4_t q;
  q[0] = 0.0;
  q[1] = 0.0;
  q[2] = -sin(angle/2.0);
  q[3] = cos(angle/2.0);

  sc_camera_q_mult (q, camera->rotation, camera->rotation);
}

void sc_camera_fov (sc_camera_t * camera, double angle)
{
  SC_ASSERT (camera != NULL);

  camera->FOV = angle;
}

void sc_camera_aspect_ratio (sc_camera_t * camera, int width, int height)
{
  SC_ASSERT(camera != NULL);

  camera->width = width;
  camera->height = height;
}

void sc_camera_clipping_dist(sc_camera_t * camera, sc_camera_coords_t near, 
  sc_camera_coords_t far)
{
  SC_ASSERT(camera != NULL);
  SC_ASSERT(near > 0);
  SC_ASSERT(far > near);

  camera->near = near;
  camera->far = far;
}

/** The mathematics are for example described here: 
 * https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion */
static void sc_camera_mat_to_q(const sc_camera_mat3x3_t A, sc_camera_vec4_t q)
{
  sc_camera_coords_t t = A[0 + 3 * 0] + A[1 + 3 * 1] + A[2 + 3 * 2];

  if (t >= 0)
  {
    sc_camera_coords_t r = sqrt(1 + t);
    sc_camera_coords_t s = 1.0 / (2.0 * r);
    q[3] = r / 2.0;
    q[0] = (A[2 + 3 * 1] - A[1 + 3 * 2]) * s;
    q[1] = (A[0 + 3 * 2] - A[2 + 3 * 0]) * s;
    q[2] = (A[1 + 3 * 0] - A[0 + 3 * 1]) * s;
  }
  else 
  {
    if (A[0 + 3 * 0] >= A[1 + 3 * 1] && A[0 + 3 * 0] >= A[2 + 3 * 2])
    {
      sc_camera_coords_t r = sqrt(1 + A[0 + 3 * 0] - A[1 + 3 * 1] - A[2 + 3 * 2]);
      sc_camera_coords_t s = 1.0 / (2.0 * r);
      q[3] = (A[2 + 3 * 1] - A[1 + 3 * 2]) * s;
      q[0] = r / 2.0; 
      q[1] = (A[0 + 3 * 1] + A[1 + 3 * 0]) * s;
      q[2] = (A[2 + 3 * 0] + A[0 + 3 * 2]) * s;
    }
    else if (A[1 + 3 * 1] >= A[2 + 3 * 2])
    {
      sc_camera_coords_t r = sqrt(1 - A[0 + 3 * 0] + A[1 + 3 * 1] - A[2 + 3 * 2]);
      sc_camera_coords_t s = 1.0 / (2.0 * r);
      q[3] = (A[0 + 3 * 2] - A[2 + 3 * 0]) * s;
      q[0] = (A[1 + 3 * 0] + A[0 + 3 * 1]) * s; 
      q[1] = r / 2.0;
      q[2] = (A[2 + 3 * 1] + A[1 + 3 * 2]) * s;
    }
    else
    {
      sc_camera_coords_t r = sqrt(1 - A[0 + 3 * 0] - A[1 + 3 * 1] + A[2 + 3 * 2]);
      sc_camera_coords_t s = 1.0 / (2.0 * r);
      q[3] = (A[1 + 3 * 0] - A[0 + 3 * 1]) * s;
      q[0] = (A[2 + 3 * 0] + A[0 + 3 * 2]) * s; 
      q[1] = (A[2 + 3 * 1] + A[1 + 3 * 2]) * s;
      q[2] = r / 2.0;
    }
  }
}

/* TODO hier fehlen noch assertion zu degenerierten fällen */
void sc_camera_look_at(sc_camera_t *camera, const sc_camera_vec3_t eye, 
  const sc_camera_vec3_t center, const sc_camera_vec3_t up)
{ 
  // memcpy(camera->position, eye, sizeof (camera->position));

  // sc_camera_vec3_t z_new;
  // sc_camera_subtract(eye, center, z_new);
  // sc_camera_scalar(1.0/sc_camera_norm(z_new), z_new, z_new);

  // sc_camera_vec3_t x_new;
  // sc_camera_cross_prod(up, z_new, x_new);
  // sc_camera_scalar(1.0/sc_camera_norm(x_new), x_new, x_new);

  // sc_camera_vec3_t y_new;
  // sc_camera_cross_prod(z_new, x_new, y_new);

  // sc_camera_mat3x3_t rotation = {
  //     x_new[0], y_new[0], z_new[0],
  //     x_new[1], y_new[1], z_new[1],
  //     x_new[2], y_new[2], z_new[2]
  // };

  // sc_camera_vec4_t q;
  // sc_camera_mat3x3_to_quaternion(rotation, q);

  // memcpy(camera->rotation, q, sizeof (camera->rotation));
}
