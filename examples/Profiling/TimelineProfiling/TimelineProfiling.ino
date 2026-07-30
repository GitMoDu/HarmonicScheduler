/*
* Harmonic Scheduler timeline profiling example, with output to Serial.
* Continuous timeline trace stream to output (BufferedSerialOutputTask),
* or one-shot dump of timeline samples (OneShotSerialOutputTask) to output.
* Output can be visualized using the provided TimelineViewer.html under HarmonicScheduler\src\Profiling.
* Select the ProfileLevel to ProfilerLevelEnum::System or ProfilerLevelEnum::Task.
* IdleSleep can be enabled or disabled as needed. WARNING: generates a lot of trace samples due to micro-sleeps.
* Optional NameProvider can be used to provide known task names for the profiler log output.
*
* See Timeline.png for a sample timeline output visualization.
* Example ASCII timeline output:
*
* 350.00 ms        400.00 ms        450.00 ms        500.00 ms        550.00 ms
*                   |                |                |                |                |
* BlinkTask   -------+----------------+----------------+----------------+--- | ----------+
* BusyTask    ||||||| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
* LightTask   --| |---||-|--|-||---|-|-|-|-|--|-|-|-||-|-|-|-|-|--|-|-|--||---|-|-||--|---
* LongTask    -------[#####]----------+----------------+----------------+----------------+
* TIMELINE    ||||||| |||||__|||_||_||__|_||___|_|_||_||||_||_|||||__||_||||__|_|_|____|||
* TRACE       ||||||| |____|____|____|____|____|____|____|____|____|____|____|____|____|__
* SLEEP       ||||||| |_||_||_||__||_|_||_||_||_||_|||_|_|||||_|||_||_||_||_|||_|_|_|||__|
* SCHEDULER   ||__||| _____|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
*/

#include <Arduino.h>
#include <HarmonicScheduler.h>

//#define USE_TIMELINE
#include "Tasks.h"

// Configure the profiling level for this example, System or Task.
static constexpr Harmonic::ProfilerLevelEnum ProfileLevel = Harmonic::ProfilerLevelEnum::Task;

// Serial output must be fast enough, otherwise the trace buffer might overflow and samples will be dropped.
static constexpr bool IdleSleep = true;

// Select the serial output type for timeline trace output.
auto& SerialOutput = Serial;
using SerialOutputType = decltype(SerialOutput);

// Fast baud rate is required otherwise the timeline trace output won't keep up with the scheduler and will drop samples.
#if defined(ARDUINO_ARCH_AVR)
#if F_CPU >= 16000000L
static constexpr uint32_t BaudRate = 1000000;
#else 
static constexpr uint32_t BaudRate = 500000;
#endif
#else
static constexpr uint32_t BaudRate = 1000000;
#endif

/// <summary>
/// Timeline trace sample count default.
/// Must be large enough to hold all samples for a complete scheduler iteration, including all tasks and idle sleep samples.
/// </summary>
static constexpr size_t TraceSampleCount = 24;


// Used tasks are enumerated.
static constexpr Harmonic::task_handle_t MaxTaskCount = static_cast<Harmonic::task_handle_t>(TaskIndexEnum::EnumCount);

// Templated scheduler based on the profiling level and idle sleep settings.
Harmonic::TemplateScheduler<MaxTaskCount, IdleSleep, Harmonic::ProfilerModeEnum::Timeline, ProfileLevel, TraceSampleCount> Runner{};

// Select the timeline output task type. 
// Buffered output continuously streams timeline samples to the serial output.
Harmonic::Profiling::Timeline::TemplateBufferedSerialOutputTask<ProfileLevel, SerialOutputType> TimelineOutput(Runner, Runner, SerialOutput);

// One-shot accumulates timeline samples until the output buffer is full, then dumps all samples to the serial output in one shot.
//Harmonic::Profiling::Timeline::TemplateOneShotSerialOutputTask<ProfileLevel, SerialOutputType> TimelineOutput(Runner, Runner, SerialOutput);

// Test tasks.
BlinkDynamicTask Blink(Runner);
BusyDynamicTask Busy(Runner);
LightDynamicTask Light(Runner);
LongDynamicTask Long(Runner);

// Task name provider for known task names (Optional).
EnumeratedTaskNameProvider NameProvider{};

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
	// Start serial for trace output.
	// Fast baud rate is required otherwise the timeline trace output won't keep up with the scheduler and will drop samples.
	Serial.begin(BaudRate);
	while (!Serial)
		;;

	// Attach test tasks.
	if (!Blink.Setup())
		halt();
	if (!Busy.Setup())
		halt();
	if (!Light.Setup())
		halt();
	if (!Long.Setup())
		halt();

	// Attach the selected output task.
	if (!TimelineOutput.Setup())
		halt();

	// Assign task handles to the name provider for known task names.
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Blink), Blink.GetHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Busy), Busy.GetHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Light), Light.GetHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Long), Long.GetHandle());
	NameProvider.SetTaskHandle(Harmonic::task_handle_t(TaskIndexEnum::Timeline), TimelineOutput.GetHandle());

	// Start the trace output.
	if (!TimelineOutput.Start(&NameProvider))
		halt();
}

void loop()
{
	Runner.Loop();
}