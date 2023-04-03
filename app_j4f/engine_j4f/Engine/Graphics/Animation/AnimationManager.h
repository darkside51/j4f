#pragma once

#include "../../Core/Common.h"

#include <vector>
#include <memory>
#include <utility>

namespace engine {

    class IAnimationUpdater {
    public:
        virtual ~IAnimationUpdater() = default;
    };

    template<typename T>
    class AnimationUpdater final : public IAnimationUpdater {
    public:
        inline void registerAnimation(T *animation) noexcept {
            if (std::find_if(_animations.begin(), _animations.end(),
                             [animation](const std::pair<T*, float>& p) {
                                return p.first == animation;
                             }) == _animations.end()) {
                _animations.emplace_back(animation, 0.0f);
                _animationsTargets.emplace_back();
            }
        }

        inline void unregisterAnimation(const T *animation) noexcept {
            auto it = std::find_if(_animations.begin(), _animations.end(), [animation](const std::pair<T*, float>& p) {
                return p.first == animation;
            });
            if (it != _animations.end()) {
                const auto i = std::distance(_animations.begin(), it);
                _animationsTargets.erase(
                        std::remove(_animationsTargets.begin(), _animationsTargets.end(), _animationsTargets[i]),
                        _animationsTargets.end());
                std::swap(*it, _animations.back());
                _animations.pop_back();
//                _animations.erase(std::remove_if(_animations.begin(), _animations.end(), [animation](const std::pair<T*, float>& p){
//                    return p.first == animation;
//                }), _animations.end());
            }
        }

        inline void addTarget(const T *animation, T::TargetType target) noexcept {
            auto it = std::find_if(_animations.begin(), _animations.end(), [animation](const std::pair<T*, float>& p){
                return p.first == animation;
            });
            if (it != _animations.end()) {
                const auto i = std::distance(_animations.begin(), it);
                _animationsTargets[i].push_back(target);
            }
        }

        inline void removeTarget(const T *animation, T::TargetType target) noexcept {
            auto it = std::find_if(_animations.begin(), _animations.end(), [animation](const std::pair<T*, float>& p){
                return p.first == animation;
            });
            if (it != _animations.end()) {
                const auto i = std::distance(_animations.begin(), it);
                _animationsTargets[i].erase(
                        std::remove(_animationsTargets[i].begin(), _animationsTargets[i].end(), target),
                        _animationsTargets[i].end());
            }
        }

        inline void update(const float delta) noexcept {
            size_t i = 0;
            for (auto && [anim, accum] : _animations) {
                bool requestUpdate = anim->getNeedUpdate();
                if (requestUpdate == false) {
                    for (auto &&target: _animationsTargets[i]) {
                        if (requestUpdate = target->requestAnimUpdate()) {
                            break;
                        }
                    }
                }

                if (requestUpdate) {
                    anim->updateAnimation(delta + accum);
                    accum = 0.0f;
                    // update targets
                    for (auto&& target : _animationsTargets[i]) {
                        target->applyFrame(anim);
                        target->completeAnimUpdate();
                    }
                } else {
                    accum += delta;
                }
                ++i;
            }
        }

    private:
        std::vector<std::pair<T*, float>> _animations;
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

        template<typename T>
        inline void addTarget(const T *animation, T::TargetType target) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->addTarget(animation, target);
            }
        }

        template<typename T>
        inline void removeTarget(const T *animation, T::TargetType target) noexcept {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->removeTarget(animation, target);
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