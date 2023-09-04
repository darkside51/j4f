#pragma once
// ♣♠♦♥

// glm options for enable vectorisation
#define GLM_FORCE_INLINE
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_ALIGNED
#define GLM_FORCE_ALIGNED_GENTYPES
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_CXX17
#define GLM_FORCE_MESSAGES
// glm options

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../../3rd_party/half_float/umHalf.h"

#include <cmath>
#include <cstdint>
#include <type_traits>

namespace engine {

    namespace math_constants {
        inline constexpr float pi       = 3.14159265359f;
        inline constexpr float pi2      = pi * 2.0f;
        inline constexpr float pi_half  = pi / 2.0f;
        inline constexpr float e        = 2.71828182846f;
        inline constexpr float radInDeg = pi / 180.0f;
        inline constexpr float degInRad = 180.0f / pi;
    }
    
    using vec2f = glm::vec2;
    using vec3f = glm::vec3;
    using vec4f = glm::vec4;
    using mat2f = glm::mat2;
    using mat3f = glm::mat3;
    using mat4f = glm::mat4;
    using quatf = glm::quat;

    using vec2d = glm::dvec2;
    using vec3d = glm::dvec3;
    using vec4d = glm::dvec4;
    using mat2d = glm::dmat2;
    using mat3d = glm::dmat3;
    using mat4d = glm::dmat4;
    using quatd = glm::dquat;

    inline static const mat4f emptyMatrix   = mat4f(1.0f);
    inline static const quatf emptyQuat     = quatf(0.0f, 0.0f, 0.0f, 0.0f);
    inline static const vec2f emptyVec2     = vec2f(0.0f, 0.0f);
    inline static const vec3f emptyVec3     = vec3f(0.0f, 0.0f, 0.0f);
    inline static const vec3f unitVec3      = vec3f(1.0f, 1.0f, 1.0f);

    inline float inv_sqrt(const float x) { // "инверсный(1.0f / sqrt) квадратный корень Кармака"
        const float xhalf = 0.5f * x;
        union {
            float x = 0.0f;
            int32_t i;
        } u;

        u.x = x;
        u.i = 0x5f3759df - (u.i >> 1);
        return u.x * (1.5f - xhalf * u.x * u.x);
    }

    inline bool is_pow2(uint32_t n) noexcept {
        return (n > 0) && (!(n & (n - 1)));
    }

    template<typename T>
    inline int32_t sign(T&& val) noexcept {
        return (T(0) < val) - (val < T(0));
    }

    template <typename VEC>
    inline float vec_length(VEC&& v) {
        return 1.0f / inv_sqrt(glm::dot(v, v));
    }

    template <typename VEC>
    inline void normalize(VEC& v) {
        v *= inv_sqrt(glm::dot(v, v));
    }

    template <typename VEC>
    inline auto as_normalized(VEC&& v) -> typename std::decay_t<VEC> {
        return v * inv_sqrt(glm::dot(v, v));
    }

    inline bool compare(const vec2f& v1, const vec2f& v2, const float eps) noexcept {
        return (fabsf(v1.x - v2.x) > eps) || (fabsf(v1.y - v2.y) > eps);
    }

    inline bool compare(const vec3f& v1, const vec3f& v2, const float eps) noexcept {
        return (fabsf(v1.x - v2.x) > eps) || (fabsf(v1.y - v2.y) > eps) || (fabsf(v1.z - v2.z) > eps);
    }

    inline bool compare(const vec4f& v1, const vec4f& v2, const float eps) noexcept {
        return (fabsf(v1.x - v2.x) > eps) || (fabsf(v1.y - v2.y) > eps) || (fabsf(v1.z - v2.z) > eps) || (fabsf(v1.w - v2.w) > eps);
    }

    inline bool compare(const quatf& q1, const quatf& q2, const float eps) noexcept {
        return (fabsf(q1.w - q2.w) > eps) || (fabsf(q1.x - q2.x) > eps) || (fabsf(q1.y - q2.y) > eps) || (fabsf(q1.z - q2.z) > eps);
    }

    enum class RotationsOrder : uint8_t {
        RO_XYZ = 0u,
        RO_XZY = 1u,
        RO_YXZ = 2u,
        RO_YZX = 3u,
        RO_ZXY = 4u,
        RO_ZYX = 5u
    };

    inline void rotateMatrix_x(mat4f& m, const float angle) noexcept {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const vec4f tmpV = m[1] * c + m[2] * s;
        m[2] = -m[1] * s + m[2] * c;
        m[1] = tmpV;
    }

    inline void rotateMatrix_y(mat4f& m, const float angle) noexcept {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const vec4f tmpV = m[2] * c + m[0] * s;
        m[0] = -m[2] * s + m[0] * c;
        m[2] = tmpV;
    }

    inline void rotateMatrix_z(mat4f& m, const float angle) noexcept {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const vec4f tmpV = m[0] * c + m[1] * s;
        m[1] = -m[0] * s + m[1] * c;
        m[0] = tmpV;
    }

    inline void directMatrix_yz(mat4f& m, const float c, const float s) noexcept {
        const vec4f tmpV = m[1] * c + m[2] * s;
        m[2] = -m[1] * s + m[2] * c;
        m[1] = tmpV;
    }

    inline void directMatrix_xz(mat4f& m, const float c, const float s) noexcept {
        const vec4f tmpV = m[2] * c + m[0] * s;
        m[0] = -m[2] * s + m[0] * c;
        m[2] = tmpV;
    }

    inline void directMatrix_xy(mat4f& m, const float c, const float s) noexcept {
        const vec4f tmpV = m[0] * c + m[1] * s;
        m[1] = -m[0] * s + m[1] * c;
        m[0] = tmpV;
    }

    inline void scaleMatrix(mat4f& matrix, const vec3f& scale) noexcept {
        matrix[0] *= scale[0];
        matrix[1] *= scale[1];
        matrix[2] *= scale[2];
    }

    inline void translateMatrixTo(mat4f& matrix, const vec3f& translate) noexcept {
        matrix[3][0] = translate.x;
        matrix[3][1] = translate.y;
        matrix[3][2] = translate.z;
    }

    inline void translateMatrix(mat4f& matrix, const vec3f& translate) noexcept {
        matrix[3] = matrix[0] * translate[0] + matrix[1] * translate[1] + matrix[2] * translate[2] + matrix[3];
    }

    inline void rotateMatrix_xyz(mat4f& transform, const vec3f& v) noexcept {
        rotateMatrix_x(transform, v.x);
        rotateMatrix_y(transform, v.y);
        rotateMatrix_z(transform, v.z);
    }

    inline void rotateMatrix_xzy(mat4f& transform, const vec3f& v) noexcept {
        rotateMatrix_x(transform, v.x);
        rotateMatrix_z(transform, v.z);
        rotateMatrix_y(transform, v.y);
    }

    inline void rotateMatrix_yxz(mat4f& transform, const vec3f& v) noexcept {
        rotateMatrix_y(transform, v.y);
        rotateMatrix_x(transform, v.x);
        rotateMatrix_z(transform, v.z);
    }

    inline void rotateMatrix_yzx(mat4f& transform, const vec3f& v) noexcept {
        rotateMatrix_y(transform, v.y);
        rotateMatrix_z(transform, v.z);
        rotateMatrix_x(transform, v.x);
    }

    inline void rotateMatrix_zxy(mat4f& transform, const vec3f& v) noexcept {
        rotateMatrix_z(transform, v.z);
        rotateMatrix_x(transform, v.x);
        rotateMatrix_y(transform, v.y);
    }

    inline void rotateMatrix_zyx(mat4f& transform, const vec3f& v) noexcept {
        rotateMatrix_z(transform, v.z);
        rotateMatrix_y(transform, v.y);
        rotateMatrix_x(transform, v.x);
    }

    inline void rotateMatrix_byOrder(mat4f& transform, const vec3f& v, const RotationsOrder r) noexcept {
        switch (r) {
            case RotationsOrder::RO_XYZ:
                rotateMatrix_xyz(transform, v);
                break;
            case RotationsOrder::RO_XZY:
                rotateMatrix_xzy(transform, v);
                break;
            case RotationsOrder::RO_YXZ:
                rotateMatrix_yxz(transform, v);
                break;
            case RotationsOrder::RO_YZX:
                rotateMatrix_yzx(transform, v);
                break;
            case RotationsOrder::RO_ZXY:
                rotateMatrix_zxy(transform, v);
                break;
            case RotationsOrder::RO_ZYX:
                rotateMatrix_zyx(transform, v);
                break;
            default:
                break;
        }
    }

    inline void quatToMatrix(const quatf& q, mat4f& matrix) noexcept {
        if (q.x == 0.0f && q.y == 0.0f && q.z == 0.0f && q.w == 1.0f) {
            matrix[0][0] = 1.0f; matrix[0][1] = 0.0f; matrix[0][2] = 0.0f;
            matrix[1][0] = 0.0f; matrix[1][1] = 1.0f; matrix[1][2] = 0.0f;
            matrix[2][0] = 0.0f; matrix[2][1] = 0.0f; matrix[2][2] = 1.0f;
        } else {
            const float qxx(q.x * q.x);
            const float qyy(q.y * q.y);
            const float qzz(q.z * q.z);
            const float qxz(q.x * q.z);
            const float qxy(q.x * q.y);
            const float qyz(q.y * q.z);
            const float qwx(q.w * q.x);
            const float qwy(q.w * q.y);
            const float qwz(q.w * q.z);

            matrix[0][0] = 1.0f - 2.0f * (qyy + qzz);
            matrix[0][1] = 2.0f * (qxy + qwz);
            matrix[0][2] = 2.0f * (qxz - qwy);

            matrix[1][0] = 2.0f * (qxy - qwz);
            matrix[1][1] = 1.0f - 2.0f * (qxx + qzz);
            matrix[1][2] = 2.0f * (qyz + qwx);

            matrix[2][0] = 2.0f * (qxz + qwy);
            matrix[2][1] = 2.0f * (qyz - qwx);
            matrix[2][2] = 1.0f - 2.0f * (qxx + qyy);
        }
    }

    inline void decomposeMatrix(
        const mat4f& m,
        vec3f& s,
        quatf& r,
        vec3f& t
    ) {
        s.x = vec_length(vec3f(m[0]));
        s.y = vec_length(vec3f(m[1]));
        s.z = vec_length(vec3f(m[2]));
        // glm::sign(m[0][0]) - и аналогичные, для знака скелинга?

        if (s == emptyVec3) {
            r = emptyQuat;
            t = emptyVec3;
        } else {
            const mat4f m1 = glm::scale(m, 1.0f / s);
            r = glm::normalize(glm::quat_cast(m1));
            t.x = m1[3].x;
            t.y = m1[3].y;
            t.z = m1[3].z;
        }
    }

    inline void lookAt(const vec3f& eye, const vec3f& center, const vec3f& up, mat4f& matrix) noexcept {
        const vec3f f(as_normalized(center - eye));
        const vec3f s(as_normalized(cross(f, up)));
        const vec3f u(cross(s, f));

        matrix[0][3] = 0.0f;
        matrix[1][3] = 0.0f;
        matrix[2][3] = 0.0f;

        matrix[0][0] = s.x;
        matrix[1][0] = s.y;
        matrix[2][0] = s.z;

        matrix[0][1] = u.x;
        matrix[1][1] = u.y;
        matrix[2][1] = u.z;

        matrix[0][2] = -f.x;
        matrix[1][2] = -f.y;
        matrix[2][2] = -f.z;

        matrix[3][0] = -dot(s, eye);
        matrix[3][1] = -dot(u, eye);
        matrix[3][2] =  dot(f, eye);
    }

    inline mat4f getBillboardViewMatrix(const mat4f& inverseViewMatrix) noexcept {
        mat4f matrix = inverseViewMatrix;
        // matrix[3].x = 0.0f; matrix[3].y = 0.0f; matrix[3].z = 0.0f;
        // matrix[3].w = matrix[3].w; // не меняем значение
        memset(&matrix[3], 0, sizeof(float) * 3);
        return matrix;
    }
}