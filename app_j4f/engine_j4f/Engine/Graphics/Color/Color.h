#pragma once

#include "../../Core/Math/mathematic.h"
#include <cstdint>

namespace engine {

	template <typename T>
	inline T step(T&& edge, T&& x) noexcept {
		return (x < edge) ? T(0) : T(1);
	}

	inline glm::vec3 rgb2hsv(const glm::vec3& c) noexcept {
		const glm::vec4 k(0.0f, -1.0f / 3.0f, 2.0f / 3.0f, -1.0f);
		const glm::vec4 p = glm::mix(glm::vec4(c.b, c.g, k.w, k.z), glm::vec4(c.g, c.b, k.x, k.y), step(c.b, c.g));
		const glm::vec4 q = glm::mix(glm::vec4(p.x, p.y, p.w, c.r), glm::vec4(c.r, p.y, p.z, p.x), step(p.x, c.r));
		const float d = q.x - std::min(q.w, q.y);
		constexpr float e = 1.0e-10;
		return glm::vec3(std::abs(q.z + (q.w - q.y) / (6.0f * d + e)), d / (q.x + e), q.x);
	}

	inline glm::vec3 hsv2rgb(const glm::vec3& c) noexcept {
		const glm::vec4 k(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
		const glm::vec3 p = glm::abs(glm::fract(glm::vec3(c.r) + glm::vec3(k.x, k.y, k.z)) * 6.0f - glm::vec3(k.w));
		return c.b * glm::mix(glm::vec3(k.x), glm::clamp(p - glm::vec3(k.x), glm::vec3(0.0f), glm::vec3(1.0f)), c.g);
	}

	inline glm::vec3 rgb2hsl(const glm::vec3& c) noexcept {
		const float max = std::max(std::max(c.r, c.g), c.b);
		const float min = std::min(std::min(c.r, c.g), c.b);

		glm::vec3 result((max + min) / 2.0f);

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

	inline glm::vec3 hsl2rgb(const glm::vec3& hsl) noexcept {
		glm::vec3 result;
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
		explicit Color(const glm::vec4 rgba) noexcept {
			_rgba = static_cast<uint8_t>(rgba.r * 255.0f) << 24 |
					static_cast<uint8_t>(rgba.g * 255.0f) << 16 |
					static_cast<uint8_t>(rgba.b * 255.0f) << 8 |
					static_cast<uint8_t>(rgba.a * 255.0f) << 0;
		}

		explicit Color(const uint32_t rgba) noexcept : _rgba(rgba) {}

		inline glm::vec4 toVec4() const noexcept {
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