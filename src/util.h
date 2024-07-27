#ifndef UTIL_H
#define UTIL_H
#include <stdbool.h>
#include <assert.h>
// TODO(@k): why line below won't work as expected
// #define MIN(a, b) (a > b ? b : a)
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))
void swap(char *a, char *b, int size);
bool is_float_close(float a, float b, float margin);
void float_clamp_inline(float *d, float min, float max);
float float_lerp(float a, float b, float t);
#endif
