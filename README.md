# Harmonic Scheduler

> API and behavior may change. Not recommended for critical applications yet.
> 
HarmonicScheduler is a C++11 header-only library for cooperative task scheduling on microcontrollers.

It supports both bare-metal and RTOS environments, allowing tasks to be scheduled at dynamic intervals.

Optimized for low-power operation, with platform-specific support for AVR, RP2040, STM32, ESP32 and nRF52.



## Features
- **Easily extensible:** Create custom task classes by overriding the `Run()` method.
- **Setup-once model:** Tasks are all attached at startup and not added/removed at runtime.
- **Flexible task management:** Manage execution via enable/disable and delay at any moment.
- **Low-power operation:** Integrates platform-specific idle/sleep functions to minimize power consumption between task runs.
- **Header-only, pure C++11:** All classes are in the `Harmonic` namespace and available via a single include.
- **RTOS compatible:** Works alongside real-time operating systems for integration with existing multitasking applications.

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
