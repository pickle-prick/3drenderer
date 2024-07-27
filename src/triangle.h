#ifndef TRIANGLE_H
#define TRIANGLE_H
#include "vector.h"
#include "texture.h"
#include <stdint.h>
// in the context of 3d
typedef struct {
        int a;
        int b;
        int c;
        tex2_t a_uv;
        tex2_t b_uv;
        tex2_t c_uv;
        uint32_t color; /* we can still render in solid color if we want */
} face_t;

// 2D points
typedef struct {
        vec4_t points[3];
        tex2_t texcoords[3];
        uint32_t color;
} triangle_t;
#endif
