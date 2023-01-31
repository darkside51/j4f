#pragma once

#include "Task.h"
#include "ThreadPool.h"
#include <vector>

namespace engine {

    template <typename T>
    class ParallelTask final {
        friend class ThreadPool;
    public:

        ParallelTask() = default;
        explicit ParallelTask(const TaskType type, const std::vector<TaskHandlerPtr<T>>& taskPtrs) {
            const size_t tasksCount = taskPtrs.size();
            _tasks.reserve(tasksCount);
            for (size_t i = 0; i < tasksCount; ++i) {
                _tasks.emplace_back(type, taskPtrs[i]);
            }
        }

        template<class F, class... Args>
        explicit ParallelTask(const TaskType type, const uint16_t count, F&& f, Args&&... args) {
            /*auto func = [this, f](const CancellationToken& token, const uint16_t c, Args&&... args) {
                f(token, c, args...);
            };*/

            _tasks.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                _tasks.emplace_back(std::make_shared<TaskHandler<T>>(type, f, i, std::forward<Args>(args)...));
            }
        }

        ~ParallelTask() = default;

        ParallelTask(const ParallelTask&) = delete;
        ParallelTask(ParallelTask&& t) noexcept : _tasks(std::move(t._tasks)) {
            t._tasks.clear();
        }

        ParallelTask& operator= (const ParallelTask&) = delete;
        ParallelTask& operator= (ParallelTask&& t) noexcept {
            _tasks = std::move(t._tasks);
            t._tasks.clear();
            return *this;
        }

        void enqueue(ThreadPool* pool, const uint16_t priority) {
            for (auto&& t : _tasks) {
                pool->enqueue(priority, t);
            }
        }

        [[nodiscard]] inline bool valid() const noexcept {
            for (auto&& t : _tasks) {
                if (t.valid()) return true;
            }

            return false;
        }

        inline void wait() const {
            for (auto&& t : _tasks) {
                t.wait();
            }
        }

        inline void cancel() {
            for (auto&& t : _tasks) { t.cancel(); }
        }

        inline void cancel(const uint16_t taskId) {
            if (taskId >= _tasks.size()) return;
            _tasks[taskId].cancel();
        }

        [[nodiscard]] inline TaskState state(const uint16_t taskId) const {
            if (taskId >= _tasks.size()) return TaskState::COMPLETE;
            return _tasks[taskId].state();
        }

        inline void setTaskName(const std::string& name) {
#ifdef _DEBUG
            uint16_t i = 0;
            const std::string name_change = name + std::string("_");
            for (auto&& t : _tasks) {
                t.setTaskName(name_change + std::to_string(i++));
            }
#endif
        }

    private:
        std::vector<TaskResult<T>> _tasks;
    };

    template<class F>
    static inline auto makeParallelTask(const TaskType type, const uint16_t count, F&& f)-> ParallelTask<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, const uint16_t>> {
        using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, const uint16_t>;
        return ParallelTask<return_type>(type, count, std::forward<F>(f));
    }

    template<class F, class... Args>
    static inline auto makeParallelTask(const TaskType type, const uint16_t count, F&& f, Args&&... args)-> ParallelTask<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, const uint16_t, std::decay_t<Args>...>> {
        using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, const uint16_t, std::decay_t<Args>...>;
        return ParallelTask<return_type>(type, count, std::forward<F>(f), std::forward<Args>(args)...);
    }
}
