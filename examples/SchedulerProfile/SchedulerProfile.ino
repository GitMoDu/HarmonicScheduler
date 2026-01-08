/*
* Harmonic Scheduler profiling example.
* Demonstrates full profiling with a trace log task.
* Switch the ProfileLevel to Full, Base or None to see different profiling levels.
* IdleSleep can be enabled or disabled as needed.
*/

#include <Arduino.h>
#include <HarmonicScheduler.h>
#include "Tasks.h"

// Scheduler configuration.
static constexpr Harmonic::ProfileLevelEnum ProfileLevel = Harmonic::ProfileLevelEnum::Base;
static constexpr bool IdleSleep = true;
static constexpr uint8_t MaxTaskCount = 10;

// Templated scheduler based on the profiling level and idle sleep setting.
Harmonic::TemplateScheduler<MaxTaskCount, IdleSleep, ProfileLevel> Runner{};

// Appropriate trace log task based on profiling level.
Harmonic::TemplateTraceLogTask<MaxTaskCount, ProfileLevel, 2000> LogTask(Runner, Runner, Serial);

// Test tasks.
BlinkDynamicTask Blink(Runner);
BusyDynamicTask Busy(Runner);
LightDynamicTask Light(Runner);
LongDynamicTask Long(Runner);

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

	// Attach and start the profiling log task.
	if (!LogTask.Start())
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

	Serial.println(F("Scheduler Profiler example started"));

	Serial.print(F("Profiling Level: "));
	switch (ProfileLevel)
	{
	case Harmonic::ProfileLevelEnum::None:
		Serial.println(F("None"));
		break;
	case Harmonic::ProfileLevelEnum::Base:
		Serial.println(F("Base"));
		break;
	case Harmonic::ProfileLevelEnum::Full:
	default:
		Serial.println(F("Full"));
		break;
	}
}

void loop()
{
	Runner.Loop();
}