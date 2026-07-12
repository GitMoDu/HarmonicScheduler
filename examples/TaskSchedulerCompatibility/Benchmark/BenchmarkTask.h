#ifndef _BENCHMARKTASK_h
#define _BENCHMARKTASK_h

// Replaces //#define _TASK_OO_CALLBACKS //#include <TSchedulerDeclarations.hpp>
#include <HarmonicSchedulerCompatibility.h>

/// <summary>
/// OOP re-implementation of TaskScheduler Scheduler_example10_Benchmark task behaviour.
/// </summary>
/// <typeparam name="BenchmarkSize"></typeparam>
template<uint32_t BenchmarkSize = 1000000>
class BenchmarkTaskOop : public TS::Task
{
private:
	uint32_t Start = 0;
	uint32_t End = 0;

public:
	BenchmarkTaskOop(TS::Scheduler& scheduler)
		: TS::Task(TASK_IMMEDIATE, BenchmarkSize, &scheduler, false)
	{}

	bool Setup()
	{
		return TS::Task::enable();
	}

	bool Callback() final
	{
		return true;
	}

protected:
	bool OnEnable() final
	{
		Start = millis();
		End = 0;

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

/// <summary>
/// Native DynamicTask based implementation of the benchmark task.
/// </summary>
/// <typeparam name="BenchmarkSize"></typeparam>
template<uint32_t BenchmarkSize = 1000000>
class BenchmarkTaskDynamic : public Harmonic::DynamicTask
{
private:
	enum class StateEnum
	{
		Starting,
		Counting,
		Ended
	};

private:
	uint32_t Start = 0;
	uint32_t End = 0;
	uint32_t Count = 0;
	StateEnum State = StateEnum::Starting;

public:
	BenchmarkTaskDynamic(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{}

	bool Setup()
	{
		Count = 0;

		return Attach(0, true) != Harmonic::TASK_INVALID_HANDLE;
	}

	void Run() final
	{
		switch (State)
		{
		case StateEnum::Starting:
			Start = millis();
			End = 0;
			State = StateEnum::Counting;
			break;
		case StateEnum::Counting:
			Count++;
			if (Count > BenchmarkSize)
			{
				State = StateEnum::Ended;
				SetEnabled(false);
				OnEnd();
			}
			break;
		case StateEnum::Ended:
		default:
			break;
		}
	}

private:
	void OnEnd()
	{
		End = millis();

		Serial.println(F("done."));
		Serial.print(F("Tstart =")); Serial.println(Start);
		Serial.print(F("Tfinish=")); Serial.println(End);
		Serial.print(F("Duration=")); Serial.println(End - Start);
	}
};
#endif

