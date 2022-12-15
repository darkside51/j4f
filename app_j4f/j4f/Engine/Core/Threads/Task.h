#pragma once

#include <atomic>
#include <future>
#include <cstdint>
#include <string>
#include <assert.h>

namespace engine {

    class ThreadPool;
    class ITaskHandler;

    template <typename T>
    class TaskHandler;

    template <typename T>
    class TaskResult;

    using ITaskHandlerPtr = std::shared_ptr<ITaskHandler>;

    template <typename T>
    using TaskHandlerPtr = std::shared_ptr<TaskHandler<T>>;

    enum class TaskState : uint8_t {
        IDLE = 0,
        RUN = 1,
        COMPLETE = 2,
        CANCELED = 3
    };

    enum class TaskType : uint8_t { // max value is 7 (using with 1 << TaskType)
        COMMON = 0,
        USER_CONTROL = 1, // cancel будет вызван только на стороне использующей стороны, сам ThreadPool сможет отменить такую задачу только при остановке в stop()
        _MAX_VALUE = 8
    };

    class CancellationToken final {
        friend class ThreadPool;
        friend class ITaskHandler;
    public:
        inline operator bool() const noexcept {
            return _cancellation_token.test(std::memory_order_consume);
        }
    private:
        inline void cancel() noexcept {
            _cancellation_token.test_and_set(std::memory_order_release);
        }

        inline void reset() noexcept {
            _cancellation_token.clear();
        }

        std::atomic_flag _cancellation_token{};
    };

    class ITaskHandler {
        friend class ThreadPool;
        virtual void fire() = 0;
        virtual void invalidate() = 0;

        inline void reset() {
            _state.store(TaskState::IDLE, std::memory_order_release);
            _cancellation_token.reset();
        }

    public:
        virtual ~ITaskHandler() = default;
        ITaskHandler() : _type(TaskType::COMMON), _state(TaskState::IDLE) { }
        ITaskHandler(const TaskType type) : _type(type), _state(TaskState::IDLE) { }
        
        inline TaskState state() const { return _state.load(std::memory_order_consume); }

        inline void operator()() {
            TaskState state = TaskState::IDLE;
            if (_state.compare_exchange_strong(state, TaskState::RUN, std::memory_order_release, std::memory_order_relaxed)) {
                fire();
                state = TaskState::RUN;
                if (_state.compare_exchange_strong(state, TaskState::COMPLETE, std::memory_order_release, std::memory_order_relaxed)) { // перевод состояния в COMPLETE
                } else {
                    // task has been cancelled
                }
            }
        }

        inline void cancel() {
            _cancellation_token.cancel();
            TaskState idle = TaskState::IDLE;
            if (!_state.compare_exchange_strong(idle, TaskState::CANCELED, std::memory_order_release, std::memory_order_relaxed)) {
                while (_state.load(std::memory_order_relaxed) == TaskState::RUN) { std::this_thread::yield(); }
            } else { // задача еще не начала свое выполнение, но, в теории, кто - то мог дернуть get() или wait()
                invalidate();
            }
        }

        virtual bool readyToRun() { return true; }

    protected:
        TaskType _type;
        std::atomic<TaskState> _state;
        CancellationToken _cancellation_token;

#ifdef _DEBUG
        std::string _taskName;
#endif
    };

    template <typename Type>
    class TaskHandler : public ITaskHandler {
        friend class ThreadPool;
        friend class TaskResult<Type>;

        inline void direct_execute() { // метод, кажется и не нужен, но пусть пока полежит, вроде не мешается
            TaskState state = TaskState::IDLE;
            if (_state.compare_exchange_strong(state, TaskState::RUN, std::memory_order_release, std::memory_order_relaxed)) {
                _task();
                state = TaskState::RUN;
                if (_state.compare_exchange_strong(state, TaskState::COMPLETE, std::memory_order_release, std::memory_order_relaxed)) { // перевод состояния в COMPLETE
                } else {
                    // task has been cancelled
                }
            }
        }

        inline void fire() override { _task(); }
        inline void invalidate() override { _task(); }

    public:
        // https://www.modernescpp.com/index.php/the-special-futures
        // std::future, созданные из std::packaged_task или std::promise не блокируют поток в своем деструкторе, если задача не была выполнена, но
        // std::future, созданные из std::async - блокируют до выполнения задачи

        virtual ~TaskHandler() { cancel(); }

        TaskHandler() = default;
        explicit TaskHandler(const TaskType type, std::packaged_task<Type()>&& ptr) : ITaskHandler(type), _task(std::move(ptr)) { }

        template<class F, class... Args>
        explicit TaskHandler(const TaskType type, F&& f, Args&&... args) : ITaskHandler(type) {
            _task = std::packaged_task<Type()>([this, f, args...]() { 
                if constexpr (std::is_same_v<Type, void>) {
                    if (!_cancellation_token) { // пропускаем вызов реальной работы, для отмены задач, если есть их ожидание где - то (чтобы invalidate отработал)
                        f(_cancellation_token, args...);
                        return;
                    } else {
                        return;
                    }
                } else {
                    if (!_cancellation_token) { // пропускаем вызов реальной работы, для отмены задач, если есть их ожидание где - то (чтобы invalidate отработал)
                        return f(_cancellation_token, args...);
                    } else {
                        return Type(0);
                    }
                }
            });
        }

        inline void setTaskName(const std::string& name) {
#ifdef _DEBUG
            _taskName = name;
#endif
        }

    private:
        std::packaged_task<Type()> _task;
    };

    template <typename T>
    class TaskResult final {
        friend class ThreadPool;
    public:
        TaskResult() = default;
        TaskResult(const TaskResult&) = delete;
        TaskResult(TaskResult&& t) noexcept : _handler(std::move(t._handler)), _future(std::move(t._future)) { }

        TaskResult& operator= (const TaskResult&) = delete;
        TaskResult& operator= (TaskResult&& t) noexcept {
            _handler = std::move(t._handler);
            _future = std::move(t._future);
            return *this;
        }

        explicit TaskResult(TaskHandlerPtr<T>&& ptr) : _handler(std::move(ptr)), _future(_handler->_task.get_future()) { }
        explicit TaskResult(const TaskHandlerPtr<T>& ptr) : _handler(ptr), _future(_handler->_task.get_future()) { }
        ~TaskResult() { _handler.reset(); }

        inline operator bool() const { return _handler != nullptr; }

        inline bool valid() const noexcept {
            return (_handler != nullptr) && _future.valid();
        }

        inline void wait() const {
            assert(_handler && "_handle is null");
            return _future.wait();
        }

        inline T get() {
            assert(_handler && "_handle is null");
            return _future.get();
        }

        template <class Rep, class Period>
        inline std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const {
            assert(_handler && "_handle is null");
            return _future.wait_for(timeout_duration);
        }

        template <class Rep, class Period>
        inline std::future_status wait_until(const std::chrono::duration<Rep, Period>& timeout_duration) const {
            assert(_handler && "_handle is null");
            return _future.wait_until(timeout_duration);
        }

        inline void cancel() {
            if (_handler) { _handler->cancel(); }
        }

        inline TaskState state() const {
            assert(_handler && "_handle is null");
            return _handler->state();
        }

        inline void setTaskName(const std::string& name) {
            assert(_handler && "_handle is null");
            _handler->setTaskName(name);
        }

        inline const TaskHandlerPtr<T>& getHandler() const { return _handler; }
        inline TaskHandlerPtr<T>& getHandler() { return _handler; }

        void reset() {
            cancel();
            invalidate();
        }

    private:
        inline void invalidate() { _handler.reset(); _future = {}; }

        TaskHandlerPtr<T> _handler;
        std::future<T> _future;
    };

    /*template<class F>
    inline auto makeRawTaskResult(const TaskType type, F&& f)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>>> {
        using return_type = typename std::invoke_result_t<std::decay_t<F>>;
        return TaskResult<return_type>(std::make_shared<TaskHandler<return_type>>(type, std::packaged_task<return_type()>(std::bind(std::forward<F>(f)))));
    }

    template<class F, class... Args>
    inline auto makeRawTaskResult(const TaskType type, F&& f, Args&&... args)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
        using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
        return TaskResult<return_type>(std::make_shared<TaskHandler<return_type>>(type, std::packaged_task<return_type()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...))));
    }*/

    template<class F>
    inline auto makeTaskResult(const TaskType type, F&& f)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>>> {
        using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>>;
        return TaskResult<return_type>(std::make_shared<TaskHandler<return_type>>(type, std::forward<F>(f)));
    }

    template<class F, class... Args>
    inline auto makeTaskResult(const TaskType type, F&& f, Args&&... args)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>> {
        using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>;
        return TaskResult<return_type>(std::make_shared<TaskHandler<return_type>>(type, std::forward<F>(f), std::forward<Args>(args)...));
    }

}