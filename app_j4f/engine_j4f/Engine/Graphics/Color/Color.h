#pragma once

#include "../../Core/Math/mathematic.h"
#include <cstdint>

namespace engine {

	template <typename T>
	inline T step(T&& edge, T&& x) noexcept {
		return (x < edge) ? T(0) : T(1);
	}

	inline vec3f rgb2hsv(const vec3f& c) noexcept {
		const vec4f k(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
		const vec4f p = glm::mix(vec4f(c.b, c.g, k.w, k.z), vec4f(c.g, c.b, k.x, k.y), step(c.b, c.g));
		const vec4f q = glm::mix(vec4f(p.x, p.y, p.w, c.r), vec4f(c.r, p.y, p.z, p.x), step(p.x, c.r));
		const float d = q.x - std::min(q.w, q.y);
		constexpr float e = 1.0e-10;
		return vec3f(std::abs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
	}

	inline vec3f hsv2rgb(const vec3f& c) noexcept {
		const vec4f k(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
		const vec3f p = glm::abs(glm::fract(vec3f(c.r) + vec3f(k.x, k.y, k.z)) * 6.0f - vec3f(k.w));
		return c.b * glm::mix(vec3f(k.x), glm::clamp(p - vec3f(k.x), vec3f(0.0f), vec3f(1.0f)), c.g);
	}

	inline vec3f rgb2hsl(const vec3f& c) noexcept {
		const float max = std::max(std::max(c.r, c.g), c.b);
		const float min = std::min(std::min(c.r, c.g), c.b);

		vec3f result((max + min) / 2.0f);

		if (max == min) {
			result.r = result.g = 0.0f;
		} else {
			const float d = max - min;
			result.g = (result.b > 0.5) ? d / (2.0f - max - min) : d / (max + min);

			if (max == c.r) {
				result.r = (c.g - c.b) / d + (c.g < c.b ? 6.0f : 0.0f);
			} else if (max == c.g) {
				result.r = (c.b - c.r) / d + 2.0f;
			} else if (max == c.b) {
				result.r = (c.r - c.g) / d + 4.0f;
			}

			result.r /= 6.0f;
		}

		return result;
	}

	inline float hue2rgb(float p, float q, float t) noexcept {
		if (t < 0.0f) {
			t += 1;
		}

		if (t > 1.0f) {
			t -= 1.0f;
		}

		if (t < 1.0f / 6.0f) {
			return p + (q - p) * 6.0f * t;
		}

		if (t < 1.0f / 2.0f) {
			return q;
		}

		if (t < 2.0f / 3.0f) {
			return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		}

		return p;
	}

	inline vec3f hsl2rgb(const vec3f& hsl) noexcept {
		vec3f result;
		if (hsl.y == 0.0f) {
			result.r = result.g = result.b = hsl.z;
		} else {
			const float q = hsl.z < 0.5f ? hsl.z * (1.0 + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
			const float p = 2.0f * hsl.z - q;
			result.r = hue2rgb(p, q, hsl.x + 1.0f / 3.0f) * 255.0f;
			result.g = hue2rgb(p, q, hsl.x) * 255.0f;
			result.b = hue2rgb(p, q, hsl.x - 1.0f / 3.0f) * 255.0f;
		}

		return result;
	}

	class Color {
	public:
		explicit Color(const vec4f rgba) noexcept {
			_rgba = static_cast<uint8_t>(rgba.r * 255.0f) << 24 |
					static_cast<uint8_t>(rgba.g * 255.0f) << 16 |
					static_cast<uint8_t>(rgba.b * 255.0f) << 8 |
					static_cast<uint8_t>(rgba.a * 255.0f) << 0;
		}

		explicit Color(const uint32_t rgba) noexcept : _rgba(rgba) {}

		inline vec4f toVec4() const noexcept {
			return {
					static_cast<float>((_rgba >> 24) & 0xff) / 255.0f,
					static_cast<float>((_rgba >> 16) & 0xff) / 255.0f,
					static_cast<float>((_rgba >> 8) & 0xff)	/ 255.0f,
					static_cast<float>((_rgba >> 0) & 0xff)	/ 255.0f
			};
		}

		inline operator uint32_t() const noexcept { return _rgba; }

		inline uint8_t r() const noexcept { return (_rgba >> 24) & 0xff; }
		inline uint8_t g() const noexcept { return (_rgba >> 16) & 0xff; }
		inline uint8_t b() const noexcept { return (_rgba >> 8) & 0xff; }
		inline uint8_t a() const noexcept { return (_rgba >> 0) & 0xff; }

	private:
		uint32_t _rgba;
	};

}