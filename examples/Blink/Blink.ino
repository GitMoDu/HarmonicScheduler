/*
* Harmonic Task Blink example.
* Showcases Blink LED with dynamic task running on Harmonic scheduler.
* Enable USE_STATIC_TASK to try a leaner alternate implementation.
*/


//#define USE_STATIC_TASK

#include <Arduino.h>
#include <HarmonicScheduler.h>




class BlinkStaticTask final : public Harmonic::ITask
{
public:
	BlinkStaticTask() : Harmonic::ITask() {}
	bool Setup(Harmonic::TaskRegistry& registry)
	{
		pinMode(LED_BUILTIN, OUTPUT);

		Harmonic::task_id_t taskId;

		return registry.AttachTask(this, taskId, 500);
	}

	void Run() final
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
};

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

		return AttachTask(500);
	}

	void Run() final
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
};

Harmonic::TemplateScheduler<1> Runner{};

#if defined(USE_STATIC_TASK)
BlinkStaticTask Blink{};
#else
BlinkDynamicTask Blink(Runner);
#endif


void setup()
{
#if defined(USE_STATIC_TASK)
	Blink.Setup(Runner);
#else
	Blink.Setup();
#endif
}

void loop()
{
	Runner.Loop();
}