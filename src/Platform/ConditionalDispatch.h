#ifndef _HARMONIC_PLATFORM_CONDITIONAL_DISPATCH_h
#define _HARMONIC_PLATFORM_CONDITIONAL_DISPATCH_h

#include <stdint.h>

namespace Harmonic
{
	/// <summary>
	/// SFINAE-based conditional dispatch utilities.
	/// </summary>
	namespace ConditionalDispatch
	{
		struct TrueType {};
		struct FalseType {};

		template<bool Condition>
		struct conditional_type
		{
			using type = FalseType;
		};

		template<>
		struct conditional_type<true>
		{
			using type = TrueType;
		};
	}
}
#endif