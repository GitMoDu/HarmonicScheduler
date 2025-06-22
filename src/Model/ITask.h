#ifndef _HARMONIC_ITASK_h
#define _HARMONIC_ITASK_h

#include <stdint.h>

namespace Harmonic
{
	/// <summary>
	/// TaskId type.
	/// </summary>
	typedef uint8_t task_id_t;

	/// <summary>
	/// Class interface for Harmonic cooperative Task.
	/// </summary>
	class ITask
	{
	public:
		/// <summary>
		/// Execute run callback.
		/// Total execution time should be under 1 ms.
		/// </summary>
		virtual void Run() {}
	};
}
#endif