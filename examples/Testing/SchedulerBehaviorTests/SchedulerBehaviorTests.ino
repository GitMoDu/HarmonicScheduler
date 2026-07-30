/*
 * Harmonic Scheduler Behaviour Tests
 *
 * This sketch runs a set of tests to verify the behavior of the Harmonic Scheduler library.
 * It checks task management, timing accuracy, and interrupt handling.
 * On some platforms (AVR, STM32, etc...), hardware timer interrupt wake is also tested.
 *
 * Toggle the #define HARMONIC_SKIP_CHECKS to enable/disable safety checks.
 * Toggle IdleSleep to test idle sleep behavior.
 * Switch ProfilerMode to test different profiling modes (None, Metrics, Timeline).
 * Switch ProfilerLevel to test different profiling levels (System, Task).
 * 
 * All combinations must pass for full verification.
 */

 //#define HARMONIC_SKIP_CHECKS

#include <Arduino.h>
#include <HarmonicScheduler.h>
#include "TestInterface.h"
#include "TestTasks.h"
#include "TestCoordinatorTask.h"

// Configuration: profiling mode, profiling level, and idle sleep behavior.
static constexpr Harmonic::ProfilerModeEnum ProfilerMode = Harmonic::ProfilerModeEnum::Timeline;
static constexpr Harmonic::ProfilerLevelEnum ProfilerLevel = Harmonic::ProfilerLevelEnum::Task;
static constexpr bool IdleSleep = false;

// Number of test tasks in this suite.
static constexpr auto TestCount = 26;

// Main scheduler instance, manages all test tasks and the coordinator.
Harmonic::TemplateScheduler<TestCount + 1, IdleSleep, ProfilerMode, ProfilerLevel, 64> Runner{};

// Coordinator task: orchestrates execution and reporting of all test tasks.
Harmonic::TestCoordinatorTask<TestCount> TestCoordinator(Runner);

// Instantiate each test task, each one checks a specific scheduler feature.
Harmonic::TestTasks::TestTaskAttachOnConstructor Test1(Runner);
Harmonic::TestTasks::TestTaskAttachOnStart Test2(Runner);
Harmonic::TestTasks::TestTaskEnableDisable Test3(Runner);
Harmonic::TestTasks::TestTaskAttachPeriod Test4(Runner);
Harmonic::TestTasks::TestTaskDelayedEnablePeriod Test5(Runner);
Harmonic::TestTasks::TestTaskImmediateWake Test6(Runner);
Harmonic::TestTasks::TestTaskPeriodicToggle Test7(Runner);
Harmonic::TestTasks::TestTaskIsrWake Test8(Runner);
Harmonic::TestTasks::TestTaskDisableBeforeRun Test9(Runner);
Harmonic::TestTasks::TestTaskReattach Test10(Runner);
Harmonic::TestTasks::TestTaskZeroPeriod Test11(Runner);
Harmonic::TestTasks::TestTaskMaxPeriod Test12(Runner);
Harmonic::TestTasks::TestTaskRapidToggle Test13(Runner);
Harmonic::TestTasks::TestTaskDetachRegistered Test14(Runner);
Harmonic::TestTasks::TestTaskDetachUnregistered Test15(Runner);
Harmonic::TestTasks::TestTaskDetachReattach Test16(Runner);
Harmonic::TestTasks::TestTaskDoubleDetach Test17(Runner);
Harmonic::TestTasks::TestTaskDetachThenSetProperties Test18(Runner);
Harmonic::TestTasks::TestTaskOverrunHandling Test19(Runner);
Harmonic::TestTasks::TestTaskHandleCompaction Test20(Runner);
Harmonic::TestTasks::TestTaskHandleReuseIsolation Test21(Runner);
Harmonic::TestTasks::TestTaskClearInvalidatesHandles Test22(Runner);
Harmonic::TestTasks::TestTaskAttachAfterClearResetsAllocation Test23(Runner);
Harmonic::TestTasks::TestTaskHandleWraparoundAllocation Test24(Runner);
Harmonic::TestTasks::TestTaskStableHandleRoutingAfterReuse Test25(Runner);
Harmonic::TestTasks::TestTaskInvalidHandleSafetyAfterClear Test26(Runner);


void error()
{
	Serial.print(F("Setup error."));
	while (true)
		;;
}

void setup()
{
	Serial.begin(115200);

	// Wait for serial connection (for boards that require it).
	while (!Serial)
		;;

	// Register all test tasks with the coordinator.
	if (!TestCoordinator.AddTestTask(&Test1)
		|| !TestCoordinator.AddTestTask(&Test2)
		|| !TestCoordinator.AddTestTask(&Test3)
		|| !TestCoordinator.AddTestTask(&Test4)
		|| !TestCoordinator.AddTestTask(&Test5)
		|| !TestCoordinator.AddTestTask(&Test6)
		|| !TestCoordinator.AddTestTask(&Test7)
		|| !TestCoordinator.AddTestTask(&Test8)
		|| !TestCoordinator.AddTestTask(&Test9)
		|| !TestCoordinator.AddTestTask(&Test10)
		|| !TestCoordinator.AddTestTask(&Test11)
		|| !TestCoordinator.AddTestTask(&Test12)
		|| !TestCoordinator.AddTestTask(&Test13)
		|| !TestCoordinator.AddTestTask(&Test14)
		|| !TestCoordinator.AddTestTask(&Test15)
		|| !TestCoordinator.AddTestTask(&Test16)
		|| !TestCoordinator.AddTestTask(&Test17)
		|| !TestCoordinator.AddTestTask(&Test18)
		|| !TestCoordinator.AddTestTask(&Test19)
		|| !TestCoordinator.AddTestTask(&Test20)
		|| !TestCoordinator.AddTestTask(&Test21)
		|| !TestCoordinator.AddTestTask(&Test22)
		|| !TestCoordinator.AddTestTask(&Test23)
		|| !TestCoordinator.AddTestTask(&Test24)
		|| !TestCoordinator.AddTestTask(&Test25)
		|| !TestCoordinator.AddTestTask(&Test26)
		)
	{
		Serial.print(F("Task Setup failed."));
		error();
	}

	// Test task 8 uses Timer ISR to wake up the task.
	Test8.SetInterruptCallback(InterruptCallback);

	// Start the test coordinator; halt on failure.
	if (!TestCoordinator.Start())
	{
		Serial.println(F("TestCoordinator start failed."));
		error();
	}

	Serial.println(F("Scheduler Behaviour Tests"));

#if defined(HARMONIC_SKIP_CHECKS)
	Serial.println(F("\tOptimizations: Enabled"));
#else
	Serial.println(F("\tOptimizations: Disabled"));
#endif

	if (IdleSleep)
		Serial.println(F("\tIdle Sleep: Enabled"));
	else
		Serial.println(F("\tIdle Sleep: Disabled"));

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
	Serial.println();

	delay(1000);

	Serial.println(F("Tests Start..."));
}

void loop()
{
	Runner.Loop();
}

#if defined(ARDUINO_ARCH_AVR)
ISR(TIMER1_COMPA_vect) // This ISR is triggered by Timer1 compare match and calls the test's OnIsr method.
{
	Test8.OnIsr();
}
void InterruptCallback() {} // Dummy callback, ISR is handled by the AVR ISR above.
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
void InterruptCallback() // Timer2 ISR handler for Maple Mini
{
	Test8.OnIsr();
}
#else
void InterruptCallback() {} // Dummy callback for unsupported platforms.
#endif
