#pragma once

#ifdef _DEBUG
	#ifndef PROFILER_ENABLE
		#define PROFILER_ENABLE
	#endif // !PROFILER_ENABLE
#endif

#include "../../Time/Time.h"
#include "../../Log/Log.h"
#include <string>

namespace engine {

	class ExecutionTime {
	public:
		explicit ExecutionTime(const char* name, uint64_t* t = nullptr) : _startTime(steadyTime<std::chrono::microseconds>()), _outTime(t), _name(name) { }
		explicit ExecutionTime(const std::string& name, uint64_t* t = nullptr) : _startTime(steadyTime<std::chrono::microseconds>()), _outTime(t), _name(name.c_str()) { }

		~ExecutionTime() {
			const uint64_t time = steadyTime<std::chrono::microseconds>() - _startTime;
			if (_outTime) {
				*_outTime = time;
			} else {
				//LOG_TAG_LEVEL(engine::LogLevel::L_DEBUG, TIME_PROFILER, "[{}(mcs)]\t {}", time, _name);
				if (_info) {
					LOG_TAG_LEVEL(engine::LogLevel::L_DEBUG, TIME_PROFILER, "[%d(mcs)]\t %s::%s", time, _name, _info);
				} else {
					LOG_TAG_LEVEL(engine::LogLevel::L_DEBUG, TIME_PROFILER, "[%d(mcs)]\t %s", time, _name);
				}
				
			}
		}

		inline void modify(const std::string& m) { _info = m.c_str(); }

	private:
		uint64_t _startTime;
		uint64_t* _outTime;
		const char* _name = nullptr;
		const char* _info = nullptr;
	};

}

#ifdef PROFILER_ENABLE
#define PROFILE_TIME_SCOPED(x) ExecutionTime x(STR(x));
#define PROFILE_TIME_SCOPED_M(x, m) ExecutionTime et_##x(STR(x)); et_##x.modify(m);
#define PROFILE_TIME_ENTER_SCOPE(x) ExecutionTime *et_##x = new ExecutionTime(STR(x));
#define PROFILE_TIME_ENTER_SCOPE_M(x, m) ExecutionTime *et_##x = new ExecutionTime(STR(x)); et_##x->modify(m);
#define PROFILE_TIME_LEAVE_SCOPE(x) delete et_##x;
#else // PROFILER_ENABLE
#define PROFILE_TIME_SCOPED(x)
#define PROFILE_TIME_SCOPED_M(x, m)
#define PROFILE_TIME_ENTER_SCOPE(x)
#define PROFILE_TIME_ENTER_SCOPE_M(x, m)
#define PROFILE_TIME_LEAVE_SCOPE(x)
#endif // PROFILER_ENABLE