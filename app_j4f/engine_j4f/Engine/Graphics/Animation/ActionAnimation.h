#pragma once

#include "Easings.h"

#include <functional>

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

    class ActionAnimation final {
    public:
        explicit ActionAnimation(ActionExecutor && executor) : _executor(std::move(executor)) {}

        void updateAnimation(const float dt) {
            _finished = _executor(dt);
        }

        inline bool forceUpdate() const noexcept { return true; }
        inline bool finished() const noexcept { return _finished; }
        inline bool isUpdateable() const noexcept { return true; }

    private:
        ActionExecutor _executor;
        bool _finished = false;
    };
}
