#pragma once
#include "../../Core/Common.h"
#include "../../Core/BitMask.h"
#include <functional>

namespace engine {
    using FeatureInitializer = std::function<void()>;

    template<typename T>
    concept IsFeature = requires() { T::initFeatureData(); };

    class Features {
    public:
        template<typename T> requires IsFeature<T>
        inline void request() noexcept {
            _features.setBit(UniqueTypeId<Features>::getUniqueId<T>(), 1);
            _initializers.emplace_back([](){ T::initFeatureData();});
        }

        template<typename T> requires IsFeature<T>
        inline bool enabled() const noexcept {
            return _features.checkBit(UniqueTypeId<Features>::getUniqueId<T>());
        }

        void initFeatures() noexcept {
            for (auto&& f : _initializers) {
                f();
            }
            _initializers.clear();
        }

    private:
        BitMask32 _features;
        std::vector<FeatureInitializer> _initializers;
    };
}
