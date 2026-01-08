/*
* Harmonic Scheduler Benchmark.
* This is a test to benchmark TaskScheduler execution.
*
* This test executes 1,000,000 cycles of a task with a counter.
* Enabling and disable the idle sleep, to assess impact on performance.
*
* Sample execution times (in milliseconds per 1M iterations) are provided below.
* The test board is Arduino UNO 16MHz processor.
* 
* Reference execution times in a Arduino UNO @ 16MHz (lower is better):
* ProfilerLevel | IdleSleep | SKIP_CHECKS | Duration (ms)
*  None         | Disabled  | Disabled    | 12575
*  None         | Enabled   | Disabled    | 13895
*  None         | Disabled  | Enabled     | 12575
*  None         | Enabled   | Enabled     | 13895
*  Base         | Disabled  | Disabled    | 28797
*  Base         | Enabled   | Disabled    | 30054
*  Base         | Disabled  | Enabled     | 28797
*  Base         | Enabled   | Enabled     | 30054
*  Full         | Disabled  | Disabled    | 34140
*  Full         | Enabled   | Disabled    | 34140
*  Full         | Disabled  | Enabled     | 34140
*  Full         | Enabled   | Enabled     | 34140
* 
*/


//#define HARMONIC_SKIP_CHECKS // Uncomment to skip safety checks.

#include <Arduino.h>

#include <HarmonicScheduler.h>


static constexpr bool IdleSleep = false;
static constexpr auto ProfileLevel = Harmonic::ProfileLevelEnum::Base;

static constexpr uint32_t BenchmarkSize = 1000000;

class BenchmarkTask : public Harmonic::DynamicTask
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
	BenchmarkTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{
	}

	bool Setup()
	{
		Count = 0;

		return Attach(0, true);
	}

	void Run() final
	{
		switch (State)
		{
		case StateEnum::Starting:
			Start = millis();
			State = StateEnum::Counting;
			break;
		case BenchmarkTask::StateEnum::Counting:
			Count++;
			if (Count >= BenchmarkSize)
			{
				State = StateEnum::Ended;
			}
			break;
		case BenchmarkTask::StateEnum::Ended:
			SetEnabled(false);
			OnEnd();
			break;
		default:
			break;
		}
	}

private:
	void OnStart()
	{
		Start = millis();
	}

	void OnEnd()
	{
		End = millis();

		Serial.println(F("done."));
		Serial.print(F("Tstart =")); Serial.println(Start);
		Serial.print(F("Tfinish=")); Serial.println(End);
		Serial.print(F("Duration=")); Serial.println(End - Start);
	}
};

Harmonic::TemplateScheduler<1, IdleSleep, ProfileLevel> Runner{};
BenchmarkTask Benchmark(Runner);

void error()
{
	Serial.print(F("Setup error."));
}

void setup()
{
	Serial.begin(115200);

	while (!Serial)
		;;

	delay(1000);

	if (!Benchmark.Setup())
	{
		error();
	}

	Serial.print(F("Start..."));
}


void loop()
{
	Runner.Loop();
}