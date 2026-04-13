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
		Harmonic::TaskRegistry& Registry;
		Harmonic::task_id_t Handle = UINT8_MAX;

		uint32_t Iterations = 0;
		int32_t TargetIterations = INT32_MAX;

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
			, Registry(*aScheduler)
		{
			TargetIterations = aIterations;
			if (aScheduler)
			{
				Handle = Registry.Attach(this, aInterval, aEnable);
			}
		}

		virtual bool Callback() = 0;

		void Run() final
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
			if (!&Registry) return false;
			if (!Registry.IsEnabled(Handle))
			{
				if (!OnEnable())
				{
					return false;
				}
			}
			Registry.SetEnabled(Handle, true);
			return true;
		}

		bool enableIfNot()
		{
			return enable();
		}

		bool enableDelayed(unsigned long aDelay = 0)
		{
			if (!&Registry) return false;
			if (!Registry.IsEnabled(Handle))
			{
				OnEnable();
			}
			Registry.SetPeriodAndEnabled(Handle, aDelay, true);
			return isEnabled();
		}

		bool restart()
		{
			if (!&Registry) return false;
			if (!Registry.IsEnabled(Handle))
			{
				OnEnable();
			}
			const uint32_t delay = Registry.GetPeriod(Handle);
			Registry.SetPeriodAndEnabled(Handle, 0, false);
			Registry.SetPeriodAndEnabled(Handle, delay, true);
			return isEnabled();
		}

		bool restartDelayed(unsigned long aDelay = 0)
		{
			if (!&Registry) return false;
			if (!Registry.IsEnabled(Handle))
			{
				OnEnable();
			}
			Registry.SetPeriodAndEnabled(Handle, 0, false);
			Registry.SetPeriodAndEnabled(Handle, aDelay, true);
			return isEnabled();
		}

		void delay(unsigned long aDelay = 0)
		{
			Registry.SetPeriod(Handle, aDelay);
		}

		void adjust(long aInterval)
		{
			Registry.SetPeriodAndEnabled(Handle, 0, false);
			Registry.SetPeriodAndEnabled(Handle, aInterval, true);
		}

		void forceNextIteration()
		{
			if (!Registry.IsEnabled(Handle))
			{
				OnEnable();
				Registry.SetPeriodAndEnabled(Handle, 0, true);
			}
		}

		bool disable()
		{
			if (isEnabled())
			{
				Registry.SetEnabled(Handle, false);
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
			return Registry.IsEnabled(Handle);
		}

		bool canceled()
		{
			return !isEnabled();
		}

		void set(unsigned long aInterval, long aIterations)
		{
			TargetIterations = aIterations;
			Registry.SetPeriod(Handle, aInterval);
		}

		void setInterval(unsigned long aInterval)
		{
			Registry.SetPeriod(Handle, aInterval);
		}

		void setIntervalNodelay(unsigned long aInterval, unsigned int aOption)
		{
			const bool enabled = Registry.IsEnabled(Handle);
			Registry.SetPeriodAndEnabled(Handle, 0, false);
			Registry.SetPeriodAndEnabled(Handle, aInterval, enabled);
		}

		unsigned long getInterval()
		{
			return Registry.GetPeriod(Handle);
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
				return Iterations >= TargetIterations;
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