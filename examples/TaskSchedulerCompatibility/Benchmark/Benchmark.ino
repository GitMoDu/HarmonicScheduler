/*
* Harmonic Scheduler Compatibility Benchmark.
* This is a test to benchmark compatibility TaskScheduler execution with tasks builts for TS:Scheduler.
*
* This test executes 1,000,000 cycles of a task with a counter.
* Profile mode, level and idle sleep can be configured to assess impact on performance.
*
* Sample execution times (in milliseconds per 1M iterations) are provided below.
* The test board is Arduino UNO 16MHz processor.
* 
* TaskScheduler Reference Benchmark
* Measured with v4.0.8
* _________________________
* IdleSleep	| Duration (ms)
* Disabled	| 20558
* Enabled	| 26721
* _________________________
*
* 
* Harmonic Scheduler Benchmark
* Timeline does not include output to listener.
* _____________________________________________________________
* TaskType	| Mode		| Level		| IdleSleep	| Duration (ms)
* Compat	| None		| X			| Disabled	| 12889
* Compat	| None		| X			| Enabled	| 14084
* Compat	| Metrics	| System	| Disabled	| 28481
* Compat	| Metrics	| System	| Enabled	| 30179
* Compat	| Metrics	| Task		| Disabled	| 32380
* Compat	| Metrics	| Task		| Enabled	| 35335
* Compat	| Timeline	| System	| Disabled	| 26273
* Compat	| Timeline	| System	| Enabled	| 27472
* Compat	| Timeline	| Task		| Disabled	| 27975
* Compat	| Timeline	| Task		| Enabled	| 29231
* Dynamic	| None		| X			| Disabled	| 10751
* Dynamic	| None		| X			| Enabled	| 11946
* Dynamic	| Metrics	| System	| Disabled	| 26344
* Dynamic	| Metrics	| System	| Enabled	| 28042
* Dynamic	| Metrics	| Task		| Disabled	| 30243
* Dynamic	| Metrics	| Task		| Enabled	| 33197
* Dynamic	| Timeline	| System	| Disabled	| 21926
* Dynamic	| Timeline	| System	| Enabled	| 24948
* Dynamic	| Timeline	| Task		| Disabled	| 25388
* Dynamic	| Timeline	| Task		| Enabled	| 26644
* _____________________________________________________________
* 
*/


#include <Arduino.h>

#include <HarmonicScheduler.h>
#include "BenchmarkTask.h"


static constexpr bool IdleSleep = false;
static constexpr auto ProfileMode = Harmonic::ProfilerModeEnum::None;
static constexpr auto ProfileLevel = Harmonic::ProfilerLevelEnum::System;

static constexpr uint32_t BenchmarkSize = 1000000;


Harmonic::TemplateScheduler<1, IdleSleep, ProfileMode, ProfileLevel> Runner{};

// Re-implementation Scheduler_example10_Benchmark using compatibility wrapper.
BenchmarkTaskCompatibility<BenchmarkSize> Benchmark(Runner);

// Alternative implementation using DynamicTask, which is more flexible and slightly faster.
//BenchmarkTaskDynamic<BenchmarkSize> Benchmark(Runner);

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