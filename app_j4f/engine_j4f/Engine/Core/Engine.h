#pragma once

#include "Common.h"
#include "Configs.h"
#include "Ref_ptr.h"
#include "Version.h"

#include "Threads/Task2.h"
#include <deque>

#include <array>
#include <cassert>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>

namespace engine {
	class IEngineModule;
	class Graphics;
	class Statistic;
	class Application;
	class WorkerThread;

	class Engine {
        private:
            class EngineModuleEnumerator;
            class EmplacedModuleEnumerator; // focus :)

	public:
        enum class Workers: uint8_t {
            RENDER_THREAD = 0u,
            UPDATE_THREAD = 1u,
            MAX_VALUE
        };

		~Engine();

		inline static Engine& getInstance() noexcept {
			static Engine engine;
			return engine;
		}

		void init(const EngineConfig& config);
		void destroy();

		template<typename T>
		inline T& getModule() noexcept {
			const auto moduleId = UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
            assert(moduleId < _modules.size());
            return static_cast<T&>(*_modules[moduleId]);
		}

		template<typename T>
		inline const T& getModule() const noexcept {
			const auto moduleId = UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
            assert(moduleId < _modules.size());
            return static_cast<const T&>(*_modules[moduleId]);
		}

		template<typename T, typename...Args>
		void setModule(Args&&...args) {
            UniqueTypeId<EmplacedModuleEnumerator>::getUniqueId<T>();
			const auto moduleId = UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
            assert(moduleId >= _modules.size()); // module already exist
            _modules.resize(moduleId + 1u);
			_modules[moduleId] = std::make_unique<T>(std::forward<Args>(args)...);
		}

        template <typename T>
        inline bool hasModule() const noexcept {
            return UniqueTypeId<EmplacedModuleEnumerator>::getUniqueId<T>() == UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
        }

		void resize(const uint16_t w, const uint16_t h);
		void deviceDestroyed();

		void run();

		inline void setTimeMultiply(const float m) noexcept { _timeMultiply = m; }
		[[nodiscard]] inline float getTimeMultiply() const noexcept { return _timeMultiply; }

        [[nodiscard]] inline static Version version() noexcept { return {0u, 0u, 1u}; }
        [[nodiscard]] Version applicationVersion() const noexcept;

        [[nodiscard]] uint8_t getThreadCommutationId(const Workers w) const noexcept {
            if (w == Workers::MAX_VALUE) { return 0xffu; }
            return _workerIds[static_cast<uint8_t>(w)];
        }

        [[nodiscard]] uint8_t getThreadCommutationId(const uint8_t id) const noexcept {
            if (id >= static_cast<uint8_t>(Workers::MAX_VALUE)) { return 0xffu; }
            return _workerIds[id];
        }

	private:
		Engine();
		void initComplete();

		void render(const float delta, const std::chrono::steady_clock::time_point& currentTime, std::deque<linked_ptr<TaskBase>>&& tasks);
		void update(const float delta, const std::chrono::steady_clock::time_point& currentTime, std::deque<linked_ptr<TaskBase>>&& tasks);

		std::vector<std::unique_ptr<IEngineModule>> _modules;

        ref_ptr<Statistic> _statistic = nullptr;
		ref_ptr<Graphics> _graphics = nullptr;
		std::unique_ptr<Application> _application;
		std::unique_ptr<WorkerThread> _renderThread;
		std::unique_ptr<WorkerThread> _updateThread;

		float _timeMultiply = 1.0f;

        std::array<uint8_t, static_cast<size_t>(Workers::MAX_VALUE)> _workerIds = {};
	};
}
