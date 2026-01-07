#ifndef _HARMONIC_SCHEDULER_PROFILING_h
#define _HARMONIC_SCHEDULER_PROFILING_h

#include <stdint.h>

namespace Harmonic
{
	enum class ProfileLevelEnum : uint8_t
	{
		None = 0,
		Base = 1,
		Full = 2
	};

	namespace Profiling
	{
		struct TaskTrace
		{
			uint32_t Duration;
			uint32_t MaxDuration;
			uint32_t Iterations;
		};

		struct BaseTrace
		{
			uint32_t Iterations;
			uint32_t Scheduling;
			uint32_t Busy;
			uint32_t IdleSleep;
		};

		struct FullTrace
		{
			uint32_t Iterations;
			uint32_t Scheduling;
			uint32_t IdleSleep;
			uint8_t TaskCount;
		};

		struct IBaseProfiler
		{
			virtual bool GetTrace(BaseTrace& trace) = 0;
		};

		struct IFullProfiler
		{
			virtual bool GetTrace(FullTrace& trace, TaskTrace* tracesBuffer, const uint8_t maxTraces) = 0;
		};
	}
}
#endif