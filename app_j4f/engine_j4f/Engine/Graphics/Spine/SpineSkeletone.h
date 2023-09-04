#pragma once

#include <spine/spine.h>

namespace engine {
    class SpineAnimator;

    class SpineSkeleton {
    public:
        bool requestAnimUpdate() const noexcept { return _requestAnimUpdate; }
        void completeAnimUpdate() noexcept { _requestAnimUpdate = false; }
        void applyFrame(SpineAnimator* animator); // animation another vision

    private:
        spine::Skeleton *_skeleton = nullptr;
        bool _requestAnimUpdate = false;
    };
}