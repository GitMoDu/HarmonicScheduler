/*
* Harmonic Scheduler Benchmark.
* This is a test to benchmark TaskScheduler execution.
*
* This test executes 1,000,000 cycles of a task with empty callback method.
* Configured with different options, you can assess the impact  on the size of the Scheduler object
* and the execution overhead of the main execution loop.
*
* Sample execution times (in milliseconds per 1M iterations) are provided below.
* The test board is Arduino UNO 16MHz processor.
*/

#include <Arduino.h>
#include <HarmonicScheduler.h>

template<const uint32_t BenchmarkSize = 1000000>
class BenchmarkTask : public Harmonic::DynamicTask
{
private:
	enum class StateEnum
	{
		Starting,
		Counting,
		Ended
	};

protected:
	using DynamicTask::Harmony;

private:
	uint32_t Start = 0;
	uint32_t End = 0;
	uint32_t Count = 0;
	StateEnum State = StateEnum::Starting;

public:
	BenchmarkTask(Harmonic::IScheduler& scheduler)
		: Harmonic::DynamicTask(scheduler)
	{
	}

	const bool Setup()
	{
		Count = 0;

		return AttachTask(0, true);
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
			Harmonic::DynamicTask::SetEnabled(false);
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

		Serial.println("done.");
		Serial.print("Tstart ="); Serial.println(Start);
		Serial.print("Tfinish="); Serial.println(End);
		Serial.print("Duration="); Serial.println(End - Start);
	}
};

Harmonic::TemplateScheduler<1, true> Harmony{};
BenchmarkTask<> Benchmark(Harmony);

void error()
{
	Serial.print("Setup error.");
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

	Serial.print("Start...");
}


void loop()
{
	Harmony.Loop();
}