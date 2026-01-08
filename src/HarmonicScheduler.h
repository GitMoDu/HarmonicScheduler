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

// Profiling level definitions
// - Define profiling levels for use in template scheduler/profiler selection.
#include "Model/Profiling.h"

// Scheduler implementations
// - TemplateScheduler provides templated selector for scheduler configurations.
// - NoProfiling, BaseProfiling, and FullProfiling provide specific scheduler implementations.
#include "Scheduler/NoProfiling.h"
#include "Scheduler/BaseProfiling.h"
#include "Scheduler/FullProfiling.h"
#include "Scheduler/Template.h"

// Profile trace logging tasks
// - Provide templated tasks for logging profiling traces.
#include "Task/TraceLogTask.h"

// Task types and wrappers
// - DynamicTask: Base class for runtime-configurable tasks.
// - ExposedDynamicTask: Dynamic task variant exposing additional interfaces.
// - DynamicTaskWrapper: Utility for wrapping tasks with additional behavior.
// - CallableTask: Task implementation for callable objects (e.g., functions, lambdas).
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