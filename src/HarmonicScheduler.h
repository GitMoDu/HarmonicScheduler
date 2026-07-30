#ifndef _HARMONIC_SCHEDULER_INCLUDE_h
#define _HARMONIC_SCHEDULER_INCLUDE_h

// Platform abstraction headers
// - Provide platform-specific types, timestamp sources, sleep, and atomic operations.
#include "Platform/Platform.h"
#include "Platform/Timestamp.h"
#include "Platform/IdleSleep.h"
#include "Platform/Atomic.h"

// Core task interface model.
#include "Model/ITask.h"

// Registry and tracking, shared between schedulers.
#include "Model/TaskRegistry.h"
#include "Model/TaskTracker.h"

// Profiling model and interfaces for metrics and timeline event streaming.
#include "Model/Profiling.h"

// Scheduler implementations
#include "Scheduler/AbstractScheduler.h"
#include "Scheduler/NoProfiling.h"
#include "Scheduler/Metrics/SystemLevelProfiling.h"
#include "Scheduler/Metrics/TaskLevelProfiling.h"
#include "Scheduler/Timeline/SystemLevelProfiling.h"
#include "Scheduler/Timeline/TaskLevelProfiling.h"

// TemplateScheduler provides templated selector for scheduler configurations.
#include "Scheduler/TemplateScheduler.h"

// Shared text logging utilities.
#include "Profiling/Logging.h"
#include "Profiling/TaskNameProviders.h"

// Profiling metrics log tasks.
#include "Profiling/Metrics/SystemLevelLogTask.h"
#include "Profiling/Metrics/TaskLevelLogTask.h"

// Profiling timeline output tasks.
#include "Profiling/Timeline/DirectSerialOutput.h"
#include "Profiling/Timeline/BufferedSerialOutputTask.h"
#include "Profiling/Timeline/OneShotSerialOutputTask.h"

// Profiling timeline-to-metrics aggregation tasks.
#include "Profiling/Timeline/MetricsTaskLevelProfilerTask.h"
#include "Profiling/Timeline/MetricsSystemLevelProfilerTask.h"

// Templated selector for profile logging tasks.
#include "Profiling/TemplateLogTask.h"

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