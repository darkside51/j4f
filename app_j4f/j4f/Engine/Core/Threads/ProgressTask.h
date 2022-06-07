#pragma once

#include "Task.h"

namespace engine {
	class ThreadPool;

	class CommonProgressionTask {
	public:
		virtual ~CommonProgressionTask() = default;

		inline void increaseProgress() { _step.fetch_add(1, std::memory_order_release); }

		inline void setStep(const uint16_t step) { _step.store(step, std::memory_order_release); }
		inline void setTotalSteps(const uint16_t steps) { _totalSteps.store(steps, std::memory_order_release); }

		inline uint16_t getCurrentStep() const { return _step.load(std::memory_order_acquire); }
		inline uint16_t getTotalSteps() const { return _totalSteps.load(std::memory_order_acquire); }

		float getProgress() const {
			return static_cast<float>(getCurrentStep()) / getTotalSteps();
		}

	protected:
		std::atomic_uint16_t _totalSteps;
		std::atomic_uint16_t _step;
	};

	template <typename T>
	class ProgressionTask : public TaskHandler<T> {
		friend class ThreadPool;
	public:

		template<class F, class... Args>
		explicit ProgressionTask(const TaskType type, const uint16_t steps, F&& f, Args&&... args) : TaskHandler<T>(type, std::forward<F>(f), &_progress, std::forward<Args>(args)...) {
			_progress.setTotalSteps(steps);
		}

		~ProgressionTask() = default;

		ProgressionTask(const ProgressionTask&) = delete;
		ProgressionTask(ProgressionTask&& t) noexcept : TaskHandler<T>(t) {
			_progress.setStep(t._progress.getCurrentStep());
			_progress.setTotalSteps(t._progress.getTotalSteps());
		}

		ProgressionTask& operator= (const ProgressionTask&) = delete;
		ProgressionTask& operator= (ProgressionTask&& t) noexcept {
			TaskHandler<T>::operator=(t);
			_progress.setStep(t._progress.getCurrentStep());
			_progress.setTotalSteps(t._progress.getTotalSteps());
			return *this;
		}

		const CommonProgressionTask& getProgress() const { return _progress; }
		CommonProgressionTask& getProgress() { return _progress; }

	private:
		CommonProgressionTask _progress;
	};

	template<class F>
	static inline auto makeProgressionTaskResult(const TaskType type, const uint16_t steps, F&& f)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, CommonProgressionTask*>> {
		using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, CommonProgressionTask*>;
		return TaskResult<return_type>(std::make_shared<ProgressionTask<return_type>>(type, steps, std::forward<F>(f)));
	}

	template<class F, class... Args>
	static inline auto makeProgressionTaskResult(const TaskType type, const uint16_t steps, F&& f, Args&&... args)-> TaskResult<typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, CommonProgressionTask*, std::decay_t<Args>...>> {
		using return_type = typename std::invoke_result_t<std::decay_t<F>, std::decay_t<const CancellationToken&>, CommonProgressionTask*, std::decay_t<Args>...>;
		return TaskResult<return_type>(std::make_shared<ProgressionTask<return_type>>(type, steps, std::forward<F>(f), std::forward<Args>(args)...));
	}

	// helper methods
	template<class T>
	static inline const CommonProgressionTask* getTaskProgressionConst(const TaskResult<T>& task) {
		if (const ProgressionTask<T>* progression = dynamic_cast<const ProgressionTask<T>*>(task.getHandler().get())) {
			return &(progression->getProgress());
		}
		return nullptr;
	}

	template<class T>
	static inline CommonProgressionTask* getTaskProgression(const TaskResult<T>& task) {
		if (ProgressionTask<T>* progression = const_cast<ProgressionTask<T>*>(dynamic_cast<const ProgressionTask<void>*>(task.getHandler().get()))) {
			return &(progression->getProgress());
		}
		return nullptr;
	}
}