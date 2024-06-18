#pragma once
#include "../Core/EngineModule.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <atomic>
#include <algorithm>

#ifdef ENABLE_STATISTIC
#define STATISTIC_ADD_DRAW_CALL engine::Engine::getInstance().getModule<engine::Statistic>().addDrawCall();
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

	class Statistic final : public IEngineModule {
	public:
        explicit Statistic(const float time = 1.0f) : _calculationTime(time) {}

        inline void update(const float /*delta*/) noexcept {
            _updateFrameCounter.fetch_add(1u, std::memory_order_relaxed);
        }

        inline void render(const float /*delta*/) noexcept {
            _renderFrameCounter.fetch_add(1u, std::memory_order_relaxed);
        }

		inline void frame(const float delta) {
            _timeCounter += delta;

			if (_timeCounter >= _calculationTime) {
                const float framesCount = static_cast<float>(_renderFrameCounter.exchange(0u, std::memory_order_relaxed));
                _renderFps = static_cast<uint16_t>(framesCount / _calculationTime);
                _updateFps = static_cast<uint16_t>(static_cast<float>(_updateFrameCounter.exchange(0u, std::memory_order_relaxed)) / _calculationTime);
				_drawCalls = static_cast<uint16_t>(std::roundf(static_cast<float>(_drawCallsCounter.exchange(0u, std::memory_order_relaxed)) / framesCount));
				_cpuFrameTime = _cpuTimeCounter / _renderFps;
				updateValues();
                _timeCounter = 0.0f;
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

		[[nodiscard]] inline uint16_t renderFps() const noexcept { return _renderFps; }
        [[nodiscard]] inline uint16_t updateFps() const noexcept { return _updateFps; }
		[[nodiscard]] inline uint16_t drawCalls() const noexcept { return _drawCalls; }
		[[nodiscard]] inline float cpuFrameTime() const noexcept { return _cpuFrameTime; }

		inline void addDrawCall() noexcept {
            _drawCallsCounter.fetch_add(1u, std::memory_order_relaxed);
		}

		inline void addFramePrepareTime(const float t) noexcept {
            _cpuTimeCounter += t;
		}

	private:
        float _calculationTime = 1.0f;
		uint16_t _renderFps = 0u;
        uint16_t _updateFps = 0u;
		uint16_t _drawCalls = 0u;
		float _cpuFrameTime = 0.0f;

		std::atomic<uint32_t> _drawCallsCounter = 0u;
		float _timeCounter = 0.0f;
		float _cpuTimeCounter = 0.0f;

        std::atomic<uint16_t> _renderFrameCounter = 0u;
        std::atomic<uint16_t> _updateFrameCounter = 0u;

		std::vector<IStatisticObserver*> _observers;
	};
}