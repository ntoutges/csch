# csch

A lightweight cooperative task scheduler for embedded systems.

`csch` (**C**ooperative **SCH**eduler) provides simple multitasking for microcontrollers without requiring interrupts, context switching, dynamic allocation, or an RTOS.

Tasks execute cooperatively, voluntarily yielding execution back to the scheduler whenever they finish or enter a wait state.

Designed specifically for embedded and Arduino-style projects.

Typical applications include:

* Sensor polling
* LED animations
* Periodic communications
* Motor control
* State machines
* Cooperative protocol handling
* Replacing large `loop()` functions with independent tasks

---

# How It Works

Unlike a preemptive RTOS, `csch` never interrupts a running task.

Instead:

```
Scheduler
    │
    ▼
Run task
    │
Task returns,
queues itself,
or sleeps
    │
    ▼
Run next task
```

Each task is simply a callback function.

```c
void blinkTask() {
    ...
}
```

Whenever the scheduler decides the task should run, it simply calls the function again.

Because tasks are repeatedly invoked instead of preserving stack context, `csch` has an extremely small memory footprint.

---

# Features

* Cooperative multitasking
* Static allocation
* No interrupts required
* No context switching
* Process IDs
* Task creation and destruction
* Delayed execution
* Sleeping tasks
* Hibernating tasks
* Current-task introspection
* Configurable tick duration
* Arduino-friendly

---

# Concepts

## Tasks

A task is simply a function.

```c
void sensorTask() {
    ...
}
```

Tasks execute whenever scheduled.

---

## Queueing

Queueing delays the **next invocation** of a task.

```text
Run task
   │
   ▼
queue(100)
   │
   ▼
Task exits
   │
   ▼
100 ticks later
   │
   ▼
Task runs again
```

The current invocation immediately finishes.

---

## Sleeping

Sleeping pauses execution **inside** the task.

Internally, the scheduler continues executing other queued tasks until the sleep expires.

```text
Task
   │
   ▼
sleep()
   │
   ▼
Other tasks execute
   │
   ▼
Wake up
   │
   ▼
Continue after sleep
```

This allows blocking-style code without blocking the rest of the scheduler.

Only one task may be sleeping at any time. Any calls to `sleep` after the first will exit immediately.

---

## Hibernation

Hibernated tasks never wake automatically.

They remain registered but inactive until explicitly queued again.

Useful for event-driven tasks.

---

## Ticks

A tick is the smallest unit of time that the scheduler can deal with, and can range from 1 ms to 255 ms.

This decoupling is created to allow the scheduler to effectively deal with a large range of timescales, while minimizing memory footprint.

---

# Quick Start

## 1. Allocate Memory

```c
#include <Arduino.h>
#include <csch.h>

#define TASK_COUNT 4

csch_proc_t procBuf[TASK_COUNT];
csch_t sched;
```

---

## 2. Create the Scheduler

```c
void setup() {
    sched = csch_create(
        1,
        millis,
        procBuf,
        TASK_COUNT
    );
}
```

This creates a scheduler using 1 ms ticks.

---

## 3. Create Tasks

```c
void blinkTask();
void sensorTask();

void setup() {

    ...

    csch_task_fork(&sched, blinkTask);
    csch_task_fork(&sched, sensorTask);
}
```

Each call returns a process ID.

---

## 4. Run the Scheduler

```c
void loop() {
    csch_tick(&sched);
}
```

That's it.

Every queued task will execute whenever its scheduled wake time arrives.

> Note that, due to a quirk of the scheduling algorithm, tasks are run in the opposite order they are forked. In this example, `sensorTask` is run _before_ `blinkTask`, in the same tick

---

# Task Management

## Create a Task

```c
uint8_t pid = csch_task_fork(&sched, myTask);
```

Returns the PID or `0xFF` if out of space.

---

## Kill a Task

```c
csch_task_kill(&sched, pid);
```

Removes the task immediately.

---

## Get Current Task

```c
csch_curr_t current = csch_ctask();

Serial.println(current.pid);
```

Useful inside library code that doesn't already know its own PID.

---

# Delaying Execution

## Queue Another Execution

```c
csch_cqueue(100);
```

Run the _current_ task again after 100 ticks. Task body may continue after this function with no consequence.

This may be called at any time within the task callback function. The scheduler treats the passed in tick value as the desired delay after the initial invocation of the function (queue delay ignores function execution time)

---

## Queue Another Task

```c
csch_queue(&sched, pid, 50);
```

Schedules another task to run after some number of ticks. Overrides any previous queueing.

---

# Sleeping

Sleep without blocking other tasks.

```c
void task() {

    ...

    // T = 0

    bool didSleep = csch_csleep(csch_cms_to_ticks(250));

    // T = 250 iff didSleep, T = 0 otherwise ; Blocked this task _without_ blocking others

    ...
}
```

Internally this repeatedly services the scheduler until the sleep expires.

Only one task may sleep at once.

---

# Cooperative Blocking

Sometimes you need to wait inside a loop while still allowing the scheduler to run.

```c
while (!Serial.available()) {
    csch_ctick();
}
```

This allows every other queued task to continue executing.

While running this function, the task is considered to be sleeping, and thus inherits the same limitations of only one task able to sleep at any given time. Otherwise, this function is inert.

---

# Hibernation

Pause a task indefinitely.

```c
csch_chibernate();
```

Or:

```c
csch_hibernate(&sched, pid);
```

The task remains registered but will never run until explicitly queued.

---

# Time Conversion

Convert milliseconds into scheduler ticks.

```c
uint16_t ticks = csch_cms_to_ticks(500);
```

Or for a specific scheduler:

```c
uint16_t ticks = csch_ms_to_ticks(&sched, 500);
```

Conversions are rounded down.

---

# Memory Model

All scheduler memory is provided by the application.

```c
csch_proc_t procBuf[8];
```

No heap allocation is performed.

Each process stores:

* Task callback
* Process links
* Queue timer
* Sleep state
* Process flags

This predictable memory usage makes `csch` suitable for small microcontrollers.

---

# Calling Conventions

Many functions in csch have two forms:
- Pure: `csch_<fn>`
- **C**urrent: `csch_c<fn>`

The "current" form is used as shorthand for calling the pure function on the current scheduler with the current task.

Eg: `csch_cqueue` is generally used within task functions to avoid needing to pass in the scheduler and task PID. For scheduler `sched` running task with PID `pid`, the following are functionally equivalent:

```c
void task() {

    // Pure form
    csch_queue(&sched, pid, 100);

    // Currnet form
    csch_cqueue(100);
}
```

Exceptions to this duality include:
- `csch_task_fork` / `csch_task_kill`: A "current" mode doesn't make much sense for these functions
- `csch_csleep`: Intended to put the current task to sleep. A "pure" mode would enable improper invocations with no benefit

---

# Design Philosophy

`csch` intentionally avoids many RTOS features.

It does **not** provide:

* Threads
* Context switching
* Interrupt scheduling
* Priorities
* Mutexes
* Semaphores

Instead, it focuses on:

* Tiny code size
* Minimal RAM usage
* Deterministic execution
* Easy debugging
* Simple cooperative multitasking

---

# Typical Applications

## LED Animation

```text
blink
fade
status LED
```

---

## Sensor Polling

```text
temperature
humidity
pressure
```

---

## Communications

```text
UART parsing
I²C polling
CAN messaging
```

---

## Robotics

```text
Motor control
Odometry
Battery monitoring
Telemetry
```

---

# Notes

* Built specifically for embedded systems
* No dynamic allocation
* One sleeping task at a time
* Cooperative execution only
* Tasks should avoid long-running computations unless periodically calling `csch_ctick()`
* Tick duration is configurable from 1–255 ms

---

# Limitations

Because `csch` is a cooperative scheduler, tasks are responsible for yielding execution. A task that never returns or omits calling `csch_ctick()` during blocking operations will prevent every other task from running.

Sleeping (`csch_csleep()`) and cooperative blocking (`csch_ctick()`) rely on a single active sleeping task. Only one task may use these mechanisms at a time. Attempting otherwise will yield no operation.

Tasks do not preserve stack state between invocations. Each scheduled execution is a fresh call to the task function, so persistent state should be stored in static variables, global variables, or user-defined structures rather than local automatic variables. This design dramatically reduces RAM usage compared to traditional RTOS threads while still supporting structured cooperative multitasking.
