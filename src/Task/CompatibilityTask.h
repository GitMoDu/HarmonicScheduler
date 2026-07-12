#ifndef _HARMONIC_COMPATIBILITY_WRAPPER_h
#define _HARMONIC_COMPATIBILITY_WRAPPER_h

#include "../Model/ITask.h"
#include "../Model/TaskRegistry.h"

/// <summary>
/// Wrapper for TaskScheduler::Scheduler and TaskScheduler::Task, for migration and testing purposes, wrapped in a Harmonic::DynamicTask.
/// Covers the core scheduling, iteration, and enable/disable logic of the original TaskScheduler::Task.
/// However, it does not implement features such as chaining, dynamic scheduler assignment, or function - pointer - based callbacks.
/// </summary>
namespace TS
{
	using Scheduler = Harmonic::TaskRegistry;

	static constexpr uint8_t TASK_IMMEDIATE = 0;
	static constexpr int8_t TASK_FOREVER = -1;
	static constexpr uint8_t TASK_ONCE = 1;

	class Task : public Harmonic::ITask
	{
	private:
		Harmonic::TaskRegistry* Registry;

	private:
		uint32_t Iterations = 0;
		int32_t TargetIterations = INT32_MAX;

		Harmonic::task_handle_t Handle = Harmonic::TASK_INVALID_HANDLE;

	protected:
		virtual bool OnEnable()
		{
			return true;
		}

		virtual void OnDisable()
		{}

	public:
		Task(unsigned long aInterval, long aIterations, Scheduler* aScheduler, bool aEnable)
			: Harmonic::ITask()
			, Registry(aScheduler)
		{
			TargetIterations = aIterations;
			if (Registry != nullptr)
			{
				Handle = Registry->Attach(this, aInterval, aEnable);
			}
			else
			{
				Handle = Harmonic::TASK_INVALID_HANDLE;
				Registry = nullptr;
			}
		}

		~Task() override = default;

		virtual bool Callback() = 0;

		void Run() override final
		{
			if (isLastIteration())
			{
				disable();
			}
			else
			{
				Callback();
				Iterations++;
			}
		}

		bool enable()
		{
			if (Registry == nullptr) return false;
			if (!Registry->IsEnabled(Handle))
			{
				if (!OnEnable())
				{
					return false;
				}
				Registry->SetEnabled(Handle, true);
			}
			return true;
		}

		bool enableIfNot()
		{
			return enable();
		}

		bool enableDelayed(unsigned long aDelay = 0)
		{
			if (Registry == nullptr) return false;
			if (!Registry->IsEnabled(Handle))
			{
				OnEnable();
			}
			Registry->SetPeriodAndEnabled(Handle, aDelay, true);
			return isEnabled();
		}

		bool restart()
		{
			if (Registry == nullptr) return false;
			if (!Registry->IsEnabled(Handle))
			{
				OnEnable();
			}
			const uint32_t delay = Registry->GetPeriod(Handle);
			Registry->SetPeriodAndEnabled(Handle, 0, false);
			Registry->SetPeriodAndEnabled(Handle, delay, true);
			return isEnabled();
		}

		bool restartDelayed(unsigned long aDelay = 0)
		{
			if (Registry == nullptr) return false;
			if (!Registry->IsEnabled(Handle))
			{
				OnEnable();
			}
			Registry->SetPeriodAndEnabled(Handle, 0, false);
			Registry->SetPeriodAndEnabled(Handle, aDelay, true);
			return isEnabled();
		}

		void delay(unsigned long aDelay = 0)
		{
			if (Registry == nullptr) return;
			Registry->SetPeriod(Handle, aDelay);
		}

		void adjust(long aInterval)
		{
			if (Registry == nullptr) return;
			Registry->SetPeriodAndEnabled(Handle, 0, false);
			Registry->SetPeriodAndEnabled(Handle, aInterval, true);
		}

		void forceNextIteration()
		{
			if (Registry == nullptr) return;
			if (!Registry->IsEnabled(Handle))
			{
				OnEnable();
				Registry->SetPeriodAndEnabled(Handle, 0, true);
			}
		}

		bool disable()
		{
			if (Registry != nullptr && Registry->IsEnabled(Handle))
			{
				Registry->SetEnabled(Handle, false);
				OnDisable();
				return true;
			}
			return false;
		}

		void abort()
		{
			disable();
		}

		void cancel()
		{
			disable();
		}

		bool isEnabled()
		{
			if (Registry == nullptr) return false;
			return Registry->IsEnabled(Handle);
		}

		bool canceled()
		{
			return !isEnabled();
		}

		void set(unsigned long aInterval, long aIterations)
		{
			TargetIterations = aIterations;
			if (Registry == nullptr) return;
			Registry->SetPeriod(Handle, aInterval);
		}

		void setInterval(unsigned long aInterval)
		{
			if (Registry == nullptr) return;
			Registry->SetPeriod(Handle, aInterval);
		}

		void setIntervalNodelay(unsigned long aInterval, unsigned int aOption)
		{
			if (Registry == nullptr) return;
			const bool enabled = Registry->IsEnabled(Handle);
			Registry->SetPeriodAndEnabled(Handle, 0, false);
			Registry->SetPeriodAndEnabled(Handle, aInterval, enabled);
		}

		unsigned long getInterval()
		{
			if (Registry != nullptr)
			{
				return Registry->GetPeriod(Handle);
			}
			return 0;
		}

		void setIterations(long aIterations)
		{
			TargetIterations = aIterations;
		}

		long getIterations()
		{
			return Iterations;
		}

		unsigned long getRunCounter()
		{
			return getIterations();
		}

		bool isFirstIteration()
		{
			return Iterations == 0;
		}

		bool isLastIteration()
		{
			if (TargetIterations >= 0)
			{
				return Iterations >= static_cast<uint32_t>(TargetIterations);
			}
			else
			{
				return false;
			}
		}

		void reset()
		{
			restart();
		}
	};
}

#define TASK_IMMEDIATE       TS::TASK_IMMEDIATE
#define TASK_FOREVER         TS::TASK_FOREVER
#define TASK_ONCE            TS::TASK_ONCE

#endif