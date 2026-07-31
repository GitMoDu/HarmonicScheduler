# Harmonic Scheduler

> API and behavior may change. Not recommended for critical applications yet.

 A C++11 header-only library for cooperative task scheduling on microcontrollers.

# ![Harmonic Timeline Output](media/TimelineCapture.png)

```text
ID          CPU(%)  CALLS  TIME(us) MAX(us)
BUSY        23             231588   975120
IDLE        23             229184
SLEEP       52             514348
-------------------------------------------
LOG         1       3      18688    18624
BlinkTask   0       2      72       40
BusyTask    12      239    126392   584
LightTask   5       172    56284    368
LongTask    3       3      30152    10060
```


## Library
- **Header-only, Pure C++11:** All classes are in the `Harmonic` namespace and available via a single include; zero `std::` dependencies.
- **Templated Scheduler**: Schedulers can be configured with fixed number of tasks, profiler options, and idle sleep enabled/disabled at compile time.
- **Integrated Profiling System:** Configurable **`ProfilerMode`** (*Metrics* polling vs. *Timeline* streaming) and **`ProfilerLevel`** (*System* vs. *Task* granularities).
- **Template Profiling Log**: Built-in log tasks formatted for real-time console/serial metric output.
- **RTOS compatible:** Supports bare-metal, RTOS, and desktop operating-system environments.
- **Backwards compatible:** Drop-in compatibility wrappers for TaskScheduler codebases (TS::CompatibilityTask).


## Scheduler
- **Fast Cooperative Dispatch:** Optimized execution path with very low overhead when profiling is disabled.
- **Flexible task scheduling:** Dynamic enable/disable, re-arm, and manage execution delay at any time.
- **Dynamic task management:** Safely add and remove tasks at runtime (outside ISR context).
- **Low-power operation:** Platform-agnostic idle/sleep integration to minimize MCU power consumption between task passes.
- **Metrics Logging:** Tracks system-level timing metrics (busy time, scheduling, idle sleep) and task-level metrics (call counts, total duration, max duration).
- **Timeline Streaming:** Streams timestamped event markers directly to custom output handlers (e.g., Serial, ring buffer, network).


## Tasks
- **Simple Task Extension:** Inherit via standard subclassing (`DynamicTask`).
- **Flexible Task Bases:** composition, lambdas and function pointers: (`CallableTask`).
- **Extensible Task Model:** Create custom task classes by overriding the `Run()` method.
- **Interrupt-Driven Task Support:** Built-in ISR-safe notification mechanisms:
    - `InterruptFlag::CallbackTask`: Handles flag-based interrupts. Notifies a listener when the flag is set from an ISR.
    - `InterruptSignal::CallbackTask<signal_t>`: Handles counting interrupts of type `signal_t`. Notifies a listener with a signal count.
    - `InterruptEventTask::CallbackTask<TimestampSource, interrupt_count_t>`: Handles timestamped event interrupts, passing both timestamp and count to the listener.

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


## Scheduling Behavior

HarmonicScheduler uses **cooperative scheduling** with the following timing contract:

### Time Base
- The scheduler uses **milliseconds** as its timestamp unit, derived from the Platform::GetTimestamp().
- Task periods are specified in **milliseconds**.
- Profiling timestamps use microseconds for measurement, derived from Platform::GetProfilerTimestamp().

### Period Resolution and Jitter
- **Timing resolution:** Tasks are evaluated once per `Loop()` call; actual callback timing is quantized to the timestamp tick (1 ms) plus scheduler loop overhead.
- **Phase jitter:** A task scheduled with `period = N` will fire at approximately `N ms` or later, depending on alignment to the timestamp tick boundary and scheduler loop overhead.
  - Example: a 1 ms period task will fire approximately 1-2 ms after enable in wall-clock time.
- **Expected accuracy:** Over multiple periods, timing converges to the requested period. The elapsed-time check ensures tasks never run before their configured period.

### Task Execution Policy
- A task becomes **due** when `period == 0` or `(now - LastRun) >= period`.
  - The scheduler will never execute a task prior to its period expiring.
- After execution, `LastRun` is updated based on mode:
  - **Phase-locked mode:** `LastRun += period` to maintain stable cadence and eliminate long-term drift.
  - **Resync on overrun:** If execution delays cause a task to miss more than one full period, LastRun automatically resyncs (LastRun = now) to prevent burst catch-up loops.

### ISR Wake Behavior
- `WakeFromISR()` is safe to call from interrupt context and incurs minimal overhead.
- Tasks woken from an ISR will execute on the **next scheduler loop iteration**.

### Profiling Impact
- **No profiling (`ProfilerModeEnum::None`):** Zero profiler timestamp reads and zero trace buffer operations. Only standard scheduling timestamps are read.
- **Metrics profiling (`ProfilerModeEnum::Metrics`):** Reads high-resolution profiler timestamps to measure loop execution, scheduling overhead, task runtimes, and idle sleep. System-level metrics measure global busy/idle split; task-level metrics track call count, total duration, and peak duration per task.
- **Timeline profiling (`ProfilerModeEnum::Timeline`):** Writes raw event samples into internal trace buffers around scheduler and task boundaries. Overhead scales strictly with event count and buffer flush frequency.
- **Profiler levels:** `ProfilerLevelEnum::System` records scheduler-wide data; `ProfilerLevelEnum::Task` includes per-task granularity.
- **Retrieval:** Metrics are retrieved asynchronously with `RequestMetrics(listener)`. Timeline sample blocks are passed to a registered listener when buffers reach capacity. Data stores can be cleared using `ResetMetrics()` and `ResetTimeline()` respectively.

### Timeline Output

Timeline profiling streams contiguous sample blocks out of a fixed-capacity trace buffer rather than dispatching individual callbacks per event.

Because listener callbacks run synchronously inside the scheduler loop, listeners should adhere to the following contract:

- Copy Immediately: Copy samples into application-managed storage. The pointer passed to OnTimelineResult points to internal scheduler memory and is invalidated after the callback returns.
- Non-Blocking Execution: Keep callback execution brief. Avoid inline I/O operations (such as blocking Serial, SPI, network, or file access) inside the callback.
- Decoupled Transport: Queue copied sample blocks to a secondary output/transport task to process formatting and transmission asynchronously.
