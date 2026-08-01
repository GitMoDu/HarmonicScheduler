/*
* Harmonic Scheduler metrics profiling example.
* Demonstrates the Metrics profiling on System and Task profiling levels.
* IdleSleep can be enabled or disabled as needed.
* Timeline profiling is also available (#define USE_TIMELINE), falling back to Timeline-to-Metrics profiler,
* but requires a separate Timeline output task to aggregate the Metrics' results.
* Use #define USE_TIMELINE to enable timeline profiling.
* Optional NameProvider can be used to provide known task names for the profiler log output.
*
* Example output on Arduino Uno (Metrics profiling, Task level, IdleSleep enabled):
* ID		CPU(%)	CALLS	TIME(us)	MAX(us)
* BUSY		23				227268		974476
* IDLE		23				230888
* SLEEP		52				516320
* -------------------------------------------------------
* LOG		1		3		19188		19124
* BlinkTask	0		2		72			40
* BusyTask	12		239		126432		588
* LightTask	5		157		51440		348
* LongTask	3		3		30136		10056
*/

#include <Arduino.h>
#include <HarmonicScheduler.h>

//#define USE_TIMELINE // Uncomment to enable timeline profiling, otherwise metrics profiling is used.

#include "Tasks.h"


#if defined(USE_TIMELINE)
static constexpr Harmonic::ProfilerModeEnum ProfilerMode = Harmonic::ProfilerModeEnum::Timeline;
#else
static constexpr Harmonic::ProfilerModeEnum ProfilerMode = Harmonic::ProfilerModeEnum::Metrics;
#endif

// Scheduler configuration.
static constexpr Harmonic::ProfilerLevelEnum ProfilerLevel = Harmonic::ProfilerLevelEnum::Task;
static constexpr bool IdleSleep = false;
static constexpr uint32_t LogPeriod = 1000; // Log period in milliseconds.

// Enumerated tasks count, including the log task and timeline task if enabled.
static constexpr uint8_t MaxTaskCount = static_cast<uint8_t>(TaskIndexEnum::EnumCount);

// Templated scheduler based on the profiling mode, level and idle sleep settings.
Harmonic::TemplateScheduler<MaxTaskCount, IdleSleep, ProfilerMode, ProfilerLevel> Runner{};

// Appropriate trace log task based on profiling level.
Harmonic::Profiling::TemplateLogTask<MaxTaskCount, ProfilerMode, ProfilerLevel, LogPeriod> LogTask(Runner, Runner, Serial);

// Test tasks.
BlinkDynamicTask Blink(Runner);
BusyDynamicTask Busy(Runner);
LightDynamicTask Light(Runner);
LongDynamicTask Long(Runner);

// Task name provider for known task names (Optional).
EnumeratedTaskNameProvider NameProvider{};
//Harmonic::Profiling::CachedTaskNameRegistry<MaxTaskCount> NameProvider{};

void halt()
{
	Serial.println(F("Setup error!"));
	while (true)
	{
		delay(1000);
	}
}

void setup()
{
	// Start serial for logging.
	Serial.begin(115200);

	// Attach and start the profiling log task, with optional name provider.
	if (!LogTask.Start(&NameProvider))
		halt();

	// Attach test tasks.
	if (!Blink.Setup())
		halt();
	if (!Busy.Setup())
		halt();
	if (!Light.Setup())
		halt();
	if (!Long.Setup())
		halt();

	// Assign task handles to the name provider for known task names, after all tasks have been attached.
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Blink), Blink.GetTaskHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Busy), Busy.GetTaskHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Light), Light.GetTaskHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Long), Long.GetTaskHandle());

	// Assign the log task handles, depending on the profiling mode.
#if defined(USE_TIMELINE)
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::ProfilerLog), LogTask.GetLogTaskHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Timeline), LogTask.GetTaskHandle());
#else
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::ProfilerLog), LogTask.GetTaskHandle());
#endif


	Serial.println(F("Scheduler Profiler example started"));
	bool hasProfiling = false;
	Serial.println(F("Profiling"));
	Serial.print(F("\tMode: "));
	switch (ProfilerMode)
	{
	case Harmonic::ProfilerModeEnum::None:
		Serial.println(F("No profiling"));
		break;
	case Harmonic::ProfilerModeEnum::Metrics:
		Serial.println(F("Metrics"));
		hasProfiling = true;
		break;
	case Harmonic::ProfilerModeEnum::Timeline:
		Serial.println(F("Timeline"));
		hasProfiling = true;
		break;
	default:
		Serial.println(F("Unknown"));
		break;
	}

	if (hasProfiling)
	{
		Serial.print(F("\tLevel: "));
		switch (ProfilerLevel)
		{
		case Harmonic::ProfilerLevelEnum::System:
			Serial.println(F("System"));
			break;
		case Harmonic::ProfilerLevelEnum::Task:
			Serial.println(F("System + Tasks"));
			break;
		default:
			Serial.println(F("Unknown"));
			break;
		}
	}
}

void loop()
{
	Runner.Loop();
}