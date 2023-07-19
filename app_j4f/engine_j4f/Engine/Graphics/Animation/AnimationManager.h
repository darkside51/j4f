#pragma once

#include "../../Core/Common.h"
#include "AnimationUpdater.h"
#include "Easings.h"

#include <functional>
#include <vector>
#include <memory>
#include <utility>

namespace engine {

//    template <typename ActionExecutor>

    using ActionExecutor = std::function<bool(const float)>;

    inline auto makeProgressionExecutor(const float duration, ActionExecutor && executor, const easing::Easing e) -> ActionExecutor {
        return [elapsed = 0.0f, duration,
            executor = std::move(executor), e] (const float dt) mutable -> bool {
            const float progress = easing::calculateProgress(elapsed / duration, e); // [0.0f, 1.0f]
            if (const bool finished = executor(progress); !finished) {
                elapsed += dt;
                return (elapsed >= duration);
            }
            return true;
        };
    }

    inline auto makeDurationExecutor(const float duration, ActionExecutor && executor) -> ActionExecutor {
        return [elapsed = 0.0f, duration,
                executor = std::move(executor)] (const float dt) mutable -> bool {
            if (const bool finished = executor(dt); !finished) {
                elapsed += dt;
                return (elapsed >= duration);
            }
            return true;
        };
    }

    class ActionAnimation {
    public:
        ActionAnimation(ActionExecutor && executor) : _executor(std::move(executor)) {}

        void updateAnimation(const float dt) {
            _finished = _executor(dt);
        }

        inline bool needUpdate() const noexcept { return true; }
        inline bool finished() const noexcept { return _finished; }

    private:
        ActionExecutor _executor;
        bool _finished = false;
    };

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
        inline void addTarget(const T *animation, T::TargetType target) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->addTarget(animation, target);
            }
        }

        template<typename T> requires AllowTarget<T>
        inline void removeTarget(const T *animation, T::TargetType target) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->removeTarget(animation, target);
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
        inline typename std::enable_if<sizeof...(Args) == 0>::type update(const float /*delta*/) noexcept {}

        template<typename T>
        inline void update_strict(const float delta) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() <= animId) {
                _animUpdaters.push_back(std::make_unique<AnimationUpdater<T>>());
            }
            static_cast<AnimationUpdater<T> *>(_animUpdaters[animId].get())->update(delta);
        }

        std::vector<std::unique_ptr<IAnimationUpdater>> _animUpdaters;
    };

}