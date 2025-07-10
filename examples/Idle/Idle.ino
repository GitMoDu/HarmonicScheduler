/*
* Harmonic Task Idle example.
* This is a test to prove that processor really goes into IDLE sleep.
* Compile and run once with IdleSleepEnabled enabled, then with IdleSleepEnabled disabled.
* Compare the results.
*/

#include <Arduino.h>
#include <HarmonicScheduler.h>

static constexpr bool IdleSleep = true;

class ForeverTask : public Harmonic::DynamicTask
{
private:
	volatile uint32_t& Counter2;

public:
	ForeverTask(Harmonic::TaskRegistry& registry, uint32_t& counter2)
		: Harmonic::DynamicTask(registry)
		, Counter2(counter2)
	{
	}

	void Stop()
	{
		DynamicTask::SetEnabled(false);
	}

	bool Setup()
	{
		return DynamicTask::Attach(10, true);
	}

	void Run() final
	{
		Counter2++;
	}
};


class OneShotTask : public Harmonic::DynamicTask
{
private:
	enum class StateEnum
	{
		Starting,
		Stopping
	};

private:
	ForeverTask& Forever;
	uint32_t& Counter1;
	uint32_t& Counter2;
	StateEnum State = StateEnum::Starting;

public:
	OneShotTask(Harmonic::TaskRegistry& registry
		, ForeverTask& foreverTask
		, uint32_t& counter1, uint32_t& counter2)
		: Harmonic::DynamicTask(registry)
		, Forever(foreverTask)
		, Counter1(counter1)
		, Counter2(counter2)
	{
	}

	bool Setup()
	{
		return DynamicTask::Attach(0, true);
	}

	void Run() final
	{
		switch (State)
		{
		case StateEnum::Starting:
			Counter1 = 0;
			Counter2 = 0;
			State = StateEnum::Stopping;
			DynamicTask::SetDelay(10000);
			break;
		case StateEnum::Stopping:
		default:
			DynamicTask::SetEnabled(false);
			Forever.Stop();

			Serial.print(F("c1=")); Serial.println(Counter1);
			Serial.print(F("c2=")); Serial.println(Counter2);
			Serial.flush();
			break;
		}
	}
};


Harmonic::TemplateScheduler<2, IdleSleep> Runner{};

uint32_t Counter1 = 0;
uint32_t Counter2 = 0;

ForeverTask Forever(Runner, Counter2);
OneShotTask Once(Runner, Forever, Counter1, Counter2);


void setup()
{
	Serial.begin(115200);
	while (!Serial)
		;;

	delay(1000);
	Serial.println(F("Start"));

	if (!Forever.Setup() || !Once.Setup())
	{
		Serial.println(F("Setup Error"));
		while (true)
			;;
	}
}

void loop()
{
	Runner.Loop();

	Counter1++;
}