#ifndef _PINLISTENERS_h
#define _PINLISTENERS_h

#include <HarmonicScheduler.h>

class PinInterruptListener : public Harmonic::InterruptFlag::InterruptListener
{
private:
	Harmonic::InterruptFlag::CallbackTask WakeTask;
	uint8_t Pin;

public:
	PinInterruptListener(Harmonic::TaskRegistry& registry, const uint8_t pin)
		: WakeTask(registry)
		, Pin(pin)
	{
	}

	void OnInterrupt()
	{
		// Set the interrupt flag and wake the task.
		WakeTask.OnInterrupt();
	}

	bool Setup(void (*isrFunction)(void))
	{
		if (isrFunction != nullptr && WakeTask.AttachListener(this))
		{
			pinMode(Pin, INPUT_PULLUP); // Set pin as input with pull-up resistor.
			attachInterrupt(digitalPinToInterrupt(Pin), isrFunction, FALLING);

			return true;
		}

		return false;
	}

public:
	void OnFlagInterrupt() final
	{
		// Safely print from outside the ISR.
		Serial.print(F("Pin "));
		Serial.print(Pin);
		Serial.println(F(" Interrupt."));
	}
};

class PinCountListener : public Harmonic::InterruptSignal::InterruptListener<>
{
private:
	Harmonic::InterruptSignal::CallbackTask<> WakeTask;
	uint8_t Pin;

public:
	PinCountListener(Harmonic::TaskRegistry& registry, const uint8_t pin)
		: WakeTask(registry)
		, Pin(pin)
	{
	}

	void OnInterrupt()
	{
		// Set the interrupt flag and wake the task.
		WakeTask.OnInterrupt();
	}

	bool Setup(void (*isrFunction)(void))
	{
		if (isrFunction != nullptr && WakeTask.AttachListener(this))
		{
			pinMode(Pin, INPUT_PULLUP); // Set pin as input with pull-up resistor.
			attachInterrupt(digitalPinToInterrupt(Pin), isrFunction, FALLING);

			return true;
		}

		return false;
	}

public:
	void OnSignalInterrupt(const uint8_t signalCount) final
	{
		// Safely print from outside the ISR.
		Serial.print(F("Pin "));
		Serial.print(Pin);
		Serial.print(F(" Interrupt ("));
		Serial.print(signalCount);
		Serial.println(F(" counts)."));
	}
};

#endif

