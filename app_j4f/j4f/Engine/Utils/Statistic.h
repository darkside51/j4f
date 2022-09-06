#pragma once
#include "../Core/EngineModule.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <atomic>

#ifdef ENABLE_STATISTIC
#define STATISTIC_ADD_DRAW_CALL engine::Engine::getInstance().getModule<engine::Statistic>()->addDrawCall();
#else
#define STATISTIC_ADD_DRAW_CALL
#endif


namespace engine {
	class Statistic;

	class IStatisticObserver {
	public:
		virtual ~IStatisticObserver() = default;
		virtual void statisticUpdate(const Statistic* s) = 0;
	};

	class Statistic : public IEngineModule {
	public:
		inline void nextFrame(const float delta) {
			_tmpTime += delta;
			++_tmpFrameCount;

			if (_tmpTime >= 1.0f) {
				_fps = _tmpFrameCount;
				_drawCalls = std::roundf(float(_tmpDrawCalls.exchange(0, std::memory_order_release)) / _fps);
				_cpuFrameTime = _tmpCpuTime / _fps;
				updateValues();
				_tmpFrameCount = 0;
				_tmpTime = 0;
				_tmpCpuTime = 0.0f;
			}
		}

		inline void updateValues() const {
			for (IStatisticObserver* o : _observers) {
				o->statisticUpdate(this);
			}
		}

		inline void addObserver(IStatisticObserver* o) {
			_observers.push_back(o);
		}

		inline void removeObserver(IStatisticObserver* o) {
			_observers.erase(std::remove(_observers.begin(), _observers.end(), o), _observers.end());
		}

		inline uint16_t fps() const { return _fps; }
		inline uint16_t drawCalls() const { return _drawCalls; }
		inline float cpuFrameTime() const { return _cpuFrameTime; }

		inline void addDrawCall() { 
			_tmpDrawCalls.fetch_add(1, std::memory_order_release);
		}

		inline void addFramePrepareTime(const float t) {
			_tmpCpuTime += t;
		}

	private:
		uint16_t _fps = 0;
		uint16_t _drawCalls = 0;
		float _cpuFrameTime = 0.0f;

		uint16_t _tmpFrameCount = 0;
		std::atomic<uint32_t> _tmpDrawCalls = 0;
		float _tmpTime = 0.0f;
		float _tmpCpuTime = 0.0f;
		std::vector<IStatisticObserver*> _observers;
	};
}