#ifndef _TASKS_h
#define _TASKS_h

#include <HarmonicScheduler.h>

// Enumerated task indices for known tasks.
enum class TaskIndexEnum : Harmonic::task_handle_t
{
	Blink,
	Busy,
	Light,
	Long,
	Timeline,
	EnumCount
};

/// <summary>
/// Provides task names for enumerated tasks. 
/// Handles are assigned after tasks have been attached to the scheduler.
/// </summary>
class EnumeratedTaskNameProvider : public Harmonic::Profiling::VirtualIndexedTaskNameProvider<static_cast<Harmonic::task_handle_t>(TaskIndexEnum::EnumCount)>
{
public:
	EnumeratedTaskNameProvider() : Harmonic::Profiling::VirtualIndexedTaskNameProvider<static_cast<Harmonic::task_handle_t>(TaskIndexEnum::EnumCount)>()
	{}

protected:
	virtual const char* GetIndexedTaskName(const Harmonic::task_index_t index) const override
	{
		switch (static_cast<TaskIndexEnum>(index))
		{
		case TaskIndexEnum::Blink:
			return "BlinkTask";
		case TaskIndexEnum::Busy:
			return "BusyTask";
		case TaskIndexEnum::Light:
			return "LightTask";
		case TaskIndexEnum::Long:
			return "LongTask";
		case TaskIndexEnum::Timeline:
			return "TIMELINE";
		default:
			return "Unknown";
		}
	}
};

/// <summary>
/// Example blink task that toggles the built-in LED on and off at a fixed interval.
/// </summary>
class BlinkDynamicTask final : public Harmonic::DynamicTask
{
public:
	BlinkDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{}

	bool Setup()
	{
		pinMode(LED_BUILTIN, OUTPUT);

		return Attach(500, true);
	}

	void Run() final
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
};

/// <summary>
/// Example busy task that simulates a blocking operation by performing a short delay in its Run method.
/// </summary>
class BusyDynamicTask final : public Harmonic::DynamicTask
{
public:
	BusyDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{}

	bool Setup()
	{
		return Attach(4, true);
	}

	void Run() final
	{
		// Simulate a busy task by performing a blocking delay.
		delayMicroseconds(500);
	}
};

/// <summary>
/// Example light task that performs a short blocking delay and sets a new random period.
/// </summary>
class LightDynamicTask final : public Harmonic::DynamicTask
{
public:
	LightDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{}

	bool Setup()
	{
		return Attach(0, true);
	}

	void Run() final
	{
		// Simulate a light task by performing a short blocking delay.
		delayMicroseconds(200);

		// Set a new random period.	
		SetDelay((random(11) + 1));
	}
};

/// <summary>
/// Example long task that performs a blocking delay.
/// </summary>
class LongDynamicTask final : public Harmonic::DynamicTask
{
public:
	LongDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{}

	bool Setup()
	{
		return Attach(333, true);
	}

	void Run() final
	{
		// Simulate a long task by performing a blocking delay.
		delay(10);
	}
};
#endif