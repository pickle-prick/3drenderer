#ifndef MATRIX_H
#define MATRIX_H
#include "vector.h"

// NOTE(@k): since we sometimes need to return a matrix from function, we can use float[4][4]
typedef struct {
        float m[4][4];
} mat4_t;

mat4_t mat4_eye(void);
mat4_t mat4_make_scale(float sx, float sy, float sz);
mat4_t mat4_make_translation(float tx, float ty, float tz);
mat4_t mat4_make_rotation_x(float r);
mat4_t mat4_make_rotation_y(float r);
mat4_t mat4_make_rotation_z(float r);
vec4_t mat4_mul_vec4(mat4_t m, vec4_t v);
mat4_t mat4_mul_mat4(mat4_t a, mat4_t b);
mat4_t mat4_make_orthographic(float fov, int wh, int ww, float zn, float zf);
mat4_t mat4_make_perspective(float fov, int wh, int ww, float zn, float zf);
mat4_t mat4_transpose(mat4_t m);
#endif
