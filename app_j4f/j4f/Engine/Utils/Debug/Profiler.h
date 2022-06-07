#pragma once

#ifdef _DEBUG
	#define PROFILER_ENABLE
#endif

#ifdef PROFILER_ENABLE

#define PROFILE_TIME_SCOPED(x) ExecutionTime x(STR(x));
#define PROFILE_TIME_SCOPED_M(x, m) ExecutionTime x(STR(x)); x.modifyName(m);
#define PROFILE_TIME_ENTER_SCOPE(x) ExecutionTime *x = new ExecutionTime(STR(x));
#define PROFILE_TIME_ENTER_SCOPE_M(x, m) ExecutionTime *x = new ExecutionTime(STR(x)); x->modifyName(m);
#define PROFILE_TIME_LEAVE_SCOPE(x) delete x;

#include "../../Time/Time.h"
#include "../../Log/Log.h"
#include <string>

namespace engine {

	class ExecutionTime {
	public:
		explicit ExecutionTime(const char* name) : _startTime(steadyTime()), _name(name) { }
		explicit ExecutionTime(std::string&& name) : _startTime(steadyTime()), _name(name) { }
		explicit ExecutionTime(const std::string& name) : _startTime (steadyTime()), _name(name) { }

		~ExecutionTime() {
			const uint64_t time = steadyTime() - _startTime;
			LOG_TAG(TIME_PROFILER, "[%d(ms)] \t %s", time, _name.c_str());
		}

		inline void modifyName(const std::string& m) { _name += "::" + m; }

	private:
		uint64_t _startTime;
		std::string _name;
	};

}

#else // PROFILER_ENABLE
#define PROFILE_TIME_SCOPED(x)
#define PROFILE_TIME_SCOPED_M(x, m)
#define PROFILE_TIME_ENTER_SCOPE(x)
#define PROFILE_TIME_ENTER_SCOPE_M(x, m)
#define PROFILE_TIME_LEAVE_SCOPE(x)
#endif // PROFILER_ENABLE