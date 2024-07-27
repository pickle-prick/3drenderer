#include "vector.h"
#include <math.h>

/////////////////////////////////////////////////////////////////////////////////////////
// 3d vector functions
/////////////////////////////////////////////////////////////////////////////////////////
vec3_t vec3_rotate_z(vec3_t v, float angle) {
        vec3_t rotated_vector = {
                .x = v.x * cosf(angle) - v.y * sinf(angle),
                .y = v.x * sinf(angle) + v.y * cosf(angle),
                .z = v.z,
        };
        return rotated_vector;
}

vec3_t vec3_rotate_x(vec3_t v, float angle) {
        vec3_t rotated_vector = {
                .x = v.x,
                .y = v.y * cosf(angle) - v.z * sinf(angle),
                .z = v.y * sinf(angle) + v.z * cosf(angle),
        };
        return rotated_vector;
}

vec3_t vec3_rotate_y(vec3_t v, float angle) {
        vec3_t rotated_vector = {
                .x = v.x * cosf(angle) - v.z * sinf(angle),
                .y = v.y,
                .z = v.x * sinf(angle) + v.z * cosf(angle),
        };
        return rotated_vector;
}

vec3_t vec3_rotate(vec3_t v, vec3_t rotation) {
        vec3_t p = vec3_rotate_x(v, rotation.x);
        p = vec3_rotate_y(p, rotation.y);
        p = vec3_rotate_z(p, rotation.z);
        return p;
}

float vec3_length(vec3_t v) { return sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

vec3_t vec3_add(vec3_t a, vec3_t b) {
        vec3_t out = {
                .x = a.x + b.x,
                .y = a.y + b.y,
                .z = a.z + b.z,
        };
        return out;
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
        vec3_t out = {
                .x = a.x - b.x,
                .y = a.y - b.y,
                .z = a.z - b.z,
        };
        return out;
}

vec3_t vec3_mul(vec3_t v, float factor) {
        vec3_t out = {
                .x = v.x * factor,
                .y = v.y * factor,
                .z = v.z * factor,
        };
        return out;
}

vec3_t vec3_div(vec3_t v, float factor) {
        vec3_t out = {
                .x = v.x / factor,
                .y = v.y / factor,
                .z = v.z / factor,
        };
        return out;
}

vec3_t vec3_cross(vec3_t a, vec3_t b) {
        vec3_t n = {
                .x = a.y * b.z - a.z * b.y,
                .y = a.z * b.x - a.x * b.z,
                .z = a.x * b.y - a.y * b.x,
        };
        return n;
}

float vec3_dot(vec3_t a, vec3_t b) {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

void vec3_normalize(vec3_t *v) {
        float length = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
        v->x /= length;
        v->y /= length;
        v->z /= length;
}

vec3_t vec3_inverse(vec3_t v) {
        vec3_t ret = {
                .x = -v.x,
                .y = -v.y,
                .z = -v.z,
        };
        return ret;
}
/////////////////////////////////////////////////////////////////////////////////////////

// 2d vector functions
/////////////////////////////////////////////////////////////////////////////////////////
float vec2_length(vec2_t v) { return sqrt(v.x * v.x + v.y * v.y); }

vec2_t vec2_add(vec2_t a, vec2_t b) {
        vec2_t out = {
                .x = a.x + b.x,
                .y = a.y + b.y,
        };
        return out;
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
        vec2_t out = {
                .x = a.x - b.x,
                .y = a.y - b.y,
        };
        return out;
}

vec2_t vec2_mul(vec2_t v, float factor) {
        vec2_t out = {
                .x = v.x * factor,
                .y = v.y * factor,
        };
        return out;
}

vec2_t vec2_div(vec2_t v, float factor) {
        vec2_t out = {
                .x = v.x / factor,
                .y = v.y / factor,
        };
        return out;
}

float vec2_dot(vec2_t a, vec2_t b) { return a.x * b.x + a.y * b.y; }

float vec2_cross(vec2_t a, vec2_t b) { return (a.x * b.y) - (a.y * b.x); }

void vec2_normalize(vec2_t *v) {
        float length = sqrt(v->x * v->x + v->y * v->y);
        v->x /= length;
        v->y /= length;
}
/////////////////////////////////////////////////////////////////////////////////////////

// 4d vector functions
/////////////////////////////////////////////////////////////////////////////////////////
vec4_t vec4_from_vec3(vec3_t v, float w) {
        vec4_t ret = {.x = v.x, .y = v.y, .z = v.z, .w = w};
        return ret;
}

vec3_t vec3_from_vec4(vec4_t v) {
        vec3_t ret = {.x = v.x, .y = v.y, .z = v.z};
        return ret;
}
