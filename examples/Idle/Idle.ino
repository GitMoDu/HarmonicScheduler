/* 
* Co-Op Task Idle example.
* This is a test to prove that processor really goes into IDLE sleep.
* Compile and run once with IdleSleepEnabled enabled, then with IdleSleepEnabled disabled.
* Compare the results.
*/

#define IdleSleepEnabled true

#include <Arduino.h>
#include <HarmonicScheduler.h>

class OneShotTask : public Harmonic::DynamicTask
{
private:
	enum class StateEnum
	{
		Starting,
		Stopping
	};

private:
	Harmonic::DynamicTask* ForeverTask;
	uint32_t& Counter1;
	uint32_t& Counter2;
	StateEnum State = StateEnum::Starting;

public:
	OneShotTask(Harmonic::IScheduler& scheduler
		, Harmonic::DynamicTask* foreverTask
		, uint32_t& counter1, uint32_t& counter2)
		: Harmonic::DynamicTask(scheduler)
		, ForeverTask(foreverTask)
		, Counter1(counter1)
		, Counter2(counter2)
	{
	}

	const bool Setup()
	{
		return AttachTask(0);
	}

	virtual void Run() final
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
			ForeverTask->SetEnabled(false);

			Serial.print(F("c1=")); Serial.println(Counter1);
			Serial.print(F("c2=")); Serial.println(Counter2);
			Serial.flush();
			break;
		}
	}
};

class ForeverTask : public Harmonic::DynamicTask
{
private:
	volatile uint32_t& Counter2;

public:
	ForeverTask(Harmonic::IScheduler& scheduler, uint32_t& counter2)
		: Harmonic::DynamicTask(scheduler)
		, Counter2(counter2)
	{
	}

	const bool Setup()
	{
		return AttachTask(10);
	}

	virtual void Run() final
	{
		Counter2++;
	}
};

Harmonic::TemplateScheduler<2, IdleSleepEnabled> Harmony{};

uint32_t Counter1 = 0;
uint32_t Counter2 = 0;

ForeverTask Forever(Harmony, Counter2);
OneShotTask Once(Harmony, &Forever, Counter1, Counter2);


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
	Harmony.Loop();

	Counter1++;
}