/*
* Harmonic Scheduler Compatibility Benchmark.
* This is a test to benchmark compatibility TaskScheduler execution with tasks builts for TS:Scheduler.
*
* This test executes 1,000,000 cycles of a task with a counter.
* Enabling and disable the idle sleep, to assess impact on performance.
*
* Sample execution times (in milliseconds per 1M iterations) are provided below.
* The test board is Arduino UNO 16MHz processor.
*/

#include <Arduino.h>

// Replaces 
//#define _TASK_OO_CALLBACKS
//#include <TScheduler.hpp>
#include <HarmonicSchedulerCompatibility.h>

#include "BenchmarkTask.h"

static constexpr bool IdleSleep = true;

static constexpr uint32_t BenchmarkSize = 1000000;


// Global scheduler can still be explicit Harmonic one.
Harmonic::TemplateScheduler<1, IdleSleep> Runner{};

BenchmarkTask<> Benchmark(Runner);

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

	Serial.print(F("Start..."));

	if (!Benchmark.Setup())
	{
		error();
	}
}


void loop()
{
	Runner.Loop();
}