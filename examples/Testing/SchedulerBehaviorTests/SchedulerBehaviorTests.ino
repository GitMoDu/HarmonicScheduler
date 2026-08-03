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
static constexpr Harmonic::ProfilerModeEnum ProfilerMode = Harmonic::ProfilerModeEnum::None;
static constexpr Harmonic::ProfilerLevelEnum ProfilerLevel = Harmonic::ProfilerLevelEnum::Task;
static constexpr bool IdleSleep = false;

// Number of test tasks in this suite.
static constexpr auto TestCount = 29
#if defined(HARMONIC_TEST_HAS_FREERTOS_TIMER) || (!defined(HARMONIC_PLATFORM_RTOS) && !defined(HARMONIC_PLATFORM_OS))
+ 1
#endif
#if defined(HARMONIC_TEST_HAS_FREERTOS_TASK)
+1
#endif
;
static constexpr uint8_t SchedulerCapacity = 3; // Coordinator + active test + periodic/helper task.

// Main scheduler instance, executes all test tasks and the coordinator.
Harmonic::TemplateScheduler<SchedulerCapacity, IdleSleep, ProfilerMode, ProfilerLevel> Runner{};

// Coordinator task: orchestrates execution and reporting of all test tasks.
Harmonic::TestCoordinatorTask<TestCount> TestCoordinator(Runner);

// Instantiate each test task, each one checks a specific scheduler feature.
Harmonic::TestTasks::TestTaskAttachOnConstructor Test1(Runner);
Harmonic::TestTasks::TestTaskAttachOnStart Test2(Runner);
Harmonic::TestTasks::TestTaskEnableDisable Test3(Runner);
Harmonic::TestTasks::TestTaskAttachDelay Test4(Runner);
Harmonic::TestTasks::TestTaskDelayedEnableDelay Test5(Runner);
Harmonic::TestTasks::TestTaskImmediateWake Test6(Runner);
Harmonic::TestTasks::TestTaskRepeatedDelayToggle Test7(Runner);
#if defined(HARMONIC_TEST_HAS_FREERTOS_TIMER)
Harmonic::TestTasks::TestTaskRtosTimerWake Test8(Runner);
#elif !defined(HARMONIC_PLATFORM_RTOS) && !defined(HARMONIC_PLATFORM_OS)
Harmonic::TestTasks::TestTaskIsrWake Test8(Runner);
#endif
#if defined(HARMONIC_TEST_HAS_FREERTOS_TASK)
Harmonic::TestTasks::TestTaskRtosPreemptiveWake Test30(Runner);
#endif
Harmonic::TestTasks::TestTaskDisableBeforeRun Test9(Runner);
Harmonic::TestTasks::TestTaskReattach Test10(Runner);
Harmonic::TestTasks::TestTaskZeroDelay Test11(Runner);
Harmonic::TestTasks::TestTaskMaxDelay Test12(Runner);
Harmonic::TestTasks::TestTaskRapidToggle Test13(Runner);
Harmonic::TestTasks::TestTaskDetachRegistered Test14(Runner);
Harmonic::TestTasks::TestTaskDetachUnregistered Test15(Runner);
Harmonic::TestTasks::TestTaskDetachReattach Test16(Runner);
Harmonic::TestTasks::TestTaskDoubleDetach Test17(Runner);
Harmonic::TestTasks::TestTaskDetachThenSetProperties Test18(Runner);
Harmonic::TestTasks::TestTaskSchedulerOverrunHandling Test19(Runner);
Harmonic::TestTasks::TestTaskPeriodicPhaseLock Test20(Runner);
Harmonic::TestTasks::TestTaskPeriodicReanchor Test21(Runner);
Harmonic::TestTasks::TestTaskHandleCompaction Test22(Runner);
Harmonic::TestTasks::TestTaskHandleReuseIsolation Test23(Runner);
Harmonic::TestTasks::TestTaskClearInvalidatesHandles Test24(Runner);
Harmonic::TestTasks::TestTaskAttachAfterClearResetsAllocation Test25(Runner);
Harmonic::TestTasks::TestTaskHandleWraparoundAllocation Test26(Runner);
Harmonic::TestTasks::TestTaskStableHandleRoutingAfterReuse Test27(Runner);
Harmonic::TestTasks::TestTaskInvalidHandleSafetyAfterClear Test28(Runner);
Harmonic::TestTasks::TestTaskTrackerBoundary Test29(Runner);


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
	if (false
		|| !TestCoordinator.AddTestTask(&Test1)
		|| !TestCoordinator.AddTestTask(&Test2)
		|| !TestCoordinator.AddTestTask(&Test3)
		|| !TestCoordinator.AddTestTask(&Test4)
		|| !TestCoordinator.AddTestTask(&Test5)
		|| !TestCoordinator.AddTestTask(&Test6)
		|| !TestCoordinator.AddTestTask(&Test7)
#if defined(HARMONIC_TEST_HAS_FREERTOS_TIMER) || (!defined(HARMONIC_PLATFORM_RTOS) && !defined(HARMONIC_PLATFORM_OS))
		|| !TestCoordinator.AddTestTask(&Test8)
#else
		// No timer or hardware interrupt source is available on this platform.
#endif
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
		|| !TestCoordinator.AddTestTask(&Test27)
		|| !TestCoordinator.AddTestTask(&Test28)
		|| !TestCoordinator.AddTestTask(&Test29)
#if defined(HARMONIC_TEST_HAS_FREERTOS_TASK)
		|| !TestCoordinator.AddTestTask(&Test30)
#endif
		)
	{
		Serial.print(F("Task Setup failed."));
		error();
	}

	// Test task 8 uses the platform-specific asynchronous wake source.
#if !defined(HARMONIC_TEST_HAS_FREERTOS_TIMER) && !defined(HARMONIC_PLATFORM_RTOS) && !defined(HARMONIC_PLATFORM_OS)
	Test8.SetInterruptCallback(InterruptCallback);
#endif

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

#if defined(HARMONIC_PLATFORM_RTOS) || defined(HARMONIC_PLATFORM_OS)
#else
	delay(1000);
#endif

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
#elif (defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)) && !defined(HARMONIC_PLATFORM_RTOS)
void InterruptCallback()
{
	Test8.OnIsr();
}
#else
void InterruptCallback() {} // Dummy callback for unsupported platforms.
#endif
