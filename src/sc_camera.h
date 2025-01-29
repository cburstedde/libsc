/*
    scetch of a possible camera type interface
*/

/* View space has right handed coordinate system with z-axis pointing backwards,
   y-axis pointing up and x-axis pointing to the right
*/

#include /* sc_array_t */

typedef double sc_camera_coords_t; /* p4est uses double most compoter graphics (GPU) float*/

typedef sc_camera_coords_t sc_camera_vec3_t[3];
typedef sc_camera_coords_t sc_camera_vec4_t[4];

typedef sc_camera_coords_t sc_camera_mat4x4_t[16]; /* column major for easy to use export? */

typedef struct sc_camera
{
    /* extern */
    sc_camera_vec3_t position;

    sc_camera_vec4_t orientation; // quaternion
    /* or 
    * float up[3];
    * float front[3]; 
    */
    
    /* intern */
    double FOV; /* vertical? */
    double aspect_ratio; /* width/height */

    sc_camera_coords_t near, far; /* sign? */

} sc_camera_t;


sc_camera_t *sc_camera_new(); 

void sc_camera_init(sc_camera_vec3_t position, sc_camera_vec4_t orientation, double FOV, 
    double aspect_ratio, sc_camera_coords_t near, sc_camera_coords_t far);

sc_camera_t *sc_camera_destroy(sc_camera_t *camera);

void sc_camera_set_extern(sc_camera_t *camera, sc_camera_vec3_t position, 
    sc_camera_vec4_t orientation);

void sc_camera_set_intern(double FOV, double aspect_ratio, 
    sc_camera_coords_t near, sc_camera_coords_t far);

void sc_camera_look_at(sc_camera_t *camera, sc_camera_vec3_t eye, 
    sc_camera_vec3_t center, sc_camera_vec3_t up);

/* lots of other functions possible to move the camera around or zoom in */

void sc_camera_get_view(sc_camera_t *camera, sc_camera_mat4x4_t matrix);

void sc_camera_get_projection(sc_camera_t *camera, sc_camera_mat4x4_t matrix);

/* How the points should be passed to the function below and how to discard points */

/* points of form sc_camera_vec3_t? */
void sc_camera_view_transform(sc_camera_t *camera, sc_array_t *points_in,
     sc_array_t *points_out);

/* the idea is to return the indices of points that are in the camera view */
/* the function does not easily know if the points are already in camera space */
void sc_camera_clipping_pre(sc_camera_t *camera, sc_array_t *points, sc_array_t *indices);

/* probably no clipping in this function but what is with points that have z=0? */
/* transform to canonical view volume [-1,1]^3 */
/* what is if user wants to only transorm points with certain indices (because he called clipping_pre before)? */
void sc_camera_projection_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out);

/* clipping post has nothing to do with the camera itself so the carameter parameter is not needed */
void sc_camera_clipping_post(sc_camera_t *camera, sc_array_t *points, sc_array_t *indices);

/* does all steps in one go (world -> view -> canonical view volume (with clipping) */
void sc_camera_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out, sc_array_t *indices);


/* what format should planes have? (currently vec4 = (a,b,c,d) -> ax + by + cz + d = 0) */
void sc_camera_get_frustum(sc_camera_t *camera, vec4 near, vec4 far, vec4 left,
     vec4 right, vec4 up, vec4 down);