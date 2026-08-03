#ifndef _HARMONIC_PLATFORM_RTOS_h
#define _HARMONIC_PLATFORM_RTOS_h

#include "Platform.h"

#if defined(HARMONIC_PLATFORM_OS)
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#elif defined(HARMONIC_PLATFORM_RTOS)
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#else
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#endif
#endif

namespace Harmonic
{
#if defined(HARMONIC_PLATFORM_OS)
	struct DesktopSemaphore
	{
		std::mutex Mutex;
		std::condition_variable Condition;
		bool Signaled = false;
	};

	using SemaphoreHandle_t = DesktopSemaphore*;
	using BaseType_t = int;

	inline SemaphoreHandle_t xSemaphoreCreateBinary()
	{
		return new DesktopSemaphore();
	}

	inline void vSemaphoreDelete(SemaphoreHandle_t semaphore)
	{
		delete semaphore;
	}

	inline bool xSemaphoreTake(SemaphoreHandle_t semaphore, const uint32_t timeoutMs)
	{
		std::unique_lock<std::mutex> lock(semaphore->Mutex);
		if (!semaphore->Signaled)
		{
			if (!semaphore->Condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return semaphore->Signaled; }))
				return false;
		}

		semaphore->Signaled = false;
		return true;
	}

	inline void xSemaphoreGiveFromISR(SemaphoreHandle_t semaphore, BaseType_t* /*higherPriorityTaskWoken*/)
	{
		{
			std::lock_guard<std::mutex> lock(semaphore->Mutex);
			semaphore->Signaled = true;
		}
		semaphore->Condition.notify_one();
	}

	inline void xSemaphoreGive(SemaphoreHandle_t semaphore)
	{
		xSemaphoreGiveFromISR(semaphore, nullptr);
	}

	static constexpr BaseType_t pdFALSE = 0;
	inline void portYIELD_FROM_ISR(const BaseType_t) {}
#endif

	namespace Platform
	{
		#if defined(HARMONIC_PLATFORM_RTOS)
		inline bool IsInISR()
		{
#if defined(portCHECK_IF_IN_ISR)
			return portCHECK_IF_IN_ISR() != pdFALSE;
#elif defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
			return xPortInIsrContext() != pdFALSE;
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2350)
			return xPortIsInsideInterrupt() != pdFALSE;
#else
			return false;
#endif
		}
		#endif

		#if defined(HARMONIC_PLATFORM_RTOS) || defined(HARMONIC_PLATFORM_OS)
		inline void SignalSemaphore(SemaphoreHandle_t semaphore)
		{
#if defined(HARMONIC_PLATFORM_RTOS)
			BaseType_t higherPriorityTaskWoken = pdFALSE;
			if (IsInISR())
			{
				xSemaphoreGiveFromISR(semaphore, &higherPriorityTaskWoken);
				portYIELD_FROM_ISR(higherPriorityTaskWoken);
			}
			else
				xSemaphoreGive(semaphore);
#else
			xSemaphoreGive(semaphore);
#endif
		}
#endif
	}
}

#endif