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
	/// The registry is limited to 252 (TASK_MAX_COUNT) tasks.
	/// Values 253-255 are reserved for special purposes, such as TASK_INVALID_HANDLE, TRACE_SLEEP_HANDLE and TRACE_TASK_HANDLE.
	///
	/// A handle is stable (but not lifetime-unique) while its task is attached, even when tasks are removed and added.
	/// Task identifiers must not be retained after removal.
	/// </summary>
	using task_handle_t = uint8_t;

	/// <summary>
	/// Explicit alias for task_handle_t indexing and counting.
	/// </summary>
	using task_index_t = uint_fast8_t;

	/// <summary>
	/// Sentinel returned when a task could not be attached or has no handle.
	/// </summary>
	static constexpr task_handle_t TASK_INVALID_HANDLE = UINT8_MAX;

	/// <summary>
	/// Profiling alias for the scheduler's own trace handle,
	/// used to mark timeline samples when the scheduler is running.
	/// </summary>
	static constexpr task_handle_t TRACE_SCHEDULER_HANDLE = TASK_INVALID_HANDLE;

	/// <summary>
	/// Profiling reserved trace handle for idle sleep state,
	/// used to mark timeline samples when the scheduler is in idle sleep.
	/// </summary>
	static constexpr task_handle_t TRACE_SLEEP_HANDLE = TASK_INVALID_HANDLE - 1;

	/// <summary>
	/// Profiling reserved trace handle for timeline tracing work,
	/// used to mark timeline samples when the trace output is running.
	/// </summary>
	static constexpr task_handle_t TRACE_TASK_HANDLE = TRACE_SLEEP_HANDLE - 1;

	/// <summary>
	/// The maximum number of tasks supported by a task registry.
	/// </summary>
	static constexpr task_index_t TASK_MAX_COUNT = TRACE_TASK_HANDLE - 1;

	/// <summary>
	/// Generic Id for Timeline trace samples that are not associated with a specific task.
	/// Not reserved as task handle and does not influence max task count.
	/// </summary>
	static constexpr task_handle_t TRACE_ACTIVE_HANDLE = 0;
}
#endif