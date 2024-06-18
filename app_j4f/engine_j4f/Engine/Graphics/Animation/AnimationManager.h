#pragma once

#include "../../Core/Common.h"
#include "AnimationUpdater.h"
#include "ActionAnimation.h"

#include <vector>
#include <memory>
#include <utility>

namespace engine {

    class Animation;

    class AnimationManager final {
    public:
        ~AnimationManager() = default;

        template<typename T>
        void registerAnimation(T *animation) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() <= animId) {
                _animUpdaters.push_back(std::make_unique<AnimationUpdater<T>>());
            }
            static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->registerAnimation(animation);
        }

        template<typename T>
        void unregisterAnimation(T *animation) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->unregisterAnimation(animation);
            }
        }

        template<typename T> requires AllowTarget<T>
        inline void addTarget(const T *animation, typename T::TargetType target) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->addTarget(animation, target);
            }
        }

        template<typename T> requires AllowTarget<T>
        inline void removeTarget(const T *animation, typename T::TargetType target) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->removeTarget(animation, target);
            }
        }

        inline void update(const float delta) noexcept { // virtual update mechanic
            if (delta == 0.0f) { // disable update if delta is 0.0f
                return;
            }

            for (auto & updater : _animUpdaters) {
                updater->update(delta);
            }
        }

        template<typename T, typename... Args>
        inline void update(const float delta) noexcept {
            if (delta == 0.0f) { // disable update if delta is 0.0f
                return;
            }

            update_strict<T>(delta);
            update<Args...>(delta);
        }

    private:
        template<typename... Args>
        inline constexpr typename std::enable_if<sizeof...(Args) == 0>::type update(const float /*delta*/) noexcept {}

        template<typename T>
        inline void update_strict(const float delta) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->update(delta);
            }
        }

        std::vector<std::unique_ptr<IAnimationUpdater>> _animUpdaters;
    };

}