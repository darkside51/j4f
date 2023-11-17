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
        template<typename T>
        inline constexpr T pi = T(3.14159265359);

        template<typename T>
        inline constexpr T pi2 = pi<T> * T(2.0);

        template<typename T>
        inline constexpr T pi_2 = pi<T> / T(2.0);

        template<typename T>
        inline constexpr T e = T(2.71828182846);

        template<typename T>
        inline constexpr T radInDeg = pi<T> / T(180.0);

        template<typename T>
        inline constexpr T degInRad = T(180.0) / pi<T>;

        namespace f32 {
            inline constexpr float pi       = math_constants::pi<float>;
            inline constexpr float pi2      = math_constants::pi2<float>;
            inline constexpr float pi_2     = math_constants::pi_2<float>;
            inline constexpr float e        = math_constants::e<float>;
            inline constexpr float radInDeg = math_constants::radInDeg<float>;
            inline constexpr float degInRad = math_constants::degInRad<float>;
        }

        namespace d64 {
            inline constexpr double pi       = math_constants::pi<double>;
            inline constexpr double pi2      = math_constants::pi2<double>;
            inline constexpr double pi_2     = math_constants::pi_2<double>;
            inline constexpr double e        = math_constants::e<double>;
            inline constexpr double radInDeg = math_constants::radInDeg<double>;
            inline constexpr double degInRad = math_constants::degInRad<double>;
        }
    }

    template <typename T>
    using vec2 = glm::vec<2, T, glm::defaultp>;

    template <typename T>
    using vec3 = glm::vec<3, T, glm::defaultp>;

    template <typename T>
    using vec4 = glm::vec<4, T, glm::defaultp>;

    template <typename T>
    using mat2 = glm::mat<2, 2, T, glm::defaultp>;

    template <typename T>
    using mat3 = glm::mat<3, 3, T, glm::defaultp>;

    template <typename T>
    using mat4 = glm::mat<4, 4, T, glm::defaultp>;

    template <typename T>
    using quat = glm::qua<T, glm::defaultp>;

    using vec2f = vec2<float>;
    using vec3f = vec3<float>;
    using vec4f = vec4<float>;
    using mat2f = mat2<float>;
    using mat3f = mat3<float>;
    using mat4f = mat4<float>;
    using quatf = quat<float>;

    using vec2d = vec2<double>;
    using vec3d = vec3<double>;
    using vec4d = vec4<double>;
    using mat2d = mat2<double>;
    using mat3d = mat3<double>;
    using mat4d = mat4<double>;
    using quatd = quat<double>;

    inline static const mat4f emptyMatrix   = mat4f(1.0f);
    inline static const quatf emptyQuat     = quatf(0.0f, 0.0f, 0.0f, 0.0f);
    inline static const vec2f emptyVec2     = vec2f(0.0f, 0.0f);
    inline static const vec3f emptyVec3     = vec3f(0.0f, 0.0f, 0.0f);
    inline static const vec3f unitVec3      = vec3f(1.0f, 1.0f, 1.0f);

    template <typename T>
    inline std::decay_t<T> step(T&& edge, T&& x) noexcept {
        return std::decay_t<T>(x >= edge);
    }

    template <typename T>
    inline std::decay_t<T> smoothstep(T&& edge0, T&& edge1, T&& x) noexcept {
        T t;
        t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

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

    inline bool is_pow2(const uint32_t n) noexcept {
        return (n > 0u) && (!(n & (n - 1u)));
    }

    inline uint32_t next_pow2(uint32_t v) noexcept {
        if (v == 0u) return 1u;
        --v;
        v |= v >> 1u;
        v |= v >> 2u;
        v |= v >> 4u;
        v |= v >> 8u;
        v |= v >> 16u;
        ++v;
        return v;
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
        memset(&matrix[3u], 0, sizeof(float) * 3u);
        return matrix;
    }
}