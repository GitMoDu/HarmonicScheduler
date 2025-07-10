#ifndef _HARMONIC_PLATFORM_TIMESTAMP_h
#define _HARMONIC_PLATFORM_TIMESTAMP_h

#include "Platform.h"

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(WINDOWS)
#endif

namespace Harmonic
{
	/// <summary>
	/// Platform specific implementations for timestamp source and idle sleep.
	/// </summary>
	namespace Platform
	{
		/// <summary>
		/// Get the current time.
		/// </summary>
		/// <returns>Timestamp in milliseconds.</returns>
		static uint32_t GetTimestamp()
		{
#if defined(ARDUINO)
			return millis();
#else
#error No timestamp source for scheduler.
#endif
		}
	}
}
#endif