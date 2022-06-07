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
		Looper() : _lock(false) { }
		~Looper() {
			AtomicLock l(_lock);
			_tasks.clear();
		}
		
		void nextFrame(const float delta) {
			if (_tasks.empty()) return;

			std::vector<LooperTask> tasks;
			{
				AtomicLock l(_lock);
				tasks = std::move(_tasks);
				_tasks.clear();
			}

			for (auto&& task : tasks) {
				task();
			}
		}

		inline void pushTask(LooperTask&& t) {
			AtomicLock l(_lock);
			_tasks.push_back(std::move(t));
		}

		inline void pushTask(const LooperTask& t) {
			AtomicLock l(_lock);
			_tasks.push_back(t);
		}

	private:
		std::atomic_bool _lock;
		std::vector<LooperTask> _tasks;
	};

}
