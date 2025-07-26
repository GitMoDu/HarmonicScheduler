/*
 * Async Serial Print from Interrupt Example
 *
 * Demonstrates how to safely handle interrupts with deferred listeners.
 * Interrupt Service Routines (ISRs) quickly signal tasks,
 * and all time-consuming operations (like printing)
 * are performed asynchronously within the main loop.
 *
 * This pattern avoids unsafe operations inside ISRs and enables responsive,
 * non-blocking event handling in embedded applications.
 */


#include <Arduino.h>
#include <HarmonicScheduler.h>
#include "PinListeners.h"

#if defined(ARDUINO_ARCH_AVR)
#define PIN_A 2
#define PIN_B 3
#endif


// Harmonic scheduler.
Harmonic::TemplateScheduler<2> Runner{};

// Interrupt flag listener for pin A.
PinInterruptListener PinListenerA(Runner, PIN_A);

// Interrupt count listener for pin B.
PinCountListener PinListenerB(Runner, PIN_B);

void error()
{
	Serial.print(F("Setup error."));
	while (true)
		;;
}

void setup()
{
	Serial.begin(115200);

	// Wait for serial connection (for boards that require it).
	while (!Serial)
		;;

	delay(1000);

	if (!PinListenerA.Setup(onPinAInterrupt)
		|| !PinListenerB.Setup(onPinBInterrupt))
	{
		error();
	}

	Serial.println();
	Serial.println(F("Async Serial Print from Interrupt Start..."));
}

void loop()
{
	Runner.Loop();
}

void onPinAInterrupt()
{
	// Notify the task that an interrupt occurred.
	PinListenerA.OnInterrupt();
}

void onPinBInterrupt()
{
	// Notify the task that an interrupt occurred.
	PinListenerB.OnInterrupt();
}