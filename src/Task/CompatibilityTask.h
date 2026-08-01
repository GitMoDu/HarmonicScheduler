#ifndef _HARMONIC_COMPATIBILITY_WRAPPER_h
#define _HARMONIC_COMPATIBILITY_WRAPPER_h

#include "../Model/ITask.h"
#include "../Model/TaskRegistry.h"

namespace TS
{
	using Scheduler = Harmonic::TaskRegistry;
	using TaskCallback = bool(*)();
	using TaskOnEnable = bool(*)();
	using TaskOnDisable = void(*)();

	static constexpr uint8_t TASK_IMMEDIATE = 0;
	static constexpr int8_t TASK_FOREVER = -1;
	static constexpr uint8_t TASK_ONCE = 1;
	static constexpr unsigned int TASK_INTERVAL_KEEP = 0;
	static constexpr unsigned int TASK_INTERVAL_RECALC = 1;
	static constexpr unsigned int TASK_INTERVAL_RESET = 2;

	class Task : public Harmonic::ITask
	{
	private:
		Harmonic::TaskRegistry* Registry;
		uint32_t Interval = 0;
		uint32_t DelayValue = 0;
		long StartDelay = 0;
		uint32_t RunCounter = 0;
		long SetIterationsValue = TASK_FOREVER;
		long IterationsRemaining = TASK_FOREVER;
		bool Canceled = false;
		bool InOnEnable = false;
		TaskCallback CallbackPointer = nullptr;
		TaskOnEnable OnEnablePointer = nullptr;
		TaskOnDisable OnDisablePointer = nullptr;
		Harmonic::task_handle_t Handle = Harmonic::TASK_INVALID_HANDLE;

	protected:
		virtual bool OnEnable()
		{
			return true;
		}

		virtual void OnDisable()
		{}

		bool InvokeOnEnable()
		{
			if (InOnEnable)
			{
				return false;
			}
			InOnEnable = true;
			const bool enabled = OnEnablePointer ? OnEnablePointer() : OnEnable();
			InOnEnable = false;
			return enabled;
		}

		void InvokeOnDisable()
		{
			if (OnDisablePointer)
			{
				OnDisablePointer();
			}
			else
			{
				OnDisable();
			}
		}

		bool InvokeCallback()
		{
			if (CallbackPointer)
			{
				return CallbackPointer();
			}
			return Callback();
		}

	public:
		Task(unsigned long aInterval, long aIterations, Scheduler* aScheduler, bool aEnable)
			: Harmonic::ITask()
			, Registry(aScheduler)
		{
			Interval = aInterval;
			setIterations(aIterations);
			if (Registry != nullptr)
			{
				Handle = Registry->Attach(this, Interval, aEnable);
				if (aEnable)
				{
					enable();
				}
			}
			else
			{
				Handle = Harmonic::TASK_INVALID_HANDLE;
			}
		}

		~Task() override = default;
		virtual bool Callback() = 0;

		void Run() override final
		{
			if (isLastIteration())
			{
				disable();
				return;
			}

			Registry->SetDelay(Handle, Interval);
			DelayValue = Interval;
			RunCounter++;
			if (IterationsRemaining > 0)
			{
				IterationsRemaining--;
			}

			InvokeCallback();

			if (isLastIteration())
			{
				disable();
			}
		}

		bool enable()
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE)
			{
				return false;
			}
			RunCounter = 0;
			Canceled = false;

			const bool enabled = InvokeOnEnable();
			Registry->SetEnabled(Handle, enabled);
			if (!enabled)
			{
				return false;
			}

			DelayValue = Interval;
			StartDelay = 0;
			Registry->SetDelay(Handle, Interval);
			Registry->WakeFromISR(Handle);
			return true;
		}

		bool enableIfNot()
		{
			const bool previousEnabled = isEnabled();
			if (!previousEnabled)
			{
				enable();
			}
			return previousEnabled;
		}

		bool enableDelayed(unsigned long aDelay = 0)
		{
			if (!enable())
			{
				return false;
			}
			delay(aDelay);
			return isEnabled();
		}

		bool restart()
		{
			setIterations(SetIterationsValue);
			return enable();
		}

		bool restartDelayed(unsigned long aDelay = 0)
		{
			setIterations(SetIterationsValue);
			return enableDelayed(aDelay);
		}

		void delay(unsigned long aDelay = 0)
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE)
			{
				return;
			}
			DelayValue = aDelay ? aDelay : Interval;
			StartDelay = static_cast<long>(DelayValue);
			Registry->SetDelayFromNow(Handle, DelayValue);
		}

		void adjust(long aInterval)
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE || aInterval == 0)
			{
				return;
			}
			int32_t adjustedDelay = static_cast<int32_t>(DelayValue) + static_cast<int32_t>(aInterval);
			if (adjustedDelay < 0)
			{
				adjustedDelay = 0;
			}
			DelayValue = static_cast<uint32_t>(adjustedDelay);
			Registry->SetDelay(Handle, DelayValue);
		}

		void forceNextIteration()
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE)
			{
				return;
			}
			DelayValue = Interval;
			Registry->SetDelay(Handle, DelayValue);
			Registry->WakeFromISR(Handle);
		}

		bool disable()
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE)
			{
				return false;
			}
			const bool previousEnabled = Registry->IsEnabled(Handle);
			Registry->SetEnabled(Handle, false);
			InOnEnable = false;
			if (previousEnabled)
			{
				InvokeOnDisable();
			}
			return previousEnabled;
		}

		void abort()
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE)
			{
				return;
			}
			Canceled = true;
			InOnEnable = false;
			Registry->SetEnabled(Handle, false);
		}

		void cancel()
		{
			Canceled = true;
			disable();
		}

		bool isEnabled()
		{
			if (Registry == nullptr || Handle == Harmonic::TASK_INVALID_HANDLE)
			{
				return false;
			}
			return Registry->IsEnabled(Handle);
		}

		bool isCanceled()
		{
			return Canceled;
		}

		bool canceled()
		{
			return isCanceled();
		}

		void set(unsigned long aInterval, long aIterations)
		{
			setIterations(aIterations);
			setInterval(aInterval);
		}

		void set(unsigned long aInterval, long aIterations, TaskCallback aCallback, TaskOnEnable aOnEnable = nullptr, TaskOnDisable aOnDisable = nullptr)
		{
			setOnEnable(aOnEnable);
			setOnDisable(aOnDisable);
			CallbackPointer = aCallback;
			set(aInterval, aIterations);
		}

		void setInterval(unsigned long aInterval)
		{
			Interval = aInterval;
			delay();
		}

		void setIntervalNodelay(unsigned long aInterval, unsigned int aOption = TASK_INTERVAL_KEEP)
		{
			switch (aOption)
			{
			case TASK_INTERVAL_RECALC:
			{
				const int32_t d = static_cast<int32_t>(aInterval) - static_cast<int32_t>(Interval);
				int32_t newDelay = static_cast<int32_t>(DelayValue) + d;
				if (newDelay < 0)
				{
					newDelay = 0;
				}
				DelayValue = static_cast<uint32_t>(newDelay);
				Interval = aInterval;
				break;
			}
			case TASK_INTERVAL_RESET:
				Interval = aInterval;
				DelayValue = aInterval;
				break;
			default:
				if (Interval == DelayValue)
				{
					Interval = aInterval;
					DelayValue = aInterval;
				}
				else
				{
					Interval = aInterval;
				}
				break;
			}

			if (Registry != nullptr && Handle != Harmonic::TASK_INVALID_HANDLE)
			{
				Registry->SetDelay(Handle, DelayValue);
			}
		}

		unsigned long getInterval()
		{
			return Interval;
		}

		void setIterations(long aIterations)
		{
			SetIterationsValue = aIterations;
			IterationsRemaining = aIterations;
		}

		long getIterations()
		{
			return IterationsRemaining;
		}

		unsigned long getRunCounter()
		{
			return RunCounter;
		}

		void setOnEnable(TaskOnEnable aCallback)
		{
			OnEnablePointer = aCallback;
		}

		void setOnDisable(TaskOnDisable aCallback)
		{
			OnDisablePointer = aCallback;
		}

		long getStartDelay()
		{
			return StartDelay;
		}

		bool isFirstIteration()
		{
			return RunCounter <= 1;
		}

		bool isLastIteration()
		{
			return IterationsRemaining == 0;
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
#define TASK_INTERVAL_KEEP   TS::TASK_INTERVAL_KEEP
#define TASK_INTERVAL_RECALC TS::TASK_INTERVAL_RECALC
#define TASK_INTERVAL_RESET  TS::TASK_INTERVAL_RESET

#endif
