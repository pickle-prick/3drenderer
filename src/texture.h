#ifndef TEXTURE_H
#define TEXTURE_H
#include <stdint.h>
#include "upng.h"
typedef struct {
        float u;
        float v;
} tex2_t;

// TODO(@k): Global texture for now
extern int texture_width;
extern int texture_height;
extern const uint8_t REDBRICK_TEXTURE[];
extern uint32_t *mesh_texture;
extern upng_t *upng;

void load_png_texture(char *file);
#endif
