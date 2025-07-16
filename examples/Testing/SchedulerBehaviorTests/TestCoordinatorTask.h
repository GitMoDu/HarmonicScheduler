#ifndef _TESTCOORDINATORTASK_h
#define _TESTCOORDINATORTASK_h

#include "TestInterface.h"
#include <HarmonicScheduler.h>

namespace Harmonic
{
	template<uint8_t Capacity>
	class TestCoordinatorTask : public ITester, public DynamicTask
	{
	private:
		ITestTask* TestTasks[Capacity]{};
		uint8_t Count = 0;
		uint8_t TestIndex = 0;
		bool AllPass = false;

	public:
		TestCoordinatorTask(TaskRegistry& registry)
			: ITester()
			, DynamicTask(registry)
		{
		}

		bool AddTestTask(ITestTask* testTask)
		{
			if (Count < Capacity)
			{
				TestTasks[Count] = testTask;
				Count++;

				return true;
			}

			return false;
		}

		void Run() final
		{
			SetEnabled(false);
			if (TestIndex < Count)
			{
				Serial.print(F("Starting "));
				TestTasks[TestIndex]->PrintName();
				Serial.println();
				TestTasks[TestIndex]->StartTest(this);
			}
			else
			{
				if (AllPass)
				{
					Serial.println();
					Serial.println(F("All Task Tests Passed."));
					Serial.println();
				}
			}
		}

		bool Start()
		{
			AllPass = DynamicTask::Attach(0, true);
			TestIndex = 0;
			if (AllPass)
			{
				Serial.print(F("Running "));
				Serial.print(Count);
				Serial.println(F(" Task Tests"));
				Serial.println();
			}
			return AllPass;
		}

		void OnTestTaskDone(const bool pass) final
		{
			Serial.print('\t');
			TestTasks[TestIndex]->PrintName();
			if (pass)
			{
				Serial.println(F(" Passed"));
			}
			else
			{
				AllPass = false;
				Serial.println(F(" Failed"));
			}
			TestIndex++;
			SetPeriodAndEnabled(0, true);
		}
	};
}

#endif

