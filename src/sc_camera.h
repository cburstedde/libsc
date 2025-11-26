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
 *    - Translate \b p by the camera position.
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
 *    - The **near_plane** and **far_plane** values: the positive distances from
 *      the camera to the near and far clipping planes.
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

/**
 * Defines the order of the 6 planes of the camera's view frustum.
 * The planes are enumerated as: near, far, left, right, top, bottom.
 */
typedef enum sc_camera_plane
{
  SC_CAMERA_PLANE_NEAR,
  SC_CAMERA_PLANE_FAR,
  SC_CAMERA_PLANE_LEFT,
  SC_CAMERA_PLANE_RIGHT,
  SC_CAMERA_PLANE_TOP,
  SC_CAMERA_PLANE_BOTTOM
} sc_camera_plane_t;


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

  sc_camera_coords_t  near_plane;/**< Distance to the near clipping plane. */
  sc_camera_coords_t  far_plane; /**< Distance to the far clipping plane. */

  sc_camera_vec4_t    frustum_planes[6];
      /**< The bounding planes of the view frustum (see \ref sc_camera_get_frustum).*/
} sc_camera_t;

/** Creates a new camera structure with the default values (see \ref
 * sc_camera_init).
 *
 * The returned pointer must be eventually freed with \ref sc_camera_destroy.
 *
 * \return Camera with default values.
 */
sc_camera_t        *sc_camera_new (void);

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
 * - near_plane : 0.01
 * - far_plane : 100.0.
 *
 * \param [out] camera Camera that is initialized.
 */
void                sc_camera_init (sc_camera_t * camera);

/** Destroys a camera structure.
 *
 * This function destroys a camera object allocated with \ref sc_camera_new.
 * It is not valid to call this function on stack-allocated camera objects.
 *
 * \param [in] camera The camera to be destroyed.
 */
void                sc_camera_destroy (sc_camera_t * camera);

/** Sets the position.
 *
 * \param [out] camera The camera object.
 * \param [in] position The new position of the camera object in world space.
 */
void                sc_camera_position (sc_camera_t * camera,
                                        const sc_camera_vec3_t position);

/** Rotating the camera around the up axis.
 *
 * The camera is rotated around the y-axis (up direction). The function rotates
 * the camera about angle (radians) amount by the right hand rule.
 * This means a positive angle rotates the camera to the left.
 *
 * \param [out] camera The camera object.
 * \param [in] angle The angle the camera is rotated.
 */
void                sc_camera_yaw (sc_camera_t * camera, double angle);

/** Rotating the camera around the axis to the right.
 *
 * The camera is rotated around the x-axis (right direction). The function rotates
 * the camera about angle (radians) amount by the right hand rule.
 * This means a positive angle rotates the camera up.
 *
 * \param [out] camera The camera object.
 * \param [in] angle The angle the camera is rotated.
 */
void                sc_camera_pitch (sc_camera_t * camera, double angle);

/** Rotating the camera around the backward axis.
 *
 * The camera is rotated around the z-axis (backwards direction). The function
 * rotates the camera about angle (radians) amount by the right hand rule.
 * This means a positive angle tilts the camera counter clockwise.
 *
 * \param [out] camera The camera object.
 * \param [in] angle The angle the camera is rotated.
 */
void                sc_camera_roll (sc_camera_t * camera, double angle);

/** Sets the horizontal field of view.
 *
 * \param [out] camera The camera object.
 * \param [in] angle The new horizontal field of view angle of the camera.
 */
void                sc_camera_fov (sc_camera_t * camera, double angle);

/** Sets the aspect ratio.
 *
 * The aspect ratio is determined by the ratio of a width and a height value.
 * \param [out] camera The camera object.
 * \param [in] width A width value for the view.
 * \param [in] height A height value for the view.
 */
void                sc_camera_aspect_ratio (sc_camera_t * camera, int width,
                                            int height);
/** Sets the clipping distances.
 *
 * \param [out] camera The camera object.
 * \param [in] near_plane The new distance from the camera to the near clipping plane.
 * \param [in] far_plane The new distance from the camera to the far clipping plane.
 */
void                sc_camera_clipping_dist (sc_camera_t * camera,
                                             sc_camera_coords_t near_plane,
                                             sc_camera_coords_t far_plane);

/** Sets the position and rotation of the camera object.
 * The vectors (eye - center) and up have to be linear independent (thus also not zero).
 * \param [out] camera Camera that is changed.
 * \param [in] eye Position of the camera.
 * \param [in] center Point that is in the center of the image.
 * \param [in] up Upward direction of the camera.
 */
void                sc_camera_look_at (sc_camera_t * camera,
                                       const sc_camera_vec3_t eye,
                                       const sc_camera_vec3_t center,
                                       const sc_camera_vec3_t up);

/**
 * Performs the view transformation from world space to view space.
 *
 * This function transforms an array of points from world coordinates into
 * view space as defined by the given camera. Each input point is a 3D vector
 * (x, y, z), and each output point is represented in the same way.
 *
 * The input array must store points as triples of coordinates, i.e.:
 *     points_in->elem_size = 3 * sizeof(sc_camera_coords_t)
 *
 * The output array stores points in the same format:
 *     points_out->elem_size = 3 * sizeof(sc_camera_coords_t)
 *
 * \param [in]  camera     The camera object defining the view transformation.
 * \param [in]  points_in  An array of points, each represented by 3 coordinates
 *                         (x, y, z) in world space.
 * \param [out] points_out An array that will contain the transformed points in
 *                         view space.
 */
void                sc_camera_view_transform (const sc_camera_t * camera,
                                              sc_array_t * points_in,
                                              sc_array_t * points_out);

/**
 * Returns the view transformation matrix.
 *
 * Let R ∈ ℝ^{3×3} be the rotation matrix corresponding to the camera's
 * rotation quaternion, and p ∈ ℝ³ be the camera's position vector. Let I be
 * the 3×3 identity matrix.
 * \a view_matrix is defined as:
 *
 * \code
 * [ R  -R*p ]   [ R  0 ]   [ I  -p ]
 * [ 0    1  ] = [ 0  1 ] * [ 0   1 ]
 * \endcode
 *
 * This matrix transforms coordinates from world space into the camera's
 * local (view) coordinate system.
 *
 * \param[in]  camera       The camera object defining the view transformation.
 * \param[out] view_matrix  The resulting 4×4 view transformation matrix in
 *                          column-major order. Must be allocated by the user.
 */
void
sc_camera_get_view_mat(const sc_camera_t *camera, sc_camera_mat4x4_t view_matrix);

/**
 * Returns the projection transformation matrix.
 *
 * The projection matrix transforms coordinates from view space into normalized
 * device coordinates (NDC).
 *
 * 1. Switch from right-handed to left-handed coordinate system by negating the z-axis.
 * 2. Apply the perpective projection into the cuboid
 *    [-2/s_x, 2/s_x] x [-2/s_y, 2/s_y] x [n,f] with use of homogeneous coordinates.
 *    Here s_x and s_y are width and height of the visible picture on the near plane.
 * 3. Map the cuboid to the NDC cube [-1, 1]^3.
 *
 * \code
 *
 * [ 2/s_x  0      0       0          ]   [ n   0   0      0   ]   [ 1  0  0  0 ]
 * [ 0      2/s_y  0       0          ]   [ 0   n   0      0   ]   [ 0  1  0  0 ]
 * [ 0      0      2/(f-n) -2nf/s_z-1 ] * [ 0   0   (f+n)  -fn ] * [ 0  0  -1 0 ]
 * [ 0      0      0       1          ]   [ 0   0   1      0   ]   [ 0  0  0  1 ]
 *
 * \endcode
 *
 * \param [in] camera       The camera object defining the projection.
 * \param [out] proj_matrix The resulting 4x4 projection transformation matrix
 *                          in column-major order. Must be allocated by the user.
 */
void
sc_camera_get_projection_mat (const sc_camera_t * camera,
                              sc_camera_mat4x4_t proj_matrix);

/**
 * Performs the projection transformation from view space to normalized device
 * coordinates.
 *
 * This function transforms an array of points from view space into normalized
 * device coordinates (NDC) as described in the example. Each input point is a
 * 3D vector (x, y, z), and each output point is represented in homogeneous
 * coordinates (x, y, z, w).
 *
 * The input array must store points as triples of coordinates, i.e.:
 *     points_in->elem_size = 3 * sizeof(sc_camera_coords_t)
 *
 * The output array stores points as quadruples of coordinates, i.e.:
 *     points_out->elem_size = 4 * sizeof(sc_camera_coords_t)
 *
 * \param [in]  camera     The camera object defining the projection.
 * \param [in]  points_in  An array of points, each represented by 3 coordinates
 *                         (x, y, z) to be transformed.
 * \param [out] points_out An array that will contain the transformed points in
 *                         homogeneous coordinates (x, y, z, w).
 */
void                sc_camera_projection_transform (const sc_camera_t * camera,
                                                    sc_array_t * points_in,
                                                    sc_array_t * points_out);

/**
 * Returns the 6 planes of the view frustum.
 *
 * This function returns the bounding planes of the visible world space (the
 * view frustum). The planes are ordered as: near, far, left, right, top, and
 * bottom (see enum \ref sc_camera_plane). The naming is from the perspective of
 * the  camera; for example, the left plane bounds the view to the left.
 * The near and far planes bound the field of view so that objects, that are too
 * close or too far, are not visible. The distances to these planes are defined
 * by the user in the camera object.
 *
 * The array `planes` contains 6 entries, each consisting of 4 values (a, b, c, d):
 *     planes->elem_count = 6
 *     planes->elem_size  = 4 * sizeof(sc_camera_coords_t)
 *
 * Each plane is represented by its outward-facing normal vector (a, b, c) and
 * offset d, such that the plane equation is:
 *     a*x + b*y + c*z + d = 0
 * Points (x, y, z) satisfying this equation lie on the plane.
 *
 * \param [in]  camera The camera object.
 * \param [out] planes The 6 planes of the view frustum, in the order near, far,
 *                     left, right, top, and bottom (see enum \ref
 *                     sc_camera_plane).
 *                     This is a read-only view with the same lifetime as the
 *                     camera object.
 */
void                sc_camera_get_frustum (const sc_camera_t * camera,
                                           sc_array_t * planes);

/**
 * Determines the indices of points that are visible within the camera's view frustum.
 *
 * This function takes an array of 3D points in world space (x, y, z) and performs
 * clipping against the camera's view frustum. It returns the indices of the points
 * that are inside the frustum and thus visible to the camera.
 *
 * The input array `points` must store the 3D coordinates of the points, i.e.:
 *     points->elem_size = 3 * sizeof(sc_camera_coords_t)
 *
 * The output array `indices` stores the indices of the points from `points` that
 * are inside the view frustum. Each index is stored as a `size_t` value.
 * Thus \a indices must be initialized \ref sc_array with elem_size = sizeof(size_t).
 *
 * \param [in]  camera  The camera object.
 * \param [in]  points  An array of 3D points in world space (x, y, z).
 * \param [out] indices The indices (as size_t) of the points that are visible
 *                      within the camera's view frustum.
 */
void                sc_camera_clipping_pre (const sc_camera_t * camera,
                                            sc_array_t * points,
                                            sc_array_t * indices);

/**
 * Computes the signed distances of a point to the 6 planes of the camera's view
 * frustum.
 *
 * For a point in world space, this function calculates its signed orthogonal
 * distance to each frustum plane (near, far, left, right, top, bottom), stored
 * in `distances[0..5]` in this order, which must be allocated by the user.
 *
 * A positive distance means that a point lies on the outside side of this plane.
 * If all values are negative the point lies in the view frustum.
 *
 * \param [in]  camera    The camera object.
 * \param [in]  point     A 3D point in world space.
 * \param [out] distances Six signed distances (sc_camera_coords_t) to the
 *                        frustum planes. Must be allocated on input.
 */
void                sc_camera_frustum_dist (const sc_camera_t * camera,
                                            const sc_camera_vec3_t point,
                                            sc_camera_coords_t distances[6]);

#endif
