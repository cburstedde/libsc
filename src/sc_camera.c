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

  camera->near_plane = 0.01;
  camera->far_plane = 100.0;
}

void
sc_camera_destroy (sc_camera_t * camera)
{
  SC_FREE (camera);
}
