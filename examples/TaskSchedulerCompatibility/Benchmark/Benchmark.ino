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
* Compat	| None		| X			| Disabled	| 20057
* Compat	| None		| X			| Enabled	| 20748
* Compat	| Metrics	| System	| Disabled	| 34832
* Compat	| Metrics	| System	| Enabled	| 35587
* Compat	| Metrics	| Task		| Disabled	| 38038
* Compat	| Metrics	| Task		| Enabled	| 40113
* Compat	| Timeline	| System	| Disabled	| 31042
* Compat	| Timeline	| System	| Enabled	| 33561
* Compat	| Timeline	| Task		| Disabled	| 33938
* Compat	| Timeline	| Task		| Enabled	| 34751
* 
* Dynamic	| None		| X			| Disabled	| 9117
* Dynamic	| None		| X			| Enabled	| 9683
* Dynamic	| Metrics	| System	| Disabled	| 23829
* Dynamic	| Metrics	| System	| Enabled	| 24458
* Dynamic	| Metrics	| Task		| Disabled	| 27099
* Dynamic	| Metrics	| Task		| Enabled	| 29049
* Dynamic	| Timeline	| System	| Disabled	| 20103
* Dynamic	| Timeline	| System	| Enabled	| 22495
* Dynamic	| Timeline	| Task		| Disabled	| 22999
* Dynamic	| Timeline	| Task		| Enabled	| 23684
* _____________________________________________________________
* 
*/


#include <Arduino.h>

#include <HarmonicScheduler.h>
#include "BenchmarkTask.h"


static constexpr auto ProfileMode = Harmonic::ProfilerModeEnum::None;
static constexpr auto ProfileLevel = Harmonic::ProfilerLevelEnum::System;
static constexpr bool IdleSleep = false;

static constexpr uint32_t BenchmarkSize = 1000000;


Harmonic::TemplateScheduler<1, IdleSleep, ProfileMode, ProfileLevel> Runner{};

// Re-implementation Scheduler_example10_Benchmark using compatibility wrapper.
BenchmarkTaskCompatibility<BenchmarkSize> Benchmark(Runner);

// Alternative implementation using native DynamicTask.
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