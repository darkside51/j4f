#pragma once

#include <chrono>
#include <ctime>
#include <cstdlib>

namespace engine {

	template <typename T = std::chrono::milliseconds>
	inline uint64_t steadyTime() {
		const auto now = std::chrono::steady_clock::now();
		auto now_time = std::chrono::time_point_cast<T>(now);
		return static_cast<uint64_t>(now_time.time_since_epoch().count());
	}

	template <typename T = std::chrono::milliseconds>
	inline uint64_t systemTime() {
		const auto now = std::chrono::system_clock::now();
		auto now_time = std::chrono::time_point_cast<T>(now);
		return static_cast<uint64_t>(now_time.time_since_epoch().count());
	}

	template <typename T = std::chrono::milliseconds>
	inline uint64_t highResoutionTime() {
		const auto now = std::chrono::high_resolution_clock::now();
		auto now_time = std::chrono::time_point_cast<T>(now);
		return static_cast<uint64_t>(now_time.time_since_epoch().count());
	}

	inline uint64_t unixUTCTime() {
#ifdef j4f_PLATFORM_WINDOWS
		// https://stackoverflow.com/questions/20370920/convert-current-time-from-windows-to-unix-timestamp-in-c-or-c
		// c++20 introduced a guarantee that time_since_epoch is relative to the UNIX epoch (but c++20)

		time_t ltime;
		tm timeinfo;

		time(&ltime); // get time
		gmtime_s(&timeinfo, &ltime); /* Convert to UTC */

		return static_cast<uint64_t>(mktime(&timeinfo));
#elif defined(j4f_PLATFORM_LINUX)
        return systemTime();
#endif
	}
}