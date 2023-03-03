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
        inline void registerAnimation(T *animation) {
            if (std::find(_animations.begin(), _animations.end(), animation) == _animations.end()) {
                _animations.push_back(animation);
            }
        }

        inline void unregisterAnimation(T *animation) {
            _animations.erase(std::remove(_animations.begin(), _animations.end(), animation), _animations.end());
        }

        inline void update(const float delta) {
            for (auto && anim : _animations) {
                anim->updateAnimation(delta);
            }
        }

    private:
        std::vector<T*> _animations;
    };

    class Animation;

    class AnimationManager final {
    public:
        ~AnimationManager() = default;

        template<typename T>
        void registerAnimation(T *animation) {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() <= animId) {
                _animUpdaters.push_back(std::make_unique<AnimationUpdater<T>>());
            }
            static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->registerAnimation(animation);
        }

        template<typename T>
        void unregisterAnimation(T *animation) {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() > animId) {
                static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->unregisterAnimation(animation);
            }
        }

        template <typename... Args>
        inline typename std::enable_if<sizeof...(Args) == 0>::type update(const float /*delta*/) { }

        template<typename T, typename... Args>
        inline void update(const float delta) {
            update_strict<T>(delta);
            update<Args...>(delta);
        }

    private:
        template<typename T>
        inline void update_strict(const float delta) {
            static const auto animId = UniqueTypeId<Animation>::getUniqueId<T>();
            if (_animUpdaters.size() <= animId) {
                _animUpdaters.push_back(std::make_unique<AnimationUpdater<T>>());
            }
            static_cast<AnimationUpdater<T>*>(_animUpdaters[animId].get())->update(delta);
        }

        std::vector<std::unique_ptr<IAnimationUpdater>> _animUpdaters;
    };

}