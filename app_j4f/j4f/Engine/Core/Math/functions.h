#pragma once
// ♣♠♦♥

#include "mathematic.h"

#include <limits>
#include <vector>

namespace engine {

    inline glm::vec3 projection(const glm::vec3& a, const glm::vec3& b) { // проекция вектора a на вектор b
        const glm::vec3& axis = as_normalized(b);
        return axis * glm::dot(axis, a);
    }

    inline void normalizePlain(glm::vec4& plain) { // нормализация плосксти
        const float invSqrt = inv_sqrt(plain.x * plain.x + plain.y * plain.y + plain.z * plain.z);
        plain *= invSqrt;
    }

    inline glm::vec4 plainForm(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) { // общее уравнение плоскости по 3м точкам
        const float a = p2.x - p1.x;
        const float b = p2.y - p1.y;
        const float c = p2.z - p1.z;

        const float d = p3.x - p1.x;
        const float e = p3.y - p1.y;
        const float f = p3.z - p1.z;

        const float pA = b * f - e * c;
        const float pB = d * c - a * f;
        const float pC = a * e - d * b;

        const float pD = -p1.x * pA - p1.y * pB - p1.z * pC;
        return { pA, pB, pC, pD };
    }

    inline glm::vec4 plainForm(const glm::vec3& normal, const glm::vec3& point) { // общее уравнение плоскости из нормали и точки
        return { normal.x, normal.y, normal.z, -glm::dot(normal, point) };
    }

    inline float lenFromPointToLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec3& p) { // расстояние от точки до линии
        const glm::vec3 direction = as_normalized(end - begin);
        const glm::vec3 directionToVec = p - begin;
        return vec_length(glm::cross(direction, directionToVec));
    }

    inline float lenFromPointToPlane(const glm::vec4& plane, const glm::vec3& p) { // расстояние от точки до плоскости
        return plane.x * p.x + plane.y * p.y + plane.z * p.z + plane.w;
    }

    inline float lenFromLineToLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec3& begin2, const glm::vec3& end2) { // расстояние от между линиями
        // http://www.cleverstudents.ru/line_and_plane/distance_between_skew_lines.html

        const glm::vec3 direction = as_normalized(end - begin); // направляющий вектор первой прямой
        const glm::vec3 direction2 = as_normalized(end2 - begin2); //направляющий вектор второй прямой

        const glm::vec3 cross = glm::cross(direction, direction2); // будет нормальный вектор плоскости, проходящей через прямую b параллельно прямой a

        const float D = -cross.x * begin2.x - cross.y * begin2.y - cross.z * begin2.z; // это чтобы проходил через b
        const float nMult = 1.0f / vec_length(cross); // нормирующий множитель

        const float len = glm::abs(cross.x * begin.x + cross.y * begin.y + cross.z * begin.z + D) * nMult; // чтобы параллельно a и длину сразу посчитаем

        return len;
    }

    inline glm::vec3 reflectPointWithPlain(const glm::vec4& plain, const glm::vec3& p, const bool plainIsNormalized) { // отражает точку относительно плоскости
        const glm::vec3 plainNormal(plain.x, plain.y, plain.z);
        const float len = lenFromPointToPlane(plain, p);

        const float normalizer = plainIsNormalized ? 2.0f : 2.0f / (plain.x * plain.x + plain.y * plain.y + plain.z * plain.z);
        return p - plainNormal * (normalizer * len);
    }

    inline glm::vec3 intersectionLinePlane(const glm::vec3& a, const glm::vec3& b, const glm::vec4& plain) { // точка пересеченя прямой и плоскости
        const glm::vec3 d = as_normalized(b - a);
        const float t = -(plain.x * a.x + plain.y * a.y + plain.z * a.z + plain.w) / (plain.x * d.x + plain.y * d.y + plain.z * d.z);
        return { a.x + d.x * t, a.y + d.y * t, a.z + d.z * t };
    }

    inline glm::vec3 crossingLinesPoint(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const float eps) { // точка "скрещивания" прямых (это точка на прямой ab в моменте скрещивания с прямой bc, такие штуки иногда полезны, например при определении различных попаданий)
        const glm::vec3 v_v = c + glm::cross(d - c, b - a);
        const glm::vec4 pl = plainForm(c, d, v_v);
        const glm::vec3 crossing = intersectionLinePlane(a, b, pl);
        return lenFromPointToLine(c, d, crossing) <= eps ? crossing : glm::vec3(std::numeric_limits<float>::max());
    }

    inline float crossDotMult(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) { // смешанное произведение векторов
        return glm::dot(glm::cross(a, b), c);
        // тройка правая если crossDotMult(a, b, c) > 0, если crossDotMult(a, b, c) < 0 - левая, если crossDotMult(a, b, c) == 0 - векторы компланарны (в одной плоскости)
    }

    inline float triangleSquare(const float a, const float b, const float c) { // формула Герона
        const float p = (a + b + c) * 0.5f;
        const float sqrtv = 1.0f / inv_sqrt(p * (p - a) * (p - b) * (p - c));
        return sqrtv;
    }

    inline bool pointIntoPolygon(const glm::vec3& p, const std::vector<glm::vec3>& points) { // точка на плоскости внутри многоугольника с вершинами points (вершины нужны в порядке следования ребер)
        // идея похожа на frustum - если точка лежит с одной и той же стороны относительно каждого ребра, то она внутри фигуры
        // для невыпуклых многогранников нужен именно контур, выпуклые сами являются своим контуром
        const size_t psz = points.size();
        if (psz < 3) return false;

        const glm::vec3 n1 = glm::cross(points[1] - points[0], p - points[1]);

        for (size_t i = 2; i < psz; ++i) {
            const glm::vec3 ni = glm::cross(points[i] - points[i - 1], p - points[i]);
            if (glm::dot(n1, ni) <= 0.0f) {
                return false;
            }
        }

        return true;
    }

    inline bool pointIntoTriangle(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) { // точка на плоскости внутри треугольника abc ?
        // идея похожа на frustum - если точка лежит с одной и той же стороны относительно каждого ребра, то она внутри фигуры
        const glm::vec3 n1 = glm::cross(b - a, p - b);
        const glm::vec3 n2 = glm::cross(c - b, p - c);

        if (glm::dot(n1, n2) > 0.0f) {
            const glm::vec3 n3 = glm::cross(a - c, p - a);
            if (glm::dot(n1, n3) > 0.0f) {
                return true;
            }

            return false;
        }

        return false;
    }

    inline bool hit(const glm::vec3& begin, const glm::vec3& end, const glm::mat4x4& wtr, glm::vec3& vmin, glm::vec3& vmax, float& len) { // попадание по заданному boundingBox (точное)
        // алгоритм через точки пересечения с плоскостями и их попадание в параллелограммы

        const glm::vec3 ldb(wtr * glm::vec4(vmin.x, vmin.y, vmin.z, 1.0f));
        const glm::vec3 ldt(wtr * glm::vec4(vmin.x, vmin.y, vmax.z, 1.0f));
        const glm::vec3 rdb(wtr * glm::vec4(vmax.x, vmin.y, vmin.z, 1.0f));
        const glm::vec3 rdt(wtr * glm::vec4(vmax.x, vmin.y, vmax.z, 1.0f));

        const glm::vec3 direction = as_normalized(end - begin);
        const glm::vec3 position = static_cast<glm::vec3>(wtr * 0.5f * glm::vec4(vmin.x + vmax.x, vmin.y + vmax.y, vmin.z + vmax.z, 2.0f)); // центр баундинг бокса

        // нижняя
        const glm::vec4 down = plainForm(ldb, ldt, rdt);
        const glm::vec3 intersect_d = intersectionLinePlane(begin, end, down);
        if (pointIntoTriangle(intersect_d, ldb, ldt, rdt) || pointIntoTriangle(intersect_d, ldb, rdb, rdt)) {
            const glm::vec3 directionToVec = intersect_d - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_d - position);
                const float lenToScreen = vec_length(begin - intersect_d);
                len += lenToScreen;
                return true;
            }
        }

        const glm::vec4 lub(wtr * glm::vec4(vmin.x, vmax.y, vmin.z, 1.0f));
        const glm::vec4 rub(wtr * glm::vec4(vmax.x, vmax.y, vmin.z, 1.0f));

        // задняя
        const glm::vec4 bottom = plainForm(lub, ldb, rub);
        const glm::vec3 intersect_b = intersectionLinePlane(begin, end, bottom);
        if (pointIntoTriangle(intersect_b, lub, ldb, rub) || pointIntoTriangle(intersect_b, rdb, ldb, rub)) {
            const glm::vec3 directionToVec = intersect_b - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_b - position);
                const float lenToScreen = vec_length(begin - intersect_b);
                len += lenToScreen;
                return true;
            }
        }

        const glm::vec3 lut(wtr * glm::vec4(vmin.x, vmax.y, vmax.z, 1.0f));

        // левая
        const glm::vec4 left = plainForm(ldb, ldt, lut);
        const glm::vec3 intersect_l = intersectionLinePlane(begin, end, left);
        if (pointIntoTriangle(intersect_l, ldb, ldt, lut) || pointIntoTriangle(intersect_l, ldb, lub, lut)) {
            const glm::vec3 directionToVec = intersect_l - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_l - position);
                const float lenToScreen = vec_length(begin - intersect_l);
                len += lenToScreen;
                return true;
            }
        }

        const glm::vec3 rut(wtr * glm::vec4(vmax.x, vmax.y, vmax.z, 1.0f));

        // верхняя
        const glm::vec4 up = plainForm(lut, lub, rut);
        const glm::vec3 intersect_u = intersectionLinePlane(begin, end, up);
        if (pointIntoTriangle(intersect_u, lut, lub, rut) || pointIntoTriangle(intersect_u, rub, lub, rut)) {
            const glm::vec3 directionToVec = intersect_u - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_u - position);
                const float lenToScreen = vec_length(begin - intersect_u);
                len += lenToScreen;
                return true;
            }
        }

        // передняя
        const glm::vec4 top = plainForm(ldt, lut, rut);
        const glm::vec3 intersect_t = intersectionLinePlane(begin, end, top);
        if (pointIntoTriangle(intersect_t, ldt, lut, rut) || pointIntoTriangle(intersect_t, ldt, rdt, rut)) {
            const glm::vec3 directionToVec = intersect_t - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_t - position);
                const float lenToScreen = vec_length(begin - intersect_t);
                len += lenToScreen;
                return true;
            }
        }

        // правая
        const glm::vec4 right = plainForm(rdb, rdt, rut);
        const glm::vec3 intersect_r = intersectionLinePlane(begin, end, right);
        if (pointIntoTriangle(intersect_r, rdb, rdt, rut) || pointIntoTriangle(intersect_r, rdb, rub, rut)) {
            const glm::vec3 directionToVec = intersect_r - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_r - position);
                const float lenToScreen = vec_length(begin - intersect_r);
                len += lenToScreen;
                return true;
            }
        }

        len = std::numeric_limits<float>::max();
        return false;
    }
}