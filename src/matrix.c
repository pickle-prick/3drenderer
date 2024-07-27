#include <math.h>
#include "matrix.h"

mat4_t mat4_eye(void) {
        // | 1  0  0  0 |
        // | 0  1  0  0 |
        // | 0  0  1  0 |
        // | 0  0  0  1 |
        mat4_t m = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
        return m;
}

mat4_t mat4_make_scale(float sx, float sy, float sz) {
        // | sx  0  0  0 |
        // |  0 sy  0  0 |
        // |  0  0 sz  0 |
        // |  0  0  0  1 |
        mat4_t m = mat4_eye();
        m.m[0][0] = sx;
        m.m[1][1] = sy;
        m.m[2][2] = sz;
        return m;
}

mat4_t mat4_make_translation(float tx, float ty, float tz) {
        // | 1  0  0 tx |   |x|
        // | 0  1  0 ty |   |y|
        // | 0  0  1 tz | * |z|
        // | 0  0  0  1 |   |1|
        mat4_t m = mat4_eye();
        m.m[0][3] = tx;
        m.m[1][3] = ty;
        m.m[2][3] = tz;
        return m;
}

mat4_t mat4_make_rotation_x(float r) {
        // | 1  0   0  0 |
        // | 0  c  -s  0 |
        // | 0  0   0  1 |
        // | 0  s   c  0 |
        float c = cosf(r);
        float s = sinf(r);

        mat4_t m = mat4_eye();
        m.m[1][1] = c;
        m.m[1][2] = -s;
        m.m[2][1] = s;
        m.m[2][2] = c;
        return m;
}

mat4_t mat4_make_rotation_y(float r) {
        // |  c  0   s  0 |
        // |  0  c   0  0 |
        // | -s  0   c  0 |
        // |  0  0   0  1 |
        float c = cosf(r);
        float s = sinf(r);

        mat4_t m = mat4_eye();
        // NOTE(@k): counter clock-wise via y
        m.m[0][0] = c;
        m.m[0][2] = s;
        m.m[2][0] = -s;
        m.m[2][2] = c;
        return m;
}

mat4_t mat4_make_rotation_z(float r) {
        float c = cosf(r);
        float s = sinf(r);

        mat4_t m = mat4_eye();
        m.m[0][0] = c;
        m.m[0][1] = -s;
        m.m[1][0] = s;
        m.m[1][1] = c;
        return m;
}

vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
        // treat vec4 as column-major, thus post-multiplication
        vec4_t ret;
        ret.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
        ret.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
        ret.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
        ret.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
        return ret;
}

mat4_t mat4_mul_mat4(mat4_t ma, mat4_t mb) {
        mat4_t m = mat4_eye();

        for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                        m.m[i][j] = ma.m[i][0] * mb.m[0][j] + ma.m[i][1] * mb.m[1][j] + ma.m[i][2] * mb.m[2][j] + ma.m[i][3] * mb.m[3][j];
                }
        }
        return m;
}

mat4_t mat4_make_orthographic(float fov, int wh, int ww, float zn, float zf) {
        // | rf  0          0            0 |
        // |  0  f          0            0 |
        // |  0  0  1/(zf-zn)  -zn/(zf-zn) |
        // |  0  0          0            1 |
        mat4_t m = {{{ 0 }}};
        float r = (float)wh / ww;
        float f = 1 / tanf(fov / 2);
        float d = 1 / (zf - zn);

        m.m[0][0] = r * f;
        m.m[1][1] = f;
        m.m[2][2] = d;
        m.m[2][3] = -zn * d;
        m.m[3][3] = 1;
        return m;
}

// if we did frustum clipping, x and y will fall into [-1, 1] after the z division, z will fall into [0, 1]
mat4_t mat4_make_perspective(float fov, int wh, int ww, float zn, float zf) {
        // | zn  0  0          0 |
        // |  0 zn  0          0 |
        // |  0  0  zn+zf -zn*zf |
        // |  0  0  1          0 |
        // copy z to w, w is for depth division later
        mat4_t m = {{{ 0 }}};
        m.m[0][0] = zn;
        m.m[1][1] = zn;
        m.m[2][2] = zn + zf;
        m.m[2][3] = -zn * zf;
        m.m[3][2] = 1;
        m.m[3][3] = 0;

        mat4_t ret = mat4_make_orthographic(fov, wh, ww, zn, zf);
        ret = mat4_mul_mat4(ret, m);
        return ret;
}

mat4_t mat4_transpose(mat4_t m) {
        mat4_t ret = {{{ 0 }}};

        for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                        ret.m[i][j] = m.m[j][i];
                }
        }

        return ret;
}
