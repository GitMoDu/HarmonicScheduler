/*
* Harmonic Task Blink example.
* Showcases Blink LED task running on Harmonic scheduler.
* Example implementations for OOP, static, function, and lambda task variants.
* Enable one of the USE_X_TASK to use a specific variants.
*/

// Uncomment an option.
#define USE_OOP_TASK
//#define USE_STATIC_TASK
//#define USE_FUNCTION_TASK
//#define USE_LAMBDA_TASK

#include <Arduino.h>
#include <HarmonicScheduler.h>

Harmonic::TemplateScheduler<1> Runner{};

#if defined(USE_OOP_TASK)
// Object-oriented task: uses a class derived from Harmonic::DynamicTask.
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
} Blink(Runner);
#elif defined(USE_STATIC_TASK)
// Static task: uses a class derived from pure virtua Harmonic::ITask.
class BlinkStaticTask final : public Harmonic::ITask
{
public:
	BlinkStaticTask() : Harmonic::ITask() {}
	bool Setup(Harmonic::TaskRegistry& registry)
	{
		pinMode(LED_BUILTIN, OUTPUT);

		Harmonic::task_id_t taskId;

		return registry.Attach(this, 500, true);
	}

	// Ignore task ID updates for static tasks.
	void OnTaskIdUpdated(const Harmonic::task_id_t taskId)  final {}

	void Run() final
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
} Blink{};
#elif defined(USE_FUNCTION_TASK)
// Function pointer task: uses a statically allocated Harmonic::CallableTask to wrap a plain function.
void BlinkFunction()
{
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
}
Harmonic::CallableTask BlinkTask(Runner, BlinkFunction); // Statically allocated task for scheduled blink function.
#elif defined(USE_LAMBDA_TASK)
// Lambda task: uses a statically allocated Harmonic::CallableTask with a lambda (no captures).
Harmonic::CallableTask BlinkTask(Runner,
	[]() {
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle the LED state.
	}
);
#else
#error No option selected for task. Options: USE_OOP_TASK, USE_STATIC_TASK, USE_FUNCTION_TASK or USE_LAMBDA_TASK.
#endif


void setup()
{
	// Initialize and attach the selected task variant
#if defined(USE_OOP_TASK)
	Blink.Setup();
#elif defined(USE_STATIC_TASK)
	Blink.Setup(Runner);
#elif defined(USE_FUNCTION_TASK)
	BlinkTask.Attach(500, true); // Attach the function as a periodic task.
	pinMode(LED_BUILTIN, OUTPUT);
#elif defined(USE_LAMBDA_TASK)
	BlinkTask.Attach(500, true); // Attach the function as a periodic task.
	pinMode(LED_BUILTIN, OUTPUT);
#endif
}

void loop()
{
	Runner.Loop();
}