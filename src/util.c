#include <string.h>
#include "util.h"
#include <math.h>
#include <stdbool.h>

void swap(char *a, char *b, int size) {
        char tmp[size];
        memcpy(tmp, a, size);
        memcpy(a, b, size);
        memcpy(b, tmp, size);
}

bool is_float_close(float a, float b, float margin) {
        return fabs(a - b) < margin;
}

void float_clamp_inline(float *d, float min, float max) {
    if (*d > max) { *d = max; return; }
    if (*d < min) { *d = min; return; }
}

float float_lerp(float a, float b, float t) {
        //I = Q1 + t < Q2 − Q1 >
        return a + t * (b - a);
}
