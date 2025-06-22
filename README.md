# Harmonic Scheduler
Experimental C++11 library for low-power cooperative task scheduling, on embedded platforms (Arduino compatible).

## Features
- **Easily extensible:** Create custom task classes by overriding the `Run()` method.
- **Flexible task management:** Supports dynamic and fixed (lightweight) task models.
- **Low-power operation:** Integrates platform-specific idle/sleep functions (AVR, RP2040, STM32, ESP32, etc.) to minimize power consumption between tasks.
- **Setup-once model:** Tasks are attached at startup (no removal at runtime); manage execution via enable/disable and delay.
- **Header-only, pure C++11:** All classes are in the `Harmonic` namespace and available via a single include.

## Quick Start

```cpp
#include <HarmonicScheduler.h>

class BlinkTask : public Harmonic::DynamicTask
{
public:
    BlinkTask(Harmonic::IScheduler& s) : DynamicTask(s) {}

    void Run() override { /* toggle LED */ }
};

static constexpr uint8_t MaxNumberOfTask = 4;
Harmonic::TemplateScheduler<MaxNumberOfTask> scheduler{};

BlinkTask blink(scheduler);

void setup() { blink.AttachTask(1000, true); }
void loop() { scheduler.Loop(); }
```
