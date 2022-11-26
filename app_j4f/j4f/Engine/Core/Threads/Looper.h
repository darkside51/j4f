#pragma once

#include "../EngineModule.h"
#include "Synchronisations.h"

#include <cstdint>
#include <vector>
#include <functional>

namespace engine {

	using LooperTask = std::function<void()>;

	class Looper : public IEngineModule {
	public:
		Looper() { }
		~Looper() {
			AtomicLockF l(_lock);
			_tasks.clear();
		}
		
		void nextFrame(const float delta) {
			if (_tasks.empty()) return;

			std::vector<LooperTask> tasks;
			{
				AtomicLockF l(_lock);
				tasks = std::move(_tasks);
				_tasks.clear();
			}

			for (auto&& task : tasks) {
				task();
			}
		}

		inline void pushTask(LooperTask&& t) {
			AtomicLockF l(_lock);
			_tasks.push_back(std::move(t));
		}

		inline void pushTask(const LooperTask& t) {
			AtomicLockF l(_lock);
			_tasks.push_back(t);
		}

	private:
		std::atomic_flag _lock{};
		std::vector<LooperTask> _tasks;
	};

}
