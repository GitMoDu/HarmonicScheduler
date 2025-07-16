/*
 * Harmonic Scheduler Behaviour Tests
 *
 * This sketch runs a set of tests to verify the behavior of the Harmonic Scheduler library.
 * It checks task management, timing accuracy, and interrupt handling.
 *
 *
 * On AVR platforms, hardware timer interrupt wake is also tested.
 */

#include <Arduino.h>
#include <HarmonicScheduler.h>
#include "TestInterface.h"
#include "TestTasks.h"
#include "TestCoordinatorTask.h"

// Number of test tasks in this suite.
static constexpr auto TestCount = 8;

// Main scheduler instance, manages all tasks (including coordinator).
Harmonic::TemplateScheduler<TestCount + 1, false> Runner{};

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

	delay(1000);

	Serial.print(F("Task Tests Start..."));

	delay(1000);

	// Register all test tasks with the coordinator.
	TestCoordinator.AddTestTask(&Test1);
	TestCoordinator.AddTestTask(&Test2);
	TestCoordinator.AddTestTask(&Test3);
	TestCoordinator.AddTestTask(&Test4);
	TestCoordinator.AddTestTask(&Test5);
	TestCoordinator.AddTestTask(&Test6);
	TestCoordinator.AddTestTask(&Test7);
	TestCoordinator.AddTestTask(&Test8);

	// Start the test coordinator; halt on failure.
	if (!TestCoordinator.Start())
	{
		error();
	}
}

void loop()
{
	Runner.Loop();
}

#if defined(ARDUINO_ARCH_AVR)
// Test task 8 uses Timer1 ISR to wake up the task.
// This ISR is triggered by Timer1 compare match and calls the test's OnIsr method.
ISR(TIMER1_COMPA_vect)
{
	Test8.OnIsr();
}
#endif