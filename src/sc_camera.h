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

#ifndef SC_CAMERA_H
#define SC_CAMERA_H

/** \file sc_camera.h 
 * \ingroup sc_camera
 * 
 *  Camera data structure for computer graphic applications.
 *
 *  The [camera module page](\ref sc_camera) provides a detailed example of
 *  the rendering workflow.
 */

/** \defgroup sc_camera Camera
 * \ingroup sc
 * 
 * A camera data structure for maintaining the position, orientation and
 * view frustum of the camera.
 * 
 * ## Example Workflow: Rendering a point p with sc_camera
 * 
 * 1. **World Space:**  
 *    Initially, \b p lies in the right-handed world coordinate system.
 * 
 * 2. **View Transformation:**  
 *    \b p is transformed into the right-handed view coordinate system.
 *    In this system:
 *    - The camera is at the origin, looking down the negative z-axis.
 *    - The x-axis points to the right, and the y-axis points upwards.
 * 
 *    Transformation steps:
 *    - Translate p by the camera position.
 *    - Rotate \b p by the camera rotation.
 * 
 *    The camera rotation is stored as a unit quaternion q = (q_x, q_y, q_z, q_w),
 *    where (q_x, q_y, q_z) are the imaginary parts (i, j, k) and q_w is the 
 *    real part.
 *
 *    To rotate a point \b p = (p_x, p_y, p_z), treat it as a pure quaternion 
 *    (p_x i + p_y j + p_z k), and compute the conjugation:
 *      p' = q * p * q^{-1}
 * 
 *    Explicitly:
 *      p' = (q_x i + q_y j + q_z k + q_w) * (p_x i + p_y j + p_z k) * (-q_x i - q_y j - q_z k + q_w)
 *    where quaternion multiplication rules apply (i.e., i^2 = j^2 = k^2 = ijk = -1).
 *    The camera rotation is defined such that it rotates the world space so 
 *    that the camera looks along the negative z-axis. It is the \b inverse of 
 *    the rotation that would align the default camera orientation with the 
 *    preferred camera direction.
 * 
 *    In summary, the view transformation is:
 *    
 *      p_view = q * (\b p - camera->position) * q^-1
 * 
 * 3. **Projection to Normalized Device Coordinates (NDC):**  
 *    After the view transformation, \b p is projected into normalized device 
 *    coordinates (NDC):
 *    - The x- and y-coordinates represent the 2D position of the point in the 
 *      image.
 *    - The z-coordinate represents depth information, which is used for
 *      visibility and rendering order.
 * 
 *    The NDC space is left-handed, as in OpenGL:
 *    - The visible region is the cube [-1, 1]^3.
 *    - (x, y) = (-1, -1) is the bottom-left corner of the image.
 *    - (x, y) = (1, 1) is the top-right corner of the image.
 *    - Increasing x moves to the right, increasing y moves upwards.
 *    - For the z-axis:
 *        - z = -1  corresponds to the near clipping plane (closest to the camera),
 *        - z = 1 corresponds to the far clipping plane (farthest from the camera).
 * 
 *    The projection is defined by:
 *    - The **horizontal field of view (FOV)**: the opening angle in radians in 
 *      the x-direction.
 *    - The **aspect ratio**: the ratio of image width to height, which 
 *      determines the vertical field of view.
 *    - The **near** and **far** values: the positive distances from the camera 
 *      to the near and far clipping planes.
 * 
 *    Points closer than the near plane or farther than the far plane are clipped, 
 *    meaning they are not mapped inside the visible cube [-1, 1]^3.
 */

#include <sc_containers.h>

/** The data type of the coordinates for sc_camera
 */
/* p4est uses double, most computer graphic applications float on GPU.*/
/* Using float here would result in implicit conversions. */
typedef double      sc_camera_coords_t;

/** Points in R^3 for sc_camera.
 */
typedef sc_camera_coords_t sc_camera_vec3_t[3];

/** Points in R^4 for sc_camera.
 */
typedef sc_camera_coords_t sc_camera_vec4_t[4];

/** 4 times 4 matrix for sc_camera.
 * The entries of the matrix are in column-major order.
 */
typedef sc_camera_coords_t sc_camera_mat4x4_t[16];

/** 3 times 3 matrix for sc_camera.
 * The entries of the matrix are in column-major order.
 */
typedef sc_camera_coords_t sc_camera_mat3x3_t[9];

/** Represents a camera with parameters for rendering a 3D scene.
 * 
 * The sc_camera object stores essential properties of a camera,
 * including its position, rotation, field of view, viewport dimensions, 
 * and clipping planes. These parameters define how the camera projects 
 * the 3D world onto the 2D image plane.
 *
 */
typedef struct sc_camera
{
  sc_camera_vec3_t    position;
                             /**< The position of the camera in world coordinates. */

  sc_camera_vec4_t    rotation;
                              /**< Rotation from world space to view space, 
                               *   represented as a unit quaternion. */

  double              FOV;
              /**< Horizontal field of view (in radians), defining the frustum's
               *   opening angle horizontally. */

  int                 width;
              /**< Image width in pixels, used to determine the aspect ratio. */
  int                 height;
              /**< Image height in pixels, used to determine the aspect ratio. */

  sc_camera_coords_t  near;/**< Distance to the near clipping plane. */
  sc_camera_coords_t  far; /**< Distance to the far clipping plane. */

} sc_camera_t;

/** Creates a new camera structure with the default values (see sc_camera_init).
 *
 * \return Camera with default values.
 */
sc_camera_t        *sc_camera_new ();

/** Initializes a camera with the default parameters.
 * 
 * As default the camera is positioned at (0, 0, 1) and looking at (0, 0, 0) 
 * with the upwards direction (0, 1, 0). The default horizontal field of view is
 * Pi/2, the image has aspect ratio 1 (width = height) and near is set at 1/100
 * far at 100. The default is realised by the following values.
 * - position : (0, 0, 1)
 * - rotation : (0, 0, 0, 1) 
 * - FOV : Pi / 2 = 1.57079632679
 * - width : 1000
 * - height : 1000
 * - near : 0.01
 * - far : 100.0.
 * 
 * \param [out] camera Camera that is initialized.
 */
void                sc_camera_init (sc_camera_t * camera);

/** Destroys a camera structure.
 * \param [in] camera The camera to be destroyed.
 */
void                sc_camera_destroy (sc_camera_t * camera);


void sc_camera_position (sc_camera_t * camera, sc_camera_vec3_t position);

/* TODO : i have to specify axis and angle direction (right hand rule?) */

/* All rotation are in the right handed direction (meaning the camera rotates righthanded and 
  the objects rotate the other way i.e. left handed)*/

/* yaw is rotating around the up driection in our case the y-axis */
void sc_camera_yaw (sc_camera_t * camera, double angle);

/* pitch is the rotation around the right axis in our case the x-axis */
void sc_camera_pitch (sc_camera_t * camera, double angle);

/**
 *  roll is the rotation around the forward direction in our case the -z-axis 
 *  
 *  This means that we are rotating left-handed around the z-axis!!! 
 *  this is not the typical convention for euler angles
 */
void sc_camera_roll (sc_camera_t * camera, double angle);

void sc_camera_fov (sc_camera_t * camera, double angle);

void sc_camera_aspect_ratio (sc_camera_t * camera, int width, int height);

void sc_camera_clipping_dist(sc_camera_t * camera, sc_camera_coords_t near, 
  sc_camera_coords_t far);

/** Sets the position and rotation of the camera object.
 * The vectors (eye - center) and up have to be linear independent (thus also not zero).
 * \param [out] camera Camera that is changed.
 * \param [in] eye Position of the camera.
 * \param [in] center Point that is in the center of the image.
 * \param [in] up Upward direction of the camera.
 */
void sc_camera_look_at(sc_camera_t *camera, const sc_camera_vec3_t eye, 
  const sc_camera_vec3_t center, const sc_camera_vec3_t up);

/* 3D -> 3D */
void sc_camera_view_transform(sc_camera_t *camera, sc_array_t *points_in, 
  sc_array_t *points_out);

#endif
