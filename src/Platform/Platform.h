#ifndef _HARMONIC_PLATFORM_h
#define _HARMONIC_PLATFORM_h

#include <stdint.h>

namespace Harmonic
{
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_AVR_MEGA2560)
#elif defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4)
#elif defined(ARDUINO_ARCH_STM32)
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
#if defined(__FREERTOS)
#define HARMONIC_PLATFORM_RTOS
#endif
#elif defined(ARDUINO_ARCH_NRF52)
#define HARMONIC_PLATFORM_RTOS
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#define HARMONIC_PLATFORM_RTOS
#elif defined(_WIN32) || defined(_WIN64) || defined(__linux__)
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
	/// Registry reference used to address an attached task, as well as task count type.
	/// The registry is limited to 254 tasks.
	///
	/// A handle remains stable while its task is attached, even when the
	/// registry compacts its dense task list. Handles are registry-local and
	/// may be recycled after Detach() or Clear(); they are not lifetime-unique
	/// task identifiers and must not be retained after removal.
	/// </summary>
	using task_handle_t = uint_fast8_t;

	/// <summary>
	/// Sentinel returned when a task could not be attached or has no handle.
	/// This limits the maximum number of tasks to 254, as the handle type is 8 bits.
	/// </summary>
	static constexpr task_handle_t TASK_INVALID_HANDLE = UINT8_MAX;

	/// <summary>
	/// The maximum number of tasks that can be attached to a scheduler registry.
	/// </summary>
	static constexpr size_t TASK_MAX_COUNT = UINT8_MAX - 1;
}
#endif