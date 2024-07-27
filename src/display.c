#include <assert.h>
#include <math.h>
#include "display.h"
#include "font.h"
#include "vector.h"
#include "util.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
uint32_t *color_buffer = NULL;
float *z_buffer = NULL;
SDL_Texture *color_buffer_texture = NULL;
int window_width = 800;
int window_height = 600;

static vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p);
static float edge_function(vec2_t *a, vec2_t *b, vec2_t* p);

void draw_grid(uint32_t color) {
        // draw a background grid that fills the entire window
        // lines should be rendered at every row/col multiple of 10
        int grid_size = 50;

        // TODO: brush size
        // int line_size = 30;
        for (int y = 0; y < window_height; y++) {
                for (int x = 0; x < window_width; x++) {
                        if (y % grid_size == 1 || x % grid_size == 1) {
                                color_buffer[(y * window_width) + x] = color;
                        }
                }
        }
}

inline void draw_rect(int x, int y, int width, int height, uint32_t color) {
        // filled rectangle
        for (int i = 0; i < width; i++) {
                for (int j = 0; j < height; j++) {
                        int curr_x = x + i;
                        int curr_y = y + j;
                        if (curr_x >=0 && curr_x < window_width && curr_y >=0 && curr_y < window_height)
                                draw_pixel(curr_x, curr_y, color);
                }
        }
}

inline void draw_pixel(int x, int y, uint32_t color) {
        // TODO(@k): not sure if we can solve this issue
        // if (x >= 0 && x < window_width && y >= 0 && y < window_height) {
        //         color_buffer[(y * window_width) + x] = color;
        // }
        // return;

        // NOTE(@k): handle precesion issue
        float allow_margin = 1.1;
        if (x >= window_width && (x - window_width) < allow_margin) x = window_width - 1;
        if (y >= window_height && (y - window_height) < allow_margin) y = window_height - 1;

        // NOTE(@k): after clipping, x,y should faill into a health range in screen space
        assert(x >= 0 && x < window_width);
        assert(y >= 0 && y < window_height);
        color_buffer[(y * window_width) + x] = color;
}

void draw_simple_integer(int number, int x_start, int y_start, int width) {
        // 3X6 scale
        int height = width * 2;
        float scale = 3.0 / width;
        int offset_x = 0;

        int divisor = 1;
        int temp = number;
        while (temp > 9) {
                divisor *= 10;
                temp /= 10;
        }

        // loop through individual numbers
        while (divisor > 0) {
                int curr_dig = number / divisor;
                number %= divisor;
                divisor /= 10;

                for (int y = 0; y < height; y++)  {
                        for (int x = 0; x < width; x++) {
                               int u = x * scale;
                               int v = y * scale;
                               assert(u >= 0 && u < 3);
                               assert(v >= 0 && v < 6);
                               if (simple_number_font[curr_dig][(v * 3) + u]) {
                                       draw_pixel(x_start + x + offset_x, y_start + y, 0xFF00FF00);
                               }
                        }
                }
                offset_x += (width + 3);
        }
}

void destroy_window(void) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
}

inline void clear_color_buffer(uint32_t color) {
        // for (int y = 0; y < window_height; y++) {
        //         for (int x = 0; x < window_width; x++) {
        //                 color_buffer[(y * window_width) + x] = color;
        //         }
        // }

        for (int i = 0; i < window_width * window_height; i++) color_buffer[i] = color;
}

inline void clear_z_buffer(float *z_buffer, int len) {
        for (int i = 0; i < len; i++) z_buffer[i] = 1.1;
}

// copy color_buffer to color buffer texture
void render_color_buffer(void) {
        SDL_UpdateTexture(color_buffer_texture, NULL, color_buffer, (int)(window_width * sizeof(uint32_t)));
        SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
}

bool initialize_window(void) {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                fprintf(stderr, "Error initializing SDL.\n");
                return false;
        }

        // use sdl to query what is the full screen max width and height
        SDL_DisplayMode display_mode;
        // 0 means the primary display device
        SDL_GetCurrentDisplayMode(0, &display_mode);
        // window_width = display_mode.w / 2;
        // window_height = display_mode.h / 2;

        printf("window width: %d\n", window_width);
        printf("window height: %d\n", window_height);
        printf("refresh rate: %d\n", display_mode.refresh_rate);

        // pass title with null, then it would not have a window title bar
        window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, window_width,
                                  window_height, SDL_WINDOW_RESIZABLE);
        if (!window) {
                fprintf(stderr, "Error creating SDL window.\n");
                return false;
        }

        // -1 get the default display, 0 no specific flags required
        renderer = SDL_CreateRenderer(window, -1, 0);
        if (!renderer) {
                fprintf(stderr, "Error creating SDL renderer.\n");
                return false;
        }

        // change video mode to full screen
        // SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        return true;
}

void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
        draw_line(x0, y0, x1, y1, color);
        draw_line(x1, y1, x2, y2, color);
        draw_line(x2, y2, x0, y0, color);
}

///////////////////////////////////////////////////////////////////////////////
// Draw a filled a triangle with a flat bottom (Deprecated)
///////////////////////////////////////////////////////////////////////////////
//
//        (x0,y0)
//          / \
//         /   \
//        /     \
//       /       \
//      /         \
//  (x1,y1)------(x2,y2)
//
///////////////////////////////////////////////////////////////////////////////
void draw_flat_bottom_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
        assert(y1 == y2);

        // NOTE(@k): since we need delta x over delta y, we call it the invert
        // slope
        float inv_slope_1 = (float)(x0 - x1) / (y0 - y1);
        float inv_slope_2 = (float)(x0 - x2) / (y0 - y2);

        // TODO(@k): we could make those variables c register variable
        float x_start = x0;
        float x_end = x0;

        // loop all the scan lines from top to bottom
        for (int y = y0; y <= y1; y++) {
                draw_line(x_start, y, x_end, y, color);
                x_start += inv_slope_1;
                x_end += inv_slope_2;
        }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a filled a triangle with a flat top (Deprecated)
///////////////////////////////////////////////////////////////////////////////
//
//  (x0,y0)------(x1,y1)
//      \         /
//       \       /
//        \     /
//         \   /
//          \ /
//        (x2,y2)
//
///////////////////////////////////////////////////////////////////////////////
void draw_flat_top_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
        assert(y0 == y1);
        float inv_slope_1 = (float)(x2 - x0) / (y2 - y0);
        float inv_slope_2 = (float)(x2 - x1) / (y2 - y1);

        // TODO(@k): we could make those variables c register variable
        float x_start = x2;
        float x_end = x2;

        // from bottom to top
        for (int y = y2; y >= y0; y--) {
                draw_line(x_start, y, x_end, y, color);

                x_start -= inv_slope_1;
                x_end -= inv_slope_2;
        }
}

///////////////////////////////////////////////////////////////////////////////
// Draw a filled triangle with the flat-top/flat-bottom method
// Basic scanline rasterization
// We split the original triangle in two, half flat-bottom and half flat-top
///////////////////////////////////////////////////////////////////////////////
//
//          (x0,y0)
//            / \
//           /   \
//          /     \
//         /       \
//        /         \
//   (x1,y1)------(Mx,My)
//       \_           \
//          \_         \
//             \_       \
//                \_     \
//                   \    \
//                     \_  \
//                        \_\
//                           \
//                         (x2,y2)
//
///////////////////////////////////////////////////////////////////////////////
void draw_filled_triangle(
        int x0, int y0, float z0, float w0,
        int x1, int y1, float z1, float w1,
        int x2, int y2, float z2, float w2,
        float *z_buffer, uint32_t color
) {
        // TODO(@k): could be a line or point due precesion loss, refactor this code, kind ugly
        // TODO(@k): may have some performance issue here
        vec2_t ab = { x1 - x0, y1 - y0 };
        vec2_t ac = { x2 - x0, y2 - y0 };
        if (ab.x == 0 && ab.y == 0) return;
        if (ac.x == 0 && ac.y == 0) return;
        vec2_normalize(&ab);
        vec2_normalize(&ac);
        if (is_float_close(fabs(vec2_dot(ab, ac)), 1.0, 0.0001)) return;

        // we need to sort the vertices by y-coordinate ascending (y0 <= y1 <= y2)
        // insertion sort
        if (y0 > y1) {
                swap((char *)&x0, (char *)&x1, sizeof(int));
                swap((char *)&y0, (char *)&y1, sizeof(int));
                swap((char *)&z0, (char *)&z1, sizeof(float));
                swap((char *)&w0, (char *)&w1, sizeof(float));
        }

        if (y0 > y2) {
                swap((char *)&x0, (char *)&x2, sizeof(int));
                swap((char *)&y0, (char *)&y2, sizeof(int));
                swap((char *)&z0, (char *)&z2, sizeof(float));
                swap((char *)&w0, (char *)&w2, sizeof(float));
        }

        if (y1 > y2) {
                swap((char *)&x1, (char *)&x2, sizeof(int));
                swap((char *)&y1, (char *)&y2, sizeof(int));
                swap((char *)&z1, (char *)&z2, sizeof(float));
                swap((char *)&w1, (char *)&w2, sizeof(float));
        }

        vec2_t a = { x0, y0 };
        vec2_t b = { x1, y1 };
        vec2_t c = { x2, y2 };

        float w0_reciprocal = 1.0 / w0;
        float w1_reciprocal = 1.0 / w1;
        float w2_reciprocal = 1.0 / w2;

        float corrected_z0 = z0 * w0_reciprocal;
        float corrected_z1 = z1 * w1_reciprocal;
        float corrected_z2 = z2 * w2_reciprocal;

        float inv_l = 0.0;
        float inv_r = 0.0;

        float x_start, x_end;

        // top triangle
        //          (x0,y0)
        //            / \
        //           /   \
        //          /     \
        //         /       \
        //        /         \
        //   (x1,y1)---------\

        if (y1 != y0) {
                x_end = (x_start = x0);
                inv_l = (float)(x1 - x0) / (y1 - y0);
                inv_r = (float)(x2 - x0) / (y2 - y0);
                if ((x_start + inv_l) > (x_end + inv_r)) swap((char *)&inv_l, (char *)&inv_r, sizeof(float));

                for (int y = y0; y <= y1; y++) {
                        assert(x_start <= x_end);
                        for (int x = x_start; x <= x_end; x++) {
                                vec2_t p = { x, y};
                                vec3_t weights = barycentric_weights(a, b, c, p);

                                // NOTE(@k): p could outside of the triangle due to precision loss 
                                // NOTE(@k): if the p fall out of the triangle, the weights will be all-zero
                                if (weights.x == 0.0 && weights.y == 0.0 && weights.z == 0.0) {
                                        continue;
                                }

                                float w = 1 / (w0_reciprocal * weights.x + w1_reciprocal * weights.y + w2_reciprocal * weights.z);
                                float z = (corrected_z0 * weights.x + corrected_z1 * weights.y + corrected_z2 * weights.z) * w;
                                assert(z >= 0.0 && z <= 1.0);

                                // don't render this pixel if the z is larger than the last painted one
                                float *pixel_depth = &z_buffer[(window_width * y) + x];
                                if (*pixel_depth < z) continue;
                                *pixel_depth = z;

                                draw_pixel(x, y, color);
                        }

                        x_start += inv_l;
                        x_end += inv_r;
                }
        }

        // bottom triangle
        //      \---------(x1,y1)
        //       \_           \
        //          \_         \
        //             \_       \
        //                \_     \
        //                   \    \
        //                     \_  \
        //                        \_\
        //                           \
        //                         (x2,y2)
        if (y1 != y2) {
                x_end = (x_start = x2);
                inv_l = (float)(x2 - x0) / (y2 - y0);
                inv_r = (float)(x2 - x1) / (y2 - y1);
                if ((x_start - inv_l) > (x_end - inv_r)) swap((char *)&inv_l, (char *)&inv_r, sizeof(float));

                for (int y = y2; y > y1; y--) {
                        assert(x_start <= x_end);
                        for (int x = x_start; x <= x_end; x++) {
                                vec2_t p = { x, y};
                                vec3_t weights = barycentric_weights(a, b, c, p);

                                // NOTE(@k): p could outside of the triangle due to precision loss 
                                // NOTE(@k): if the p fall out of the triangle, the weights will be all-zero
                                if (weights.x == 0.0 && weights.y == 0.0 && weights.z == 0.0) {
                                        continue;
                                }

                                float w = 1 / (w0_reciprocal * weights.x + w1_reciprocal * weights.y + w2_reciprocal * weights.z);
                                float z = (corrected_z0 * weights.x + corrected_z1 * weights.y + corrected_z2 * weights.z) * w;
                                assert(z >= 0.0 && z <= 1.0);

                                // don't render this pixel if the z is larger than the last painted one
                                if (z_buffer[(window_width * y) + x] < z) continue;

                                float *pixel_depth = &z_buffer[(window_width * y) + x];
                                if (*pixel_depth < z) continue;
                                *pixel_depth = z;

                                draw_pixel(x, y, color);
                        }

                        x_start -= inv_l;
                        x_end -= inv_r;
                }
        }
}

/*
 * using edge function
 * using top-left rule
 *
 *         (A)
 *         /|\
 *        / | \
 *       /  |  \
 *      /  (P)  \
 *     /  /   \  \
 *    / /       \ \
 *   //           \\
 *  (B)------------(C)
 *
 */
void draw_filled_triangle_v2(
        vec4_t *a,
        vec4_t *b,
        vec4_t *c,
        float *z_buffer, uint32_t color
) {
        // find the bounding box for the triangle
        int x_min = ceil(MIN(MIN(a->x, b->x), c->x));
        int y_min = ceil(MIN(MIN(a->y, b->y), c->y));
        int x_max = floor(MAX(MAX(a->x, b->x), c->x));
        int y_max = floor(MAX(MAX(a->y, b->y), c->y));

        vec2_t a_2 = { .x = a->x, .y = a->y };
        vec2_t b_2 = { .x = b->x, .y = b->y };
        vec2_t c_2 = { .x = c->x, .y = c->y };

        vec2_t ab = vec2_sub(b_2, a_2);
        vec2_t bc = vec2_sub(c_2, b_2);
        vec2_t ca = vec2_sub(a_2, c_2);

        // TODO(@k): sub-pixel

        // ||ABXAC||
        float area_x2 = vec2_cross(vec2_sub(b_2, a_2), vec2_sub(c_2, a_2));

        float w0_reciprocal = 1.0 / a->w;
        float w1_reciprocal = 1.0 / b->w;
        float w2_reciprocal = 1.0 / c->w;

        float corrected_z0 = a->z * w0_reciprocal;
        float corrected_z1 = b->z * w1_reciprocal;
        float corrected_z2 = c->z * w2_reciprocal;

        // top-left rule
        float bias_0 = ((ab.y == 0 && ab.x > 0) || ab.y < 0) ? 0 : 0.0001;
        float bias_1 = ((bc.y == 0 && bc.x > 0) || bc.y < 0) ? 0 : 0.0001;
        float bias_2 = ((ca.y == 0 && ca.x > 0) || ca.y < 0) ? 0 : 0.0001;

        for (int y = y_min; y <= y_max; y++) {
                for (int x = x_min; x <= x_max; x++) {
                        vec2_t p = { x, y };
                        float w0 = edge_function(&a_2, &b_2, &p);
                        float w1 = edge_function(&b_2, &c_2, &p);
                        float w2 = edge_function(&c_2, &a_2, &p);

                        bool overlaps = w0 >= bias_0 && w1 >= bias_1 && w2 >= bias_2;
                        if (!overlaps) continue;

                        w0 /= area_x2;
                        w1 /= area_x2;
                        w2 /= area_x2;

                        // TODO(@k): handle precesion issue
                        // assert((w0 + w1 + w2) <= 1.0);

                        float w = 1 / (w0_reciprocal * w0 + w1_reciprocal * w1 + w2_reciprocal * w2);
                        float z = (w0 * corrected_z0 + w1 * corrected_z1 + w2 * corrected_z2) * w;
                        assert(z >= 0.0 && z <= 1.0);

                        float *pixel_depth = &z_buffer[(window_width * y) + x];
                        if (*pixel_depth < z) continue;
                        *pixel_depth = z;

                        draw_pixel(x, y, color);
                }
        }

}

///////////////////////////////////////////////////////////////////////////////
// Draw a textured triangle, linear interpolation within uv map
// We split the original triangle in two, half flat-bottom and half flat-top
///////////////////////////////////////////////////////////////////////////////
//
//          (x0,y0)
//            / \
//           /   \
//          /     \
//         /       \
//        /         \
//   (x1,y1)------(Mx,My)
//       \_           \
//          \_         \
//             \_       \
//                \_     \
//                   \    \
//                     \_  \
//                        \_\
//                           \
//                         (x2,y2)
//
///////////////////////////////////////////////////////////////////////////////
void draw_textured_triangle(
    int x0, int y0, float z0, float w0, float u0, float v0,
    int x1, int y1, float z1, float w1, float u1, float v1,
    int x2, int y2, float z2, float w2, float u2, float v2,
    float *z_buffer,
    uint32_t *texture,
    int texture_width,
    int texture_height
) {
        // TODO(@k): could be a line or point due precesion loss, refactor this code, kind ugly
        // TODO(@k): may have some performance issue here
        vec2_t ab = { x1 - x0, y1 - y0 };
        vec2_t ac = { x2 - x0, y2 - y0 };
        if (ab.x == 0 && ab.y == 0) return;
        if (ac.x == 0 && ac.y == 0) return;
        vec2_normalize(&ab);
        vec2_normalize(&ac);
        if (is_float_close(fabs(vec2_dot(ab, ac)), 1.0, 0.0001)) return;

        // loop all the pixels of the triangle to render them based on the color that comes 
        // from the texture array

        // we need to sort the vertices by y-coordinate ascending (y0 <= y1 <= y2)
        // insertion sort
        if (y0 > y1) {
                swap((char *)&x0, (char *)&x1, sizeof(int));
                swap((char *)&y0, (char *)&y1, sizeof(int));
                swap((char *)&z0, (char *)&z1, sizeof(float));
                swap((char *)&w0, (char *)&w1, sizeof(float));
                swap((char *)&u0, (char *)&u1, sizeof(float));
                swap((char *)&v0, (char *)&v1, sizeof(float));
        }

        if (y0 > y2) {
                swap((char *)&x0, (char *)&x2, sizeof(int));
                swap((char *)&y0, (char *)&y2, sizeof(int));
                swap((char *)&z0, (char *)&z2, sizeof(float));
                swap((char *)&w0, (char *)&w2, sizeof(float));
                swap((char *)&u0, (char *)&u2, sizeof(float));
                swap((char *)&v0, (char *)&v2, sizeof(float));
        }

        if (y1 > y2) {
                swap((char *)&x1, (char *)&x2, sizeof(int));
                swap((char *)&y1, (char *)&y2, sizeof(int));
                swap((char *)&z1, (char *)&z2, sizeof(float));
                swap((char *)&w1, (char *)&w2, sizeof(float));
                swap((char *)&u1, (char *)&u2, sizeof(float));
                swap((char *)&v1, (char *)&v2, sizeof(float));
        }

        // NOTE(@k): because we are moving y and calculate x, we call them inverse slope of the line
        float inv_l = 0.0;
        float inv_r = 0.0;

        // NOTE(@k): x_start needs to be float, otherwise the slop won't accumulate
        float x_start;
        float x_end;

        vec2_t a = { x0, y0 };
        vec2_t b = { x1, y1 };
        vec2_t c = { x2, y2 };

        assert(a.y <= b.y);
        assert(b.y <= c.y);


        float w0_reciprocal = 1.0 / w0;
        float w1_reciprocal = 1.0 / w1;
        float w2_reciprocal = 1.0 / w2;

        float corrected_z0 = z0 * w0_reciprocal;
        float corrected_u0 = u0 * w0_reciprocal;
        float corrected_v0 = v0 * w0_reciprocal;

        float corrected_z1 = z1 * w1_reciprocal;
        float corrected_u1 = u1 * w1_reciprocal;
        float corrected_v1 = v1 * w1_reciprocal;

        float corrected_z2 = z2 * w2_reciprocal;
        float corrected_u2 = u2 * w2_reciprocal;
        float corrected_v2 = v2 * w2_reciprocal;

        // top triangle
        if (y1 != y0) {
                x_start = x0;
                x_end = x0;
                inv_l = (float)(x1 - x0) / (y1 - y0);
                inv_r = (float)(x2 - x0) / (y2 - y0);
                if ((x_start + inv_l) > (x_end + inv_r)) swap((char *)&inv_l, (char *)&inv_r, sizeof(float));

                for (int y = y0; y <= y1; y++) {
                        // handle precesion issue
                        if (x_start < 0 && x_start > -0.01) x_start = 0;
                        assert(x_start <= x_end);
                        assert(x_start >= 0);
                        assert(x_end >= 0);

                        for (int x = x_start; x <= x_end; x++) {
                                // sample color from texture based the x,y, use barycentric
                                vec2_t p = { x, y };

                                vec3_t weights = barycentric_weights(a, b, c, p);

                                // NOTE(@k): p could outside of the triangle due to precision loss 
                                // NOTE(@k): if the p fall out of the triangle, the weights will be all-zero
                                if (weights.x == 0.0 && weights.y == 0.0 && weights.z == 0.0) {
                                        continue;
                                }

                                float w = 1 / (w0_reciprocal * weights.x + w1_reciprocal * weights.y + w2_reciprocal * weights.z);

                                // TODO(@k): handle potentially precesion issue
                                float z = (corrected_z0 * weights.x + corrected_z1 * weights.y + corrected_z2 * weights.z) * w;
                                float u = (corrected_u0 * weights.x + corrected_u1 * weights.y + corrected_u2 * weights.z) * w;
                                float v = (corrected_v0 * weights.x + corrected_v1 * weights.y + corrected_v2 * weights.z) * w;

                                assert(z >= 0.0 && z <= 1.0);
                                assert(u >= 0.0 && u <= 1.0);
                                assert(v >= 0.0 && v <= 1.0);
                                v = 1.0 - v; // flip v

                                // don't render this pixel if the z is larger than the last painted one
                                float *pixel_depth = &z_buffer[(window_width * y) + x];
                                if (*pixel_depth < z) continue;
                                *pixel_depth = z;

                                int iu = u * (texture_width - 1);
                                int iv = v * (texture_height - 1);

                                assert(u < texture_width);
                                assert(v < texture_height);

                                uint32_t color = texture[(texture_width * iv) + iu];
                                draw_pixel(x, y, color);
                        }
                        x_start += inv_l;
                        x_end += inv_r;
                }
        }

        // bottom triangle
        // NOTE(@k): we should walk from bottom to up, there is a reason related to x_start and x_end
        if (y1 != y2) {
                x_start = x2;
                x_end = x2;
                inv_l = (float)(x2 - x0) / (y2 - y0);
                inv_r = (float)(x2 - x1) / (y2 - y1);
                if ((x_start - inv_l) > (x_end - inv_r)) swap((char *)&inv_l, (char *)&inv_r, sizeof(float));

                for (int y = y2; y > y1; y--) {
                        assert(x_start <= x_end);
                        for (int x = x_start; x <= x_end; x++) {
                                // sample color from texture based the x,y, use barycentric
                                vec2_t p = { x, y };

                                vec3_t weights = barycentric_weights(a, b, c, p);

                                // NOTE(@k): p could outside of the triangle due to precision loss 
                                // NOTE(@k): if the p fall out of the triangle, the weights will be all-zero
                                if (weights.x == 0.0 && weights.y == 0.0 && weights.z == 0.0) {
                                        continue;
                                }

                                float w = 1 / (w0_reciprocal * weights.x + w1_reciprocal * weights.y + w2_reciprocal * weights.z);

                                float z = (corrected_z0 * weights.x + corrected_z1 * weights.y + corrected_z2 * weights.z) * w;
                                float u = (corrected_u0 * weights.x + corrected_u1 * weights.y + corrected_u2 * weights.z) * w;
                                float v = (corrected_v0 * weights.x + corrected_v1 * weights.y + corrected_v2 * weights.z) * w;

                                assert(v >= 0.0 && v <= 1.0);
                                assert(u >= 0.0 && u <= 1.0);
                                assert(v >= 0.0 && v <= 1.0);
                                v = 1.0 - v; // flip v

                                // don't render this pixel if the z is larger than the last painted one
                                float *pixel_depth = &z_buffer[(window_width * y) + x];
                                if (*pixel_depth < z) continue;
                                *pixel_depth = z;

                                int iu = u * (texture_width - 1);
                                int iv = v * (texture_height - 1);

                                assert(u < texture_width);
                                assert(v < texture_height);

                                uint32_t color = texture[(texture_width * iv) + iu];
                                draw_pixel(x, y, color);

                        }
                        x_start -= inv_l;
                        x_end -= inv_r;
                }
        }
}

// TODO(@k): could have some performance issue here
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
        int delta_x = x1 - x0;
        int delta_y = y1 - y0;

        int run_distance = abs(delta_x) >= abs(delta_y) ? abs(delta_x) : abs(delta_y);
        float x_inc = delta_x / (float)run_distance;
        float y_inc = delta_y / (float)run_distance;

        float curr_x = x0;
        float curr_y = y0;
        for (int i = 0; i <= run_distance; i++) {
                draw_pixel(roundf(curr_x), roundf(curr_y), color);
                curr_x += x_inc;
                curr_y += y_inc;
        }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Return the barycentric weights alpha, beta, and gamma for point p
// TODO(@k): if we enable backface-culling, all projected triangle should in clock-wise winding in NDC space, but once projected to screen space which y-coordinates grow downward, the winding order would be inverted, thus counter-clock-wise
/////////////////////////////////////////////////////////////////////////////////////////
//
//         (A)
//         /|\
//        / | \
//       /  |  \
//      /  (P)  \
//     /  /   \  \
//    / /       \ \
//   //           \\
//  (B)------------(C)
//
/////////////////////////////////////////////////////////////////////////////////////////
static vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p) {
        vec3_t ret = {0};

        // find the vectors between the vertices ABC and point p
        vec2_t ac = vec2_sub(c, a);
        vec2_t ab = vec2_sub(b, a);
        vec2_t pc = vec2_sub(c, p);
        vec2_t pb = vec2_sub(b, p);
        vec2_t ap = vec2_sub(p, a);

        // || AB X AC ||
        float abc_area_parallelogram = fabs(ab.x * ac.y - ab.y * ac.x);
        assert(abc_area_parallelogram != 0.0); /* not expecting a line here */

        // || PB X PC ||
        float alpha = fabs(pb.x * pc.y - pb.y * pc.x) / abc_area_parallelogram;
        // || AP X AC ||
        float beta =  fabs(ap.x * ac.y - ap.y * ac.x) / abc_area_parallelogram;

        // NOTE(@k): p is not within the traignle abc due to precision loss
        if (alpha + beta > 1.0) return ret;
        float gamma = 1.0 - (alpha + beta);

        assert(alpha >= 0.0 && alpha <= 1.0);
        assert(beta >= 0.0 && beta <= 1.0);
        assert(gamma >= 0.0 && gamma <= 1.0);

        ret.x = alpha;
        ret.y = beta;
        ret.z = gamma;
        return ret;
}

/*
 * NOTE(@k): expecting counter-clock-wise winding order
 * TODO(@k): if we enable backface-culling, all projected triangle should in clock-wise winding in NDC space, but once projected to screen space which y-coordinates grow downward, the winding order would be inverted, thus counter-clock-wise
 *
 *         (A)
 *         /|\
 *        / | \
 *       /  |  \
 *      /  (P)  \
 *     /  /   \  \
 *    / /       \ \
 *   //           \\
 *  (B)------------(C)
 *
 */
static float edge_function(vec2_t *a, vec2_t *b, vec2_t* p) {
        return (p->y - a->y) * (b->x - a->x) - (p->x - a->x) * (b->y - a->y);
}
