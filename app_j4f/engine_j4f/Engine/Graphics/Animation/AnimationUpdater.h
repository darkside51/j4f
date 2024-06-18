#pragma once

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

namespace engine {
    class IAnimationUpdater {
    public:
        virtual ~IAnimationUpdater() = default;
        virtual void update(const float delta) noexcept = 0;
    };

    template<class T> concept AllowTarget = requires {
        typename T::TargetType;
    };

    template<typename T>
    class AnimationUpdater final : public IAnimationUpdater {
    public:
        inline void registerAnimation(T *animation) noexcept {
            if (std::find_if(_animations.begin(), _animations.end(),
                             [animation](const std::pair<T *, float> &p) {
                                 return p.first == animation;
                             }) == _animations.end()) {
                _animations.emplace_back(animation, 0.0f);
            }
        }

        inline void unregisterAnimation(const T *animation) noexcept {
            const auto it = std::find_if(_animations.cbegin(), _animations.cend(), [animation](const std::pair<T *, float> &p) {
                return p.first == animation;
            });
            if (it != _animations.cend()) {
                _animations.erase(std::remove(_animations.begin(), _animations.end(), *it), _animations.end());
            }
        }

        inline void update(const float delta) noexcept override {
            size_t i = 0;
            std::vector<uint32_t> toRemove;
            toRemove.reserve(_animations.size());
            for (auto && [anim, accum]: _animations) {
                if (anim->finished()) {
                    toRemove.emplace_back(i);
                    ++i;
                    continue;
                }

                if (!anim->isActive()) {
                    ++i;
                    continue;
                }

                if (anim->forceUpdate()) {
                    anim->updateAnimation(delta + accum);
                    accum = 0.0f;
                } else {
                    accum += delta;
                }
                ++i;
            }

            for (auto n : toRemove) {
                _animations.erase(_animations.begin() + n);
            }
        }

    private:
        std::vector<std::pair<T *, float>> _animations;
    };

    /////////////////////////////////////////////////////////////////////////////////
    template<AllowTarget T>
    class AnimationUpdater<T> final : public IAnimationUpdater {
    public:
        inline void registerAnimation(T *animation) noexcept {
            if (std::find_if(_animations.begin(), _animations.end(),
                             [animation](const std::pair<T *, float> &p) {
                                 return p.first == animation;
                             }) == _animations.end()) {
                _animations.emplace_back(animation, 0.0f);
                _animationsTargets.emplace_back();
            }
        }

        inline void unregisterAnimation(const T *animation) noexcept {
            auto it = std::find_if(_animations.begin(), _animations.end(), [animation](const std::pair<T *, float> &p) {
                return p.first == animation;
            });
            if (it != _animations.end()) {
                const auto i = std::distance(_animations.begin(), it);
//                _animationsTargets.erase(
//                        std::remove(_animationsTargets.begin(), _animationsTargets.end(), _animationsTargets[i]),
//                        _animationsTargets.end());
                _animationsTargets.erase(_animationsTargets.begin() + i);
                //std::swap(*it, _animations.back());
                //_animations.pop_back();
                _animations.erase(std::remove(_animations.begin(), _animations.end(), *it), _animations.end());
            }
        }

        inline void addTarget(const T *animation, typename T::TargetType target) noexcept {
            auto it = std::find_if(_animations.begin(), _animations.end(), [animation](const std::pair<T *, float> &p) {
                return p.first == animation;
            });
            if (it != _animations.end()) {
                const auto i = std::distance(_animations.begin(), it);
                _animationsTargets[i].push_back(target);
            }
        }

        inline void removeTarget(const T *animation, typename T::TargetType target) noexcept {
            auto it = std::find_if(_animations.begin(), _animations.end(), [animation](const std::pair<T *, float> &p) {
                return p.first == animation;
            });
            if (it != _animations.end()) {
                const auto i = std::distance(_animations.begin(), it);
                _animationsTargets[i].erase(
                        std::remove(_animationsTargets[i].begin(), _animationsTargets[i].end(), target),
                        _animationsTargets[i].end());
            }
        }

        inline void update(const float delta) noexcept override {
            size_t i = 0u;
            std::vector<uint32_t> toRemove;
            toRemove.reserve(_animations.size());
            for (auto && [anim, accum]: _animations) {
                if (anim->finished()) {
                    toRemove.emplace_back(i);
                    ++i;
                    continue;
                }

                if (!anim->isActive()) {
                    ++i;
                    continue;
                }

                bool requestUpdate = false;
                for (auto&& target : _animationsTargets[i]) {
                    if (target->requestAnimUpdate()) {
                        if (!requestUpdate) {
                            anim->updateAnimation(delta + accum);
                            accum = 0.0f;
                            requestUpdate = true;
                        }

                        target->applyFrame(anim);
                        target->completeAnimUpdate();
                    }
                }

                if (!requestUpdate) {
                    if (anim->forceUpdate()) {
                        anim->updateAnimation(delta + accum);
                        accum = 0.0f;
                    } else {
                        accum += delta;
                    }
                }

                ++i;
            }

            for (auto n : toRemove) {
                _animationsTargets.erase(_animationsTargets.begin() + n);
                _animations.erase(_animations.begin() + n);
            }
        }

    private:
        std::vector<std::pair<T *, float>> _animations;
        std::vector<std::vector<typename T::TargetType>> _animationsTargets;
    };
}
