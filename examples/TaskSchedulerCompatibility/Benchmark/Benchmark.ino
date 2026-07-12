/*
* Harmonic Scheduler Compatibility Benchmark.
* This is a test to benchmark compatibility TaskScheduler execution with tasks builts for TS:Scheduler.
*
* This test executes 1,000,000 cycles of a task with a counter.
* Enabling and disable the idle sleep, to assess impact on performance.
*
* Sample execution times (in milliseconds per 1M iterations) are provided below.
* The test board is Arduino UNO 16MHz processor.
*
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


#define HARMONIC_SKIP_CHECKS // Uncomment to skip safety checks.

#include <Arduino.h>

#include <HarmonicScheduler.h>
#include "BenchmarkTask.h"


static constexpr bool IdleSleep = false;
static constexpr auto ProfileLevel = Harmonic::ProfileLevelEnum::None;

static constexpr uint32_t BenchmarkSize = 1000000;


Harmonic::TemplateScheduler<1, IdleSleep, ProfileLevel> Runner{};

// Re-implementation Scheduler_example10_Benchmark using compatibility wrapper.
//BenchmarkTaskOop<BenchmarkSize> Benchmark(Runner);

// Alternative implementation using DynamicTask, which is more flexible and slightly faster.
BenchmarkTaskDynamic<BenchmarkSize> Benchmark(Runner);

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