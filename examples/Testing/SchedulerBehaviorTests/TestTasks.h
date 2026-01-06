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
			static constexpr int32_t IsrWakeMicros = 100;
			static constexpr uint32_t ZeroPeriodMicros = 999;
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
			{
			}

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
				if (Attach(10, true) && GetTaskId() != TASK_INVALID_ID)
				{
					const bool detached = Detach();
					const bool pass = detached && !Registry.TaskExists(this) && GetTaskId() == TASK_INVALID_ID;
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
					AttachedOnce = Attach(10, true);
					if (AttachedOnce && GetTaskId() != TASK_INVALID_ID)
					{
						DetachedOnce = Detach();
						if (DetachedOnce
							&& !Registry.TaskExists(this)
							&& GetTaskId() == TASK_INVALID_ID)
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
				AbstractTestTask::StartTest(testListener);
				if (Attach(10, true))
				{
					bool firstDetach = Detach();
					bool secondDetach = Detach();
					const bool pass = firstDetach && !secondDetach && GetTaskId() == TASK_INVALID_ID && !Registry.TaskExists(this);
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
				AbstractTestTask::StartTest(testListener);
				if (Attach(10, true))
				{
					bool detached = Detach();
					SetEnabled(true);
					SetPeriod(123);
					SetPeriodAndEnabled(456, true);
					const bool pass = detached
						&& !IsEnabled()
						&& GetPeriod() == UINT32_MAX
						&& GetTaskId() == TASK_INVALID_ID
						&& !Registry.TaskExists(this);
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
				if (!Attach(TargetPeriodMillis, true))
				{
					if (TestListener)
						TestListener->OnTestTaskDone(false);
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

