#pragma once

#include "mathematic.h"
#include <vector>
#include <queue>
#include <utility>
#include <cstdint>

namespace engine {
    class EuclidHeuristic {
    public:
        inline static float execute(const std::vector<vec3f>& vertexes, const size_t n, const vec3f& finishV) {
            const vec3f sub = vertexes[n] - finishV;
            return vec_length(sub); // вычисение квадратного корня через "квадратный корень Кармака"
        }
    };

    // алгоритм A* поиска кратчайшего пути https://habr.com/ru/post/331220/
    // http://e-maxx.ru/algo/dijkstra_sparse (описание кучи для алгоритма Дейкстры, тут используется она же)
    template<typename HEURISTIC>
    std::vector<size_t> a_star(
        std::vector<std::vector<std::pair<size_t, float>>>& graph, // граф как набор смежности
        const std::vector<vec3f>& vertexes, // фактические вершины, для евристической оценки
        const size_t start, // номер стартовой вершины
        const size_t finish, // номер финишной вершины
        const vec3f& startV,
        const vec3f& finishV
    ) {

        const size_t n = graph.size();

        std::vector<float> d(n, FLT_MAX);
        std::vector<size_t> p(n);

        d[start] = 0.0f;
        std::priority_queue <std::pair<float, int>> q;
        q.emplace(0, start);

        size_t st = 0;
        while (!q.empty()) {
            const auto v = static_cast<size_t>(q.top().second);
            q.pop();

            if (v == finish) { break; } // ранний выход

            for (auto& pair : graph[v]) {
                const size_t num = pair.first;
                const float len = pair.second;
                const float cost = d[v] + len;
                if (cost < d[num]) {
                    d[num] = cost;
                    p[num] = v;

                    const float priority = cost + (num == start ? vec_length(startV - finishV) : (num == finish ? 0.0f : HEURISTIC::execute(vertexes, num, finishV))); //HEURISTIC::execute(vertexes, num, finishV);
                    q.emplace(-priority, num);
                }
            }
            ++st;
        }

        // обратный проход
        std::vector<size_t> ret;

        size_t f = finish;
        while (f != start) {
            ret.push_back(f);
            f = p[f];
        }

        ret.push_back(start);
        ret.shrink_to_fit();

        return ret;
    }
}