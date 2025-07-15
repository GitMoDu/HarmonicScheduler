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
		Harmonic::task_id_t TaskId = UINT8_MAX;

		uint32_t Iterations = 0;
		int32_t TargetIterations = INT32_MAX;

	protected:
		virtual bool OnEnable()
		{
			return true;
		}

		virtual void OnDisable()
		{
		}

	public:
		Task(unsigned long aInterval, long aIterations, Scheduler* aScheduler, bool aEnable)
			: Harmonic::ITask()
			, Registry(*aScheduler)
		{
			TargetIterations = aIterations;
			if (aScheduler)
			{
				Registry.Attach(this, TaskId, aInterval, aEnable);
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
			if (!Registry.IsEnabled(TaskId))
			{
				if (!OnEnable())
				{
					return false;
				}
			}
			Registry.SetEnabled(TaskId, true);
			return true;
		}

		bool enableIfNot()
		{
			return enable();
		}

		bool enableDelayed(unsigned long aDelay = 0)
		{
			if (!&Registry) return false;
			if (!Registry.IsEnabled(TaskId))
			{
				OnEnable();
			}
			Registry.SetDelayEnabled(TaskId, aDelay, true);
			return isEnabled();
		}

		bool restart()
		{
			if (!&Registry) return false;
			if (!Registry.IsEnabled(TaskId))
			{
				OnEnable();
			}
			const uint32_t delay = Registry.GetDelay(TaskId);
			Registry.SetDelayEnabled(TaskId, 0, false);
			Registry.SetDelayEnabled(TaskId, delay, true);
			return isEnabled();
		}

		bool restartDelayed(unsigned long aDelay = 0)
		{
			if (!&Registry) return false;
			if (!Registry.IsEnabled(TaskId))
			{
				OnEnable();
			}
			Registry.SetDelayEnabled(TaskId, 0, false);
			Registry.SetDelayEnabled(TaskId, aDelay, true);
			return isEnabled();
		}

		void delay(unsigned long aDelay = 0)
		{
			Registry.SetDelay(TaskId, aDelay);
		}

		void adjust(long aInterval)
		{
			Registry.SetDelayEnabled(TaskId, 0, false);
			Registry.SetDelayEnabled(TaskId, aInterval, true);
		}

		void forceNextIteration()
		{
			if (!Registry.IsEnabled(TaskId))
			{
				OnEnable();
				Registry.SetDelayEnabled(TaskId, 0, true);
			}
		}

		bool disable()
		{
			if (isEnabled())
			{
				Registry.SetEnabled(TaskId, false);
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
			return Registry.IsEnabled(TaskId);
		}

		bool canceled()
		{
			return !isEnabled();
		}

		void set(unsigned long aInterval, long aIterations)
		{
			TargetIterations = aIterations;
			Registry.SetDelay(TaskId, aInterval);
		}

		void setInterval(unsigned long aInterval)
		{
			Registry.SetDelay(TaskId, aInterval);
		}

		void setIntervalNodelay(unsigned long aInterval, unsigned int aOption)
		{
			const bool enabled = Registry.IsEnabled(TaskId);
			Registry.SetDelayEnabled(TaskId, 0, false);
			Registry.SetDelayEnabled(TaskId, aInterval, enabled);
		}

		unsigned long getInterval()
		{
			return Registry.GetDelay(TaskId);
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