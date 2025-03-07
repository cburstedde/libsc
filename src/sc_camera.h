/*
    scetch of a possible camera type interface
*/

/* View space has right handed coordinate system with z-axis pointing backwards,
   y-axis pointing up and x-axis pointing to the right
*/

#ifndef SC_CAMERA_H
#define SC_CAMERA_H

#include <sc_containers.h>/* sc_array_t */
/* p4est uses double most compoter graphics (GPU) float*/
typedef double sc_camera_coords_t; 

typedef sc_camera_coords_t sc_camera_vec3_t[3];
typedef sc_camera_coords_t sc_camera_vec4_t[4];
/* column major  */
typedef sc_camera_coords_t sc_camera_mat4x4_t[16];
typedef sc_camera_coords_t sc_camera_mat3x3_t[9]; 

typedef struct sc_camera
{
    /* extern */
    sc_camera_vec3_t position;

    sc_camera_vec4_t orientation; /* quaternion */
    /* or 
    * float up[3];
    * float front[3]; 
    */
    
    /* intern */
    double FOV; /* horizontal */
    /* double aspect_ratio; */ /* width/height */
    int width, height; /* Width and Height for the aspect ratio ()*/

    sc_camera_coords_t near, far; /* postive values */

} sc_camera_t;

/* matrix multiplikation */ /* could make size variable (TODO: add static) */
void sc_camera_mult_4x4_v4(const sc_camera_mat4x4_t mat, 
    const sc_camera_vec4_t vec, sc_camera_vec4_t out);

sc_camera_t *sc_camera_new(); 

void sc_camera_init(sc_camera_t *camera, sc_camera_vec3_t position, 
    sc_camera_vec4_t orientation, double FOV, int width, int height, 
    sc_camera_coords_t near, sc_camera_coords_t far);

void sc_camera_destroy(sc_camera_t *camera);

void sc_camera_look_at(sc_camera_t *camera, const sc_camera_vec3_t eye, 
    const sc_camera_vec3_t center, const sc_camera_vec3_t up);

/* also transformations for single points */

/* lots of other functions possible to move the camera around or zoom in */

void sc_camera_get_view(sc_camera_t *camera, sc_camera_mat4x4_t matrix);

void sc_camera_get_projection(sc_camera_t *camera, sc_camera_mat4x4_t matrix);

/* How the points should be passed to the function below and how to discard points */

/* points of form sc_camera_vec3_t? */
void sc_camera_view_transform(sc_camera_t *camera, sc_array_t *points_in,
     sc_array_t *points_out);

/* the idea is to return the indices of points that are in the camera view */
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
void sc_camera_clipping_post(sc_camera_t *camera, sc_array_t *points, 
    sc_array_t *indices);

/* does all steps in one go (world -> view -> canonical view volume (with clipping) */
void sc_camera_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out, sc_array_t *indices);


/* what format should planes have? (currently vec4 = (a,b,c,d) -> ax + by + cz + d = 0) */
void sc_camera_get_frustum(sc_camera_t *camera, sc_camera_vec4_t near, 
    sc_camera_vec4_t far, sc_camera_vec4_t left,
    sc_camera_vec4_t right, sc_camera_vec4_t up, sc_camera_vec4_t down);


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