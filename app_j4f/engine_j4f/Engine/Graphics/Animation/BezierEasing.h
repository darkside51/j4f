#pragma once
////////////////////////////////////////////////////////////////////////////
//BezierEasing provides Cubic Bezier Curve easing which generalizes easing functions (ease-in, ease-out, ease-in-out, ...any other custom curve) exactly like in CSS Transitions.
// start and end positions is always (0.,0.) and (1.,1.)
// you have to provide only control points
// source code - https://github.com/gre/bezier-easing/blob/master/src/index.js
////////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <cmath>
#include <functional>
#include <map>

namespace engine::easing {
    constexpr uint8_t kSplineTableSize = 11u;
    constexpr float kSampleStepSize = 1.0f / (static_cast<float>(kSplineTableSize) - 1.0f);

    inline float cubicBezierFuncA(const float aA1, const float aA2) { return 1.0f - 3.0f * aA2 + 3.0f * aA1; }

    inline float cubicBezierFuncB(const float aA1, const float aA2) { return 3.0f * aA2 - 6.0f * aA1; }

    inline float cubicBezierFuncC(const float aA1) { return 3.0f * aA1; }

// Returns x(t) given t, x1, and x2, or y(t) given t, y1, and y2.
    inline float calcBezier(const float aT, const float aA1, const float aA2) {
        return ((cubicBezierFuncA(aA1, aA2) * aT + cubicBezierFuncB(aA1, aA2)) * aT + cubicBezierFuncC(aA1)) * aT;
    }

// Returns dx/dt given t, x1, and x2, or dy/dt given t, y1, and y2.
    inline float getSlope(const float aT, const float aA1, const float aA2) {
        return 3.0f * cubicBezierFuncA(aA1, aA2) * aT * aT + 2.0f * cubicBezierFuncB(aA1, aA2) * aT +
               cubicBezierFuncC(aA1);
    }

    inline float binarySubdivide(float aX, float aA, float aB, float mX1, float mX2) {
        constexpr float SUBDIVISION_PRECISION = 0.0000001f;
        constexpr uint8_t SUBDIVISION_MAX_ITERATIONS = 10u;
        float currentX, currentT = 0.0f;
        uint8_t i = 0u;
        do {
            currentT = aA + (aB - aA) / 2.0f;
            currentX = calcBezier(currentT, mX1, mX2) - aX;
            if (currentX > 0.0f) {
                aB = currentT;
            } else {
                aA = currentT;
            }
        } while (abs(currentX) > SUBDIVISION_PRECISION && ++i < SUBDIVISION_MAX_ITERATIONS);
        return currentT;
    }

    inline float newtonRaphsonIterate(float aX, float aGuessT, float mX1, float mX2) {
        for (uint8_t i = 0; i < 4; ++i) {
            const float currentSlope = getSlope(aGuessT, mX1, mX2);
            if (currentSlope == 0.0f) {
                return aGuessT;
            }
            const float currentX = calcBezier(aGuessT, mX1, mX2) - aX;
            aGuessT -= currentX / currentSlope;
        }
        return aGuessT;
    }

    static inline float getTForX(float aX, float mX1, float mX2, std::map<int, float> &&sampleValues) {
        float intervalStart = 0.0f;
        uint8_t currentSample = 0u;
        const uint8_t lastSample = kSplineTableSize - 1u;

        for (; currentSample != lastSample && sampleValues[currentSample] <= aX; ++currentSample) {
            intervalStart += kSampleStepSize;
        }
        --currentSample;

        // Interpolate to provide an initial guess for t
        const float dist =
                (aX - sampleValues[currentSample]) / (sampleValues[currentSample + 1u] - sampleValues[currentSample]);
        const float guessForT = intervalStart + dist * kSampleStepSize;

        const float initialSlope = getSlope(guessForT, mX1, mX2);
        if (initialSlope >= 0.001f) {
            return newtonRaphsonIterate(aX, guessForT, mX1, mX2);
        } else if (initialSlope == 0.0f) {
            return guessForT;
        } else {
            return binarySubdivide(aX, intervalStart, intervalStart + kSampleStepSize, mX1, mX2);
        }
    }

    inline auto getCubicBezierEasing(const float x1, const float y1, const float x2, const float y2) {
        // Precompute samples table
        std::map<int, float> sampleValues;
        if (x1 != y1 || x2 != y2) {
            for (int i = 0; i < kSplineTableSize; ++i) {
                sampleValues[i] = calcBezier(i * kSampleStepSize, x1, x2);
            }
        }

        auto func = [x1, y1, x2, y2, sampleValues = std::move(sampleValues)](const float dx) mutable -> float {
            if (x1 == y1 && x2 == y2) {
                return dx; // linear
            }
            return calcBezier(getTForX(dx, x1, x2, std::move(sampleValues)), y1, y2);
        };
        return func;
    }
}
