#ifndef _HARMONIC_ITASK_h
#define _HARMONIC_ITASK_h

#include <stdint.h>

namespace Harmonic
{
	/// <summary>
	/// Abstract interface for a cooperative task in the Harmonic framework.
	/// 
	/// Classes implementing ITask must provide a Run() method containing the task's run callback.
	/// Tasks are expected to be non-blocking and complete execution quickly (typically under 1 ms).
	/// </summary>
	class ITask
	{
	public:
		/// <summary>
		/// Task execution callback.
		/// Override this method to implement the task's behavior.
		/// The method should return quickly and must not block.
		/// </summary>
		virtual void Run() = 0;
	};
}
#endif