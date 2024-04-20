#pragma once

#include "Common.h"
#include "Configs.h"
#include "ref_ptr.h"
#include "Version.h"

#include "Threads/TaskCommon.h"

#include <Platform_device.h>

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

    class LogManager;
    class ThreadPool2;
    class WorkerThreadsCommutator;
    class MemoryManager;
    class CacheManager;
    class FileManager;
    class AssetManager;
    class Input;
//    class Device;
    class Bus;
    class TimerManager;

	class Engine {
    private:
        class EngineModuleEnumerator;
        class EmplacedModuleEnumerator; // focus :)

	public:
		enum class Endian : uint8_t {
			LittleEndian = 0u,
			BigEndian = 1u
		};

        enum class Workers : uint8_t {
            RENDER_THREAD = 0u,
            UPDATE_THREAD = 1u,
            MAX_VALUE
        };

		~Engine();

        inline static Engine& getInstance() noexcept {

            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<LogManager>() == 0);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<Statistic>() == 1);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<ThreadPool2>() == 2);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<WorkerThreadsCommutator>() == 3);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<MemoryManager>() == 4);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<CacheManager>() == 5);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<FileManager>() == 6);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<AssetManager>() == 7);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<Input>() == 8);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<Graphics>() == 9);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<Device>() == 10);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<Bus>() == 11);
            static_assert(ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<TimerManager>() == 12);

            static Engine engine;
            return engine;
        }


		void init(const EngineConfig& config);
		void destroy();

		template<typename T>
		inline T& getModule() noexcept {
			const auto moduleId = ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
            assert(moduleId < _modules.size());
            return static_cast<T&>(*_modules[moduleId]);
		}

		template<typename T>
		inline const T& getModule() const noexcept {
			const auto moduleId = ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
            assert(moduleId < _modules.size());
            return static_cast<const T&>(*_modules[moduleId]);
		}

		template<typename T, typename...Args>
		void setModule(Args&&...args) {
            ctti::UniqueTypeId<EmplacedModuleEnumerator>::getUniqueId<T>();
			const auto moduleId = ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
            assert(moduleId >= _modules.size()); // module already exist
            _modules.resize(moduleId + 1u);
			_modules[moduleId] = std::make_unique<T>(std::forward<Args>(args)...);
		}

        template <typename T>
        inline bool hasModule() const noexcept {
            return ctti::UniqueTypeId<EmplacedModuleEnumerator>::getUniqueId<T>() == ctti::UniqueTypeId<EngineModuleEnumerator>::getUniqueId<T>();
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

		inline Endian endian() const noexcept { return _endian; }

	private:
		Engine();
		void initComplete();

		void render(const float delta, const std::chrono::steady_clock::time_point& currentTime, WorkerTasks && tasks);
		void update(const float delta, const std::chrono::steady_clock::time_point& currentTime, WorkerTasks && tasks);

		std::vector<std::unique_ptr<IEngineModule>> _modules;

        ref_ptr<Statistic> _statistic = nullptr;
		ref_ptr<Graphics> _graphics = nullptr;
		std::unique_ptr<Application> _application;
		std::unique_ptr<WorkerThread> _renderThread;
		std::unique_ptr<WorkerThread> _updateThread;

		float _timeMultiply = 1.0f;

        std::array<uint8_t, static_cast<size_t>(Workers::MAX_VALUE)> _workerIds = {};
		Endian _endian = Endian::LittleEndian;
	};
}
