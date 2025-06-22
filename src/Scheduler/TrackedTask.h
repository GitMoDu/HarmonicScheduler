#ifndef _HARMONIC_TRACKED_TASK_h
#define _HARMONIC_TRACKED_TASK_h

#include "../Model/ITask.h"

namespace Harmonic
{
	/// <summary>
	/// Companion structure to run and track an Harmonic ITask.
	/// </summary>
	class TrackedTask
	{
	private:
		ITask* Task = nullptr;

		volatile uint32_t Delay = 0;

		uint32_t LastRun = 0;

		volatile bool Enabled = false;

	public:
		void SetTask(ITask* task, const uint32_t timestamp, const uint32_t delay, const bool enabled)
		{
			Task = task;
			LastRun = timestamp;
			Set(delay, enabled);
		}

		ITask* GetTask() const
		{
			return Task;
		}

		void SetDelay(const uint32_t delay)
		{
			Delay = delay;
		}

		void SetEnabled(const bool enabled)
		{
			Enabled = enabled;
		}

		void Set(const uint32_t delay, const bool enabled)
		{
			SetDelay(delay);
			SetEnabled(enabled);
		}

		/// <summary>
		/// Execute base callback and store run timestamp.
		/// </summary>
		void Run(const uint32_t timestamp)
		{
			Task->Run();
			LastRun = timestamp;
		}

		/// <summary>
		/// Rollback the last run timestamp.
		/// </summary>
		/// <param name="offset"></param>
		void Rollback(const uint32_t offset)
		{
			LastRun -= offset;
		}

		void RunIfTime(const uint32_t timestamp)
		{
			if (Enabled &&
				(Delay == 0 || ((timestamp - LastRun) >= Delay)))
			{
				Task->Run();
				LastRun = timestamp;
			}
		}

		bool ShouldRun(const uint32_t timestamp) const
		{
			if (!Enabled)
			{
				return false;
			}
			else if (Delay == 0)
			{
				return true;
			}
			else
			{
				const uint32_t elapsedSinceLastRun = timestamp - LastRun;

				if (elapsedSinceLastRun >= Delay)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		}

		uint32_t TimeUntilNextRun(const uint32_t timestamp) const
		{
			if (!Enabled)
			{
				return UINT32_MAX;
			}
			else if (Delay == 0)
			{
				return 0;
			}
			else
			{
				const uint32_t elapsedSinceLastRun = timestamp - LastRun;

				if (elapsedSinceLastRun >= Delay)
				{
					return 0;
				}
				else
				{
					return Delay - elapsedSinceLastRun;
				}
			}
		}
	};
}
#endif