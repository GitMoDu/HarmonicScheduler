#ifndef _HARMONIC_PLATFORM_h
#define _HARMONIC_PLATFORM_h

#include <stdint.h>

namespace Harmonic
{
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_AVR_MEGA2560)
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
#elif defined(ARDUINO_ARCH_STM32)
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
#define HARMONIC_PLATFORM_OS
#elif defined(ARDUINO_ARCH_NRF52)
#define HARMONIC_PLATFORM_OS
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#define HARMONIC_PLATFORM_OS
#elif defined(WINDOWS)
#define HARMONIC_PLATFORM_OS
#else
#error Harmonic::Platform not supported
#endif

#if !defined(UINTPTR_MAX)  || (defined(UINTPTR_MAX) && (UINTPTR_MAX < 0xFFFFFFFF))
	// Use atomic protection on platforms with pointer size < 32 bits,
	// or if UINTPTR_MAX is not defined (safe fallback).
#define HARMONIC_PLATFORM_ATOMIC_NARROW
#endif

	/// <summary>
	/// TaskId type and count type.
	/// </summary>
	typedef uint_fast8_t task_id_t;

	static constexpr task_id_t TASK_INVALID_ID = UINT8_MAX;
	static constexpr size_t TASK_MAX_COUNT = UINT8_MAX - 1;
}
#endif