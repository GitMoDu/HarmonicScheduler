/* 
* Co-Op Task Blink example.
* Showcases Blink LED with dynamic task running on Harmonic scheduler.
* Enable USE_FIXED_TASK to try a leaner alternate implementation.
*/


//#define USE_FIXED_TASK 

#include <Arduino.h>
#include <HarmonicScheduler.h>


Harmonic::TemplateScheduler<1> Harmony{};

class BlinkDynamicTask final : public Harmonic::DynamicTask
{
public:
	BlinkDynamicTask(Harmonic::IScheduler& scheduler)
		: Harmonic::DynamicTask(scheduler)
	{
	}

	const bool Setup()
	{
		pinMode(LED_BUILTIN, OUTPUT);

		return AttachTask(500);
	}

	void Run() final
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
};

class BlinkFixedTask final : public Harmonic::FixedTask
{
public:
	BlinkFixedTask() : Harmonic::FixedTask() {}

	const bool Setup(Harmonic::IScheduler& scheduler)
	{
		pinMode(LED_BUILTIN, OUTPUT);

		return Harmonic::FixedTask::AttachTask(scheduler, 500);
	}

	void Run() final
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
};

#if defined(USE_FIXED_TASK)
BlinkFixedTask Blink{};
#else
BlinkDynamicTask Blink(Harmony);
#endif


void setup()
{
#if defined(USE_FIXED_TASK)
	Blink.Setup(Harmony);
#else
	Blink.Setup();
#endif
}

void loop()
{
	Harmony.Loop();
}