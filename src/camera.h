#ifndef CAMERA_H
#define CAMERA_H
#include "vector.h"
#include "matrix.h"

typedef struct {
       vec3_t position;
       float yaw;
       float pitch;
} camera_t;

mat4_t mat4_look_at(vec3_t target_pos, vec3_t camera_pos, vec3_t up);
mat4_t mat4_from_camera(vec3_t camera_pos, float yaw, float pitch);
mat4_t rotate_around_it(vec3_t it, float rx, float ry, float rz);
#endif
