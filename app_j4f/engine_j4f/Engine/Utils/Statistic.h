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
        inline void update(const float /*delta*/) noexcept {
            _updateFrameCounter.fetch_add(1u, std::memory_order_relaxed);
        }

        inline void render(const float /*delta*/) noexcept {
            _renderFrameCounter.fetch_add(1u, std::memory_order_relaxed);
        }

		inline void frame(const float delta) {
            _timeCounter += delta;

			if (_timeCounter >= 1.0f) {
                _renderFps = _renderFrameCounter.exchange(0u, std::memory_order_relaxed);
                _updateFps = _updateFrameCounter.exchange(0u, std::memory_order_relaxed);
				_drawCalls = std::roundf(static_cast<float>(_drawCallsCounter.exchange(0, std::memory_order_relaxed)) / _renderFps);
				_cpuFrameTime = _cpuTimeCounter / _renderFps;
				updateValues();
                _timeCounter = 0;
                _cpuTimeCounter = 0.0f;
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

		inline uint16_t renderFps() const noexcept { return _renderFps; }
        inline uint16_t updateFps() const noexcept { return _updateFps; }
		inline uint16_t drawCalls() const noexcept { return _drawCalls; }
		inline float cpuFrameTime() const noexcept { return _cpuFrameTime; }

		inline void addDrawCall() noexcept {
            _drawCallsCounter.fetch_add(1, std::memory_order_relaxed);
		}

		inline void addFramePrepareTime(const float t) noexcept {
            _cpuTimeCounter += t;
		}

	private:
		uint16_t _renderFps = 0;
        uint16_t _updateFps = 0;
		uint16_t _drawCalls = 0;
		float _cpuFrameTime = 0.0f;

		std::atomic<uint32_t> _drawCallsCounter = 0;
		float _timeCounter = 0.0f;
		float _cpuTimeCounter = 0.0f;

        std::atomic<uint16_t> _renderFrameCounter = 0;
        std::atomic<uint16_t> _updateFrameCounter = 0;

		std::vector<IStatisticObserver*> _observers;
	};
}