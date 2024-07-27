#include "camera.h"
#include "matrix.h"
#include "vector.h"

/*
 * NOTE(@k): in mose case we choose { 0, 1, 0 } as the pivot
 * ref: https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/lookat-function/framing-lookat-function.html
 * The look-at method is straightforward and generally effective for placing a camera in a 3D scene.
 * However, it does have a notable limitation, particularly when the camera's orientation is vertical, either looking straight down or straight up.
 * In such orientations, the forward axis aligns closely or even becomes parallel to the arbitrary axis employed in calculating the right axis.
 * This alignment issue is most critical when the forward vector and the arbitrary axis are perfectly parallel, for example, when the forward vector is (0,1,0) or (0,-1,0).
 * In these scenarios, the cross-product operation, which is pivotal in generating the right vector, fails to yield a valid result due to the lack of a perpendicular component between the parallel vectors.
 *
 * Unfortunately, there's no straightforward fix for this issue.
 * One practical approach is to detect when this situation occurs and manually set the vectors, leveraging the understanding of their expected configuration in such cases.
 * A more sophisticated and elegant solution involves the use of quaternion interpolation.
 * Quaternion interpolation offers a way to smoothly transition between orientations, avoiding the pitfalls of gimbal lock and the issue described here, by providing a continuous path between rotational states even when approaching vertical orientations.
 * This method can maintain a stable and consistent camera orientation, circumventing the limitations of the cross-product in extreme cases.)
 */
mat4_t mat4_look_at(vec3_t target_pos, vec3_t camera_pos, vec3_t pivot) {
        // Mt for translation
        // | 1  0  0 -cx |
        // | 0  1  0 -cy |
        // | 0  0  1 -cz |
        // | 0  0  0   1 |
        mat4_t m_t = mat4_eye();
        m_t.m[0][3] = -camera_pos.x;
        m_t.m[1][3] = -camera_pos.y;
        m_t.m[2][3] = -camera_pos.z;

        vec3_t z = vec3_sub(target_pos, camera_pos);
        vec3_normalize(&z); // forward-z
        vec3_t x = vec3_cross(pivot, z); // right-x
        vec3_normalize(&x);
        vec3_t y = vec3_cross(z, x); // up-y

        // Mr for rotation
        // | x.x  y.x  z.x  0 |
        // | x.y  y.y  z.y  0 |
        // | x.z  y.z  z.z  0 |
        // |   0    0    0  1 |
        // transpose matrix, also is inverse matrix in this case
        // | x.x  x.y  x.z  0 |
        // | y.x  y.y  y.y  0 |
        // | z.x  z.y  z.z  0 |
        // |   0    0    0  1 |
        mat4_t m_r = mat4_eye();
        m_r.m[0][0] = x.x;
        m_r.m[0][1] = y.x;
        m_r.m[0][2] = z.x;

        m_r.m[1][0] = x.y;
        m_r.m[1][1] = y.y;
        m_r.m[1][2] = z.y;

        m_r.m[2][0] = x.z;
        m_r.m[2][1] = y.z;
        m_r.m[2][2] = z.z;

        // NOTE(@k): Mr is an orthogonal matrix, so we can compute its transpose to get its inverse
        //           Some Properties of an Orthogonal Matrix 
        //           * Inverse and Transpose are equivalent. i.e., A-1 = AT.
        //           * An identity matrix is the outcome of A and its transpose. That is, AAT = ATA = I.
        //           * In light of the fact that its determinant is never 0, an orthogonal matrix is always non-singular.
        //           * An orthogonal diagonal matrix is one whose members are either 1 or -1.
        //           * AT is orthogonal as well. A-1 is also orthogonal because A-1 = AT.
        //           * The eigenvalues of A are ±1 and the eigenvectors are orthogonal.
        //           * As I × I = I × I = I, and IT = I. Thus, I an identity matrix (I) is orthogonal.)
        mat4_t m_r_inv = mat4_transpose(m_r);

        // TODO(@k): we can pre-computed m_r * mr_t
        mat4_t ret = mat4_mul_mat4(m_r_inv, m_t);
        return ret;
}

mat4_t mat4_from_camera(vec3_t camera_pos, float yaw, float pitch) {
        // Mt for translation
        // | 1  0  0 -cx |
        // | 0  1  0 -cy |
        // | 0  0  1 -cz |
        // | 0  0  0   1 |
        mat4_t m_t = mat4_eye();
        m_t.m[0][3] = -camera_pos.x;
        m_t.m[1][3] = -camera_pos.y;
        m_t.m[2][3] = -camera_pos.z;

        vec4_t i_hat = { 1, 0, 0, 1 }; // x or right
        vec4_t j_hat = { 0, 1, 0, 1 }; // y or up
        vec4_t k_hat = { 0, 0, 1, 1 }; // z or forward

        mat4_t yaw_rotation = mat4_make_rotation_y(yaw);
        i_hat = mat4_mul_vec4(yaw_rotation, i_hat);
        k_hat = mat4_mul_vec4(yaw_rotation, k_hat);

        mat4_t pitch_rotation = mat4_make_rotation_x(pitch);
        j_hat = mat4_mul_vec4(pitch_rotation, j_hat);
        k_hat = mat4_mul_vec4(pitch_rotation, k_hat);

        // Mr for rotation
        // | i.x  j.x  k.x  0 |
        // | i.y  j.y  k.y  0 |
        // | i.z  j.z  k.z  0 |
        // |   0    0    0  1 |
        // transpose matrix, also is inverse matrix in this case
        // | i.x  i.y  i.z  0 |
        // | j.x  j.y  j.y  0 |
        // | k.x  k.y  k.z  0 |
        // |   0    0    0  1 |
        mat4_t m_r = mat4_eye();
        m_r.m[0][0] = i_hat.x;
        m_r.m[0][1] = j_hat.x;
        m_r.m[0][2] = k_hat.x;

        m_r.m[1][0] = i_hat.y;
        m_r.m[1][1] = j_hat.y;
        m_r.m[1][2] = k_hat.y;

        m_r.m[2][0] = i_hat.z;
        m_r.m[2][1] = j_hat.z;
        m_r.m[2][2] = k_hat.z;

        // NOTE(@k): Mr is an orthogonal matrix, so we can compute its transpose to get its inverse
        //           Some Properties of an Orthogonal Matrix 
        //           * Inverse and Transpose are equivalent. i.e., A-1 = AT.
        //           * An identity matrix is the outcome of A and its transpose. That is, AAT = ATA = I.
        //           * In light of the fact that its determinant is never 0, an orthogonal matrix is always non-singular.
        //           * An orthogonal diagonal matrix is one whose members are either 1 or -1.
        //           * AT is orthogonal as well. A-1 is also orthogonal because A-1 = AT.
        //           * The eigenvalues of A are ±1 and the eigenvectors are orthogonal.
        //           * As I × I = I × I = I, and IT = I. Thus, I an identity matrix (I) is orthogonal.)
        mat4_t m_r_inv = mat4_transpose(m_r);

        mat4_t ret = mat4_mul_mat4(m_r_inv, m_t); // translation first then rotation
        return ret;
}

mat4_t rotate_around_it(vec3_t it, float rx, float ry, float rz) {
        mat4_t ret = mat4_eye();

        // Mt for translation
        // | 1  0  0 -dx |
        // | 0  1  0 -dy |
        // | 0  0  1 -dz |
        // | 0  0  0   1 |
        mat4_t m_t = mat4_eye();
        m_t.m[0][3] = it.x;
        m_t.m[1][3] = it.y;
        m_t.m[2][3] = it.z;

        ret = mat4_mul_mat4(ret, m_t);

        ret = mat4_mul_mat4(ret, mat4_make_rotation_x(rx));
        ret = mat4_mul_mat4(ret, mat4_make_rotation_y(ry));
        ret = mat4_mul_mat4(ret, mat4_make_rotation_z(rz));

        // | 1  0  0 dx |
        // | 0  1  0 dy |
        // | 0  0  1 dz |
        // | 0  0  0  1 |
        mat4_t m_t_inv = mat4_eye();
        m_t_inv.m[0][3] = -it.x;
        m_t_inv.m[1][3] = -it.y;
        m_t_inv.m[2][3] = -it.z;

        ret = mat4_mul_mat4(ret, m_t_inv);
        return ret;
}
