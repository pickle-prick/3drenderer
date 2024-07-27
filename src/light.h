#ifndef LIGHT_H
#define LIGHT_H
#include "vector.h"
#include <stdint.h>

typedef struct global_light {
        vec3_t direction; 
} global_light;

uint32_t light_apply_intensity(uint32_t original_color, float intensity);
#endif
