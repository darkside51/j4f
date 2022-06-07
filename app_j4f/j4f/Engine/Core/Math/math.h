#pragma once

// glm options
#define GLM_FORCE_INLINE
#define GLM_FORCE_INTRINSICS
#define GLM_FORCE_ALIGNED_GENTYPES
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_CXX17
#define GLM_FORCE_MESSAGES
// glm options

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <type_traits>

namespace engine {

    namespace math_constants {
        inline constexpr float pi       = 3.14159265359f;
        inline constexpr float e        = 2.71828182846f;
        inline constexpr float radInDeg = pi / 180.0f;
        inline constexpr float degInRad = 180.0f / pi;
    }
    
    inline static const glm::mat4 emptyMatrix   = glm::mat4(1.0f);
    inline static const glm::quat emptyQuat     = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
    inline static const glm::vec3 emptyVec3     = glm::vec3(0.0f, 0.0f, 0.0f);
    inline static const glm::vec3 unitVec3      = glm::vec3(1.0f, 1.0f, 1.0f);

    inline float inv_sqrt(const float x) { // "квадратный корень Крамака"(алгоритм Ньютона)
        const float xhalf = 0.5f * x;
        union {
            float x;
            int32_t i;
        } u;

        u.x = x;
        u.i = 0x5f3759df - (u.i >> 1);
        return u.x * (1.5f - xhalf * u.x * u.x);
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

    inline bool compare(const glm::vec2& v1, const glm::vec2& v2, const float eps) noexcept {
        return (std::fabsf(v1.x - v2.x) > eps) || (std::fabsf(v1.y - v2.y) > eps);
    }

    inline bool compare(const glm::vec3& v1, const glm::vec3& v2, const float eps) noexcept {
        return (std::fabsf(v1.x - v2.x) > eps) || (std::fabsf(v1.y - v2.y) > eps) || (std::fabsf(v1.z - v2.z) > eps);
    }

    inline bool compare(const glm::vec4& v1, const glm::vec4& v2, const float eps) noexcept {
        return (std::fabsf(v1.x - v2.x) > eps) || (std::fabsf(v1.y - v2.y) > eps) || (std::fabsf(v1.z - v2.z) > eps) || (std::fabsf(v1.w - v2.w) > eps);
    }

    inline bool compare(const glm::quat& q1, const glm::quat& q2, const float eps) noexcept {
        return (std::fabsf(q1.w - q2.w) > eps) || (std::fabsf(q1.x - q2.x) > eps) || (std::fabsf(q1.y - q2.y) > eps) || (std::fabsf(q1.z - q2.z) > eps);
    }

    enum class RotationsOrder : uint8_t {
        RO_XYZ = 0,
        RO_XZY = 1,
        RO_YXZ = 2,
        RO_YZX = 3,
        RO_ZXY = 4,
        RO_ZYX = 5
    };

    inline void rotateMatrix_x(glm::mat4& m, const float angle) {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const glm::vec4 tmpV = m[1] * c + m[2] * s;
        m[2] = -m[1] * s + m[2] * c;
        m[1] = tmpV;
    }

    inline void rotateMatrix_y(glm::mat4& m, const float angle) {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const glm::vec4 tmpV = m[2] * c + m[0] * s;
        m[0] = -m[2] * s + m[0] * c;
        m[2] = tmpV;
    }

    inline void rotateMatrix_z(glm::mat4& m, const float angle) {
        if (angle == 0.0f) return;

        const float c = cosf(angle);
        const float s = sinf(angle);

        const glm::vec4 tmpV = m[0] * c + m[1] * s;
        m[1] = -m[0] * s + m[1] * c;
        m[0] = tmpV;
    }

    inline void directMatrix_yz(glm::mat4& m, const float c, const float s) {
        const glm::vec4 tmpV = m[1] * c + m[2] * s;
        m[2] = -m[1] * s + m[2] * c;
        m[1] = tmpV;
    }

    inline void directMatrix_xz(glm::mat4& m, const float c, const float s) {
        const glm::vec4 tmpV = m[2] * c + m[0] * s;
        m[0] = -m[2] * s + m[0] * c;
        m[2] = tmpV;
    }

    inline void directMatrix_xy(glm::mat4& m, const float c, const float s) {
        const glm::vec4 tmpV = m[0] * c + m[1] * s;
        m[1] = -m[0] * s + m[1] * c;
        m[0] = tmpV;
    }

    inline void scaleMatrix(glm::mat4& matrix, const glm::vec3& scale) {
        matrix[0] *= scale[0];
        matrix[1] *= scale[1];
        matrix[2] *= scale[2];
    }

    inline void translateMatrixTo(glm::mat4& matrix, const glm::vec3& translate) {
        matrix[3][0] = translate.x;
        matrix[3][1] = translate.y;
        matrix[3][2] = translate.z;
    }

    inline void translateMatrix(glm::mat4& matrix, const glm::vec3& translate) {
        matrix[3] = matrix[0] * translate[0] + matrix[1] * translate[1] + matrix[2] * translate[2] + matrix[3];
    }

    inline void rotateMatrix_xyz(glm::mat4& transform, const glm::vec3& v) {
        rotateMatrix_x(transform, v.x);
        rotateMatrix_y(transform, v.y);
        rotateMatrix_z(transform, v.z);
    }

    inline void rotateMatrix_xzy(glm::mat4& transform, const glm::vec3& v) {
        rotateMatrix_x(transform, v.x);
        rotateMatrix_z(transform, v.z);
        rotateMatrix_y(transform, v.y);
    }

    inline void rotateMatrix_yxz(glm::mat4& transform, const glm::vec3& v) {
        rotateMatrix_y(transform, v.y);
        rotateMatrix_x(transform, v.x);
        rotateMatrix_z(transform, v.z);
    }

    inline void rotateMatrix_yzx(glm::mat4& transform, const glm::vec3& v) {
        rotateMatrix_y(transform, v.y);
        rotateMatrix_z(transform, v.z);
        rotateMatrix_x(transform, v.x);
    }

    inline void rotateMatrix_zxy(glm::mat4& transform, const glm::vec3& v) {
        rotateMatrix_z(transform, v.z);
        rotateMatrix_x(transform, v.x);
        rotateMatrix_y(transform, v.y);
    }

    inline void rotateMatrix_zyx(glm::mat4& transform, const glm::vec3& v) {
        rotateMatrix_z(transform, v.z);
        rotateMatrix_y(transform, v.y);
        rotateMatrix_x(transform, v.x);
    }

    inline void rotateMatrix_byOrder(glm::mat4& transform, const glm::vec3& v, const RotationsOrder r) {
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

    inline void quatToMatrix(const glm::quat& q, glm::mat4& matrix) {
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
        const glm::mat4& m,
        glm::vec3& s,
        glm::quat& r,
        glm::vec3& t
    ) {
        s.x = vec_length(glm::vec3(m[0]));
        s.y = vec_length(glm::vec3(m[1]));
        s.z = vec_length(glm::vec3(m[2]));
        // glm::sign(m[0][0]) - и аналогичные, для знака скелинга?

        if (s == emptyVec3) {
            r = emptyQuat;
            t = emptyVec3;
        } else {
            const glm::mat4 m1 = glm::scale(m, 1.0f / s);
            r = glm::normalize(glm::quat_cast(m1));
            t.x = m1[3].x;
            t.y = m1[3].y;
            t.z = m1[3].z;
        }
    }
}