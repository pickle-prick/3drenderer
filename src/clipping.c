#include "clipping.h"
#include "vector.h"
#include <assert.h>
#include "util.h"
#include "texture.h"
#include <math.h>

#include "universe.h"

// Game idea
// TODO(@kkk): jumping spider (color, jump)
// TODO(@kkk): illusion (time e.g.)
// TODO(@kkk): higher demesion
// TODO(@kkk): does we have free will, or just computation
// TODO(@kkk): entropy
// TODO(@kkk): many world
// TODO(@kkk): better, offline version of excalidraw, (math, table, animation, 3D, shader)
// TODO(@kkk): alien world
// TODO(@kkk): dragon/bird
// TODO(@kkk): Heretic

#define MAX_VERTICES_PER_POLYGON 10

typedef struct {
        vec3_t point;
        vec3_t normal;
} plane_t;

typedef struct {
        vec4_t vertices[MAX_VERTICES_PER_POLYGON];
        tex2_t texcoords[MAX_VERTICES_PER_POLYGON];
        uint32_t color;
        int num_vertices;
} polygon_t;

// enum frustum_plane { NEAR, FAR, LEFT, RIGHT, TOP, BOTTOM };

static polygon_t triangle_to_polygon(triangle_t *t);
static void clip_triangle_for_plane(polygon_t *p, plane_t *plane);
static void initialize_frustum_planes(float z_near, float z_far, plane_t ret_frust_planes[6]);
static vec3_t vec3_from_fake_vec4(vec4_t v);
static int polygon_to_triangles(polygon_t *p, triangle_t *ret_triangles);

// NOTE(@k): https://fabiensanglard.net/polygon_codec/clippingdocument/Clipping.pdf
int clip_triangle(triangle_t *t, float z_near, float z_far, triangle_t *clipped_triangles) {
        polygon_t p = triangle_to_polygon(t);

        // TODO(@k): we could do cache here, don't need to initialize frustum planes every time if z_near and z_far is not changing
        // static float z_near_cached = 1.0;
        // static float z_far_cached = 0.0;

        plane_t frustum_planes[6] = {};
        initialize_frustum_planes(z_near, z_far, frustum_planes);

        for (int i = 0; i < 6; i++) {
                clip_triangle_for_plane(&p, &frustum_planes[i]);
        }

        return polygon_to_triangles(&p, clipped_triangles);
}

static void clip_triangle_for_plane(polygon_t *p, plane_t *plane) {
        if (p->num_vertices == 0) return;

        vec4_t in[MAX_VERTICES_PER_POLYGON] = {};
        tex2_t texcoords[MAX_VERTICES_PER_POLYGON] = {};
        int c = 0;

        // TODO(@k): there is a better way to decide the if a vertice in or out of plane by comparing x,y with w/znear which don't require dot product
        // but much more condition checks and branch code will be required which could be a mess

        assert(p->num_vertices >= 3);

        float curr_d = vec3_dot(vec3_sub(vec3_from_fake_vec4(p->vertices[0]), plane->point), plane->normal);
        for (int i = 0; i < p->num_vertices; i++) {
                vec4_t *curr_p = &p->vertices[i];
                int next_idx = (i+1) % p->num_vertices;
                vec4_t *next_p = &p->vertices[next_idx];

                // TODO(@k): handle precesion issue here, should we consider near-0 point
                if (curr_d > 0.0) {
                        in[c] = *curr_p;
                        texcoords[c] = p->texcoords[i];
                        c++;
                }

                float next_d = vec3_dot(vec3_sub(vec3_from_fake_vec4(*next_p), plane->point), plane->normal);

                // < Q−P > ·~n = 0 happens infrequently when using floating point arithmetic
                // Normally this must be approximated by
                // | < Q − P > ·~n| < 0.01

                // one side each, get the intersection point I = Q1 + t(Q2-Q1)
                if ((curr_d * next_d) < 0) {
                        float t = curr_d / (curr_d - next_d);
                        assert(t >= 0.0 && t <= 1.0);
                        vec4_t i_p = {
                                // I = Q1 + t < Q2 − Q1 >
                                .x = float_lerp(curr_p->x, next_p->x, t),
                                .y = float_lerp(curr_p->y, next_p->y, t),
                                .z = float_lerp(curr_p->z, next_p->z, t),
                                .w = float_lerp(curr_p->w, next_p->w, t),
                        };
                        // handle texture interpolation here
                        tex2_t uv_coord = {
                                .u = float_lerp(p->texcoords[i].u, p->texcoords[next_idx].u, t),
                                .v = float_lerp(p->texcoords[i].v, p->texcoords[next_idx].v, t),
                        };
                        in[c] = i_p;
                        texcoords[c] = uv_coord;
                        c++;
                }

                curr_d = next_d;
        }

        if (c > 0) assert(c >= 3);
        assert(c <= MAX_VERTICES_PER_POLYGON);

        p->num_vertices = c;
        for (int i = 0; i < c; i++) {
                p->vertices[i] = in[i];
                p->texcoords[i] = texcoords[i];
        }
}


static polygon_t triangle_to_polygon(triangle_t *t) {
        assert(MAX_VERTICES_PER_POLYGON > 3);

        polygon_t ret = {
                .vertices = { t->points[0], t->points[1], t->points[2] },
                .texcoords = { t->texcoords[0], t->texcoords[1], t->texcoords[2] },
                .color = t->color,
                .num_vertices = 3,
        };
        return ret; 
}

/*
 * return the number of triangles generated
 */
static int polygon_to_triangles(polygon_t *p, triangle_t *ret_triangles) {
        // 1 0 0 0 0 0
        for (int i = 1; i < p->num_vertices - 1; i++) {
                ret_triangles[i-1] = (triangle_t){
                        .points = { p->vertices[0], p->vertices[i], p->vertices[i+1] },
                        .texcoords = { p->texcoords[0], p->texcoords[i], p->texcoords[i+1] },
                        .color = p->color,
                };
        }
        return p->num_vertices - 2;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Frustum planes are defined by a point and a normal vector
//////////////////////////////////////////////////////////////////////////////////////
// Near plane   :  P=(0, 0, znear), N=(0, 0,  1)
// Far plane    :  P=(0, 0, zfar),  N=(0, 0, -1)
// Top plane    :  P=(0, 0, 0),     N=(0, -cos(fov/2), sin(fov/2))
// Bottom plane :  P=(0, 0, 0),     N=(0, cos(fov/2), sin(fov/2))
// Left plane   :  P=(0, 0, 0),     N=(cos(fov/2), 0, sin(fov/2))
// Right plane  :  P=(0, 0, 0),     N=(-cos(fov/2), 0, sin(fov/2))
/////////////////////////////////////////////////////////////////////////////////////////
//           /|\
//         /  | | 
//       /\   | |
//     /      | |
//  P*|-->  <-|*|   ----> +z-axis
//     \      | |
//       \/   | |
//         \  | | 
//           \|/
/////////////////////////////////////////////////////////////////////////////////////////
static void initialize_frustum_planes(float z_near, float z_far, plane_t ret_frustum_planes[6]) {
        // TODO(@k): precomputed the cosf(M_PI/4) and sinf(M_PI/4)

        // NEAR
        ret_frustum_planes[0] = (plane_t){
                .point = { 0, 0, z_near },
                .normal = { 0, 0, 1 },
        };

        // FAR 
        ret_frustum_planes[1] = (plane_t){
                .point = { 0, 0, z_far },
                .normal = { 0, 0, -1 }
        };

        // TOP
        ret_frustum_planes[4] = (plane_t){
                .point = { 0, 0, 0 },
                .normal = { 0, cosf(M_PI/4), sinf(M_PI/4) },
        };

        // BOTTOM
        ret_frustum_planes[5] = (plane_t){
                .point = { 0, 0, 0 },
                .normal = { 0, -cosf(M_PI/4), sinf(M_PI/4) },
        };

        // LEFT
        ret_frustum_planes[2] = (plane_t){
                .point = { 0, 0, 0 },
                .normal = { cosf(M_PI/4), 0, sinf(M_PI/4) },
        };

        // RIGHT
        ret_frustum_planes[3] = (plane_t){
                .point = { 0, 0, 0 },
                .normal = { -cosf(M_PI/4), 0, sinf(M_PI/4) },
        };
}

static vec3_t vec3_from_fake_vec4(vec4_t v) {
        vec3_t ret = {.x = v.x, .y = v.y, .z = v.w };
        return ret;
}
