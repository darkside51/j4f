#pragma once

#include <Engine/Core/Common.h>
#include <Engine/Core/ref_ptr.h>
#include <vector>

namespace game {

    class ServiceLocator {
    public:
        inline static ServiceLocator& instance() noexcept {
            static ServiceLocator locator;
            return locator;
        }

        template <typename T>
        void registerService(engine::ref_ptr<T> service) {
            const auto id = engine::UniqueTypeId<ServiceLocator>::getUniqueId<T>();
            if (id >= _services.size()) {
                _services.resize(id + 1u);
            }

            _services[id] = static_cast<void*>(service.get());
        }

        template <typename T>
        engine::ref_ptr<T> getService() {
            const auto id = engine::UniqueTypeId<ServiceLocator>::getUniqueId<T>();
            if (id >= _services.size()) {
                return nullptr;
            }

            return static_cast<T*>(_services[id].get());
        }

    private:
        ServiceLocator() = default;

        std::vector<engine::ref_ptr<void>> _services;
    };
}