#include <stdint.h>
#include <assert.h>
#include "darray.h"
#include "clipping.h"
#include "settings.h"
#include "texture.h"
#include "display.h"
#include "mesh.h"
#include "matrix.h"
#include "light.h"
#include "triangle.h"
#include "camera.h"
#include "vector.h"
#include "universe.h"

/////////////////////////////////////////////////////////////////////////////////////////
// render settings 
static enum cull_method {
        CULL_NONE,
        CULL_BACKFACE
} cull_method;

static enum projection_method {
        PERSPECTIVE,
        ORTHOGRAPHIC,
} projection_method;

static enum render_method {
        RENDER_WIRE,
        RENDER_WIRE_VERTEX,
        RENDER_FILL_TRIANGLE,
        RENDER_FILL_TRIANGLE_WIRE,
        RENDER_TEXTURED,
        RENDER_TEXTURED_WIRE,
} render_method;
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
// array of triangles that should be rendered frame by frame
/////////////////////////////////////////////////////////////////////////////////////////
static triangle_t *triangles_to_render = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
// global variables for execution status and game loop
/////////////////////////////////////////////////////////////////////////////////////////
static bool is_running = false;
static int previous_frame_time = 0;
static mat4_t projection_matrix;
static global_light light;
static float delta_time = 0;
static int fps = 0;
static int previous_fps_time = 0;
static bool paused = false;
static bool mouse_down = false;

static camera_t camera;
// TODO(@k): we could have two fov, fov-x and fov-y, in that case, we need to modify orthographic matrix
static float fov = M_PI / 2;
static float zn = 1.0; /* TODO(@k): if we want the near plane in NDC, we should set it 1, right? */
static float zf = 300.0;

/////////////////////////////////////////////////////////////////////////////////////////
// setup function to initialize variables and game objects
/////////////////////////////////////////////////////////////////////////////////////////
static void setup(void) {
        render_method = RENDER_FILL_TRIANGLE_WIRE;
        projection_method = PERSPECTIVE;
        cull_method = CULL_BACKFACE;

        // allocate the required memory in bytes to hold the color buffer
        color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);
        z_buffer = (float *)malloc(sizeof(float) * window_width * window_height);

        // creating a SDL texture that is used to display the color buffer
        // SDL_TEXTUREACCESS_STREAMING for a fast write access
        color_buffer_texture = SDL_CreateTexture(renderer,
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 window_width, window_height);

        // loads the cube values in the mesh data structure

        // load the hardcoded cube mesh and its texture
        // load_cube_mesh_data();
        // mesh_texture = (uint32_t *)REDBRICK_TEXTURE;

        // load_obj("./assets/cube.obj");
        // load_png_texture("./assets/cube.png");
        // load_obj("./assets/f22.obj");
        // load_png_texture("./assets/f22.png");
        // load_obj("./assets/drone.obj");
        // load_png_texture("./assets/drone.png");
        // load_obj("./assets/f117.obj");
        // load_png_texture("./assets/f117.png");
        load_obj("./assets/crab.obj");
        load_png_texture("./assets/crab.png");
        // load_obj("./assets/suzanne.obj");

        // initial settings for mesh
        mesh.scale.x = 1.3;
        mesh.scale.y = 1.3;
        mesh.scale.z = 1.3;
        mesh.translation.z = 8; // z index grows further inside the monitor, since we are using left-handed coordinate system

        // create projection matrix (perspective projection or orthographic projection)
        // the NDC we will be using is the Vulkan's Canonical Viewing Volume
        //    x: [-1, 1], y: [-1, 1], z: [0, 1]
        // 1. Aspect ratio: adjust x and y values based on the screen width and height values
        // 2. Field of view(angle of opening): adjust x and y values based on the FOV angle
        // 3. Normalization: adjust x and y values to sit between -1 and 1 (NDC: Normalized Device Coordinates)
        //    [3D world space] (Project)=> (Perspective divide)=> [NDC]
        // NOTE(@k): maybe we could seprate orthographic projection matrix and perspective projection matrix 
        //           since perspective projection matrix is all about depth division, we can seprate those projection into individual matrix 
        //           in the last step, we do depth division
        projection_matrix = projection_method == PERSPECTIVE ? mat4_make_perspective(fov, window_height, window_width, zn, zf) : mat4_make_orthographic(fov, window_height, window_width, zn, zf);

        // camera
        // NOTE(@k): this syntax is only valid in C99 standard and beyond
        camera.position = (vec3_t){0, 0, 0};
        camera.yaw = 0;
        camera.pitch = 0;

        // global iluminacion
        light.direction = (vec3_t){0, 0, 1};
}

/////////////////////////////////////////////////////////////////////////////////////////
// poll system events and handle keyboard input
/////////////////////////////////////////////////////////////////////////////////////////
static void process_input(void) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                        is_running = false;
                        break;
                case SDL_MOUSEWHEEL:
                        mesh.scale.x += event.wheel.y * 0.06;
                        mesh.scale.y += event.wheel.y * 0.06;
                        mesh.scale.z += event.wheel.y * 0.06;
                        break;
                case SDL_MOUSEMOTION:
                        if (mouse_down) {
                                mat4_t m_x = rotate_around_it(mesh.translation, 0, event.motion.xrel * 0.01, 0);
                                // TODO(@k): don't increase the angle to 90 degrees, otherwise the x axis will flip
                                mat4_t m_y = rotate_around_it(mesh.translation, event.motion.yrel * 0.01, 0, 0);
                                mat4_t m = mat4_mul_mat4(m_x, m_y);
                                camera.position = vec3_from_vec4(mat4_mul_vec4(m, vec4_from_vec3(camera.position, 1.0)));
                                printf("x: %f, y: %f, z: %f\n", camera.position.x, camera.position.y, camera.position.z);
                        }
                        break;
                case SDL_MOUSEBUTTONDOWN:
                        mouse_down = true;
                        break;
                case SDL_MOUSEBUTTONUP:
                        mouse_down = false;
                        break;
                case SDL_KEYDOWN:
                        // Pressing “1” displays the wireframe and a small red dot for each triangle vertex
                        // Pressing “2” displays only the wireframe lines
                        // Pressing “3” displays filled triangles with a solid color
                        // Pressing “4” displays both filled triangles and wireframe lines
                        // Pressing “5” render textured mesh
                        // Pressing “6” render texture mesh with wireframe
                        // Pressing “b” toggle back-face culling
                        // Pressing "o" to switch to orthographic projection
                        // Pressing "p" to switch to perspective projection

                        if (event.key.keysym.sym == SDLK_ESCAPE) is_running = false;
                        if (event.key.keysym.sym == SDLK_1) render_method = RENDER_WIRE_VERTEX;
                        if (event.key.keysym.sym == SDLK_2) render_method = RENDER_WIRE;
                        if (event.key.keysym.sym == SDLK_3) render_method = RENDER_FILL_TRIANGLE;
                        if (event.key.keysym.sym == SDLK_4) render_method = RENDER_FILL_TRIANGLE_WIRE;
                        if (event.key.keysym.sym == SDLK_5) render_method = RENDER_TEXTURED;
                        if (event.key.keysym.sym == SDLK_6) render_method = RENDER_TEXTURED_WIRE;

                        // pause
                        if (event.key.keysym.sym == SDLK_SPACE) {
                                paused = !paused;
                        }

                        // camera
                        if (event.key.keysym.sym == SDLK_w) {
                                mesh.translation.y += 6 * delta_time;
                        }
                        if (event.key.keysym.sym == SDLK_s) {
                                mesh.translation.y -= 6 * delta_time;
                        }
                        if (event.key.keysym.sym == SDLK_a) {
                                mesh.translation.x -= 6 * delta_time;
                        }
                        if (event.key.keysym.sym == SDLK_d) {
                                mesh.translation.x += 6 * delta_time;
                        }

                        // projection method
                        if (event.key.keysym.sym == SDLK_p && projection_method != PERSPECTIVE) {
                                projection_method = PERSPECTIVE;
                                projection_matrix = mat4_make_perspective(fov, window_height, window_width, zn, zf);
                        }
                        if (event.key.keysym.sym == SDLK_o && projection_method != ORTHOGRAPHIC) {
                                projection_method = ORTHOGRAPHIC;
                                cull_method = abs((int)cull_method - 1);
                                projection_matrix = mat4_make_orthographic(fov, window_height, window_width, zn, zf);
                        }

                        // enable back-face culling
                        // TODO(@k): it's kind hacky and kind unnecessary
                        if (event.key.keysym.sym == SDLK_b) cull_method = abs((int)cull_method - 1);

                        break;
                }
        }

}

/////////////////////////////////////////////////////////////////////////////////////////
// update function frame by frame with a fixed time step
// Model space => World space => Camera space => [Projection] => Clipping spcae => [Perspective divide] => Image space(NDC) => Screen space
/////////////////////////////////////////////////////////////////////////////////////////
static void update(void) {
        if (paused) return;

        // NOTE(@k): lock fps if we want to
        // wait some time until the reach the target frame time in milliseconds
        // int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

        // only delay execution if we are running too fast
        // if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME) {
        //         SDL_Delay(time_to_wait);
        // }

        delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;
        if ((SDL_GetTicks() - previous_fps_time) > 100.0) {
                fps = 1 / delta_time;
                previous_fps_time = SDL_GetTicks();
        }
        previous_frame_time = SDL_GetTicks();

        // rotate frame by frame, aka animation
        // mesh.rotation.x += 1 * delta_time;
        mesh.rotation.y += 1 * delta_time;
        // mesh.rotation.z += 1 * delta_time;

        // camera.position.x += 1 * delta_time;
        // camera.position.y += 1 * delta_time;

        // create the transform (rotation, scale, translate) matrix
        mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
        mat4_t translate_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);
        mat4_t rotation_x_matrix = mat4_make_rotation_x(mesh.rotation.x);
        mat4_t rotation_y_matrix = mat4_make_rotation_y(mesh.rotation.y);
        mat4_t rotation_z_matrix = mat4_make_rotation_z(mesh.rotation.z);

        vec3_t up = { 0, 1, 0 };
        mat4_t view_matrix = mat4_look_at(mesh.translation, camera.position, up);
        // mat4_t view_matrix = mat4_from_camera(camera.position, camera.yaw, camera.pitch);

        // face -> triangle
        for (int i = 0; i < darray_size(mesh.faces); i++) {
                face_t mesh_face = mesh.faces[i];

                // 3 vertices per face
                vec3_t face_vertices[3];
                face_vertices[0] = mesh.vertices[mesh_face.a - 1];
                face_vertices[1] = mesh.vertices[mesh_face.b - 1];
                face_vertices[2] = mesh.vertices[mesh_face.c - 1];

                vec4_t transformed_vertices[3];

                // process every vertices in the face
                // transform (rotation, scale, translate)
                for (int j = 0; j < 3; j++) {
                        vec4_t v = vec4_from_vec3(face_vertices[j], 1.0);

                        // scale, rotate, then translate, the order here matters
                        mat4_t world_matrix = mat4_eye();
                        world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
                        world_matrix = mat4_mul_mat4(rotation_x_matrix, world_matrix);
                        world_matrix = mat4_mul_mat4(rotation_y_matrix, world_matrix);
                        world_matrix = mat4_mul_mat4(rotation_z_matrix, world_matrix);
                        world_matrix = mat4_mul_mat4(translate_matrix, world_matrix);
                        world_matrix = mat4_mul_mat4(view_matrix, world_matrix);

                        v = mat4_mul_vec4(world_matrix, v);
                        transformed_vertices[j] = v;
                }

                // get face_normal
                vec3_t v_a = vec3_from_vec4(transformed_vertices[0]);
                vec3_t v_b = vec3_from_vec4(transformed_vertices[1]);
                vec3_t v_c = vec3_from_vec4(transformed_vertices[2]);

                vec3_t ab = vec3_sub(v_b, v_a);
                vec3_t ac = vec3_sub(v_c, v_a);

                vec3_normalize(&ab);
                vec3_normalize(&ac);
                // NOTE(@k): the winding order should be clock-wise
                vec3_t face_normal = vec3_cross(ab, ac);
                vec3_normalize(&face_normal);

                // backface culling test to see if the current face should be projected
                if (cull_method == CULL_BACKFACE) {
                        // performing back-face culling
                        // vec3_t camera_ray = vec3_sub(camera.position, v_a);
                        // NOTE(@k): since we already in camera view, the camera position is origin
                        vec3_t camera_ray = { -v_a.x, -v_a.y, -v_a.z };
                        vec3_normalize(&camera_ray);
                        // calculate how aligned the camera ray is with the face
                        // normal (using dot product)
                        float alignment = vec3_dot(camera_ray, face_normal);
                        // bypass the triangles that are looking away from the camera, cos(angle) < 0
                        if (alignment <= 0) continue; /* skip the current face */
                }

                triangle_t triangle = {
                        .points = {},
                        .texcoords = { mesh_face.a_uv, mesh_face.b_uv, mesh_face.c_uv },
                        .color = mesh_face.color,
                };

                // loop all three vertices in the face to perform projection
                for (int j = 0; j < 3; j++) {
                        // project 3d vertices to 2d point
                        vec4_t p = mat4_mul_vec4(projection_matrix, transformed_vertices[j]);
                        triangle.points[j] = p;
                }

                // frustum culling
                // TODO(@k): if we have a bounding box for the mesh, we could test early if we could skip the whole mesh
                // NOTE(@k): codes blow is logical wrong
                // bool is_triangle_in_frustum = false;
                // for (int i = 0; i < 3; i++) {
                //         vec4_t *p = &triangle.points[i];
                //         // NOTE(@k): zner is not necessarly 1.0
                //         float w = p->w * zn;
                //         // -zn <= x/w <= zn
                //         // -zn <= y/w <= zn
                //         //   0 <= z/w <=  1
                //         if (p->x >= -w && p->x <= w && p->y >= -w && p->y <= w && p->z >= 0 && p->z <= p->w) {
                //                 is_triangle_in_frustum = true;
                //                 break;
                //         }
                // }
                // if (!is_triangle_in_frustum) continue; // skip current face, since the whole face is outside the frustum

                // handle light [-1, 1] => [0, 1]
                // TODO(@k): we may later try smooth shading, it's a per pixel processing (some kind of linear interpolation) (ground shading algorithm, phong reflection model)
                // flat shading (easy and fast)
                float alignment = vec3_dot(vec3_inverse(light.direction), face_normal);
                float intensity = 0.5 * alignment + 0.5; /* it's better to do linear interp here, instead of clamping */
                uint32_t face_color = light_apply_intensity(mesh_face.color, intensity); 

                // frustum clipping
                // TODO(@k): handle orthographic projection issue
                triangle_t clipped_triangles[MAX_CLIPPED_TRIANGLES] = {};
                int num_clipped_triangles = clip_triangle(&triangle, zn, zf, clipped_triangles);

                // NOTE(@k): after clipping, we could end up more than one triangles
                for (int i = 0; i < num_clipped_triangles; i++) {
                        triangle_t *t = &clipped_triangles[i] ;
                        t->color = face_color;

                        // projection division
                        for (int i = 0; i < 3; i++) {
                                assert(t->points[i].w != 0.0);
                                t->points[i].x /= t->points[i].w;
                                t->points[i].y /= t->points[i].w;
                                t->points[i].z /= t->points[i].w;

                                // clipping space to screen space
                                // // translate based on window position
                                // // screen
                                // // [X X X X X X X]
                                // // [X X X X X X X]
                                // // [X X X O X X X] origin is in the center
                                // // [X X X X X X X]
                                // // [X X X X X X X]
                                float half_ww = window_width / 2.0;
                                float half_wh = window_height / 2.0;

                                // scale, and translate the projected points to the middle of the screen
                                // TODO(@k): not 100% sure if we need to multiply (1/zn) here
                                t->points[i].x = t->points[i].x * half_ww * (1 / zn) + half_ww;
                                // NOTE(@k): y grow towards downside in the screen coordinate system, so we negate y here
                                t->points[i].y = (-t->points[i].y) * half_wh * (1 / zn) + half_wh;

                                // NOTE(@k): handle precesion issue
                                float allow_margin = -0.01;
                                if (t->points[i].x < 0.0) {
                                        assert(t->points[i].x > allow_margin);
                                        t->points[i].x = 0.0;
                                }

                                if (t->points[i].y < 0.0) {
                                        assert(t->points[i].y > allow_margin);
                                        t->points[i].y = 0.0;
                                }
                        }

                        // calculate the average depth for each face based on the vertices after transformation
                        // NOTE(@k): this is a naive approach, better off to use z-buffer
                        // float avg_depth = (projected_points[0].z + projected_points[1].z + projected_points[2].z) / 3.0;

                        darray_push(triangles_to_render, *t);
                }
        }

        // NOTE(@k): this is an naive implementation to render base on the depth, z-buffer is better 
        // TODO(@k): bubble sort will do the job for now, but it could be a performance hit if we have much more triangle to render, consider quick-sort/merge-sort later
        // sort the triangles to render by their avg_depth
        // we need to render triangle with larger avg_depth first
        // int end = darray_size(triangles_to_render) - 1;
        // while (end > 0) {
        //         for (int i = 0; i < end; i++) {
        //                 if (triangles_to_render[i].avg_depth < triangles_to_render[i+1].avg_depth) {
        //                         // swap 
        //                         swap((char*)&triangles_to_render[i], (char*)&triangles_to_render[i+1], sizeof(triangle_t));
        //                 }
        //         }

        //         end--;
        // }
}

/////////////////////////////////////////////////////////////////////////////////////////
// render function to draw objects on the display
/////////////////////////////////////////////////////////////////////////////////////////
static void render(void) {
        if (paused) return;

        draw_grid(GRID_COLOR);

        // loop all projected triangles and render them
        for (int i = 0; i < darray_size(triangles_to_render); i++) {
                triangle_t triangle = triangles_to_render[i];

                // draw filled triangle
                if (render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE) {
                        // draw_filled_triangle(
                        //         triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w,
                        //         triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w,
                        //         triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w,
                        //         z_buffer, triangle.color
                        // );

                        draw_filled_triangle_v2(
                                &triangle.points[0],
                                &triangle.points[1],
                                &triangle.points[2],
                                z_buffer, triangle.color
                        );
                }

                // draw textured triangle
                if (render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE) {
                        draw_textured_triangle(
                                triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v,
                                triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v,
                                triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v,
                                z_buffer, mesh_texture, texture_width, texture_height
                        );
                }

                // draw triangle wireframe
                if (render_method == RENDER_WIRE || render_method == RENDER_WIRE_VERTEX || render_method == RENDER_TEXTURED_WIRE) {
                        draw_triangle(
                                triangle.points[0].x, triangle.points[0].y,
                                triangle.points[1].x, triangle.points[1].y,
                                triangle.points[2].x, triangle.points[2].y,
                                triangle.color
                        );
                }

                // draw triangle vertex points
                if (render_method == RENDER_WIRE_VERTEX) {
                        for (int j = 0; j < 3; j++) {
                                vec4_t point = triangles_to_render[i].points[j];
                                draw_rect(point.x - 1, point.y - 1, 6, 6, 0xFFFFFFFF);
                        }
                }
        }

        // ui stuff
        // draw FPS counter
        draw_simple_integer(fps, 30, 30, 9);
        draw_simple_integer(darray_size(triangles_to_render), 30, 60, 9);
        draw_simple_integer(previous_frame_time / 1000, 30, 90, 9);

        // clear the array of triangles to render every frame loop
        // we don't need to throw it away, just reuse the memory we allocated
        darray_clear(triangles_to_render);

        render_color_buffer();

        clear_color_buffer(BG_COLOR);
        clear_z_buffer(z_buffer, window_height * window_width);

        SDL_RenderPresent(renderer);
}

/////////////////////////////////////////////////////////////////////////////////////////
// free the memory that was dynamically allocated by the program
/////////////////////////////////////////////////////////////////////////////////////////
static void free_resources(void) {
        free(color_buffer);
        darray_free(mesh.vertices);
        darray_free(mesh.faces);
}

int main(void) {
        is_running = initialize_window();
        setup();

        while (is_running) {
                process_input();
                update();
                render();
        }

        destroy_window();
        free_resources();
        return 0;
}
