// BenchmarkTask.h

#ifndef _BENCHMARKTASK_h
#define _BENCHMARKTASK_h

// Replaces
//#define _TASK_OO_CALLBACKS
//#include <TSchedulerDeclarations.hpp>
#include <HarmonicSchedulerCompatibility.h>

template<uint32_t BenchmarkSize = 1000000>
class BenchmarkTask : public TS::Task
{
private:
	uint32_t Start = 0;
	uint32_t End = 0;
	uint32_t Count = 0;

public:
	BenchmarkTask(TS::Scheduler& scheduler)
		: TS::Task(TASK_IMMEDIATE, TASK_FOREVER, &scheduler, false)
	{
	}

	bool Setup()
	{
		Count = 0;

		return TS::Task::enable();
	}

	bool Callback() final
	{
		Count++;
		if (Count >= BenchmarkSize)
		{
			TS::Task::disable();
		}

		return true;
	}

protected:
	bool OnEnable() final
	{
		Start = millis();

		return true;
	}

	void OnDisable() final
	{
		End = millis();

		Serial.println(F("done."));
		Serial.print(F("Tstart =")); Serial.println(Start);
		Serial.print(F("Tfinish=")); Serial.println(End);
		Serial.print(F("Duration=")); Serial.println(End - Start);
	}
};

#endif

