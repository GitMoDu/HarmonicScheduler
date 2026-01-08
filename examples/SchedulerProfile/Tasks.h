#ifndef _TASKS_h
#define _TASKS_h

#include <HarmonicScheduler.h>

class BlinkDynamicTask final : public Harmonic::DynamicTask
{
public:
	BlinkDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{
	}

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

class BusyDynamicTask final : public Harmonic::DynamicTask
{
public:
	BusyDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{
	}
	bool Setup()
	{
		return Attach(2, true);
	}

	void Run() final
	{
		// Simulate a busy task by performing a blocking delay.
		delayMicroseconds(500);
	}
};

class LightDynamicTask final : public Harmonic::DynamicTask
{
public:
	LightDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{
	}

	bool Setup()
	{
		//randomSeed(micros());

		return Attach(0, true);
	}

	void Run() final
	{
		// Simulate a light task by performing a short blocking delay.
		delayMicroseconds(200);

		// Set a new random period.	
		SetPeriod((random() % 100));
	}
};

class LongDynamicTask final : public Harmonic::DynamicTask
{
public:
	LongDynamicTask(Harmonic::TaskRegistry& registry)
		: Harmonic::DynamicTask(registry)
	{
	}

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

