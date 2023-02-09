#pragma once

#include "Task.h"
#include "../Common.h"

#include <atomic>
#include <cstdint>

namespace engine {

	class FlowTaskFamily {
	public:
		virtual ~FlowTaskFamily() = default;
		std::atomic_uint8_t _dependency = 0;
	};

	template <typename T>
	class FlowTask : public TaskHandler<T>, public FlowTaskFamily {
		friend class ThreadPool;
		friend class TaskResult<T>;
	public:
		template<class F, class... Args>
		explicit FlowTask(const TaskType type, F&& f, Args&&... args) : TaskHandler<T>(type, createTak<F, Args...>(std::forward<F>(f)), std::forward<Args>(args)...) {}

		bool readyToRun() override { 
			return (_dependency.fetch_sub(1, std::memory_order_release) == 1);
		}

		void addDependTask(ITaskHandlerPtr& handler) {
			if (auto* flowTask = dynamic_cast<FlowTaskFamily*>(handler.get())) {
				flowTask->_dependency.fetch_add(1, std::memory_order_release);
			}

			const auto currentState = ITaskHandler::state();
			switch (currentState) {
				case TaskState::CANCELED:
					return;
				case TaskState::RUN:
				case TaskState::IDLE:
					_dependTasks.emplace_back(handler);
					break;
				case TaskState::COMPLETE:
				default:
				{
					if (handler->readyToRun()) {
						Engine::getInstance().getModule<ThreadPool>()->enqueue(0, handler);
					}
				}
					break;
			}
		}

	private:
		/*inline void invalidate() override {
			for (auto&& h : _dependTasks) {
				h->cancel();
			}
			TaskHandler<T>::invalidate();
		}*/

		inline void callDepend() {
			for (auto&& h : _dependTasks) {
				if (h->readyToRun()) {
					Engine::getInstance().getModule<ThreadPool>()->enqueue(0, h);
				}
			}
			_dependTasks.clear();
		}

		template<class F, class... Args>
		inline auto createTak(F&& f) {
			return [this, f](const CancellationToken& token, Args... args)-> typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...> {
				using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>;
				if constexpr (std::is_same_v<return_type, void>) {
					f(token, std::forward<Args>(args)...);
					if (!token) {
						callDepend();
					}
				} else {
					auto&& result = f(token, std::forward<Args>(args)...);
					if (!token) {
						callDepend();
					}
					return result;
				}
			};
		}

		std::vector<ITaskHandlerPtr> _dependTasks;
	};


	template<class F>
	static inline auto makeFlowTaskResult(const TaskType type, F&& f)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>>> {
		using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>>;
		return TaskResult<return_type>(std::make_shared<FlowTask<return_type>>(type, std::forward<F>(f)));
	}

	template<class F, class... Args>
	static inline auto makeFlowTaskResult(const TaskType type, F&& f, Args&&... args)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>> {
		using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, std::decay_t<Args>...>;
		return TaskResult<return_type>(std::make_shared<FlowTask<return_type>>(type, std::forward<F>(f), std::forward<Args>(args)...));
	}

	template <typename T, typename T1>
	inline void addTaskToFlow(TaskResult<T>& flow, TaskResult<T1>& task) {
		engine::ITaskHandlerPtr h = task.getHandler();
		static_cast<engine::FlowTask<T>*>(flow.getHandler().get())->addDependTask(h);
	}
}