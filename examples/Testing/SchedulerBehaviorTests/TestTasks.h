#ifndef _TESTTASKS_h
#define _TESTTASKS_h

#include <HarmonicScheduler.h>
#include "TestInterface.h"
#include "TestTimer.h"


namespace Harmonic
{
	namespace TestTasks
	{
		// Centralized timing tolerances for all tests
		struct TimingTolerance
		{
#if defined(ARDUINO_ARCH_AVR)
			static constexpr uint8_t ToleranceScale = 2 / (F_CPU / 8000000);
#else
			static constexpr uint8_t ToleranceScale = 1;
#endif
			static constexpr int32_t BootMinMicros = -749 * ToleranceScale;
			static constexpr int32_t BootMaxMicros = 1550 * ToleranceScale;
			static constexpr int32_t PeriodicMicros = 999 * ToleranceScale;
			static constexpr uint32_t PeriodicAverageMicros = 1749 * ToleranceScale;
			static constexpr uint32_t ImmediateWakeMicros = 499 * ToleranceScale;
			static constexpr int32_t IsrWakeMicros = 150 * ToleranceScale;
			static constexpr int32_t ZeroDelayMicros = 999 * ToleranceScale;
		};

		class HandleProbeTask : public ExposedDynamicTask
		{
		public:
			HandleProbeTask(TaskRegistry& registry)
				: ExposedDynamicTask(registry)
			{}

			void Run() final {}

			task_handle_t AttachWithHandle(const uint32_t delay, const bool enabled)
			{
				if (Attach(delay, enabled))
				{
					return GetTaskHandle();
				}
				else
				{
					return TASK_INVALID_HANDLE;
				}
			}
		};

		static constexpr task_handle_t SharedHandleProbeCapacity = 4;
		static Platform::TaskTracker SharedHandleProbeTaskList[SharedHandleProbeCapacity]{};
		static uint8_t SharedHandleProbeHandleToSlot[SharedHandleProbeCapacity]{};
		static TaskRegistry SharedHandleProbeRegistry(SharedHandleProbeTaskList, SharedHandleProbeHandleToSlot, SharedHandleProbeCapacity, false);
		static HandleProbeTask SharedFirstHandleProbeTask(SharedHandleProbeRegistry);
		static HandleProbeTask SharedSecondHandleProbeTask(SharedHandleProbeRegistry);
		static HandleProbeTask SharedThirdHandleProbeTask(SharedHandleProbeRegistry);
		static HandleProbeTask SharedFourthHandleProbeTask(SharedHandleProbeRegistry);

		// Base class for test tasks based on DynamicTask, managing ITestTask listener.
		class AbstractTestTask : public ITestTask, public DynamicTask
		{
		protected:
			ITester* TestListener = nullptr;

		public:
			AbstractTestTask(TaskRegistry& registry)
				: ITestTask()
				, DynamicTask(registry)
			{}

			void StartTest(ITester* testListener)
			{
				TestListener = testListener;
			}
		};

		class AbstractPeriodicTestTask : public ITestTask, protected PeriodicTask
		{
		protected:
			ITester* TestListener = nullptr;

		public:
			AbstractPeriodicTestTask(TaskRegistry& registry)
				: ITestTask()
				, PeriodicTask(registry)
			{}

			void StartTest(ITester* testListener)
			{
				TestListener = testListener;
			}
		};

		// Tests that a task attached in the constructor is registered and can be enabled.
		class TestTaskAttachOnConstructor : public AbstractTestTask
		{
		public:
			TestTaskAttachOnConstructor(TaskRegistry& registry) : AbstractTestTask(registry)
			{
				Attach(0, false);
			}

			void StartTest(ITester* testListener) final
			{
				if (Registry.TaskExists(this) && !IsEnabled())
				{
					AbstractTestTask::StartTest(testListener);
					SetEnabled(true);
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void PrintName() final
			{
				Serial.print(F("TestTaskAttachOnConstructor"));
			}

			void Run() final
			{
				const bool pass = Registry.TaskExists(this) && IsEnabled();
				SetEnabled(false);
				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests that a task can be attached and enabled at the start of the test.
		class TestTaskAttachOnStart : public AbstractTestTask
		{
		public:
			TestTaskAttachOnStart(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void StartTest(ITester* testListener) final
			{
				if (Attach(0, true))
				{
					AbstractTestTask::StartTest(testListener);
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void PrintName() final
			{
				Serial.print(F("TestTaskAttachOnStart"));
			}

			void Run() final
			{
				const bool pass = Registry.TaskExists(this) && IsEnabled();
				SetEnabled(false);

				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests enabling and disabling a task after attachment.
		class TestTaskEnableDisable : public AbstractTestTask
		{
		public:
			TestTaskEnableDisable(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskEnableDisable"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(0, false) && !IsEnabled())
				{
					SetEnabled(true);
					SetDelay(0);
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				bool enabled = IsEnabled();
				SetEnabled(false);
				bool disabled = !IsEnabled();
				const bool pass = enabled && disabled;

				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests attaching a task with a specific delay and verifying its run timing.
		class TestTaskAttachDelay : public AbstractTestTask
		{
		private:
			static constexpr uint32_t TargetDelayMillis = 1111;

			uint32_t StartTimestamp = 0;

		public:
			TestTaskAttachDelay(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskAttachDelay"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(TargetDelayMillis, true))
				{
					StartTimestamp = Platform::GetProfilerTimestamp();
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = Platform::GetProfilerTimestamp();

				SetEnabled(false);
				const uint32_t runDelay = runTimestamp - StartTimestamp;
				const int32_t delayErrorMicros = (int32_t)(runDelay)-(int32_t)(TargetDelayMillis * 1000);
				const bool pass = (delayErrorMicros >= TimingTolerance::BootMinMicros)
					&& (delayErrorMicros <= TimingTolerance::BootMaxMicros);

				Serial.print(F("\tTask delay error "));
				Serial.print(delayErrorMicros);
				Serial.print(F(" out of "));
				Serial.print(TargetDelayMillis * 1000);
				Serial.println(F("us"));

				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests enabling a task and setting its delay after a delay.
		class TestTaskDelayedEnableDelay : public AbstractTestTask
		{
		private:
			static constexpr uint32_t TargetDelayMillis = 1111;
			uint32_t StartTimestamp = 0;

		public:
			TestTaskDelayedEnableDelay(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskDelayedEnableDelay"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(0, false))
				{
					StartTimestamp = Platform::GetProfilerTimestamp();
					SetEnabled(true);
					SetDelayFromNow(TargetDelayMillis);
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = Platform::GetProfilerTimestamp();

				SetEnabled(false);
				const uint32_t runDelay = runTimestamp - StartTimestamp;
				const int32_t delayErrorMicros = (int32_t)(runDelay)-(int32_t)(TargetDelayMillis * 1000);
				const bool pass = (delayErrorMicros >= TimingTolerance::BootMinMicros)
					&& (delayErrorMicros <= TimingTolerance::BootMaxMicros);

				Serial.print(F("\tTask delay error "));
				Serial.print(delayErrorMicros);
				Serial.print(F(" out of "));
				Serial.print(TargetDelayMillis * 1000);
				Serial.println(F("us"));

				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests repeated delay toggling and timing accuracy over multiple runs.
		class TestTaskRepeatedDelayToggle : public AbstractTestTask
		{
		private:
			static constexpr uint32_t ToggleDelayMillis = 20;
			static constexpr int32_t MaxToggles = 32;

			int64_t TotalDelayErrorMicros = 0;
			uint32_t ToggleStartTimestamp = 0;
			int32_t BootDelayErrorMicros = 0;
			int32_t ToggleCount = -1;

		public:
			TestTaskRepeatedDelayToggle(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskRepeatedDelayToggle"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				ToggleCount = -1;
				TotalDelayErrorMicros = 0;
				BootDelayErrorMicros = 0;
				// Ready to start toggling.
				ToggleStartTimestamp = Platform::GetProfilerTimestamp();
				if (!Attach(ToggleDelayMillis, true))
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = Platform::GetProfilerTimestamp();

				if (ToggleCount < 0)
				{
					// Set on first run to align with scheduler tick.
					BootDelayErrorMicros = (runTimestamp - ToggleStartTimestamp) - (ToggleDelayMillis * 1000);
					ToggleStartTimestamp = runTimestamp;

					const bool pass = (BootDelayErrorMicros >= TimingTolerance::BootMinMicros)
						&& (BootDelayErrorMicros <= TimingTolerance::BootMaxMicros);

					if (pass)
					{
						ToggleCount = 0;
					}
					else
					{
						Serial.print(F("\tBootDelayErrorMicros "));
						Serial.println(BootDelayErrorMicros);
						SetEnabled(false);
						if (TestListener)
							TestListener->OnTestTaskDone(false);
					}
				}
				else
				{
					const uint32_t runDelay = runTimestamp - ToggleStartTimestamp;
					ToggleStartTimestamp = runTimestamp;
					const int32_t delayErrorMicros = (int32_t)(runDelay)-(int32_t)(ToggleDelayMillis * 1000);

					TotalDelayErrorMicros += delayErrorMicros;

					const int32_t averageDelayErrorMicros = TotalDelayErrorMicros / (ToggleCount + 1);
					const uint32_t averageAbs = averageDelayErrorMicros >= 0 ? averageDelayErrorMicros : -averageDelayErrorMicros;

					const bool pass = (delayErrorMicros >= -TimingTolerance::BootMaxMicros)
						&& (delayErrorMicros <= TimingTolerance::BootMaxMicros)
						&& averageAbs <= TimingTolerance::PeriodicAverageMicros;

					if (pass)
					{
						ToggleCount++;
						if (ToggleCount >= MaxToggles)
						{

							Serial.print(F("\tTask boot delay error "));
							Serial.print(BootDelayErrorMicros);
							Serial.println(F("us"));
							Serial.print(F("\tTask repeated-delay average error "));
							Serial.print(averageDelayErrorMicros);
							Serial.println(F("us"));

							SetEnabled(false);
							if (TestListener)
								TestListener->OnTestTaskDone(true);
						}
					}
					else
					{
						Serial.print(F("\tdelayErrorMicros "));
						Serial.println(delayErrorMicros);
						Serial.print(F("\taverageAbs "));
						Serial.println(averageAbs);

						SetEnabled(false);
						if (TestListener)
							TestListener->OnTestTaskDone(false);
					}
				}

			}
		};

		// Tests immediate wake functionality, simulating an ISR wake.
		class TestTaskImmediateWake : public AbstractTestTask
		{
		private:
			uint32_t StartTimestamp = 0;

		public:
			TestTaskImmediateWake(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskImmediateWake"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (Attach(12345679, false))
				{
					StartTimestamp = Platform::GetProfilerTimestamp();
					// Simulate an immediate wake from ISR
					WakeNow();
				}
				else
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = Platform::GetProfilerTimestamp();
				const uint32_t runDelay = runTimestamp - StartTimestamp;
				const bool pass = runDelay <= TimingTolerance::ImmediateWakeMicros;

				Serial.print(F("\tTask wake delay "));
				Serial.print(runDelay);
				Serial.println(F("us"));

				SetEnabled(false);
				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests waking a task from an actual hardware ISR (Timer1).
		class TestTaskIsrWake : public AbstractTestTask
		{
		private:
			// Cross-platform test timer implementation (platform-specific behavior inside TestTimer.h)
			TestTimer Timer;

			uint32_t StartTimestamp = 0;
			void (*InterruptCallback)(void) = nullptr; // Function pointer for external ISR callback

			volatile uint32_t InterruptTimestamp = 0;
			volatile bool WokenFromIsr = false;

		public:
			TestTaskIsrWake(TaskRegistry& registry)
				: AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskIsrWake"));
			}

			void SetInterruptCallback(void (*callback)(void))
			{
				InterruptCallback = callback;
				Timer.SetCallback(callback);
			}

			void OnIsr()
			{
				InterruptTimestamp = Platform::GetProfilerTimestamp();
				// Do not call DisableTimer() from ISR/callback context - defer cleanup to task context.
				WokenFromIsr = true;
				WakeNow();
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				// Attach the test task with a delay large enough to allow the timer to fire later.
				if (!Attach((TestTimer::ExpectedDurationMicros / 1000) * 2, true))
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
					return;
				}

				DisableTimer();
				WokenFromIsr = false;
				// Set-up timer for delayed wake from ISR.
				StartTimestamp = Platform::GetProfilerTimestamp();
				if (!SetupTimerInterrupt())
				{
					// If the platform has no hardware timer, skip the ISR test at runtime.
					Serial.println(F("\tWARNING: ISR Test not performed, timer start failed."));
					if (testListener)
						testListener->OnTestTaskDone(true);
					return;
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = Platform::GetProfilerTimestamp();

				if (WokenFromIsr)
				{
					// Cleanup timer from task context to avoid calling potentially unsafe
					// platform APIs from ISR/callback context.
					DisableTimer();
					uint32_t wakeDelay;
					Platform::AtomicGuard guard;
					{
						wakeDelay = runTimestamp - InterruptTimestamp;
					}

					const uint32_t runDelay = runTimestamp - StartTimestamp;
					const int32_t delayErrorMicros = int32_t(runDelay) - int32_t(TestTimer::ExpectedDurationMicros);
					const bool pass = delayErrorMicros >= -TimingTolerance::IsrWakeMicros && delayErrorMicros <= TimingTolerance::IsrWakeMicros;

					Serial.print(F("\tTask interrupt delay error "));
					Serial.print(delayErrorMicros);
					Serial.println(F("us"));
					Serial.print(F("\tTask interrupt wake delay "));
					Serial.print(wakeDelay);
					Serial.println(F("us"));

					SetEnabled(false);
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
				else
				{
					Serial.println(F("\tTask interrupt didn't fire in time."));
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
			}

		private:
			void DisableTimer()
			{
				// Delegate to cross-platform TestTimer implementation
				Timer.Disable();
			}

			bool SetupTimerInterrupt()
			{
				// Ensure TestTimer will invoke the configured interrupt callback.
				Timer.SetCallback(InterruptCallback);
				int32_t intervalMs = int32_t(TestTimer::ExpectedDurationMicros / 1000);
				return Timer.SetupMs((uint32_t)intervalMs);
			}

		};

		// Test disabling a task before it ever runs.
		class TestTaskDisableBeforeRun : public AbstractTestTask
		{
		public:
			TestTaskDisableBeforeRun(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskDisableBeforeRun"));
			}

			void StartTest(ITester* testListener) final
			{
				// Invalid-handle safety checks are only guaranteed when runtime checks are enabled.
#if defined(HARMONIC_SKIP_CHECKS)
				AbstractTestTask::StartTest(testListener);
				Serial.println(F("\tSkip-check mode: detached-handle safety bypassed"));
				if (TestListener)
					TestListener->OnTestTaskDone(true);
#else
				AbstractTestTask::StartTest(testListener);
				if (Attach(10, true))
				{
					SetEnabled(false);
					const bool pass = !IsEnabled();
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
				else
				{
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
#endif
			}

			void Run() final
			{
				// Should never be called if disabled before run
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Test re-attaching a task after detaching (should fail or be handled gracefully).
		class TestTaskReattach : public AbstractTestTask
		{
		private:
			bool AttachedOnce = false;

		public:
			TestTaskReattach(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskReattach"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (!AttachedOnce)
				{
					AttachedOnce = Attach(10, true);
					if (AttachedOnce)
					{
						// Try to attach again, should pass and update the delay and enabled state
						const bool pass = Attach(20, true)
							&& Registry.TaskExists(this)
							&& IsEnabled()
							&& GetDelay() == 20;

						if (TestListener)
							TestListener->OnTestTaskDone(pass);
					}
					else
					{
						if (TestListener)
							TestListener->OnTestTaskDone(false);
					}
				}
			}

			void Run() final
			{
				SetEnabled(false);
			}
		};

		// Test attaching a task with zero delay and verify it runs as fast as possible.
		class TestTaskZeroDelay : public AbstractTestTask
		{
		private:
			static constexpr uint8_t TargetRunCount = 8;
			uint32_t StartTimestamp = 0;
			uint8_t RunCount = 0;

		public:
			TestTaskZeroDelay(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskZeroDelay"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				RunCount = 0;
				if (Attach(0, true))
				{
					StartTimestamp = Platform::GetProfilerTimestamp();
				}
				else
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				RunCount++;
				if (RunCount >= TargetRunCount)
				{
					const uint32_t runTimestamp = Platform::GetProfilerTimestamp();
					const uint32_t runDelay = runTimestamp - StartTimestamp;
					const bool pass = runDelay < TimingTolerance::ZeroDelayMicros;

					Serial.print(F("\tTask zero delay duration "));
					Serial.print(runDelay);
					Serial.println(F("us"));

					SetEnabled(false);
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
			}
		};

		// Test attaching a task with maximum allowed delay and verify correct scheduling.
		class TestTaskMaxDelay : public AbstractTestTask
		{
		private:
			static constexpr uint32_t MaxDelayMillis = UINT32_MAX;
			uint32_t StartTimestamp = 0;

		public:
			TestTaskMaxDelay(TaskRegistry& registry)
				: AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskMaxDelay"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (Attach(MaxDelayMillis, true))
				{
					// Just verify attach succeeds.
					const bool pass = IsEnabled() && Registry.TaskExists(this);
					SetEnabled(false);
					testListener->OnTestTaskDone(pass);
				}
				else
				{
					testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				SetEnabled(false);
			}
		};

		// Tests rapid toggling of the enabled state to stress scheduler state transitions.
		// This test repeatedly enables and disables the task in quick succession,
		// verifying that the scheduler and registry remain consistent and do not
		// enter an invalid state due to frequent state changes.
		class TestTaskRapidToggle : public AbstractTestTask
		{
		private:
			static constexpr uint16_t MaxToggles = 1000; // Number of enable/disable cycles to perform
			uint16_t ToggleCount = 0;
			bool AllStatesCorrect = true;

		public:
			TestTaskRapidToggle(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskRapidToggle"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				ToggleCount = 0;
				AllStatesCorrect = true;
				// Attach with a short delay to allow rapid toggling
				if (!Attach(2, true))
				{
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				// Toggle enabled state on each run
				const bool shouldBeEnabled = (ToggleCount % 2 == 0);
				SetEnabled(shouldBeEnabled);

				// Check if the enabled state matches expectation
				const bool actualEnabled = IsEnabled();
				if (actualEnabled == shouldBeEnabled)
				{
					SetEnabled(true);
					ToggleCount++;
					if (ToggleCount >= MaxToggles)
					{
						SetEnabled(false);
						if (TestListener)
							TestListener->OnTestTaskDone(AllStatesCorrect);
					}
				}
				else
				{
					AllStatesCorrect = false;
					Serial.print(F("\tToggle error at count "));
					Serial.print(ToggleCount);
					Serial.print(F(": expected "));
					Serial.print(shouldBeEnabled);
					Serial.print(F(", got "));
					Serial.println(actualEnabled);

					SetEnabled(false);
					if (TestListener)
						TestListener->OnTestTaskDone(AllStatesCorrect);
				}
			}
		};

		// Tests detaching a registered task and verifies it is removed from the registry.
		class TestTaskDetachRegistered : public AbstractTestTask
		{
		public:
			TestTaskDetachRegistered(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskDetachRegistered"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (Attach(10, true) && GetTaskHandle() != TASK_INVALID_HANDLE)
				{
					Detach();
					const bool detached = !Registry.TaskExists(this) && GetTaskHandle() == TASK_INVALID_HANDLE;
					const bool pass = detached && !Registry.TaskExists(this) && GetTaskHandle() == TASK_INVALID_HANDLE;
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
				else
				{
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				// Should never run after detachment
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests detaching an unregistered task and expects graceful failure.
		class TestTaskDetachUnregistered : public AbstractTestTask
		{
		public:
			TestTaskDetachUnregistered(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskDetachUnregistered"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				// Detach without attaching first
				Detach();
				const bool pass = !Registry.TaskExists(this) && GetTaskHandle() == TASK_INVALID_HANDLE;
				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				// Should never run if not attached
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests detaching and then re-attaching a task to ensure registry consistency.
		class TestTaskDetachReattach : public AbstractTestTask
		{
		private:
			bool AttachedOnce = false;
			bool DetachedOnce = false;

		public:
			TestTaskDetachReattach(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskDetachReattach"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (!AttachedOnce)
				{
					AttachedOnce = Attach(10, true);
					if (AttachedOnce && GetTaskHandle() != TASK_INVALID_HANDLE)
					{
						Detach();
						DetachedOnce = Registry.TaskExists(this) == false && GetTaskHandle() == TASK_INVALID_HANDLE;
						if (DetachedOnce
							&& !Registry.TaskExists(this)
							&& GetTaskHandle() == TASK_INVALID_HANDLE)
						{
							// Try to re-attach
							const bool reattached = Attach(20, true);
							const bool pass = reattached && Registry.TaskExists(this) && IsEnabled();
							if (TestListener)
								TestListener->OnTestTaskDone(pass);
						}
						else
						{
							if (TestListener)
								TestListener->OnTestTaskDone(false);
						}
					}
					else
					{
						if (TestListener)
							TestListener->OnTestTaskDone(false);
					}
				}
			}

			void Run() final
			{
				SetEnabled(false);
			}
		};

		// Tests detaching a task, then calling Detach again to ensure idempotency.
		class TestTaskDoubleDetach : public AbstractTestTask
		{
		public:
			TestTaskDoubleDetach(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskDoubleDetach"));
			}

			void StartTest(ITester* testListener) final
			{
#if defined(HARMONIC_SKIP_CHECKS)
				// In skip-check mode, detached-handle safety is intentionally not enforced.
				AbstractTestTask::StartTest(testListener);
				if (TestListener)
					TestListener->OnTestTaskDone(true);
#else
				AbstractTestTask::StartTest(testListener);
				if (Attach(10, true))
				{
					Detach();
					bool firstDetach = !Registry.TaskExists(this) && GetTaskHandle() == TASK_INVALID_HANDLE;
					Detach();
					bool secondDetach = !Registry.TaskExists(this) && GetTaskHandle() == TASK_INVALID_HANDLE;
					const bool pass = firstDetach && secondDetach && GetTaskHandle() == TASK_INVALID_HANDLE && !Registry.TaskExists(this);
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
				else
				{
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
#endif
			}

			void Run() final
			{
				// Should never run after detachment
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests detaching a task and then attempting to enable or set delay (should be no-op).
		class TestTaskDetachThenSetProperties : public AbstractTestTask
		{
		public:
			TestTaskDetachThenSetProperties(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskDetachThenSetProperties"));
			}

			void StartTest(ITester* testListener) final
			{
#if defined(HARMONIC_SKIP_CHECKS)
				AbstractTestTask::StartTest(testListener);
				if (TestListener)
					TestListener->OnTestTaskDone(true);
#else
				AbstractTestTask::StartTest(testListener);
				if (Attach(10, true))
				{
					Detach();
					bool detached = !Registry.TaskExists(this) && GetTaskHandle() == TASK_INVALID_HANDLE;
					SetEnabled(true);
					SetDelay(123);
					SetDelay(456);
					const bool pass = detached
						&& !IsEnabled()
						&& GetDelay() == UINT32_MAX
						&& GetTaskHandle() == TASK_INVALID_HANDLE
						&& !Registry.TaskExists(this);
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
				else
				{
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
#endif
			}

			void Run() final
			{
				// Should never run after detachment
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests that a surviving handle still targets the same task after slot compaction.
		class TestTaskHandleCompaction : public AbstractTestTask
		{
		public:
			TestTaskHandleCompaction(TaskRegistry& registry)
				: AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleCompaction"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				SharedHandleProbeRegistry.Clear();

				const task_handle_t firstHandle = SharedFirstHandleProbeTask.AttachWithHandle(10, false);
				const task_handle_t secondHandle = SharedSecondHandleProbeTask.AttachWithHandle(20, false);
				bool pass = firstHandle != TASK_INVALID_HANDLE
					&& secondHandle != TASK_INVALID_HANDLE;

				SharedFirstHandleProbeTask.Detach();
				pass &= SharedFirstHandleProbeTask.GetTaskHandle() == TASK_INVALID_HANDLE
					&& !SharedHandleProbeRegistry.TaskExists(&SharedFirstHandleProbeTask)
					&& SharedHandleProbeRegistry.TaskExists(&SharedSecondHandleProbeTask)
					&& SharedHandleProbeRegistry.GetDelay(secondHandle) == 20
					&& !SharedHandleProbeRegistry.IsEnabled(secondHandle);

				if (pass)
				{
					SharedHandleProbeRegistry.SetDelay(secondHandle, 30);
					SharedHandleProbeRegistry.SetEnabled(secondHandle, true);
					pass = SharedSecondHandleProbeTask.GetTaskHandle() == secondHandle
						&& SharedSecondHandleProbeTask.GetDelay() == 30
						&& SharedSecondHandleProbeTask.IsEnabled();
				}

				SharedSecondHandleProbeTask.Detach();

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests that handle reuse does not disturb another live task's handle after compaction.
		class TestTaskHandleReuseIsolation : public AbstractTestTask
		{
		public:
			TestTaskHandleReuseIsolation(TaskRegistry& registry)
				: AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleReuseIsolation"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				SharedHandleProbeRegistry.Clear();

				const task_handle_t firstHandle = SharedFirstHandleProbeTask.AttachWithHandle(10, false);
				const task_handle_t movedHandle = SharedSecondHandleProbeTask.AttachWithHandle(20, false);
				bool pass = firstHandle != TASK_INVALID_HANDLE
					&& movedHandle != TASK_INVALID_HANDLE;

				SharedFirstHandleProbeTask.Detach();

				pass &= SharedFirstHandleProbeTask.GetTaskHandle() == TASK_INVALID_HANDLE
					&& !SharedHandleProbeRegistry.TaskExists(&SharedFirstHandleProbeTask)
					&& SharedHandleProbeRegistry.TaskExists(&SharedSecondHandleProbeTask);

				if (pass)
				{
					pass = true
						&& SharedHandleProbeRegistry.GetDelay(movedHandle) == 20
						&& !SharedHandleProbeRegistry.IsEnabled(movedHandle);

#if !defined(HARMONIC_SKIP_CHECKS)
					SharedHandleProbeRegistry.SetEnabled(firstHandle, true);
					SharedHandleProbeRegistry.SetDelay(firstHandle, 99);
					pass = pass
						&& SharedHandleProbeRegistry.GetDelay(firstHandle) == UINT32_MAX
						&& !SharedHandleProbeRegistry.IsEnabled(firstHandle)
						&& SharedSecondHandleProbeTask.GetDelay() == 20
						&& !SharedSecondHandleProbeTask.IsEnabled();
#endif
				}

				const task_handle_t nextHandle = SharedThirdHandleProbeTask.AttachWithHandle(30, false);
				const task_handle_t wrapSeedHandle = SharedFourthHandleProbeTask.AttachWithHandle(50, false);
				pass = pass
					&& nextHandle != TASK_INVALID_HANDLE
					&& nextHandle != firstHandle
					&& nextHandle != movedHandle
					&& SharedHandleProbeRegistry.TaskExists(&SharedThirdHandleProbeTask)
					&& SharedHandleProbeRegistry.GetDelay(nextHandle) == 30
					&& !SharedHandleProbeRegistry.IsEnabled(nextHandle)
					&& wrapSeedHandle != TASK_INVALID_HANDLE
					&& wrapSeedHandle != firstHandle
					&& SharedHandleProbeRegistry.TaskExists(&SharedFourthHandleProbeTask);

				HandleProbeTask wrappedTask(SharedHandleProbeRegistry);
				const task_handle_t wrappedHandle = wrappedTask.AttachWithHandle(40, false);
				pass = pass
					&& wrappedHandle == firstHandle
					&& SharedHandleProbeRegistry.TaskExists(&wrappedTask)
					&& SharedHandleProbeRegistry.GetDelay(wrappedHandle) == 40
					&& !SharedHandleProbeRegistry.IsEnabled(wrappedHandle);

				if (pass)
				{
					SharedHandleProbeRegistry.SetDelay(movedHandle, 60);
					SharedHandleProbeRegistry.SetEnabled(movedHandle, true);
					pass = SharedSecondHandleProbeTask.GetTaskHandle() == movedHandle
						&& SharedSecondHandleProbeTask.GetDelay() == 60
						&& SharedSecondHandleProbeTask.IsEnabled()
						&& SharedThirdHandleProbeTask.GetDelay() == 30
						&& !SharedThirdHandleProbeTask.IsEnabled()
						&& SharedFourthHandleProbeTask.GetDelay() == 50
						&& !SharedFourthHandleProbeTask.IsEnabled()
						&& wrappedTask.GetDelay() == 40
						&& !wrappedTask.IsEnabled();
				}

				SharedSecondHandleProbeTask.Detach();
				SharedThirdHandleProbeTask.Detach();
				SharedFourthHandleProbeTask.Detach();
				wrappedTask.Detach();

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests that clearing a registry invalidates all previously issued handles.
		class TestTaskClearInvalidatesHandles : public AbstractTestTask
		{
		private:
			static constexpr task_handle_t ProbeCapacity = 3;

		public:
			TestTaskClearInvalidatesHandles(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskClearInvalidatesHandles"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				Platform::TaskTracker probeTaskList[ProbeCapacity]{};
				uint8_t probeHandleToSlot[ProbeCapacity]{};
				TaskRegistry probeRegistry(probeTaskList, probeHandleToSlot, ProbeCapacity, false);
				HandleProbeTask firstTask(probeRegistry);
				HandleProbeTask secondTask(probeRegistry);
				HandleProbeTask thirdTask(probeRegistry);

				const task_handle_t firstHandle = firstTask.AttachWithHandle(10, true);
				const task_handle_t secondHandle = secondTask.AttachWithHandle(20, false);
				const task_handle_t thirdHandle = thirdTask.AttachWithHandle(30, true);

				bool pass = firstHandle != TASK_INVALID_HANDLE
					&& secondHandle != TASK_INVALID_HANDLE
					&& thirdHandle != TASK_INVALID_HANDLE
					&& probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&secondTask)
					&& probeRegistry.TaskExists(&thirdTask);

				probeRegistry.Clear();

				pass = pass
					&& probeRegistry.GetTaskCount() == 0
					&& !probeRegistry.TaskExists(&firstTask)
					&& !probeRegistry.TaskExists(&secondTask)
					&& !probeRegistry.TaskExists(&thirdTask);

#if !defined(HARMONIC_SKIP_CHECKS)

				probeRegistry.Detach(firstHandle);
				probeRegistry.Detach(secondHandle);
				probeRegistry.Detach(thirdHandle);
				pass = pass
					&& SharedFirstHandleProbeTask.GetTaskHandle() == TASK_INVALID_HANDLE
					&& SharedSecondHandleProbeTask.GetTaskHandle() == TASK_INVALID_HANDLE
					&& SharedThirdHandleProbeTask.GetTaskHandle() == TASK_INVALID_HANDLE
					&& !probeRegistry.IsEnabled(firstHandle)
					&& !probeRegistry.IsEnabled(secondHandle)
					&& !probeRegistry.IsEnabled(thirdHandle)
					&& probeRegistry.GetDelay(firstHandle) == UINT32_MAX
					&& probeRegistry.GetDelay(secondHandle) == UINT32_MAX
					&& probeRegistry.GetDelay(thirdHandle) == UINT32_MAX
					;
#endif

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests that attaching after Clear restarts handle allocation from the beginning.
		class TestTaskAttachAfterClearResetsAllocation : public AbstractTestTask
		{
		private:
			static constexpr task_handle_t ProbeCapacity = 3;

		public:
			TestTaskAttachAfterClearResetsAllocation(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskAttachAfterClearResetsAllocation"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				Platform::TaskTracker probeTaskList[ProbeCapacity]{};
				uint8_t probeHandleToSlot[ProbeCapacity]{};
				TaskRegistry probeRegistry(probeTaskList, probeHandleToSlot, ProbeCapacity, false);
				HandleProbeTask firstTask(probeRegistry);
				HandleProbeTask secondTask(probeRegistry);

				const task_handle_t firstHandle = firstTask.AttachWithHandle(10, false);
				const task_handle_t secondHandle = secondTask.AttachWithHandle(20, false);
				bool pass = firstHandle == 0
					&& secondHandle == 1
					&& probeRegistry.GetTaskCount() == 2;

				probeRegistry.Clear();
				firstTask.Detach();
				secondTask.Detach();

				const task_handle_t recycledFirstHandle = firstTask.AttachWithHandle(30, true);
				const task_handle_t recycledSecondHandle = secondTask.AttachWithHandle(40, false);
				pass = pass
					&& recycledFirstHandle == 0
					&& recycledSecondHandle == 1
					&& probeRegistry.GetTaskCount() == 2
					&& probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&secondTask)
					&& firstTask.GetTaskHandle() == recycledFirstHandle
					&& secondTask.GetTaskHandle() == recycledSecondHandle
					&& firstTask.IsEnabled()
					&& !secondTask.IsEnabled()
					&& firstTask.GetDelay() == 30
					&& secondTask.GetDelay() == 40
					;
				//pass = true;

				firstTask.Detach();
				secondTask.Detach();

				if (!pass)
				{
					Serial.println(F("Handles:"));
					Serial.print(F("\tfirstTask handle: "));
					Serial.println(firstTask.GetTaskHandle());
					Serial.print(F("\tsecondTask handle: "));
					Serial.println(secondTask.GetTaskHandle());

					Serial.println(F("Delays:"));
					Serial.print(F("\tfirstTask delay: "));
					Serial.println(firstTask.GetDelay());
					Serial.print(F("\tsecondTask delay: "));
					Serial.println(secondTask.GetDelay());

				}

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests that NextHandle wraps and reuses the first available free handle.
		class TestTaskHandleWraparoundAllocation : public AbstractTestTask
		{
		private:
			static constexpr task_handle_t ProbeCapacity = 3;

		public:
			TestTaskHandleWraparoundAllocation(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleWraparoundAllocation"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				Platform::TaskTracker probeTaskList[ProbeCapacity]{};
				uint8_t probeHandleToSlot[ProbeCapacity]{};
				TaskRegistry probeRegistry(probeTaskList, probeHandleToSlot, ProbeCapacity, false);
				HandleProbeTask firstTask(probeRegistry);
				HandleProbeTask middleTask(probeRegistry);
				HandleProbeTask lastTask(probeRegistry);
				HandleProbeTask wrappedTask(probeRegistry);

				const task_handle_t firstHandle = firstTask.AttachWithHandle(10, false);
				const task_handle_t middleHandle = middleTask.AttachWithHandle(20, true);
				const task_handle_t lastHandle = lastTask.AttachWithHandle(30, false);

				bool pass = firstHandle == 0
					&& middleHandle == 1
					&& lastHandle == 2;

				middleTask.Detach();
				pass &= !probeRegistry.TaskExists(&middleTask)
					&& probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&lastTask);

				const task_handle_t wrappedHandle = wrappedTask.AttachWithHandle(40, true);
				pass = pass
					&& wrappedHandle == middleHandle
					&& wrappedTask.GetTaskHandle() == wrappedHandle
					&& probeRegistry.GetDelay(firstHandle) == 10
					&& !probeRegistry.IsEnabled(firstHandle)
					&& probeRegistry.GetDelay(lastHandle) == 30
					&& !probeRegistry.IsEnabled(lastHandle)
					&& probeRegistry.GetDelay(wrappedHandle) == 40
					&& probeRegistry.IsEnabled(wrappedHandle);

				firstTask.Detach();
				lastTask.Detach();
				wrappedTask.Detach();

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests handle reuse behavior and verifies routing remains stable for currently attached tasks.
		class TestTaskStableHandleRoutingAfterReuse : public AbstractTestTask
		{
		private:
			static constexpr task_handle_t ProbeCapacity = 2;

		public:
			TestTaskStableHandleRoutingAfterReuse(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleReuseRouting"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				Platform::TaskTracker probeTaskList[ProbeCapacity]{};
				uint8_t probeHandleToSlot[ProbeCapacity]{};
				TaskRegistry probeRegistry(probeTaskList, probeHandleToSlot, ProbeCapacity, false);
				HandleProbeTask firstTask(probeRegistry);
				HandleProbeTask survivorTask(probeRegistry);
				HandleProbeTask reusedTask(probeRegistry);

				const task_handle_t staleHandle = firstTask.AttachWithHandle(10, false);
				const task_handle_t survivorHandle = survivorTask.AttachWithHandle(20, false);
				bool pass = staleHandle != TASK_INVALID_HANDLE
					&& survivorHandle != TASK_INVALID_HANDLE
					&& staleHandle != survivorHandle;

				firstTask.Detach();
				pass &= !probeRegistry.TaskExists(&firstTask)
					&& !probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&survivorTask);

				const task_handle_t reusedHandle = reusedTask.AttachWithHandle(30, false);
				pass = pass
					&& reusedHandle != TASK_INVALID_HANDLE
					&& reusedHandle != survivorHandle
					&& probeRegistry.TaskExists(&reusedTask)
					&& reusedTask.GetDelay() == 30
					&& !reusedTask.IsEnabled();

				if (pass)
				{
					pass = reusedHandle == staleHandle
						&& reusedTask.GetTaskHandle() == reusedHandle
						&& survivorTask.GetTaskHandle() == survivorHandle;
				}

				if (pass)
				{
					probeRegistry.SetDelay(reusedHandle, 77);
					probeRegistry.SetEnabled(reusedHandle, true);
					pass = reusedTask.GetTaskHandle() == reusedHandle
						&& reusedTask.GetDelay() == 77
						&& reusedTask.IsEnabled()
						&& survivorTask.GetTaskHandle() == survivorHandle
						&& survivorTask.GetDelay() == 20
						&& !survivorTask.IsEnabled();
				}

				if (pass)
				{
					// With recycled-handle semantics, stale numeric values can alias the reused slot.
					probeRegistry.SetDelay(staleHandle, 99);
					probeRegistry.SetEnabled(staleHandle, false);
					pass = reusedTask.GetTaskHandle() == reusedHandle
						&& reusedTask.GetDelay() == 99
						&& !reusedTask.IsEnabled()
						&& survivorTask.GetTaskHandle() == survivorHandle
						&& survivorTask.GetDelay() == 20
						&& !survivorTask.IsEnabled();
				}

				survivorTask.Detach();
				reusedTask.Detach();

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests allocation and routing after Clear when handles are recycled.
		class TestTaskInvalidHandleSafetyAfterClear : public AbstractTestTask
		{
		private:
			static constexpr task_handle_t ProbeCapacity = 2;

		public:
			TestTaskInvalidHandleSafetyAfterClear(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleReuseAfterClear"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				Platform::TaskTracker probeTaskList[ProbeCapacity]{};
				uint8_t probeHandleToSlot[ProbeCapacity]{};
				TaskRegistry probeRegistry(probeTaskList, probeHandleToSlot, ProbeCapacity, false);
				HandleProbeTask firstGenerationFirstTask(probeRegistry);
				HandleProbeTask firstGenerationSecondTask(probeRegistry);
				HandleProbeTask secondGenerationFirstTask(probeRegistry);
				HandleProbeTask secondGenerationSecondTask(probeRegistry);

				const task_handle_t staleFirstHandle = firstGenerationFirstTask.AttachWithHandle(10, false);
				const task_handle_t staleSecondHandle = firstGenerationSecondTask.AttachWithHandle(20, false);
				bool pass = staleFirstHandle != TASK_INVALID_HANDLE
					&& staleSecondHandle != TASK_INVALID_HANDLE
					&& staleFirstHandle != staleSecondHandle
					&& probeRegistry.TaskExists(&firstGenerationFirstTask)
					&& probeRegistry.TaskExists(&firstGenerationSecondTask);

				probeRegistry.Clear();

				const task_handle_t secondGenerationFirstHandle = secondGenerationFirstTask.AttachWithHandle(30, false);
				const task_handle_t secondGenerationSecondHandle = secondGenerationSecondTask.AttachWithHandle(40, false);
				pass = pass
					&& secondGenerationFirstHandle != TASK_INVALID_HANDLE
					&& secondGenerationSecondHandle != TASK_INVALID_HANDLE
					&& secondGenerationFirstHandle != secondGenerationSecondHandle
					&& secondGenerationFirstHandle == staleFirstHandle
					&& secondGenerationSecondHandle == staleSecondHandle
					&& probeRegistry.TaskExists(&secondGenerationFirstTask)
					&& probeRegistry.TaskExists(&secondGenerationSecondTask)
					&& !probeRegistry.TaskExists(&firstGenerationFirstTask)
					&& !probeRegistry.TaskExists(&firstGenerationSecondTask);

				if (pass)
				{
					// Reused numeric handles should route to the second-generation tasks.
					probeRegistry.SetDelay(staleFirstHandle, 88);
					probeRegistry.SetEnabled(staleFirstHandle, true);
					probeRegistry.SetDelay(staleSecondHandle, 99);
					probeRegistry.SetEnabled(staleSecondHandle, true);
					pass = secondGenerationFirstTask.GetTaskHandle() == secondGenerationFirstHandle
						&& secondGenerationSecondTask.GetTaskHandle() == secondGenerationSecondHandle
						&& secondGenerationFirstTask.GetDelay() == 88
						&& secondGenerationFirstTask.IsEnabled()
						&& secondGenerationSecondTask.GetDelay() == 99
						&& secondGenerationSecondTask.IsEnabled();
				}

				if (pass)
				{
					// Valid second-generation handles must still route correctly.
					probeRegistry.SetDelay(secondGenerationFirstHandle, 88);
					probeRegistry.SetEnabled(secondGenerationFirstHandle, true);
					probeRegistry.SetDelay(secondGenerationSecondHandle, 99);
					probeRegistry.SetEnabled(secondGenerationSecondHandle, true);
					pass =
						secondGenerationFirstTask.IsEnabled()
						&& secondGenerationFirstTask.GetDelay() == 88
						&& secondGenerationSecondTask.GetDelay() == 99
						&& secondGenerationSecondTask.IsEnabled();
				}

				secondGenerationFirstTask.Detach();
				secondGenerationSecondTask.Detach();

				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		// Tests scheduler-level overrun handling independently of PeriodicTask policy.
		class TestTaskSchedulerOverrunHandling : public AbstractTestTask
		{
		private:
			static constexpr uint32_t TargetDelayMillis = 10;

			uint32_t FirstRunCompletionTimestamp = 0;
			uint32_t SecondRunTimestamp = 0;
			uint8_t RunCount = 0;

		public:
			TestTaskSchedulerOverrunHandling(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskSchedulerOverrunHandling"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				RunCount = 0;
				if (!Attach(TargetDelayMillis, true))
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				if (RunCount == 0)
				{
					// The callback overruns its scheduler delay.
					delay((TargetDelayMillis * 2) + 1);
					FirstRunCompletionTimestamp = Platform::GetProfilerTimestamp();
				}
				else if (RunCount == 1)
				{
					// An overrun drifts the schedule, but preserves the delay between calls.
					SecondRunTimestamp = Platform::GetProfilerTimestamp();
					const int32_t periodError = SecondRunTimestamp - FirstRunCompletionTimestamp;
					if (periodError < -TimingTolerance::ZeroDelayMicros || periodError > TimingTolerance::ZeroDelayMicros)
					{
						Serial.print(F("\tFAIL: Scheduler delay after overrun, error: "));
						Serial.print(periodError);
						Serial.println(F("us"));
						if (TestListener)
							TestListener->OnTestTaskDone(false);
						SetEnabled(false);
						return;
					}
				}
				else
				{
					// The delay remains stable after the drifted schedule.
					const int32_t periodError = Platform::GetProfilerTimestamp() - (SecondRunTimestamp + (TargetDelayMillis * 1000) + 1000);
					if (periodError < -TimingTolerance::BootMaxMicros || periodError > TimingTolerance::BootMaxMicros)
					{
						Serial.print(F("\tFAIL: Scheduler delay after catch-up, error: "));
						Serial.print(periodError);
						Serial.println(F("us"));
						if (TestListener)
							TestListener->OnTestTaskDone(false);
						SetEnabled(false);
						return;
					}
					SetEnabled(false);
					if (TestListener)
						TestListener->OnTestTaskDone(true);
				}

				RunCount++;
			}
		};

		struct IPeriodicModeProbeListener
		{
			virtual void OnPeriodicModeProbeDone(const bool pass) = 0;
		};

		template<PeriodicTask::ScheduleModeEnum Mode>
		class PeriodicModeProbe : public PeriodicTask
		{
		private:
			IPeriodicModeProbeListener& Listener;
			uint32_t PeriodMillis = 10;
			uint32_t FirstRunTimestamp = 0;
			uint32_t FirstRunCompletionTimestamp = 0;
			uint32_t SecondRunTimestamp = 0;
			uint8_t RunCount = 0;
			static constexpr uint8_t OverrunPeriods = 1;
			static constexpr uint16_t OverrunExtraMicros = 1550;

			void Fail(const uint8_t code, const int32_t error)
			{
				Serial.print(F("\tFAIL: PeriodicProbe ("));
				switch (Mode)
				{
				case ScheduleModeEnum::PhaseLock:
					Serial.print(F("PhaseLock"));
					break;
				case ScheduleModeEnum::Reanchor:
				default:
					Serial.print(F("Reanchor"));
					break;
				}
				Serial.print(F(") code: "));
				Serial.print(code);
				Serial.print(F(", error: "));
				Serial.println(error);
				SetEnabled(false);
				Listener.OnPeriodicModeProbeDone(false);
			}

		public:
			PeriodicModeProbe(TaskRegistry& registry, IPeriodicModeProbeListener& listener)
				: PeriodicTask(registry, 0, Mode)
				, Listener(listener)
			{}

			bool StartProbe(const uint32_t periodMillis)
			{
				RunCount = 0;
				PeriodMillis = periodMillis;
				return Start(PeriodMillis);
			}

			void PeriodicRun() final
			{
				if (RunCount == 0)
				{
					FirstRunTimestamp = Platform::GetProfilerTimestamp();
					delayMicroseconds((PeriodMillis * OverrunPeriods * 1000) + OverrunExtraMicros);
					FirstRunCompletionTimestamp = Platform::GetProfilerTimestamp();
				}
				else if (RunCount == 1)
				{
					SecondRunTimestamp = Platform::GetProfilerTimestamp();
					int32_t error;
					switch (Mode)
					{
					case ScheduleModeEnum::PhaseLock:
						// After an overrun, PhaseLock should resume on the next future grid slot.
						error = static_cast<int32_t>(SecondRunTimestamp - (FirstRunTimestamp + (PeriodMillis * (OverrunPeriods + 1) * 1000)));
						if (error < TimingTolerance::BootMinMicros || error > TimingTolerance::BootMaxMicros)
						{
							Fail(0, error);
							return;
						}
						break;
					case ScheduleModeEnum::Reanchor:
						// After an overrun, Reanchor should run again immediately.
						error = static_cast<int32_t>(SecondRunTimestamp - FirstRunCompletionTimestamp);
						if (error < 0 || error > TimingTolerance::ZeroDelayMicros)
						{
							Fail(0, error);
							return;
						}
						break;
					default:
						break;
					}
				}
				else
				{
					int32_t error;
					switch (Mode)
					{
					case ScheduleModeEnum::PhaseLock:
						error = static_cast<int32_t>(Platform::GetProfilerTimestamp() - (FirstRunTimestamp + (PeriodMillis * (OverrunPeriods + 2) * 1000)));
						if (error < TimingTolerance::BootMinMicros || error > TimingTolerance::BootMaxMicros)
						{
							Fail(1, error);
							return;
						}
						break;
					case ScheduleModeEnum::Reanchor:
						error = static_cast<int32_t>(Platform::GetProfilerTimestamp() - (SecondRunTimestamp + (PeriodMillis * 1000)));
						if (error < -TimingTolerance::ZeroDelayMicros || error > TimingTolerance::BootMaxMicros)
						{
							Fail(1, error);
							return;
						}
						break;
					default:
						break;
					}

					
					SetEnabled(false);
					Listener.OnPeriodicModeProbeDone(true);
					return;
				}

				RunCount++;
			}
		};

		class TestTaskPeriodicPhaseLock : public AbstractTestTask, public IPeriodicModeProbeListener
		{
		private:
			PeriodicModeProbe<PeriodicTask::ScheduleModeEnum::PhaseLock> Probe;

		public:
			TestTaskPeriodicPhaseLock(TaskRegistry& registry)
				: AbstractTestTask(registry), Probe(registry, *this)
			{}

			void PrintName() final { Serial.print(F("TestTaskPeriodicPhaseLock")); }

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (!Probe.StartProbe(10) && TestListener)
					TestListener->OnTestTaskDone(false);
			}

			void OnPeriodicModeProbeDone(const bool pass) final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		class TestTaskPeriodicReanchor : public AbstractTestTask, public IPeriodicModeProbeListener
		{
		private:
			PeriodicModeProbe<PeriodicTask::ScheduleModeEnum::Reanchor> Probe;

		public:
			TestTaskPeriodicReanchor(TaskRegistry& registry)
				: AbstractTestTask(registry), Probe(registry, *this)
			{}

			void PrintName() final { Serial.print(F("TestTaskPeriodicReanchor")); }

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (!Probe.StartProbe(10) && TestListener)
					TestListener->OnTestTaskDone(false);
			}

			void OnPeriodicModeProbeDone(const bool pass) final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(pass);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};

		class TestTaskTrackerBoundary : public AbstractTestTask
		{
		public:
			TestTaskTrackerBoundary(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskTrackerBoundary"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				Platform::TaskTracker tracker{};
				tracker.Enabled = true;
				tracker.LastRun = 100;
				tracker.Delay = 10;

				const bool atBoundary = !tracker.ShouldRun(110)
					&& tracker.TimeUntilNextRun(110) == 0;
				const bool afterBoundary = tracker.ShouldRun(111)
					&& tracker.TimeUntilNextRun(111) == 0;

				if (TestListener)
					TestListener->OnTestTaskDone(atBoundary && afterBoundary);
			}

			void Run() final
			{
				if (TestListener)
					TestListener->OnTestTaskDone(false);
			}
		};
	}
}

#endif