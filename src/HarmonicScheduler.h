#ifndef _HARMONIC_SCHEDULER_INCLUDE_h
#define _HARMONIC_SCHEDULER_INCLUDE_h

// Platform abstraction headers
// - Provide platform-specific types, timestamp sources, sleep, and atomic operations.
#include "Platform/Platform.h"
#include "Platform/Timestamp.h"
#include "Platform/IdleSleep.h"
#include "Platform/Atomic.h"

// Core task model headers
// - Define the base task interface, registry, and tracking utilities.
#include "Model/ITask.h"
#include "Model/TaskRegistry.h"
#include "Model/TaskTracker.h"

// Scheduler implementation
// - TemplateScheduler provides the main scheduling logic for tasks.
#include "Scheduler/TemplateScheduler.h"

// Task types and wrappers
// - DynamicTask: Base class for runtime-configurable tasks.
// - DynamicTaskWrapper: Utility for wrapping tasks with additional behavior.
#include "Task/DynamicTask.h"
#include "Task/ExposedDynamicTask.h"
#include "Task/DynamicTaskWrapper.h"
#include "Task/CallableTask.h"

// Interrupt-driven task types
// - Provide ready-to-use tasks for flag, signal, and event-based interrupt handling.
#include "Task/InterruptFlagTask.h"
#include "Task/InterruptSignalTask.h"
#include "Task/InterruptEventTask.h"

#endif