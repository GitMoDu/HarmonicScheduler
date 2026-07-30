#ifndef _HARMONIC_PROFILING_TASK_NAME_PROVIDERS_h
#define _HARMONIC_PROFILING_TASK_NAME_PROVIDERS_h

#include "../Model/Profiling.h"

#if defined(ARDUINO_ARCH_AVR)
#include <avr/pgmspace.h>
#endif

namespace Harmonic
{
	namespace Profiling
	{
		static constexpr task_index_t TaskNameMaxLength = 16;

		/// <summary>
		/// Cached task name provider that uses a fixed-size array to store task names in RAM.
		/// Task names must not exceed TaskNameMaxLength characters.
		/// </summary>
		/// <typeparam name="MaxTaskCount">The maximum number of tasks that can be registered.</typeparam>
		template<task_index_t MaxTaskCount>
		class CachedTaskNameRegistry : public ITaskNameProvider
		{
		private:
			struct RegisteredTask
			{
				// Task name is stored as a fixed-size character array.
				// The +1 is for the null terminator.
				char Name[TaskNameMaxLength + 1] = { '\0' };

				// Task handle is initialized to an invalid value to indicate unregistered state.
				task_handle_t Handle = TASK_INVALID_HANDLE;
			};

		private:
			RegisteredTask Tasks[MaxTaskCount]{};
			task_index_t TaskCount = 0;

		public:
			CachedTaskNameRegistry() : ITaskNameProvider()
			{}

			virtual bool IsTaskKnown(const task_handle_t handle) const override
			{
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].Handle != TASK_INVALID_HANDLE && handle == Tasks[i].Handle)
					{
						return true;
					}
				}
				return false;
			}

			virtual const char* GetTaskName(const task_handle_t handle) const override
			{
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].Handle != TASK_INVALID_HANDLE && handle == Tasks[i].Handle)
					{
						return Tasks[i].Name;
					}
				}

				return "\0";
			}

			bool SetTaskName(const task_handle_t handle, const __FlashStringHelper* pstrName)
			{
				if (handle == TASK_INVALID_HANDLE || pstrName == nullptr)
				{
					return false;
				}

#if defined(ARDUINO_ARCH_AVR)
				// Check if the task handle is already registered
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].Handle == handle)
					{
						// Update the name for the existing task handle
						SetTaskNameFromFlash(i, pstrName);
						return true;
					}
				}

				if (TaskCount < MaxTaskCount)
				{
					SetTaskNameFromFlash(TaskCount, pstrName);
					Tasks[TaskCount].Handle = handle;
					TaskCount++;

					return true;
				}
				else
				{
					return false;
				}
#else
				// Direct pointer cast on flat-memory architectures (ARM, ESP32, RP2040, etc.)
				const char* name = reinterpret_cast<const char*>(pstrName);
				return SetTaskName(handle, name);
#endif
			}

			/// <summary>
			/// Sets the name for a given task handle.
			/// </summary>
			/// <param name="handle">The handle of the task.</param>
			/// <param name="name">The name to assign to the task. Must be not null and not exceed TaskNameMaxLength characters.</param>
			/// <returns>True if the name was successfully set, false otherwise.</returns>
			bool SetTaskName(const task_handle_t handle, const char* name)
			{
				// Check if the task handle is already registered
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					if (Tasks[i].Handle == handle && name != nullptr)
					{
						// Update the name for the existing task handle
						strncpy(Tasks[i].Name, name, TaskNameMaxLength - 1);
						Tasks[i].Name[TaskNameMaxLength] = '\0'; // Ensure null-termination

						return true;
					}
				}

				// If the task handle is not registered, add a new entry if there's space.
				if (TaskCount < MaxTaskCount && name != nullptr)
				{
					Tasks[TaskCount].Handle = handle;
					strncpy(Tasks[TaskCount].Name, name, TaskNameMaxLength - 1);
					Tasks[TaskCount].Name[TaskNameMaxLength] = '\0'; // Ensure null-termination
					TaskCount++;
					return true;
				}

				return false;
			}
		private:
#if defined(ARDUINO_ARCH_AVR)
			void SetTaskNameFromFlash(const task_index_t index, const __FlashStringHelper* pstrName)
			{
				const char* src = reinterpret_cast<const char*>(pstrName);
				size_t i = 0;
				while (i < TaskNameMaxLength)
				{
					char c = static_cast<char>(pgm_read_byte(src + i));
					Tasks[index].Name[i] = c;
					if (c == '\0') break;
					i++;
				}
				Tasks[index].Name[i] = '\0'; // Guarantee null-termination
				Tasks[index].Name[TaskNameMaxLength] = '\0'; // Ensure null-termination
			}
#endif
		};

		/// <summary>
		/// Low RAM usage task name provider that uses a virtual function to provide task names,
		/// based on indexed registration for tasks, avoiding the need to store task names in RAM.
		/// Task names must not exceed TaskNameMaxLength characters.
		/// </summary>
		/// <typeparam name="TaskCount">The number of tasks to be managed by this provider.</typeparam>
		template<task_index_t TaskCount>
		class VirtualIndexedTaskNameProvider : public ITaskNameProvider
		{
		private:
			task_handle_t TaskHandles[TaskCount]{};

		protected:
			/// <summary>
			/// Gets the name of a task based on its index.
			/// Override this method in derived classes to provide task names for known indices.
			/// Task names must not exceed TaskNameMaxLength characters.
			/// </summary>
			/// <param name="index">The index of the task.</param>
			/// <returns>The name of the task.</returns>
			virtual const char* GetIndexedTaskName(const task_index_t index) const = 0;

		public:
			VirtualIndexedTaskNameProvider() : ITaskNameProvider()
			{
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					TaskHandles[i] = TASK_INVALID_HANDLE;
				}
			}

			virtual bool IsTaskKnown(const task_handle_t handle) const override
			{
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					if (TaskHandles[i] != TASK_INVALID_HANDLE && handle == TaskHandles[i])
					{
						return true;
					}
				}
				return false;
			}

			virtual const char* GetTaskName(const task_handle_t handle) const override
			{
				for (task_index_t i = 0; i < TaskCount; i++)
				{
					if (TaskHandles[i] != TASK_INVALID_HANDLE && handle == TaskHandles[i])
					{
						return GetIndexedTaskName(static_cast<task_index_t>(i));
					}
				}
				return "Unknown";
			}


			void SetTaskHandle(const task_index_t index, const task_handle_t handle)
			{
				if (index < TaskCount)
				{
					TaskHandles[index] = handle;
				}
			}
		};
	}
}
#endif