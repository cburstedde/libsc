#include <math.h>

#include <sc_containers.h>

#include <sc_camera.h>

/* hier erstmal das ganze mathe zeug */

/* matrix multiplikation */ /* could make size variable (TODO: add static) */
void sc_camera_mult_4x4_v4(const sc_camera_mat4x4_t mat, 
    const sc_camera_vec4_t vec, sc_camera_vec4_t out)
{
    sc_camera_vec4_t result;
    for (size_t i = 0; i < 4; ++i)
    {
        result[i] = 0.0;

        for (size_t j = 0; j < 4; ++j)
        {
            result[i] += mat[i + 4 * j] * vec[j];
        }
    }

    memcpy(out, result, 4 * sizeof(sc_camera_coords_t));
}

static void sc_camera_mult_v4_4x4(const sc_camera_vec4_t vec, const sc_camera_mat4x4_t mat,
    sc_camera_vec4_t out)
{
    sc_camera_vec4_t result;
    for (size_t i = 0; i < 4; ++i)
    {
        result[i] = 0.0;

        for (size_t j = 0; j < 4; ++j)
        {
            result[i] += mat[j + 4 * i] * vec[j];
        }
    }

    memcpy(out, result, 4 * sizeof(sc_camera_coords_t));
}

/* Copy paste */
static int sc_camera_gluInvertMatrix(const sc_camera_mat4x4_t m, 
    sc_camera_mat4x4_t invOut)
{
    sc_camera_mat4x4_t inv;
    sc_camera_coords_t det;
    size_t i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return 0;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return 1;
}

/* out = A * B */
static void sc_camera_mult_4x4_4x4(const sc_camera_mat4x4_t A, const sc_camera_mat4x4_t B, 
    sc_camera_mat4x4_t out)
{
    sc_camera_mat4x4_t product;
    /* i index of rows of product */
    for (size_t i = 0; i < 4; ++i)
    {
        /* j index of columns of product */
        for (size_t j = 0; j < 4; ++j)
        {
            product[i + j * 4] = 0.0;
            for (size_t k = 0; k < 4; ++k)
            {
                product[i + 4 * j] += A[i + 4 * k] * B[k + 4 * j];
            }
        }
    }

    memcpy(out, product, 16 * sizeof(sc_camera_coords_t));
}

/* cross product out = a x b*/
static inline void sc_camera_cross_prod(const sc_camera_vec3_t a, 
    const sc_camera_vec3_t b, sc_camera_vec3_t out)
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static inline sc_camera_coords_t sc_camera_norm(const sc_camera_vec3_t x)
{
    /* this assumes coord_t = double so solution with macro or wrapper function is needed*/
    return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
}

/* scalar out = alpha * x */
static inline void sc_camera_scalar(sc_camera_coords_t alpha, 
    const sc_camera_vec3_t x, sc_camera_vec3_t out)
{
    out[0] = alpha * x[0];
    out[1] = alpha * x[1];
    out[2] = alpha * x[2];
}

/* out =  a + b */
static inline void sc_camera_add(const sc_camera_vec3_t a, 
    const sc_camera_vec3_t b, sc_camera_vec3_t out)
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

/* out =  a - b */
static inline void sc_camera_subtract(const sc_camera_vec3_t a, 
    const sc_camera_vec3_t b, sc_camera_vec3_t out)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

// TODO : remove
static void print_mat(const sc_camera_coords_t *mat, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; j < n; j++)
        {
            printf("%f ", mat[i + n * j]);
        }
        printf("\n");
    }
}

static void print_vec(const sc_camera_coords_t *vec, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        printf("%f ", vec[i]);
    }
    printf("\n");
}


/* TODO : the sqrt function used is only for double */
/* the code is from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
    with some modifications to fit in this context */
/* See also https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf */
/* matrix to quaternion */
static inline void sc_camera_mat3x3_to_quaternion(const sc_camera_mat3x3_t a, 
    sc_camera_vec4_t q)
{
    sc_camera_coords_t trace = a[0 + 3*0] + a[1 + 3*1] + a[2 + 3*2];
    if( trace > 0 ) {
        sc_camera_coords_t s = 0.5 / sqrt(trace + 1.0);
        q[3] = 0.25 / s;
        q[0] = ( a[2 + 3*1] - a[1 + 3*2] ) * s;
        q[1] = ( a[0 + 3*2] - a[2 + 3*0] ) * s;
        q[2] = ( a[1 + 3*0] - a[0 + 3*1] ) * s;
    } else {
        if ( a[0 + 3*0] > a[1 + 3*1] && a[0 + 3*0] > a[2 + 3*2] ) {
        sc_camera_coords_t s = 2.0 * sqrt( 1.0 + a[0 + 3*0] - a[1 + 3*1] - a[2 + 3*2]);
        q[3] = (a[2 + 3*1] - a[1 + 3*2] ) / s;
        q[0] = 0.25 * s;
        q[1] = (a[0 + 3*1] + a[1 + 3*0] ) / s;
        q[2] = (a[0 + 3*2] + a[2 + 3*0] ) / s;
        } else if (a[1 + 3*1] > a[2 + 3*2]) {
        sc_camera_coords_t s = 2.0 * sqrt( 1.0 + a[1 + 3*1] - a[0 + 3*0] - a[2 + 3*2]);
        q[3] = (a[0 + 3*2] - a[2 + 3*0] ) / s;
        q[0] = (a[0 + 3*1] + a[1 + 3*0] ) / s;
        q[1] = 0.25 * s;
        q[2] = (a[1 + 3*2] + a[2 + 3*1] ) / s;
        } else {
        sc_camera_coords_t s = 2.0 * sqrt( 1.0 + a[2 + 3*2] - a[0 + 3*0] - a[1 + 3*1] );
        q[3] = (a[1 + 3*0] - a[0 + 3*1] ) / s;
        q[0] = (a[0 + 3*2] + a[2 + 3*0] ) / s;
        q[1] = (a[1 + 3*2] + a[2 + 3*1] ) / s;
        q[2] = 0.25 * s;
        }
    }
}


/* quaternion zu matrix */


sc_camera_t *sc_camera_new()
{
    sc_camera_t *camera = SC_ALLOC (sc_camera_t, 1);

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

    return camera;
} 

void sc_camera_init(sc_camera_t *camera, sc_camera_vec3_t position, 
    sc_camera_vec4_t orientation, double FOV, int width, int height, 
    sc_camera_coords_t near, sc_camera_coords_t far)
{
    memcpy(camera->position, position, sizeof (camera->position));
    memcpy(camera->rotation, orientation, sizeof (camera->rotation));
    camera->FOV = FOV;
    camera->width = width;
    camera->height = height;
    /* maybe assert here that the values are positive */
    camera->near = near;
    camera->far = far;
}

void sc_camera_destroy(sc_camera_t *camera)
{
    SC_FREE (camera);
}

/* TODO hier fehlen noch assertion zu degenerierten fällen */
void sc_camera_look_at(sc_camera_t *camera, const sc_camera_vec3_t eye, 
    const sc_camera_vec3_t center, const sc_camera_vec3_t up)
{ 
    memcpy(camera->position, eye, sizeof (camera->position));

    sc_camera_vec3_t z_new;
    sc_camera_subtract(eye, center, z_new);
    sc_camera_scalar(1.0/sc_camera_norm(z_new), z_new, z_new);

    sc_camera_vec3_t x_new;
    sc_camera_cross_prod(up, z_new, x_new);
    sc_camera_scalar(1.0/sc_camera_norm(x_new), x_new, x_new);

    sc_camera_vec3_t y_new;
    sc_camera_cross_prod(z_new, x_new, y_new);

    sc_camera_mat3x3_t rotation = {
        x_new[0], y_new[0], z_new[0],
        x_new[1], y_new[1], z_new[1],
        x_new[2], y_new[2], z_new[2]
    };

    sc_camera_vec4_t q;
    sc_camera_mat3x3_to_quaternion(rotation, q);

    memcpy(camera->rotation, q, sizeof (camera->rotation));
}

int sc_camera_transform(sc_camera_t *camera, sc_camera_vec3_t point_in, 
    sc_camera_vec3_t point_out)
{
    sc_camera_vec4_t p = {point_in[0], point_in[1], point_in[2], 1.0};

    sc_camera_mat4x4_t transformation;
    sc_camera_get_view(camera, transformation);
    sc_camera_mult_4x4_v4(transformation, p, p);

    sc_camera_get_projection(camera, transformation);
    sc_camera_mult_4x4_v4(transformation, p, p);

    if (p[0] <= -p[3] || p[0] >= p[3])
    {
        return 0;
    }
    if (p[1] <= -p[3] || p[1] >= p[3])
    {
        return 0;
    }
    if (p[2] <= -p[3] || p[2] >= p[3])
    {
        return 0;
    }

    sc_camera_scalar(1./p[3], p, point_out);

    return 1;
}

/* also transformations for single points */

/* lots of other functions possible to move the camera around or zoom in */

void sc_camera_get_view(sc_camera_t *camera, sc_camera_mat4x4_t view_matrix)
{
    /* calculate rotation matrix from the quaternion and write it in upper left block*/
    sc_camera_coords_t xx = camera->rotation[0] * camera->rotation[0];
    sc_camera_coords_t yy = camera->rotation[1] * camera->rotation[1];
    sc_camera_coords_t zz = camera->rotation[2] * camera->rotation[2];
    sc_camera_coords_t wx = camera->rotation[3] * camera->rotation[0];
    sc_camera_coords_t wy = camera->rotation[3] * camera->rotation[1];
    sc_camera_coords_t wz = camera->rotation[3] * camera->rotation[2];
    sc_camera_coords_t xy = camera->rotation[0] * camera->rotation[1];
    sc_camera_coords_t xz = camera->rotation[0] * camera->rotation[2];
    sc_camera_coords_t yz = camera->rotation[1] * camera->rotation[2];

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

    view_matrix[12] = 0.0;
    view_matrix[13] = 0.0;
    view_matrix[14] = 0.0;
    view_matrix[15] = 1.0;

    sc_camera_mat4x4_t translation_matrix = {
        1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        -camera->position[0], -camera->position[1], -camera->position[2], 1. 
    };

    sc_camera_mult_4x4_4x4(view_matrix, translation_matrix, view_matrix);
}

void sc_camera_get_projection(sc_camera_t *camera, sc_camera_mat4x4_t proj_matrix)
{
    /* the factor 2 could be reduced */
    sc_camera_coords_t s_x = 2.0 * camera->near * tan(camera->FOV / 2.0);
    sc_camera_coords_t s_y = s_x * ((sc_camera_coords_t) camera->height / 
        (sc_camera_coords_t) camera->width); 
    sc_camera_coords_t s_z = camera->far - camera->near;

    proj_matrix[0] = 2.0 * camera->near / s_x;
    proj_matrix[1] = 0.0;
    proj_matrix[2] = 0.0;
    proj_matrix[3] = 0.0;

    proj_matrix[4] = 0.0;
    proj_matrix[5] = 2.0 * camera->near / s_y;
    proj_matrix[6] = 0.0;
    proj_matrix[7] = 0.0;

    proj_matrix[8] = 0.0;
    proj_matrix[9] = 0.0;
    proj_matrix[10] = -(camera->near + camera->far) / s_z;
    proj_matrix[11] = -1.0;

    proj_matrix[12] = 0.0;
    proj_matrix[13] = 0.0;
    proj_matrix[14] = -(2.0 * camera->near * camera->far) / s_z;
    proj_matrix[15] = 0.0;
}

static void sc_camera_mult_4x4_v4_for_each(sc_camera_mat4x4_t mat, sc_array_t *in, 
    sc_array_t *out)
{
    SC_ASSERT(points_in->elem_count == points_out->elem_count);
    SC_ASSERT(points_in->elem_size == 4 * sizeof(sc_camera_coords_t));
    SC_ASSERT(points_out->elem_size == 4 * sizeof(sc_camera_coords_t));

    for (size_t i = 0; i < points_in->elem_count; ++i)
    {
        sc_camera_coords_t *in = (sc_camera_coords_t *) sc_array_index(points_in, i);
        sc_camera_coords_t *out = (sc_camera_coords_t *) sc_array_index(points_out, i);

        sc_camera_mult_4x4_v4(mat, in, out);
    }
}
/**
 * i could make this less redundant by creating a function that applies the transform
 */
void sc_camera_view_transform(sc_camera_t *camera, sc_array_t *points_in,
     sc_array_t *points_out)
{
    sc_camera_mat4x4_t transformation;
    sc_camera_get_view(camera, transformation);

    sc_camera_mult_4x4_v4_for_each(transformation, points_in, points_out);
}

void sc_camera_projection_transform(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out)
{
    sc_camera_mat4x4_t transformation;
    sc_camera_get_projection(camera, transformation);

    sc_camera_mult_4x4_v4_for_each(transformation, points_in, points_out);
}

void sc_camera_clipping_post(sc_array_t *points, 
    sc_array_t *indices)
{
    SC_ASSERT(points->elem_size == 4 * sizeof(sc_camera_coords_t));
    SC_ASSERT(indices->elem_size == sizeof(size_t));

    sc_array_reset(indices);

    for (size_t i = 0; i < points->elem_count; ++i)
    {
        sc_camera_coords_t *p = sc_array_index(points, i);

        if (p[0] <= -p[3] || p[0] >= p[3])
        {
            continue;
        }
        if (p[1] <= -p[3] || p[1] >= p[3])
        {
            continue;
        }
        if (p[2] <= -p[3] || p[2] >= p[3])
        {
            continue;
        }

        sc_array_push(indices, i);
    }
}

void sc_camera_transform_arr(sc_camera_t *camera, sc_array_t *points_in, 
    sc_array_t *points_out, sc_array_t *indices)
{
    sc_camera_mat4x4_t view;
    sc_camera_get_view(camera, view);

    sc_camera_mat4x4_t projection;
    sc_camera_get_projection(camera, projection);

    sc_camera_mat4x4_t transformation;
    sc_camera_mult_4x4_4x4(projection, view, transformation);

    sc_camera_mult_4x4_v4_for_each(transformation, points_in, points_out);

    sc_camera_clipping_post(points_out, indices);

    // perspective division
    for (size_t i = 0; i < indices->elem_count; ++i)
    {
        size_t j = *((size_t *) sc_array_index(indices, i));

        sc_camera_coords_t *p = (sc_camera_coords_t *) sc_array_index(points_out, j);
        
        p[0] /= p[3];
        p[1] /= p[3];
        p[2] /= p[3];
    }
}

// /* the idea is to return the indices of points that are in the camera view */
// /* the function does not easily know if the points are already in camera space */
// void sc_camera_clipping_pre(sc_camera_t *camera, sc_array_t *points, 
//     sc_array_t *indices);

// /* probably no clipping in this function but what is with points that have z=0? */
// /* transform to canonical view volume [-1,1]^3 */
// /* what is if user wants to only transorm points with certain indices 
// (because he called clipping_pre before)? */
// void sc_camera_projection_transform(sc_camera_t *camera, sc_array_t *points_in, 
//     sc_array_t *points_out);

// /* clipping post has nothing to do with the camera itself so the carameter 
//     parameter is not needed */
// void sc_camera_clipping_post(sc_camera_t *camera, sc_array_t *points, 
//     sc_array_t *indices);

// /* does all steps in one go (world -> view -> canonical view volume (with clipping) */
// void sc_camera_transform(sc_camera_t *camera, sc_array_t *points_in, 
//     sc_array_t *points_out, sc_array_t *indices);

/* what format should planes have? (currently vec4 = (a,b,c,d) -> ax + by + cz + d = 0) */
void sc_camera_get_frustum(sc_camera_t *camera, sc_camera_vec4_t near, 
    sc_camera_vec4_t far, sc_camera_vec4_t left,
    sc_camera_vec4_t right, sc_camera_vec4_t top, sc_camera_vec4_t bottom)
{
    sc_camera_mat4x4_t view;
    sc_camera_get_view(camera, view);

    sc_camera_mat4x4_t projection;
    sc_camera_get_projection(camera, projection);

    /* TODO : maybe i want to use only 2 matrices and not 3 */
    sc_camera_mat4x4_t transform;
    sc_camera_mult_4x4_4x4(projection, view, transform);

    print_mat(projection, 4);

    /* 
    near = (0, 0, -1, -1)
    far = (0, 0, 1, -1)
    left = (-1, 0, 0, -1)
    right = (1, 0, 0, -1)
    up = (0, 1, 0, -1)
    down = (0, -1, 0, -1) 
    */

    near[0] = 0.;
    near[1] = 0.;
    near[2] = -1.;
    near[3] = -1.;

    far[0] = 0.;
    far[1] = 0.;
    far[2] = 1.;
    far[3] = -1.;

    left[0] = -1.;
    left[1] = 0.;
    left[2] = 0.;
    left[3] = -1.;

    right[0] = 1.;
    right[1] = 0.;
    right[2] = 0.;
    right[3] = -1.;

    top[0] = 0.;
    top[1] = 1.;
    top[2] = 0.;
    top[3] = -1.;

    bottom[0] = 0.;
    bottom[1] = -1.;
    bottom[2] = 0.;
    bottom[3] = -1.;

    sc_camera_mult_v4_4x4(near, transform, near);
    sc_camera_mult_v4_4x4(far, transform, far);
    sc_camera_mult_v4_4x4(left, transform, left);
    sc_camera_mult_v4_4x4(right, transform, right);
    sc_camera_mult_v4_4x4(top, transform, top);
    sc_camera_mult_v4_4x4(bottom, transform, bottom);
}

void sc_camera_get_frustum_corners(sc_camera_t *camera, sc_camera_vec3_t lbn, 
    sc_camera_vec3_t rbn, sc_camera_vec3_t ltn, sc_camera_vec3_t rtn, 
    sc_camera_vec3_t lbf, sc_camera_vec3_t rbf, sc_camera_vec3_t ltf,
    sc_camera_vec3_t rtf)
{
    sc_camera_mat4x4_t view;
    sc_camera_get_view(camera, view);

    sc_camera_mat4x4_t projection;
    sc_camera_get_projection(camera, projection);

    /* TODO : maybe i want to use only 2 matrices and not 3 */
    sc_camera_mat4x4_t inv_transform;
    sc_camera_mult_4x4_4x4(projection, view, inv_transform);
    sc_camera_gluInvertMatrix(inv_transform, inv_transform);

    printf("(PV)^-1 : \n");
    print_mat(inv_transform, 4);

    // sc_camera_vec4_t corners[8];

    // for (size_t i = 0; i < 8; ++i)
    // {

    // }

    sc_camera_vec4_t    lbn_w = {-1., -1., -1., 1.},  
                        rbn_w = { 1., -1., -1., 1.},  
                        ltn_w = {-1.,  1., -1., 1.},  
                        rtn_w = { 1.,  1., -1., 1.},  
                        lbf_w = {-1., -1.,  1., 1.},  
                        rbf_w = { 1., -1.,  1., 1.},  
                        ltf_w = {-1.,  1.,  1., 1.},  
                        rtf_w = { 1.,  1.,  1., 1.};

    sc_camera_mult_4x4_v4(inv_transform, lbn_w, lbn_w);
    sc_camera_mult_4x4_v4(inv_transform, rbn_w, rbn_w);
    sc_camera_mult_4x4_v4(inv_transform, ltn_w, ltn_w);
    sc_camera_mult_4x4_v4(inv_transform, rtn_w, rtn_w);
    sc_camera_mult_4x4_v4(inv_transform, lbf_w, lbf_w);
    sc_camera_mult_4x4_v4(inv_transform, rbf_w, rbf_w);
    sc_camera_mult_4x4_v4(inv_transform, ltf_w, ltf_w);
    sc_camera_mult_4x4_v4(inv_transform, rtf_w, rtf_w);

    sc_camera_scalar(1. / lbn_w[3], lbn_w, lbn);
    sc_camera_scalar(1. / rbn_w[3], rbn_w, rbn);
    sc_camera_scalar(1. / ltn_w[3], ltn_w, ltn);
    sc_camera_scalar(1. / rtn_w[3], rtn_w, rtn);
    sc_camera_scalar(1. / lbf_w[3], lbf_w, lbf);
    sc_camera_scalar(1. / rbf_w[3], rbf_w, rbf);
    sc_camera_scalar(1. / ltf_w[3], ltf_w, ltf);
    sc_camera_scalar(1. / rtf_w[3], rtf_w, rtf);
}

// void sc_camera_transform(sc_camera_t *camera, sc_array_t *points_in, 
//     sc_array_t *points_out, sc_array_t *indices)
// {

// }