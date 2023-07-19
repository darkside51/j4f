#pragma once

#include <cstdint>
#include "../../Core/Math/mathematic.h"
#include "BezierEasing.h"

namespace engine::easing {
    inline float easeLinear(const float t) noexcept { return t; }
    inline float easeInQuad(const float t) noexcept { return t * t; }
    inline float easeOutQuad(const float t) noexcept { return -t * (t - 2.0f); }
    inline float easeInOutQuad(float t) noexcept {
        t *= 2.0f;
        if (t < 1.0f) {
            return t * t / 2.0f;
        } else {
            --t;
            return -0.5f * (t * (t - 2.0f) - 1.0f);
        }
    }
    inline float easeOutInQuad(const float t) noexcept {
        if (t < 0.5f) return easeOutQuad (t * 2.0f) / 2.0f;
        return easeInQuad((2.0f * t) - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInCubic(const float t) noexcept { return t * t * t; }
    inline float easeOutCubic(float t) noexcept {
        t -= 1.0f;
        return t * t * t + 1.0f;
    }
    inline float easeInOutCubic(float t) noexcept {
        t *= 2.0f;
        if (t < 1.0f) {
            return 0.5f * t * t * t;
        } else {
            t -= 2.0f;
            return 0.5f * (t * t * t + 2.0f);
        }
    }
    inline float easeOutInCubic(const float t) noexcept {
        if (t < 0.5f) return easeOutCubic (2.0f * t) / 2.0f;
        return easeInCubic(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInQuart(float t) noexcept { return t * t * t * t; }
    inline float easeOutQuart(float t) noexcept {
        t -= 1.0f;
        return -(t * t * t * t - 1.0f);
    }
    inline float easeInOutQuart(float t) noexcept {
        t *= 2.0f;
        if (t < 1.0f) {
            return 0.5f * t * t * t * t;
        } else {
            t -= 2.0f;
            return -0.5f * (t * t * t * t - 2.0f);
        }
    }
    inline float easeOutInQuart(const float t) noexcept {
        if (t < 0.5f) return easeOutQuart (2.0f * t) / 2.0f;
        return easeInQuart(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInQuint(const float t) noexcept{ return t*t*t*t*t; }
    inline float easeOutQuint(float t) noexcept {
        t -= 1.0f;
        return t * t * t * t * t + 1.0f;
    }
    inline float easeInOutQuint(float t) {
        t *= 2.0f;
        if (t < 1.0f) {
            return 0.5f * t * t * t * t * t;
        } else {
            t -= 2.0f;
            return 0.5f * (t * t * t * t * t + 2.0f);
        }
    }
    inline float easeOutInQuint(const float t) noexcept {
        if (t < 0.5f) return easeOutQuint (2.0f * t) / 2.0f;
        return easeInQuint(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInSine(const float t) noexcept {
        return (t == 1.0f) ? 1.0f : -cosf(t * math_constants::pi_half) + 1.0f;
    }
    inline float easeOutSine(const float t) noexcept {
        return sinf(t * math_constants::pi_half);
    }
    inline float easeInOutSine(const float t) noexcept {
        return -0.5f * (cosf(t * math_constants::pi) - 1.0f);
    }
    inline float easeOutInSine(const float t) noexcept {
        if (t < 0.5f) return easeOutSine (2.0f * t) / 2.0f;
        return easeInSine(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInExpo(const float t) noexcept {
        return static_cast<float>((t == 0.0f || t == 1.0f) ? t : powf(2.0f, 10.0f * (t - 1.0f)) - 0.001f);
    }
    inline float easeOutExpo(const float t) noexcept {
        return (t == 1.0f) ? 1.0f : 1.0f * (-powf(2.0f, -10.0f * t) + 1.0f);
    }
    inline float easeInOutExpo(float t) noexcept {
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        t *= 2.0f;
        if (t < 1.0f) return 0.5f * powf(2.0f, 10.0f * (t - 1.0f)) - 0.0005f;
        return 0.5f * 1.0005f * (-powf(2.0f, -10.0f * (t - 1.0f)) + 2.0f);
    }
    inline float easeOutInExpo(const float t) noexcept {
        if (t < 0.5f) return easeOutExpo (2.0f * t) / 2.0f;
        return easeInExpo(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInCirc(const float t) noexcept {
        return -(::sqrtf(1.0f - t * t) - 1.0f);
    }
    inline float easeOutCirc(float t) noexcept {
        t -= 1.0f;
        return ::sqrtf(1.0f - t * t);
    }
    inline float easeInOutCirc(float t) noexcept {
        t *= 2.0f;
        if (t < 1.0f) {
            return -0.5f * (::sqrtf(1.0f - t * t) - 1.0f);
        } else {
            t -= 2.0f;
            return 0.5f * (::sqrtf(1.0f - t * t) + 1.0f);
        }
    }
    inline float easeOutInCirc(const float t) noexcept {
        if (t < 0.5f) return easeOutCirc (2.0f * t) / 2.0f;
        return easeInCirc(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInElastic(const float t) noexcept {
        return sinf(13.0f * math_constants::pi_half * t) * powf(2.0f, 10.0f * (t - 1.0f));
    }
    inline float easeOutElastic(const float t) noexcept {
        return sinf(-13.0f * math_constants::pi_half * (t + 1.0f)) * powf(2.0f, -10.0f * t) + 1.0f;
    }
    inline float easeInOutElastic(const float t) noexcept {
        if (t < 0.5f) {
            return (0.5f * sinf(13.0f * math_constants::pi_half * (2.0f * t)) * powf(2.0f, 10.0f * ((2.0f * t) - 1.0f)));
        } else {
            return (0.5f * (sinf(-13.0f * math_constants::pi_half * ((2.0f * t - 1.0f) + 1.0f)) * pow(2.0f, -10.0f * (2.0f * t - 1.0f)) + 2.0f));
        }
    }
    inline float easeOutInElastic(const float t) noexcept {
        if (t < 0.5f) return easeOutElastic (2.0f * t) / 2.0f;
        return easeInElastic(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeInBack(const float t) noexcept {
        return t * t * t - t * sinf(t * math_constants::pi);
    }
    inline float easeOutBack(const float t) noexcept {
        float f = (1.0f - t);
        return 1.0f - (f * f * f - f * sinf(f * math_constants::pi));
    }
    inline float easeInOutBack(const float t) noexcept {
        if (t < 0.5f) {
            float f = 2.0f * t;
            return 0.5f * (f * f * f - f * sinf(f * math_constants::pi));
        } else {
            float f = (1.0f - (2.0f * t - 1.0f));
            return 0.5f * (1.0f - (f * f * f - f * sinf(f * math_constants::pi))) + 0.5f;
        }
    }
    inline float easeOutInBack(const float t) noexcept {
        if (t < 0.5f) return easeOutBack (2.0f * t) / 2.0f;
        return easeInBack(2.0f * t - 1.0f) / 2.0f + 0.5f;
    }
    inline float easeOutBounce(const float t) noexcept {
        if (t < 4.0f / 11.0f) {
            return (121.0 * t * t) / 16.0f;
        } else if (t < 8.0f / 11.0f) {
            return (363.0f / 40.0f * t * t) - (99.0f / 10.0f * t) + 17.0f / 5.0f;
        } else if (t < 9.0f / 10.0f) {
            return (4356.0f / 361.0f * t * t) - (35442.0f / 1805.0f * t) + 16061.0f / 1805.0f;
        } else {
            return (54.0f / 5.0f * t * t) - (513.0f / 25.0f * t) + 268.0f / 25.0f;
        }
    }
    inline float easeInBounce(const float t) noexcept {
        return 1.0f - easeOutBounce(1.0f - t);
    }
    inline float easeInOutBounce(const float t) noexcept {
        if (t < 0.5f) {
            return 0.5f * easeInBounce(t * 2.0f);
        } else {
            return 0.5f * easeOutBounce(t * 2.0f - 1.0f) + 0.5f;
        }
    }
    inline float easeOutInBounce(const float t) noexcept {
        if (t < 0.5f) return easeOutBounce (2.0f * t) / 2.0f;
        return easeInBounce (2.0f * t - 1.0f) / 2.0f + 0.5f;
    }

    enum class Easing : uint8_t {
        Linear = 0u,
        QuadIn, QuadOut, QuadInOut, QuadOutIn,
        CubicIn, CubicOut, CubicInOut, CubicOutIn,
        QuartIn, QuartOut, QuartInOut, QuartOutIn,
        QuintIn, QuintOut, QuintInOut, QuintOutIn,
        SineIn, SineOut, SineInOut, SineOutIn,
        ExpIn, ExpOut, ExpInOut, ExpOutIn,
        CircIn, CircOut, CircInOut, CircOutIn,
        ElasticIn, ElasticOut, ElasticInOut, ElasticOutIn,
        BackIn, BackOut, BackInOut, BackOutIn,
        BounceIn, BounceOut, BounceInOut, BounceOutIn,
        Custom
    };

    inline float calculateProgress(const float t, const Easing e) noexcept {
        switch (e) {
            case Easing::Linear:
                return easeLinear(t);
            case Easing::QuadIn:
                return easeInQuad(t);
            case Easing::QuadOut:
                return easeOutQuad(t);
            case Easing::QuadInOut:
                return easeInOutQuad(t);
            case Easing::QuadOutIn:
                return easeOutInQuad(t);
            case Easing::CubicIn:
                return easeInCubic(t);
            case Easing::CubicOut:
                return easeOutCubic(t);
            case Easing::CubicInOut:
                return easeInOutCubic(t);
            case Easing::CubicOutIn:
                return easeOutInCubic(t);
            case Easing::QuartIn:
                return easeInQuart(t);
            case Easing::QuartOut:
                return easeOutQuart(t);
            case Easing::QuartInOut:
                return easeInOutQuart(t);
            case Easing::QuartOutIn:
                return easeOutInQuart(t);
            case Easing::QuintIn:
                return easeInQuint(t);
            case Easing::QuintOut:
                return easeOutQuint(t);
            case Easing::QuintInOut:
                return easeInOutQuint(t);
            case Easing::QuintOutIn:
                return easeOutInQuint(t);
            case Easing::SineIn:
                return easeInSine(t);
            case Easing::SineOut:
                return easeOutSine(t);
            case Easing::SineInOut:
                return easeInOutSine(t);
            case Easing::SineOutIn:
                return easeOutInSine(t);
            case Easing::ExpIn:
                return easeInExpo(t);
            case Easing::ExpOut:
                return easeOutExpo(t);
            case Easing::ExpInOut:
                return easeInOutExpo(t);
            case Easing::ExpOutIn:
                return easeOutInExpo(t);
            case Easing::CircIn:
                return easeInCirc(t);
            case Easing::CircOut:
                return easeOutCirc(t);
            case Easing::CircInOut:
                return easeInOutCirc(t);
            case Easing::CircOutIn:
                return easeOutInCirc(t);
            case Easing::ElasticIn:
                return easeInElastic(t);
            case Easing::ElasticOut:
                return easeOutElastic(t);
            case Easing::ElasticInOut:
                return easeInOutElastic(t);
            case Easing::ElasticOutIn:
                return easeOutInElastic(t);
            case Easing::BackIn:
                return easeInBack(t);
            case Easing::BackOut:
                return easeOutBack(t);
            case Easing::BackInOut:
                return easeInOutBack(t);
            case Easing::BackOutIn:
                return easeOutInBack(t);
            case Easing::BounceIn:
                return easeInBounce(t);
            case Easing::BounceOut:
                return easeOutBounce(t);
            case Easing::BounceInOut:
                return easeInOutBounce(t);
            case Easing::BounceOutIn:
                return easeOutInBounce(t);
        }
        return 0.0f;
    }
}
