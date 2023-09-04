#pragma once

#include <spine/spine.h>

namespace engine {

    class SpineSkeleton;

    class SpineAnimator {
    public:
        using TargetType = SpineSkeleton*;
        void updateAnimation(const float delta);

    private:
    };

};