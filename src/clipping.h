#ifndef CLIPPING_H
#define CLIPPING_H
#include "triangle.h"

#define MAX_CLIPPED_TRIANGLES 10

/*
 * return the number of clipped triangles
 */
int clip_triangle(triangle_t *t, float z_near, float z_far, triangle_t *clipped_triangles);
#endif
