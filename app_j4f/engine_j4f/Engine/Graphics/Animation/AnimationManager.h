#pragma once

#include "../../Core/Common.h"

#include <vector>
#include <memory>

namespace engine {

    class IAnimationUpdater {
    public:
        virtual ~IAnimationUpdater() = default;
    };

    template<typename T>
    class AnimationUpdater final : public IAnimationUpdater {
    public:
        inline void registerAnimation(T *animation) noexcept {
            if (std::find(_animations.begin(), _animations.end(), animation) == _animations.end()) {
                _animations.push_back(animation);
                _animationsTargets.emplace_back();
            }
        }

        inline void unregisterAnimation(T *animation) noexcept {
            // todo: remove _animationsTargets!
            _animations.erase(std::remove(_animations.begin(), _animations.end(), animation), _animations.end());
        }

        inline void update(const float delta) noexcept {
            size_t i = 0;
            for (auto && anim : _animations) {
                if (anim->getNeedUpdate()) {
                    anim->updateAnimation(delta);
                    // update targets
                    for (auto&& target : _animationsTargets[i]) {
                        target->applyFrame(anim);
                    }
                }
                ++i;
            }
        }

    private:
        std::vector<T*> _animations;
        std::vector<std::vector<typename T::TargetType>> _animationsTargets;
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
            static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->registerAnimation(animation);
        }

        template<typename T>
        void unregisterAnimation(T *animation) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->unregisterAnimation(animation);
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
        template <typename... Args>
        inline typename std::enable_if<sizeof...(Args) == 0>::type update(const float /*delta*/) noexcept { }

        template<typename T>
        inline void update_strict(const float delta) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() <= animId) {
                _animUpdaters.push_back(std::make_unique<AnimationUpdater<T>>());
            }
            static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->update(delta);
        }

        std::vector<std::unique_ptr<IAnimationUpdater>> _animUpdaters;
    };

}