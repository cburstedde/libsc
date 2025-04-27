#ifndef SC_CAMERA_H
#define SC_CAMERA_H

/** \file sc_camera.h 
 * \ingroup sc_camera
 * 
 *  Camera datastructure for graphics.
 */

/** \defgroup sc_camera Camera
 * \ingroup sc
 * 
 * A camera datastructure for maintaining the position, orientation and 
 * view frustum of the camera.
 */

#include <sc_containers.h>

/* p4est uses double most compoter graphics (GPU) float*/
/** The datatype of the coordinates for sc_camera 
 */
typedef double sc_camera_coords_t; 

/** Points in R^3 for sc_camera
 */
typedef sc_camera_coords_t sc_camera_vec3_t[3];

/** Points in R^4 for sc_camera
 */
typedef sc_camera_coords_t sc_camera_vec4_t[4];

/** 4 times 4 matrix for sc_camera
 * The entries of the matrix are in column-major order.
 */
typedef sc_camera_coords_t sc_camera_mat4x4_t[16];

/** 4 times 4 matrix for sc_camera
 * The entries of the matrix are in column-major order.
 */
typedef sc_camera_coords_t sc_camera_mat3x3_t[9]; 

/**
 * @brief Represents a camera with parameters for rendering a 3D scene.
 * 
 * The `sc_camera` object stores essential properties of a camera,
 * including its position, orientation, field of view, viewport dimensions, 
 * and clipping planes. These parameters define how the camera projects 
 * the 3D world onto a 2D image.
 * 
 * The convention of a left handed coordinate system is used.
 * 
 * The camera coordinate system (image of the sc_camera_get_view transformation)
 *  is defined such that:
 * - The camera looks down the z-axis.
 * - The y-axis points upwards.
 * - The x-axis points to the right.
 * 
 * The canonical view volume (image of the view frustum after the transformation 
 * with sc_camera_get_projection) is defined as 
 * [-1,1]^3 with the x- and y-axis being the horizontal and vertical dimensions 
 * of the image and the z-axis being a depth parameter.
 */
typedef struct sc_camera {
    sc_camera_vec3_t position; /**< The position of the camera in world coordinates. */

    /**
     * @brief The rotation from the world coordinate system to the camera coordinate system.
     * 
     * The rotation is represented as a quaternion \( q = (x, y, z, w) \),
     * where \( w \) is the real part, and \( (x, y, z) \) are the imaginary components i,j and k.
     * The convention used for rotating a point \( p \) by \( q \) is:
     * \[
     * p' = qpq^{-1}
     * \]
     */
    sc_camera_vec4_t rotation;  

    double FOV; /**< The horizontal field of view in radians, 
                   defining the frustum's opening angle in the horizontal direction. */

    int width;  /**< The width of the image in pixels, only used to determine the aspect ratio. */
    int height; /**< The height of the image in pixels, only used to determine the aspect ratio. */

    sc_camera_coords_t near; /**< The positive distance from the camera to the near clipping plane. */
    sc_camera_coords_t far;  /**< The positive distance from the camera to the far clipping plane. */

} sc_camera_t;

/* matrix multiplikation */ /* could make size variable (TODO: add static) */
void sc_camera_mult_4x4_v4(const sc_camera_mat4x4_t mat, 
    const sc_camera_vec4_t vec, sc_camera_vec4_t out);

/** Creates a new camera structure with default values
 * As default the camera is positioned at (0, 0, 1) and looking at (0, 0, 0) 
 * with the upwards direction (0, 1, 0). The default horizontal field of view is
 * Pi/2, the image has aspect ratio 1 (width = height) and near is set at 1/100
 * far at 100. The default is realised by the following values.
 * position : (0, 0, 1)
 * rotation : (0, 0, 0, 1) 
 * FOV : Pi / 2 = 1.57079632679
 * width : 1000
 * height : 1000
 * near : 0.01
 * far : 100.0
 * 
 * \return Camera with default values
 */
sc_camera_t *sc_camera_new(); 

/** Initializes the camera with given parameters.
 * \param [in, out] camera Camera to be initialized.
 * \param [in] position Position of the camera.
 * \param [in] orientation Orientation of the camera as quaternion.
 * \param [in] FOV Horizontal field of view.
 * \param [in] width Width of the image.
 * \param [in] height Height of the image.
 * \param [in] near The positive distance from the camera to the near clipping plane.
 * \param [in] far The positive distance from the camera to the far clipping plane.
 */
void sc_camera_init(sc_camera_t *camera, sc_camera_vec3_t position, 
    sc_camera_vec4_t orientation, double FOV, int width, int height, 
    sc_camera_coords_t near, sc_camera_coords_t far);

/** Destroys a camera structure.
 * \param [in] camera The camera to be destroyed.
 */
void sc_camera_destroy(sc_camera_t *camera);

/** Sets the position and orientation of the camera object.
 * The vectors (eye - center) and up have to be linear independent (thus also not zero).
 * \param [in, out] camera Camera that is changed.
 * \param [in] eye Position of the camera.
 * \param [in] center Point that is in the center of the image.
 * \param [in] up Upward direction of the camera.
 */
void sc_camera_look_at(sc_camera_t *camera, const sc_camera_vec3_t eye, 
    const sc_camera_vec3_t center, const sc_camera_vec3_t up);

/* also transformations for single points */

/* lots of other functions possible to move the camera around or zoom in */

/** Calculates the view transformation matrix.
 * The matrix transforms a homogeneus point in world space p = (xw,yw,zw,w) to the 
 * translated and rotated point p' = (x'w,y'w,z'w,w) in camera space.Wie soll ich den erstennts that are in the camera view */
/* the function does not easily know if the points are already in camera space */
void sc_camera_clipping_pre(sc_camera_t *camera, sc_array_t *points, 
    sc_array_t *indices);

/* probably no clipping in this function but what is with points that have z=0? */
/* transform to canonical view volume [-1,1]^3 */
/* what is if user wants to only transorm points with certain indices 
(because he called clipping_pre before)? */
void sc_camera_projection_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out);

/* clipping post has nothing to do with the camera itself so the carameter 
    parameter is not needed */
void sc_camera_clipping_post(sc_array_t *points, 
    sc_array_t *indices);

/* does all steps in one go world -> view -> canonical view volume (with clipping) */
void sc_camera_transform_arr(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out, sc_array_t *indices);

int sc_camera_transform(sc_camera_t *camera, sc_camera_vec3_t point_in, 
    sc_camera_vec3_t point_out);

/**
 * input and output are 4-dimensional
 */
void sc_camera_view_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out);

/**
 * input and output are 4-dimensional
 */
void sc_camera_projection_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out);



void sc_camera_get_view(sc_camera_t *camera, sc_camera_mat4x4_t view_matrix);

void sc_camera_get_projection(sc_camera_t *camera, sc_camera_mat4x4_t proj_matrix);

/* what format should planes have? (currently vec4 = (a,b,c,d) -> ax + by + cz + d = 0) */

/** Calculates the 6 planes of the view frustum.
 * Every plane is represented in world space by 4 values (a, b, c, d) such
 * that ax + by + cz + d = 0 for points on the plane. A point is inside the view
 * if for all 6 planes (a,b,c,d), ax + by + cz + d <= 0.
 * 
 * The vector (a,b,c,d) is not normalized. 
 * 
 * \param [in] camera Camera to get the view frustum from.
 * \param [out] near The near clipping plane.
 * \param [out] far The far clipping plane.
 * \param [out] left The left clipping plane.
 * \param [out] right The right clipping plane.
 * \param [out] top The top clipping plane.
 * \param [out] bottom The bottom clipping plane.
 */
void sc_camera_get_frustum(sc_camera_t *camera, sc_camera_vec4_t near, 
    sc_camera_vec4_t far, sc_camera_vec4_t left,
    sc_camera_vec4_t right, sc_camera_vec4_t top, sc_camera_vec4_t bottom);

/** Calculaates the 6 corners of the view frustum.
 * \param [in] camera Camera to get the view frustum from.
 * \param [out] lbn Intersection point of the left, bottom and near clipping plane.
 * \param [out] rbn Intersection point of the right, bottom and near clipping plane.
 * \param [out] ltn Intersection point of the left, top and near clipping plane.
 * \param [out] rtn Intersection point of the right, top and near clipping plane.
 * \param [out] lbf Intersection point of the left, bottom and far clipping plane.
 * \param [out] rbf Intersection point of the right, bottom and far clipping plane.
 * \param [out] ltf Intersection point of the left, top and far clipping plane.
 * \param [out] rtf Intersection point of the right, top and far clipping plane.
 */
void sc_camera_get_frustum_corners(sc_camera_t *camera, sc_camera_vec3_t lbn, 
    sc_camera_vec3_t rbn, sc_camera_vec3_t ltn, sc_camera_vec3_t rtn, 
    sc_camera_vec3_t lbf, sc_camera_vec3_t rbf, sc_camera_vec3_t ltf,
    sc_camera_vec3_t rtf);
/*
void sc_camera_set_extern(sc_camera_t *camera, sc_camera_vec3_t position, 
    sc_camera_vec4_t orientation);

void sc_camera_set_intern(sc_camera_t *camera, double FOV, int width, int height, 
    sc_camera_coords_t near, sc_camera_coords_t far);
*/

#endif