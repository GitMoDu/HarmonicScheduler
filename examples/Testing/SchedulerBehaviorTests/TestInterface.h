#ifndef _TESTINTERFACE_h
#define _TESTINTERFACE_h

namespace Harmonic
{
	struct ITester
	{
		virtual void OnTestTaskDone(const bool pass) = 0;
	};

	struct ITestTask
	{
		virtual void StartTest(ITester* testListener) = 0;
		virtual void PrintName() = 0;
	};
}

#endif

