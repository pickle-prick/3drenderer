#include "mesh.h"
#include "darray.h"
#include "vector.h"
#include "util.h"
#include <assert.h>
#include <stdbool.h>
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024
vec3_t cube_vertices[N_CUBE_VERTICES] = {
        {-1, -1, -1},  // 1
        {-1,  1, -1},  // 2
        { 1,  1, -1},  // 3
        { 1, -1, -1},  // 4
        { 1,  1,  1},  // 5
        { 1, -1,  1},  // 6
        {-1,  1,  1},  // 7
        {-1, -1,  1},  // 8
};

face_t cube_faces[N_CUBE_FACES] = {
        // front
        {.a = 1, .b = 2, .c = 3, .a_uv = {0, 0}, .b_uv = {0, 1}, .c_uv = {1, 1}, .color = DEMO_CUBE_COLOR},
        {.a = 1, .b = 3, .c = 4, .a_uv = {0, 0}, .b_uv = {1, 1}, .c_uv = {1, 0}, .color = DEMO_CUBE_COLOR},
        // right
        {.a = 4, .b = 3, .c = 5, .a_uv = {0, 0}, .b_uv = {0, 1}, .c_uv = {1, 1}, .color = DEMO_CUBE_COLOR},
        {.a = 4, .b = 5, .c = 6, .a_uv = {0, 0}, .b_uv = {1, 1}, .c_uv = {1, 0}, .color = DEMO_CUBE_COLOR},
        // back
        {.a = 6, .b = 5, .c = 7, .a_uv = {0, 0}, .b_uv = {0, 1}, .c_uv = {1, 1}, .color = DEMO_CUBE_COLOR},
        {.a = 6, .b = 7, .c = 8, .a_uv = {0, 0}, .b_uv = {1, 1}, .c_uv = {1, 0}, .color = DEMO_CUBE_COLOR},
        // left
        {.a = 8, .b = 7, .c = 2, .a_uv = {0, 0}, .b_uv = {0, 1}, .c_uv = {1, 1}, .color = DEMO_CUBE_COLOR},
        {.a = 8, .b = 2, .c = 1, .a_uv = {0, 0}, .b_uv = {1, 1}, .c_uv = {1, 0}, .color = DEMO_CUBE_COLOR},
        // top
        {.a = 2, .b = 7, .c = 5, .a_uv = {0, 0}, .b_uv = {0, 1}, .c_uv = {1, 1}, .color = DEMO_CUBE_COLOR},
        {.a = 2, .b = 5, .c = 3, .a_uv = {0, 0}, .b_uv = {1, 1}, .c_uv = {1, 0}, .color = DEMO_CUBE_COLOR},
        // bottom
        {.a = 6, .b = 8, .c = 1, .a_uv = {0, 0}, .b_uv = {0, 1}, .c_uv = {1, 1}, .color = DEMO_CUBE_COLOR},
        {.a = 6, .b = 1, .c = 4, .a_uv = {0, 0}, .b_uv = {1, 1}, .c_uv = {1, 0}, .color = DEMO_CUBE_COLOR},
};

void load_cube_mesh_data(void) {
        for (int i = 0; i < N_CUBE_VERTICES; i++) {
                vec3_t vertex = cube_vertices[i];
                darray_push(mesh.vertices, vertex);
        }

        for (int i = 0; i < N_CUBE_FACES; i++) {
                face_t face = cube_faces[i];
                darray_push(mesh.faces, face);
        }
}

/*
 * return the word size of EOF
 */
int read_word(FILE *fp, char *word, int max_char) {
        int c = 0;
        int n = 0;
        for (n = 0; (c = fgetc(fp)) != EOF && c != ' ' && c != '\n' && n < max_char - 1; n++) {
                *(word++) = c;
        }

        *(word++) = '\0';
        return c == EOF ? EOF : n;
}

bool str_cmp(char *s1, char *s2) {
        char c;
        while ((c = *s1++) && c == *s2++);
        return *s1 == *s2 ? true : false;
}

void load_obj(char *file) {
        FILE *fp = fopen(file, "r");
        if (fp == NULL) {
                // TODO: the error can be identified more precisely
                // see the discussion of error-handling functions at the end of
                // Section 1 in Appendix B in clang edition 2
                fprintf(stderr, "failed to open file %s\n", file);
                exit(1);
        }

        char line[MAX_LINE];

        tex2_t *uvs = NULL;
        while (fgets(line, MAX_LINE, fp)) {
                int items;
                if (strncmp(line, "v ", 2) == 0) {
                        // reading vertices
                        vec3_t vertex;
                        items = sscanf(line + 2, "%f %f %f", &vertex.x,
                                       &vertex.y, &vertex.z);
                        assert(items == 3);
                        darray_push(mesh.vertices, vertex);
                } else if (strncmp(line, "vt ", 3) == 0) {
                        tex2_t uv;
                        sscanf(line + 3, "%f%f", &uv.u, &uv.v);
                        darray_push(uvs, uv);
                } else if (strncmp(line, "f ", 2) == 0) {
                        // reading faces
                        face_t face;
                        int vertex_indices[3];
                        int texture_uv_indices[3];
                        // TODO(@k): normal_indices

                        items = sscanf(line + 2,
                                       "%d/%d/%*d %d/%d/%*d %d/%d/%*d",
                                       &vertex_indices[0], &texture_uv_indices[0],
                                       &vertex_indices[1], &texture_uv_indices[1],
                                       &vertex_indices[2], &texture_uv_indices[2]);

                        assert(uvs != NULL);
                        assert(items == 6);

                        face.a = vertex_indices[0];
                        face.b = vertex_indices[1];
                        face.c = vertex_indices[2];

                        face.a_uv = uvs[texture_uv_indices[0] - 1];
                        face.b_uv = uvs[texture_uv_indices[1] - 1];
                        face.c_uv = uvs[texture_uv_indices[2] - 1];

                        // NOTE(@k): some u,v float could slightly exceed 1.0, clamp it to 1.0
                        //           vt 1.054287 0.431093
                        if (face.a_uv.u > 1.0) face.a_uv.u = 1.0;
                        float_clamp_inline(&face.a_uv.u, 0.0, 1.0);
                        float_clamp_inline(&face.a_uv.v, 0.0, 1.0);
                        float_clamp_inline(&face.b_uv.u, 0.0, 1.0);
                        float_clamp_inline(&face.b_uv.v, 0.0, 1.0);
                        float_clamp_inline(&face.c_uv.u, 0.0, 1.0);
                        float_clamp_inline(&face.c_uv.v, 0.0, 1.0);

                        // TODO(@k): make it configable
                        face.color = 0xFFFFFFFF;
                        darray_push(mesh.faces, face);
                } else {
                        // skip other line for now
                        continue;
                }
        }

        if (uvs != NULL) darray_free(uvs);
}

// TODO(@k): try to eliminate global variables
// initialize the global mesh
mesh_t mesh = {
        .faces       = NULL,
        .vertices    = NULL,
        .rotation    = {  0,   0,   0},
        .translation = {  0,   0,   0},
        .scale       = {1.0, 1.0, 1.0},
};
