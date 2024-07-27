#ifndef MESH_H
#define MESH_H
#include "triangle.h"
#include "vector.h"

#define N_CUBE_VERTICES 8
extern vec3_t cube_vertices[N_CUBE_VERTICES];

#define N_CUBE_FACES (6 * 2) // 6 cube faces, 2 triangles per face
// basically the vertices index
extern face_t cube_faces[N_CUBE_FACES];

typedef struct {
        vec3_t *vertices;     // dynamic array of vertices
        face_t *faces;        // dynamic array of faces
        vec3_t rotation;      // rotation with x, y and z values
        vec3_t scale;         // scale with x, y, and z values
        vec3_t translation;   // translation with x, y and z values
        // position           // word position for example
} mesh_t;

extern mesh_t mesh;
void load_cube_mesh_data(void);
void load_obj(char *file);
#endif
