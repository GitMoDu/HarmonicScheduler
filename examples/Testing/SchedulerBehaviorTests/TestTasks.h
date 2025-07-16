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
			static constexpr int32_t BootMinMicros = -749;
			static constexpr int32_t BootMaxMicros = 1249;
			static constexpr int32_t PeriodicMicros = 999;
			static constexpr uint32_t PeriodicAverageMicros = 999;
			static constexpr uint32_t ImmediateWakeMicros = 499;
			static constexpr uint32_t IsrWakeMicros = 100;
			static constexpr uint32_t ZeroPeriodMicros = 1999 / (F_CPU / 8000000);
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
			{
			}

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
			{
			}

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
			{
			}

			void PrintName() final
			{
				Serial.print(F("TestTaskEnableDisable"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(0, false) && !IsEnabled())
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
			{
			}

			void PrintName() final
			{
				Serial.print(F("TestTaskAttachPeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(TargetPeriodMillis, true))
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
			{
			}

			void PrintName() final
			{
				Serial.print(F("TestTaskDelayedEnablePeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);

				if (Attach(0, false))
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
			static constexpr uint32_t MaxToggles = 32;

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
				if (Attach(TogglePeriodMillis, true))
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
					const int32_t averageAbs = averageDelayErrorMicros >= 0 ? averageDelayErrorMicros : -averageDelayErrorMicros;

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
				if (Attach(12345679, false))
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
					TestListener->OnTestTaskDone(true);
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
#endif
			uint32_t StartTimestamp = 0;
			void (*InterruptCallback)(void) = nullptr; // Function pointer for external ISR callback

			volatile uint32_t InterruptTimestamp = 0;
			volatile bool WokenFromIsr = false;

		public:
			TestTaskIsrWake(TaskRegistry& registry) : AbstractTestTask(registry) {}

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
				Platform::AtomicGuard guard;
				DisableTimer();
				WokenFromIsr = true;
				WakeFromISR();
			}

			void StartTest(ITester* testListener) final
			{
#if defined(ARDUINO_ARCH_AVR)
				AbstractTestTask::StartTest(testListener);
				if (Attach((ExpectedDurationMicros / 1000) * 2, true))
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
					const uint32_t wakeDelay = runTimestamp - InterruptTimestamp;
					const uint32_t runDelay = runTimestamp - StartTimestamp;
					const int32_t delayErrorMicros = int32_t(runDelay) - int32_t(ExpectedDurationMicros);
					const bool pass = delayErrorMicros >= 0 && delayErrorMicros < TimingTolerance::IsrWakeMicros;

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
					if (TestListener)
						TestListener->OnTestTaskDone(false);
				}
			}

#if defined(ARDUINO_ARCH_AVR)
		private:
			void DisableTimer()
			{
				Platform::AtomicGuard guard;

				TIMSK1 &= ~(1 << OCIE1A); // Disable Timer1 Compare Match A Interrupt
				TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); // Stop timer by clearing prescaler bits

				// Clear any pending interrupt flag
				TIFR1 |= (1 << OCF1A);
			}

			void SetupTimerInterrupt()
			{
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
			}
#endif
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
						// Try to attach again, should fail or be ignored
						const bool pass = !Attach(20, true);
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
				if (Attach(0, true))
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
			{
			}

			void PrintName() final
			{
				Serial.print(F("TestTaskMaxPeriod"));
			}

			void StartTest(ITester* testListener) final
			{
				AbstractTestTask::StartTest(testListener);
				if (Attach(MaxPeriodMillis, true))
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
			{
			}

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
	}
}

#endif

