#pragma once
// ♣♠♦♥

#include "mathematic.h"

#include <limits>
#include <vector>

namespace engine {

    inline vec3f projection(const vec3f& a, const vec3f& b) { // проекция вектора a на вектор b
        const vec3f& axis = as_normalized(b);
        return axis * glm::dot(axis, a);
    }

    inline void normalizePlane(vec4f& plane) { // нормализация плосксти
        const float invSqrt = inv_sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        plane *= invSqrt;
    }

    inline vec4f planeForm(const vec3f& p1, const vec3f& p2, const vec3f& p3) { // общее уравнение плоскости по 3м точкам
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

    inline vec4f planeForm(const vec3f& normal, const vec3f& point) { // общее уравнение плоскости из нормали и точки
        return { normal.x, normal.y, normal.z, -glm::dot(normal, point) };
    }

    inline float lenFromPointToLine(const vec3f& begin, const vec3f& end, const vec3f& p) { // расстояние от точки до линии
        const vec3f direction = as_normalized(end - begin);
        const vec3f directionToVec = p - begin;
        return vec_length(glm::cross(direction, directionToVec));
    }

    inline float lenFromPointToPlane(const vec4f& plane, const vec3f& p) { // расстояние от точки до плоскости
        return plane.x * p.x + plane.y * p.y + plane.z * p.z + plane.w;
    }

    inline float lenFromLineToLine(const vec3f& begin, const vec3f& end, const vec3f& begin2, const vec3f& end2) { // расстояние от между линиями
        // http://www.cleverstudents.ru/line_and_plane/distance_between_skew_lines.html

        const vec3f direction = as_normalized(end - begin); // направляющий вектор первой прямой
        const vec3f direction2 = as_normalized(end2 - begin2); //направляющий вектор второй прямой

        const vec3f cross = glm::cross(direction, direction2); // будет нормальный вектор плоскости, проходящей через прямую b параллельно прямой a

        const float D = -cross.x * begin2.x - cross.y * begin2.y - cross.z * begin2.z; // это чтобы проходил через b
        const float nMult = 1.0f / vec_length(cross); // нормирующий множитель

        const float len = glm::abs(cross.x * begin.x + cross.y * begin.y + cross.z * begin.z + D) * nMult; // чтобы параллельно a и длину сразу посчитаем

        return len;
    }

    inline vec3f reflectPointWithPlane(const vec4f& plane, const vec3f& p, const bool planeIsNormalized) { // отражает точку относительно плоскости
        const vec3f planeNormal(plane.x, plane.y, plane.z);
        const float len = lenFromPointToPlane(plane, p);

        const float normalizer = planeIsNormalized ? 2.0f : 2.0f / (plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
        return p - planeNormal * (normalizer * len);
    }

    inline vec3f intersectionLinePlane(const vec3f& a, const vec3f& b, const vec4f& plane) { // точка пересеченя прямой и плоскости
        const vec3f d = as_normalized(b - a);
        const float t = -(plane.x * a.x + plane.y * a.y + plane.z * a.z + plane.w) / (plane.x * d.x + plane.y * d.y + plane.z * d.z);
        return { a.x + d.x * t, a.y + d.y * t, a.z + d.z * t };
    }

    inline vec3f crossingLinesPoint(const vec3f& a, const vec3f& b, const vec3f& c, const vec3f& d, const float eps) { // точка "скрещивания" прямых (это точка на прямой ab в моменте скрещивания с прямой bc, такие штуки иногда полезны, например при определении различных попаданий)
        const vec3f v_v = c + glm::cross(d - c, b - a);
        const vec4f pl = planeForm(c, d, v_v);
        const vec3f crossing = intersectionLinePlane(a, b, pl);
        return lenFromPointToLine(c, d, crossing) <= eps ? crossing : vec3f(std::numeric_limits<float>::max());
    }

    inline float crossDotMult(const vec3f& a, const vec3f& b, const vec3f& c) { // смешанное произведение векторов
        return glm::dot(glm::cross(a, b), c);
        // тройка правая если crossDotMult(a, b, c) > 0, если crossDotMult(a, b, c) < 0 - левая, если crossDotMult(a, b, c) == 0 - векторы компланарны (в одной плоскости)
    }

    inline float triangleSquare(const float a, const float b, const float c) { // формула Герона
        const float p = (a + b + c) * 0.5f;
        const float sqrtv = 1.0f / inv_sqrt(p * (p - a) * (p - b) * (p - c));
        return sqrtv;
    }

    inline bool pointIntoPolygon(const vec3f& p, const std::vector<vec3f>& points) { // точка на плоскости внутри многоугольника с вершинами points (вершины нужны в порядке следования ребер)
        // идея похожа на frustum - если точка лежит с одной и той же стороны относительно каждого ребра, то она внутри фигуры
        // для невыпуклых многогранников нужен именно контур, выпуклые сами являются своим контуром
        const size_t psz = points.size();
        if (psz < 3) return false;

        const vec3f n1 = glm::cross(points[1] - points[0], p - points[1]);

        for (size_t i = 2; i < psz; ++i) {
            const vec3f ni = glm::cross(points[i] - points[i - 1], p - points[i]);
            if (glm::dot(n1, ni) <= 0.0f) {
                return false;
            }
        }

        return true;
    }

    inline bool pointIntoTriangle(const vec3f& p, const vec3f& a, const vec3f& b, const vec3f& c) { // точка на плоскости внутри треугольника abc ?
        // идея похожа на frustum - если точка лежит с одной и той же стороны относительно каждого ребра, то она внутри фигуры
        const vec3f n1 = glm::cross(b - a, p - b);
        const vec3f n2 = glm::cross(c - b, p - c);

        if (glm::dot(n1, n2) > 0.0f) {
            const vec3f n3 = glm::cross(a - c, p - a);
            if (glm::dot(n1, n3) > 0.0f) {
                return true;
            }

            return false;
        }

        return false;
    }

    inline bool hit(const vec3f& begin, const vec3f& end, const mat4f& wtr, vec3f& vmin, vec3f& vmax, float& len) { // попадание по заданному boundingBox (точное)
        // алгоритм через точки пересечения с плоскостями и их попадание в параллелограммы

        const vec3f ldb(wtr * vec4f(vmin.x, vmin.y, vmin.z, 1.0f));
        const vec3f ldt(wtr * vec4f(vmin.x, vmin.y, vmax.z, 1.0f));
        const vec3f rdb(wtr * vec4f(vmax.x, vmin.y, vmin.z, 1.0f));
        const vec3f rdt(wtr * vec4f(vmax.x, vmin.y, vmax.z, 1.0f));

        const vec3f direction = as_normalized(end - begin);
        const vec3f position = static_cast<vec3f>(wtr * 0.5f * vec4f(vmin.x + vmax.x, vmin.y + vmax.y, vmin.z + vmax.z, 2.0f)); // центр баундинг бокса

        // нижняя
        const vec4f down = planeForm(ldb, ldt, rdt);
        const vec3f intersect_d = intersectionLinePlane(begin, end, down);
        if (pointIntoTriangle(intersect_d, ldb, ldt, rdt) || pointIntoTriangle(intersect_d, ldb, rdb, rdt)) {
            const vec3f directionToVec = intersect_d - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_d - position);
                const float lenToScreen = vec_length(begin - intersect_d);
                len += lenToScreen;
                return true;
            }
        }

        const vec4f lub(wtr * vec4f(vmin.x, vmax.y, vmin.z, 1.0f));
        const vec4f rub(wtr * vec4f(vmax.x, vmax.y, vmin.z, 1.0f));

        // задняя
        const vec4f bottom = planeForm(lub, ldb, rub);
        const vec3f intersect_b = intersectionLinePlane(begin, end, bottom);
        if (pointIntoTriangle(intersect_b, lub, ldb, rub) || pointIntoTriangle(intersect_b, rdb, ldb, rub)) {
            const vec3f directionToVec = intersect_b - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_b - position);
                const float lenToScreen = vec_length(begin - intersect_b);
                len += lenToScreen;
                return true;
            }
        }

        const vec3f lut(wtr * vec4f(vmin.x, vmax.y, vmax.z, 1.0f));

        // левая
        const vec4f left = planeForm(ldb, ldt, lut);
        const vec3f intersect_l = intersectionLinePlane(begin, end, left);
        if (pointIntoTriangle(intersect_l, ldb, ldt, lut) || pointIntoTriangle(intersect_l, ldb, lub, lut)) {
            const vec3f directionToVec = intersect_l - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_l - position);
                const float lenToScreen = vec_length(begin - intersect_l);
                len += lenToScreen;
                return true;
            }
        }

        const vec3f rut(wtr * vec4f(vmax.x, vmax.y, vmax.z, 1.0f));

        // верхняя
        const vec4f up = planeForm(lut, lub, rut);
        const vec3f intersect_u = intersectionLinePlane(begin, end, up);
        if (pointIntoTriangle(intersect_u, lut, lub, rut) || pointIntoTriangle(intersect_u, rub, lub, rut)) {
            const vec3f directionToVec = intersect_u - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_u - position);
                const float lenToScreen = vec_length(begin - intersect_u);
                len += lenToScreen;
                return true;
            }
        }

        // передняя
        const vec4f top = planeForm(ldt, lut, rut);
        const vec3f intersect_t = intersectionLinePlane(begin, end, top);
        if (pointIntoTriangle(intersect_t, ldt, lut, rut) || pointIntoTriangle(intersect_t, ldt, rdt, rut)) {
            const vec3f directionToVec = intersect_t - begin;
            const float cosa = glm::dot(direction, as_normalized(directionToVec));
            if (cosa >= 0.0f) {
                len = vec_length(intersect_t - position);
                const float lenToScreen = vec_length(begin - intersect_t);
                len += lenToScreen;
                return true;
            }
        }

        // правая
        const vec4f right = planeForm(rdb, rdt, rut);
        const vec3f intersect_r = intersectionLinePlane(begin, end, right);
        if (pointIntoTriangle(intersect_r, rdb, rdt, rut) || pointIntoTriangle(intersect_r, rdb, rub, rut)) {
            const vec3f directionToVec = intersect_r - begin;
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