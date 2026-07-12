#ifndef _TESTTASKS_h
#define _TESTTASKS_h

#include "TestInterface.h"
#include <HarmonicScheduler.h>

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
			static constexpr uint32_t PeriodicAverageMicros = 999 * ToleranceScale;
			static constexpr uint32_t ImmediateWakeMicros = 499 * ToleranceScale;
			static constexpr int32_t IsrWakeMicros = 150 * ToleranceScale;
			static constexpr uint32_t ZeroPeriodMicros = 999 * ToleranceScale;
		};

		class HandleProbeTask : public DynamicTask
		{
		public:
			HandleProbeTask(TaskRegistry& registry)
				: DynamicTask(registry)
			{}

			void Run() final {}
		};

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
				if (Attach(0, true) != TASK_INVALID_HANDLE)
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

				if (Attach(0, false) != TASK_INVALID_HANDLE && !IsEnabled())
				{
					SetPeriodAndEnabled(0, true);
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

		// Tests attaching a task with a specific period and verifying its run timing.
		class TestTaskAttachPeriod : public AbstractTestTask
		{
		private:
			static constexpr uint32_t TargetPeriodMillis = 1111;

			uint32_t StartTimestamp = 0;

		public:
			TestTaskAttachPeriod(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskAttachPeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(TargetPeriodMillis, true) != TASK_INVALID_HANDLE)
				{
					StartTimestamp = micros();
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = micros();

				SetEnabled(false);
				const uint32_t runDelay = runTimestamp - StartTimestamp;
				const int32_t delayErrorMicros = (int32_t)(runDelay)-(int32_t)(TargetPeriodMillis * 1000);
				const bool pass = (delayErrorMicros >= TimingTolerance::BootMinMicros)
					&& (delayErrorMicros <= TimingTolerance::BootMaxMicros);

				Serial.print(F("\tTask delay error "));
				Serial.print(delayErrorMicros);
				Serial.print(F(" out of "));
				Serial.print(TargetPeriodMillis * 1000);
				Serial.println(F("us"));

				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests enabling a task and setting its period after a delay.
		class TestTaskDelayedEnablePeriod : public AbstractTestTask
		{
		private:
			static constexpr uint32_t TargetPeriodMillis = 1111;
			static constexpr uint32_t ToleranceMicros = 1500;
			uint32_t StartTimestamp = 0;

		public:
			TestTaskDelayedEnablePeriod(TaskRegistry& registry) : AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskDelayedEnablePeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(0, false) != TASK_INVALID_HANDLE)
				{
					StartTimestamp = micros();
					SetPeriodAndEnabled(TargetPeriodMillis, true);
				}
				else
				{
					if (testListener != nullptr)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = micros();

				SetEnabled(false);
				const uint32_t runDelay = runTimestamp - StartTimestamp;
				const int32_t delayErrorMicros = (int32_t)(runDelay)-(int32_t)(TargetPeriodMillis * 1000);
				const bool pass = (delayErrorMicros >= TimingTolerance::BootMinMicros)
					&& (delayErrorMicros <= TimingTolerance::BootMaxMicros);

				Serial.print(F("\tTask delay error "));
				Serial.print(delayErrorMicros);
				Serial.print(F(" out of "));
				Serial.print(TargetPeriodMillis * 1000);
				Serial.println(F("us"));

				if (TestListener != nullptr)
					TestListener->OnTestTaskDone(pass);
			}
		};

		// Tests periodic toggling and timing accuracy over multiple runs.
		class TestTaskPeriodicToggle : public AbstractTestTask
		{
		private:
			static constexpr int32_t ToleranceMicros = 999;
			static constexpr uint32_t ToleranceAverageMicros = 999;

			static constexpr uint32_t TogglePeriodMillis = 20;
			static constexpr int32_t MaxToggles = 32;

			int64_t TotalDelayErrorMicros = 0;
			uint32_t ToggleStartTimestamp = 0;
			int32_t ToggleCount = -1;

		public:
			TestTaskPeriodicToggle(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskPeriodicToggle"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				ToggleCount = -1;
				if (Attach(TogglePeriodMillis, true) != TASK_INVALID_HANDLE)
				{
					// Ready to start toggling.
					ToggleStartTimestamp = micros();
				}
				else
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = micros();

				if (ToggleCount < 0)
				{
					// Set on first run to align with scheduler tick.
					const int32_t delayErrorMicros = (runTimestamp - ToggleStartTimestamp) - (TogglePeriodMillis * 1000);
					ToggleStartTimestamp = runTimestamp;

					const bool pass = (delayErrorMicros >= TimingTolerance::BootMinMicros)
						&& (delayErrorMicros <= TimingTolerance::BootMaxMicros);

					Serial.print(F("\tTask boot delay error "));
					Serial.print(delayErrorMicros);
					Serial.println(F("us"));

					if (pass)
					{
						ToggleCount = 0;
					}
					else
					{
						Serial.print(F("\t\t!1!"));
						SetEnabled(false);
						if (TestListener)
							TestListener->OnTestTaskDone(false);
					}
				}
				else
				{
					const uint32_t runDelay = runTimestamp - ToggleStartTimestamp;
					ToggleStartTimestamp = runTimestamp;
					const int32_t delayErrorMicros = (int32_t)(runDelay)-(int32_t)(TogglePeriodMillis * 1000);

					TotalDelayErrorMicros += delayErrorMicros;

					const int32_t averageDelayErrorMicros = TotalDelayErrorMicros / (ToggleCount + 1);
					const uint32_t averageAbs = averageDelayErrorMicros >= 0 ? averageDelayErrorMicros : -averageDelayErrorMicros;

					const bool pass = (delayErrorMicros >= -ToleranceMicros)
						&& (delayErrorMicros <= ToleranceMicros)
						&& averageAbs <= ToleranceAverageMicros;

					if (pass)
					{
						ToggleCount++;
						if (ToggleCount >= MaxToggles)
						{
							SetEnabled(false);

							Serial.print(F("\tTask periodic average error "));
							Serial.print(averageDelayErrorMicros);
							Serial.println(F("us"));

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
				if (Attach(12345679, false) != TASK_INVALID_HANDLE)
				{
					StartTimestamp = micros();
					// Simulate an immediate wake from ISR
					WakeFromISR();
				}
				else
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				const uint32_t runTimestamp = micros();
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
#if defined(ARDUINO_ARCH_AVR)
			static constexpr uint16_t Timer1Prescaler = 64;
			static constexpr uint16_t Timer1CompareValue = (F_CPU / Timer1Prescaler) / 10;
			static constexpr uint32_t ExpectedDurationMicros = (uint64_t(Timer1CompareValue) * Timer1Prescaler * 1000000UL) / F_CPU;
#elif defined(ARDUINO_ARCH_STM32F1)  || defined(ARDUINO_ARCH_STM32F4)
#if defined(F_CPU)
			static constexpr uint32_t TimerPrescaler = (F_CPU / 10000) - 1; // ~10kHz
#endif
			static constexpr uint16_t TimerOverflow = 10000;     // 1s (10kHz * 1s)
			static constexpr uint32_t ExpectedDurationMicros = 1000000; // 1s in microseconds
			static constexpr uint8_t TestTimerIndex = 2;
			static constexpr uint8_t TestTimerChannel = 0;
			HardwareTimer TestTimer;
#else 
			static constexpr uint32_t ExpectedDurationMicros = 0;
#endif

			uint32_t StartTimestamp = 0;
			void (*InterruptCallback)(void) = nullptr; // Function pointer for external ISR callback

			volatile uint32_t InterruptTimestamp = 0;
			volatile bool WokenFromIsr = false;

		public:
			TestTaskIsrWake(TaskRegistry& registry)
				: AbstractTestTask(registry)
#if defined(ARDUINO_ARCH_STM32F1)  || defined(ARDUINO_ARCH_STM32F4)
				, TestTimer(TestTimerIndex)
#endif
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskIsrWake"));
			}

			void SetInterruptCallback(void (*callback)(void))
			{
				InterruptCallback = callback;
			}

			void OnIsr()
			{
				InterruptTimestamp = micros();
				DisableTimer();
				WokenFromIsr = true;
				WakeFromISR();
			}

			void StartTest(ITester* testListener) final
			{
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_STM32F1)  || defined(ARDUINO_ARCH_STM32F4)
				AbstractTestTask::StartTest(testListener);
				if (Attach((ExpectedDurationMicros / 1000) * 2, true) != TASK_INVALID_HANDLE)
				{
					DisableTimer();
					WokenFromIsr = false;
					// Set-up timer for delayed wake from ISR.
					StartTimestamp = micros();
					SetupTimerInterrupt();
				}
				else
				{
					if (testListener)
						testListener->OnTestTaskDone(false);
				}
#else
				Serial.println(F("\tWARNING: ISR Test not performed, only supported platform is AVR."));
				if (testListener)
					testListener->OnTestTaskDone(true);
#endif
			}

			void Run() final
			{
				const uint32_t runTimestamp = micros();

				if (WokenFromIsr)
				{
					uint32_t wakeDelay;
					Platform::AtomicGuard guard;
					{
						wakeDelay = runTimestamp - InterruptTimestamp;
					}

					const uint32_t runDelay = runTimestamp - StartTimestamp;
					const int32_t delayErrorMicros = int32_t(runDelay) - int32_t(ExpectedDurationMicros);
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
#if defined(ARDUINO_ARCH_AVR)
				Platform::AtomicGuard guard;

				TIMSK1 &= ~(1 << OCIE1A); // Disable Timer1 Compare Match A Interrupt
				TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); // Stop timer by clearing prescaler bits

				// Clear any pending interrupt flag
				TIFR1 |= (1 << OCF1A);
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
				TestTimer.pause();
				TestTimer.detachInterrupt(TestTimerChannel); // Channel 0 = update/overflow
#endif
			}

			void SetupTimerInterrupt()
			{
#if defined(ARDUINO_ARCH_AVR)
				Platform::AtomicGuard guard;

				TIMSK1 &= ~(1 << OCIE1A); // Disable Timer1 Compare Match A Interrupt
				TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); // Stop timer by clearing prescaler bits

				// Clear any pending interrupt flag
				TIFR1 |= (1 << OCF1A);

				// Pause timer and interrupt
				DisableTimer();

				TCCR1A = 0; // Normal mode
				TCCR1B = 0; // Ensure timer is stopped
				TCNT1 = 0;  // Reset counter
				OCR1A = Timer1CompareValue; // Set compare value

				TCCR1B |= (1 << WGM12); // CTC mode
				TIMSK1 |= (1 << OCIE1A); // Enable compare interrupt

				// Now start timer by setting prescaler
				TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
#elif defined(ARDUINO_ARCH_STM32F1)  || defined(ARDUINO_ARCH_STM32F4)
				DisableTimer();
				TestTimer.init();
#if defined(F_CPU)
#else
				const uint32_t TimerPrescaler = (TestTimer.getClockSpeed() / 10000) - 1; // ~10kHz
#endif
				TestTimer.setPrescaleFactor(TimerPrescaler);
				TestTimer.setOverflow(TimerOverflow);
				TestTimer.refresh();
				TestTimer.attachInterrupt(TestTimerChannel, InterruptCallback);
				TestTimer.resume();
#endif
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
				if (Attach(10, true) != TASK_INVALID_HANDLE)
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
					AttachedOnce = Attach(10, true) != TASK_INVALID_HANDLE;
					if (AttachedOnce)
					{
						// Try to attach again, should fail or be ignored
						const bool pass = Attach(20, true) == TASK_INVALID_HANDLE;
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

		// Test attaching a task with zero period and verify it runs as fast as possible.
		class TestTaskZeroPeriod : public AbstractTestTask
		{
		private:
			static constexpr uint32_t ToleranceMicros = 1999;
			static constexpr uint8_t TargetRunCount = 8;
			uint32_t StartTimestamp = 0;
			uint8_t RunCount = 0;

		public:
			TestTaskZeroPeriod(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskZeroPeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				RunCount = 0;
				if (Attach(0, true) != TASK_INVALID_HANDLE)
				{
					StartTimestamp = micros();
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
					const uint32_t runTimestamp = micros();
					const uint32_t runDelay = runTimestamp - StartTimestamp;
					const bool pass = runDelay < ToleranceMicros;

					Serial.print(F("\tTask zero delay duration "));
					Serial.print(runDelay);
					Serial.println(F("us"));

					SetEnabled(false);
					if (TestListener)
						TestListener->OnTestTaskDone(pass);
				}
			}
		};

		// Test attaching a task with maximum allowed period and verify correct scheduling.
		class TestTaskMaxPeriod : public AbstractTestTask
		{
		private:
			static constexpr uint32_t MaxPeriodMillis = UINT32_MAX;
			uint32_t StartTimestamp = 0;

		public:
			TestTaskMaxPeriod(TaskRegistry& registry)
				: AbstractTestTask(registry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskMaxPeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (Attach(MaxPeriodMillis, true) != TASK_INVALID_HANDLE)
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
				// Attach with a short period to allow rapid toggling
				if (Attach(2, true) == TASK_INVALID_HANDLE)
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
				if (Attach(10, true) != TASK_INVALID_HANDLE && GetHandle() != TASK_INVALID_HANDLE)
				{
					const bool detached = Detach();
					const bool pass = detached && !Registry.TaskExists(this) && GetHandle() == TASK_INVALID_HANDLE;
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
				const bool detached = Detach();
				const bool pass = !detached && !Registry.TaskExists(this);
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
					AttachedOnce = Attach(10, true) != TASK_INVALID_HANDLE;
					if (AttachedOnce && GetHandle() != TASK_INVALID_HANDLE)
					{
						DetachedOnce = Detach();
						if (DetachedOnce
							&& !Registry.TaskExists(this)
							&& GetHandle() == TASK_INVALID_HANDLE)
						{
							// Try to re-attach
							const bool reattached = Attach(20, true) != TASK_INVALID_HANDLE;
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
				if (Attach(10, true) != TASK_INVALID_HANDLE)
				{
					bool firstDetach = Detach();
					bool secondDetach = Detach();
					const bool pass = firstDetach && !secondDetach && GetHandle() == TASK_INVALID_HANDLE && !Registry.TaskExists(this);
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

		// Tests detaching a task and then attempting to enable or set period (should be no-op).
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
				if (Attach(10, true) != TASK_INVALID_HANDLE)
				{
					bool detached = Detach();
					SetEnabled(true);
					SetPeriod(123);
					SetPeriodAndEnabled(456, true);
					const bool pass = detached
						&& !IsEnabled()
						&& GetPeriod() == UINT32_MAX
						&& GetHandle() == TASK_INVALID_HANDLE
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
		private:
			static constexpr task_handle_t ProbeCapacity = 2;
			Platform::TaskTracker ProbeTaskList[ProbeCapacity]{};
			uint8_t ProbeHandleToSlot[ProbeCapacity]{};
			TaskRegistry ProbeRegistry;
			HandleProbeTask FirstTask;
			HandleProbeTask SecondTask;

		public:
			TestTaskHandleCompaction(TaskRegistry& registry)
				: AbstractTestTask(registry)
				, ProbeRegistry(ProbeTaskList, ProbeHandleToSlot, ProbeCapacity, false)
				, FirstTask(ProbeRegistry)
				, SecondTask(ProbeRegistry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleCompaction"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				const task_handle_t firstHandle = FirstTask.Attach(10, false);
				const task_handle_t secondHandle = SecondTask.Attach(20, false);
				bool pass = firstHandle != TASK_INVALID_HANDLE
					&& secondHandle != TASK_INVALID_HANDLE
					&& FirstTask.Detach()
					&& !ProbeRegistry.TaskExists(&FirstTask)
					&& ProbeRegistry.TaskExists(&SecondTask)
					&& ProbeRegistry.GetPeriod(secondHandle) == 20
					&& !ProbeRegistry.IsEnabled(secondHandle);

				if (pass)
				{
					ProbeRegistry.SetPeriodAndEnabled(secondHandle, 30, true);
					pass = SecondTask.GetHandle() == secondHandle
						&& SecondTask.GetPeriod() == 30
						&& SecondTask.IsEnabled();
				}

				SecondTask.Detach();

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
		private:
			static constexpr task_handle_t ProbeCapacity = 4;
			Platform::TaskTracker ProbeTaskList[ProbeCapacity]{};
			uint8_t ProbeHandleToSlot[ProbeCapacity]{};
			TaskRegistry ProbeRegistry;
			HandleProbeTask FirstTask;
			HandleProbeTask MovedTask;
			HandleProbeTask NextTask;
			HandleProbeTask WrapTask;

		public:
			TestTaskHandleReuseIsolation(TaskRegistry& registry)
				: AbstractTestTask(registry)
				, ProbeRegistry(ProbeTaskList, ProbeHandleToSlot, ProbeCapacity, false)
				, FirstTask(ProbeRegistry)
				, MovedTask(ProbeRegistry)
				, NextTask(ProbeRegistry)
				, WrapTask(ProbeRegistry)
			{}

			void PrintName() final
			{
				Serial.print(F("TestTaskHandleReuseIsolation"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				const task_handle_t firstHandle = FirstTask.Attach(10, false);
				const task_handle_t movedHandle = MovedTask.Attach(20, false);
				bool pass = firstHandle != TASK_INVALID_HANDLE
					&& movedHandle != TASK_INVALID_HANDLE
					&& FirstTask.Detach()
					&& !ProbeRegistry.TaskExists(&FirstTask)
					&& ProbeRegistry.TaskExists(&MovedTask);

				if (pass)
				{
					pass = ProbeRegistry.GetPeriod(movedHandle) == 20
						&& !ProbeRegistry.IsEnabled(movedHandle);

#if !defined(HARMONIC_SKIP_CHECKS)
					ProbeRegistry.SetPeriodAndEnabled(firstHandle, 99, true);
					pass = pass
						&& ProbeRegistry.GetPeriod(firstHandle) == UINT32_MAX
						&& !ProbeRegistry.IsEnabled(firstHandle)
						&& MovedTask.GetPeriod() == 20
						&& !MovedTask.IsEnabled();
#endif
				}

				const task_handle_t nextHandle = NextTask.Attach(30, false);
				const task_handle_t wrapSeedHandle = WrapTask.Attach(50, false);
				pass = pass
					&& nextHandle != TASK_INVALID_HANDLE
					&& nextHandle != firstHandle
					&& nextHandle != movedHandle
					&& ProbeRegistry.TaskExists(&NextTask)
					&& ProbeRegistry.GetPeriod(nextHandle) == 30
					&& !ProbeRegistry.IsEnabled(nextHandle)
					&& wrapSeedHandle != TASK_INVALID_HANDLE
					&& wrapSeedHandle != firstHandle
					&& ProbeRegistry.TaskExists(&WrapTask);

				HandleProbeTask WrappedTask(ProbeRegistry);
				const task_handle_t wrappedHandle = WrappedTask.Attach(40, false);
				pass = pass
					&& wrappedHandle == firstHandle
					&& ProbeRegistry.TaskExists(&WrappedTask)
					&& ProbeRegistry.GetPeriod(wrappedHandle) == 40
					&& !ProbeRegistry.IsEnabled(wrappedHandle);

				if (pass)
				{
					ProbeRegistry.SetPeriodAndEnabled(movedHandle, 60, true);
					pass = MovedTask.GetHandle() == movedHandle
						&& MovedTask.GetPeriod() == 60
						&& MovedTask.IsEnabled()
						&& NextTask.GetPeriod() == 30
						&& !NextTask.IsEnabled()
						&& WrapTask.GetPeriod() == 50
						&& !WrapTask.IsEnabled()
						&& WrappedTask.GetPeriod() == 40
						&& !WrappedTask.IsEnabled();
				}

				MovedTask.Detach();
				NextTask.Detach();
				WrapTask.Detach();
				WrappedTask.Detach();

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

				const task_handle_t firstHandle = firstTask.Attach(10, true);
				const task_handle_t secondHandle = secondTask.Attach(20, false);
				const task_handle_t thirdHandle = thirdTask.Attach(30, true);

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

				pass = pass
					&& !probeRegistry.Detach(firstHandle)
					&& !probeRegistry.Detach(secondHandle)
					&& !probeRegistry.Detach(thirdHandle)
					&& !probeRegistry.IsEnabled(firstHandle)
					&& !probeRegistry.IsEnabled(secondHandle)
					&& !probeRegistry.IsEnabled(thirdHandle)
					&& probeRegistry.GetPeriod(firstHandle) == UINT32_MAX
					&& probeRegistry.GetPeriod(secondHandle) == UINT32_MAX
					&& probeRegistry.GetPeriod(thirdHandle) == UINT32_MAX;
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

				const task_handle_t firstHandle = firstTask.Attach(10, false);
				const task_handle_t secondHandle = secondTask.Attach(20, false);
				bool pass = firstHandle == 0
					&& secondHandle == 1
					&& probeRegistry.GetTaskCount() == 2;

				probeRegistry.Clear();

				const task_handle_t recycledFirstHandle = firstTask.Attach(30, true);
				const task_handle_t recycledSecondHandle = secondTask.Attach(40, false);
				pass = pass
					&& recycledFirstHandle == 0
					&& recycledSecondHandle == 1
					&& probeRegistry.GetTaskCount() == 2
					&& probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&secondTask)
					&& firstTask.GetHandle() == recycledFirstHandle
					&& secondTask.GetHandle() == recycledSecondHandle
					&& firstTask.IsEnabled()
					&& !secondTask.IsEnabled()
					&& firstTask.GetPeriod() == 30
					&& secondTask.GetPeriod() == 40;

				firstTask.Detach();
				secondTask.Detach();

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

				const task_handle_t firstHandle = firstTask.Attach(10, false);
				const task_handle_t middleHandle = middleTask.Attach(20, true);
				const task_handle_t lastHandle = lastTask.Attach(30, false);

				bool pass = firstHandle == 0
					&& middleHandle == 1
					&& lastHandle == 2
					&& middleTask.Detach()
					&& !probeRegistry.TaskExists(&middleTask)
					&& probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&lastTask);

				const task_handle_t wrappedHandle = wrappedTask.Attach(40, true);
				pass = pass
					&& wrappedHandle == middleHandle
					&& wrappedTask.GetHandle() == wrappedHandle
					&& probeRegistry.GetPeriod(firstHandle) == 10
					&& !probeRegistry.IsEnabled(firstHandle)
					&& probeRegistry.GetPeriod(lastHandle) == 30
					&& !probeRegistry.IsEnabled(lastHandle)
					&& probeRegistry.GetPeriod(wrappedHandle) == 40
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

				const task_handle_t staleHandle = firstTask.Attach(10, false);
				const task_handle_t survivorHandle = survivorTask.Attach(20, false);
				bool pass = staleHandle != TASK_INVALID_HANDLE
					&& survivorHandle != TASK_INVALID_HANDLE
					&& staleHandle != survivorHandle
					&& firstTask.Detach()
					&& !probeRegistry.TaskExists(&firstTask)
					&& probeRegistry.TaskExists(&survivorTask);

				const task_handle_t reusedHandle = reusedTask.Attach(30, false);
				pass = pass
					&& reusedHandle != TASK_INVALID_HANDLE
					&& reusedHandle != survivorHandle
					&& probeRegistry.TaskExists(&reusedTask)
					&& reusedTask.GetPeriod() == 30
					&& !reusedTask.IsEnabled();

				if (pass)
				{
					pass = reusedHandle == staleHandle
						&& reusedTask.GetHandle() == reusedHandle
						&& survivorTask.GetHandle() == survivorHandle;
				}

				if (pass)
				{
					probeRegistry.SetPeriodAndEnabled(reusedHandle, 77, true);
					pass = reusedTask.GetHandle() == reusedHandle
						&& reusedTask.GetPeriod() == 77
						&& reusedTask.IsEnabled()
						&& survivorTask.GetHandle() == survivorHandle
						&& survivorTask.GetPeriod() == 20
						&& !survivorTask.IsEnabled();
				}

				if (pass)
				{
					// With recycled-handle semantics, stale numeric values can alias the reused slot.
					probeRegistry.SetPeriodAndEnabled(staleHandle, 99, false);
					pass = reusedTask.GetHandle() == reusedHandle
						&& reusedTask.GetPeriod() == 99
						&& !reusedTask.IsEnabled()
						&& survivorTask.GetHandle() == survivorHandle
						&& survivorTask.GetPeriod() == 20
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

				const task_handle_t staleFirstHandle = firstGenerationFirstTask.Attach(10, false);
				const task_handle_t staleSecondHandle = firstGenerationSecondTask.Attach(20, false);
				bool pass = staleFirstHandle != TASK_INVALID_HANDLE
					&& staleSecondHandle != TASK_INVALID_HANDLE
					&& staleFirstHandle != staleSecondHandle
					&& probeRegistry.TaskExists(&firstGenerationFirstTask)
					&& probeRegistry.TaskExists(&firstGenerationSecondTask);

				probeRegistry.Clear();

				const task_handle_t secondGenerationFirstHandle = secondGenerationFirstTask.Attach(30, false);
				const task_handle_t secondGenerationSecondHandle = secondGenerationSecondTask.Attach(40, false);
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
					probeRegistry.SetPeriodAndEnabled(staleFirstHandle, 88, true);
					probeRegistry.SetPeriodAndEnabled(staleSecondHandle, 99, true);
					pass = secondGenerationFirstTask.GetHandle() == secondGenerationFirstHandle
						&& secondGenerationSecondTask.GetHandle() == secondGenerationSecondHandle
						&& secondGenerationFirstTask.GetPeriod() == 88
						&& secondGenerationFirstTask.IsEnabled()
						&& secondGenerationSecondTask.GetPeriod() == 99
						&& secondGenerationSecondTask.IsEnabled();
				}

				if (pass)
				{
					// Valid second-generation handles must still route correctly.
					probeRegistry.SetPeriodAndEnabled(secondGenerationFirstHandle, 88, true);
					probeRegistry.SetPeriodAndEnabled(secondGenerationSecondHandle, 99, true);
					pass = secondGenerationFirstTask.GetPeriod() == 88
						&& secondGenerationFirstTask.IsEnabled()
						&& secondGenerationSecondTask.GetPeriod() == 99
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

		// Tests scheduler overrun handling: after an overrun, the second run should be ASAP (immediately),
		// and the third run should be on schedule (period after the second run).
		class TestTaskOverrunHandling : public AbstractTestTask
		{
		private:
			static constexpr uint32_t TargetPeriodMillis = 20;
			static constexpr uint32_t OverrunMicros = (TargetPeriodMillis * 1000) + 5000; // 5ms overrun
			static constexpr uint8_t RunCountTarget = 3;

			uint32_t FirstRunTimestamp = 0;
			uint32_t SecondRunTimestamp = 0;
			uint8_t RunCount = 0;
			bool Pass = true;

		public:
			TestTaskOverrunHandling(TaskRegistry& registry) : AbstractTestTask(registry) {}

			void PrintName() final
			{
				Serial.print(F("TestTaskOverrunHandling"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				RunCount = 0;
				Pass = true;
				if (Attach(TargetPeriodMillis, true) == TASK_INVALID_HANDLE)
				{
					if (TestListener)
						testListener->OnTestTaskDone(false);
				}
			}

			void Run() final
			{
				//const uint32_t now = micros();

				if (RunCount == 0)
				{
					// First run: record timestamp, then overrun the period
					delayMicroseconds(OverrunMicros); // Simulate a long-running task
					RunCount++;
					FirstRunTimestamp = micros();
				}
				else if (RunCount == 1)
				{
					// Second run: should be ASAP after the overrun
					SecondRunTimestamp = micros();
					const uint32_t elapsed = SecondRunTimestamp - FirstRunTimestamp;
					if (elapsed > ((TargetPeriodMillis * 1000) + TimingTolerance::BootMaxMicros))
					{
						Pass = false;
						Serial.print(F("\tFAIL: Second run too late: "));
						Serial.print(elapsed);
						Serial.println(F("us"));
					}
					else
					{
						Serial.print(F("\tSecond run after overrun: "));
						Serial.print(elapsed);
						Serial.println(F("us"));
					}
					RunCount++;
				}
				else if (RunCount == 2)
				{
					// Third run: should be on schedule (TargetPeriodMillis after second run)
					const uint32_t elapsed = micros() - SecondRunTimestamp;
					const int32_t error = (int32_t)elapsed - (int32_t)(TargetPeriodMillis * 1000);
					const bool onTime = (error >= TimingTolerance::BootMinMicros) && (error <= TimingTolerance::BootMaxMicros);

					if (!onTime)
					{
						Pass = false;
						Serial.print(F("\tFAIL: Third run not on schedule, error: "));
						Serial.print(error);
						Serial.println(F("us"));
					}
					else
					{
						Serial.print(F("\tThird run on schedule, error: "));
						Serial.print(error);
						Serial.println(F("us"));
					}
					SetEnabled(false);
					if (TestListener)
						TestListener->OnTestTaskDone(Pass);
					RunCount++;
				}
			}
		};
	}
}

#endif


