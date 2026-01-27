# Harmonic Scheduler

> API and behavior may change. Not recommended for critical applications yet.
> 
HarmonicScheduler is a C++11 header-only library for cooperative task scheduling on microcontrollers.


## Features
- **Easily extensible:** Create custom task classes by overriding the `Run()` method.
- **Supports lambdas and function pointers:** Schedule callbacks using `CallableTask`, no inheritance required.
- **Dynamic task management:** Add and remove tasks at any time (except from ISR context).
- **Flexible task scheduling:** Manage execution via enable/disable and delay at any moment, allowing tasks to be scheduled at dynamic intervals.
- **Low-power operation:** Optimized for low-power operation, integrates (optional) platform-specific idle/sleep functions to minimize power consumption between task runs.
- **RTOS compatible:** Supports both bare-metal and RTOS environments, works alongside real-time operating systems for integration with existing multitasking applications.
- **Header-only, pure C++11:** All classes are in the `Harmonic` namespace and available via a single include.

## Available Task Bases

HarmonicScheduler provides several base classes for implementing tasks, each with different use-cases and features:


#### DynamicTask

Core user-facing base class.
- Attach/detach at runtime to a `TaskRegistry`, set periods, enable/disable, and update scheduling as needed.
- Override `Run()` in your subclass to implement logic.

Example from `Blink.ino`:
```cpp
class BlinkTask : public Harmonic::DynamicTask {
  void Run() override {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
};
BlinkTask blink(scheduler);
blink.Attach(500, true); // 500ms period, enabled
```

---

#### CallableTask

Wraps a function pointer or lambda (with optional context pointer).
- No dynamic allocation or `std::function` required.

Example from `Blink.ino`:
```cpp
void BlinkFunction() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}
Harmonic::CallableTask BlinkTask(scheduler, BlinkFunction);
BlinkTask.Attach(500, true); // 500ms period, enabled
```
---

#### DynamicTaskWrapper

Composes a task from an external `ITaskRun` interface.
- Enables composition by letting you wrap any callable as a Harmonic task, avoiding direct inheritance.
- Allows swapping the underlying run callback at runtime.

---

#### CompatibilityTask (namespace TS)

For migration from TaskScheduler codebases.
- Mimics the core scheduling, enabling/disabling, and iteration logic of TaskScheduler's tasks.

---

### Interrupt-Driven Tasks

- `InterruptFlag::CallbackTask`: Handles flag-based interrupts. Notifies a listener when the flag is set from an ISR.
- `InterruptSignal::CallbackTask<signal_t>`: Handles counting interrupts of type `signal_t`. Notifies a listener with a signal count.
- `InterruptEventTask::CallbackTask<TimestampSource, interrupt_count_t>`: Handles timestamped event interrupts, passing both timestamp and count to the listener.

---

## Scheduling Behaviour

HarmonicScheduler uses **cooperative scheduling** with the following timing contract:

### Time Base
- The scheduler uses `millis()` as its time source, from the Arduino HAL.
- Task periods are specified in **milliseconds**.
- Profiling timestamps use `micros()` for higher resolution measurement.

### Period Resolution and Jitter
- **Timing resolution:** Tasks are evaluated once per `Loop()` call; actual callback timing is quantized to the `millis()` tick (1 ms) plus scheduler loop overhead.
- **Phase jitter:** Due to the strict late bias (`elapsed > period`), a task scheduled with `period = N` will fire between `~N ms` and `~(N+1) ms` after being enabled, depending on alignment to the `millis()` tick boundary.
  - Example: a 1 ms period task will fire approximately 1–2 ms after enable in wall-clock time.
- **Expected accuracy:** Over multiple periods, timing converges to the requested period. The late bias ensures tasks never run early, at the cost of up to +1 tick systematic delay on each firing.

### Task Execution Policy
- A task becomes **due** when `(now - LastRun) > period` (strict late bias).
  - This ensures a task will **not** run until strictly after the period has elapsed.
  - Minimum interval between runs is `period + 1 tick` in the worst case.
- After execution, `LastRun` is updated as:
  - **Phase-locked mode:** `LastRun += period` to maintain stable cadence and avoid drift.
  - **Resync on overrun:** If the scheduler detects a task has missed more than one period (e.g., due to blocking), it resyncs `LastRun = now` to prevent rapid catch-up bursts.

### ISR Wake Behavior
- `WakeFromISR()` is safe to call from interrupt context and incurs minimal overhead (does not read timestamps).
- Tasks woken from an ISR will execute on the **next scheduler loop iteration** (best-effort, typically <1 ms latency depending on loop frequency and current task load).
- For sub-millisecond ISR response requirements, consider a dedicated hardware timer ISR instead of cooperative scheduling.

### Profiling Impact
- **No profiling (`ProfileLevelEnum::None`):** Zero profiling overhead; no timestamp reads, fastest loop execution.
- **Base profiling (`ProfileLevelEnum::Base`):** Accumulates aggregate timing statistics (total busy time, idle time, scheduling overhead, iteration count) across all tasks. Adds two `micros()` calls per `Loop()` iteration.
- **Full profiling (`ProfileLevelEnum::Full`):** Accumulates per-task timing statistics (execution duration, max duration, iteration count per task) plus global metrics. Adds two `micros()` calls per loop plus two per task execution.
- Profiling data accumulates until retrieved via `GetTrace()`, which atomically snapshots and clears all counters. Typical usage: call `GetTrace()` periodically (e.g., every 1–2 seconds) from a logging task to monitor scheduler performance.


## Quick Start

```cpp
#include <Arduino.h>
#include <HarmonicScheduler.h>

class BlinkDynamicTask final : public Harmonic::DynamicTask
{
public:
    BlinkDynamicTask(Harmonic::TaskRegistry& registry)
        : Harmonic::DynamicTask(registry) {}

    bool Setup()
    {
        pinMode(LED_BUILTIN, OUTPUT);
        return AttachTask(500); // 500ms interval
    }

    void Run() final
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
};

Harmonic::TemplateScheduler<1> Runner{};
BlinkDynamicTask Blink(Runner);

void setup()
{
    Blink.Setup();
}

void loop()
{
    Runner.Loop();
}
```
