#ifndef DISPLAY_H
#define DISPLAY_H
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "vector.h"
#include <stdint.h>

#define FPS 144
#define FRAME_TARGET_TIME (1000 / FPS)

// TODO(@k): reduce the num of global variables
extern SDL_Window *window;
extern SDL_Renderer *renderer;
extern uint32_t *color_buffer;
extern float *z_buffer;
extern SDL_Texture *color_buffer_texture;
extern int window_width;
extern int window_height;

bool initialize_window(void);
void draw_grid(uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void draw_filled_triangle(
        int x0, int y0, float z0, float w0,
        int x1, int y1, float z1, float w1,
        int x2, int y2, float z2, float w2,
        float *z_buffer, uint32_t color
);
void draw_filled_triangle_v2(
        vec4_t *a,
        vec4_t *b,
        vec4_t *c,
        float *z_buffer, uint32_t color
);
void draw_textured_triangle(
        int x0, int y0, float z0, float w0, float u0, float v0,
        int x1, int y1, float z1, float w1, float u1, float v1,
        int x2, int y2, float z2, float w2, float u2, float v2,
        float *z_buffer, uint32_t *texture, int texture_width, int texture_height
);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_pixel(int x, int y, uint32_t color);
void draw_simple_integer(int number, int x_start, int y_start, int width);
void clear_color_buffer(uint32_t color);
void clear_z_buffer(float *z_buffer, int len);
void destroy_window(void);
void render_color_buffer(void);
#endif
