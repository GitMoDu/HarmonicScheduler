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
