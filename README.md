# **C**ooperative **SCH**eduler

A simple, fast, lightweight cooperative scheduler built in C.

All memory is owned by the user and managed by the scheduler. This scheduler requires no interrupts, has minimal memory/CPU usage, and is fully extensible.

## Usage

To use the `csch` scheduler, follow this simple steps:

1. Initialize memory

Create variables that will be used by the scheduler to hold important data. This is to allow the data to be allocated in whatever way the user sees fit.

```C
#define SCHED_TASKS 4

csch_t sched;
csch_proc_t sched_buf[SCHED_TASKS];
```

_Defines a scheduler able to run up-to 4 concurrent tasks_

2. Initialize scheduler

Pass the created variables into the function to create/initialize the scheduler.

```C
#define SCHED_MS_PER_TICK 10

csch_create(&sched, SCHED_MS_PER_TICK, sched_buf, sizeof(sched_buf) / sizeof(*sched_buf));
```

_Initializes the scheduler with a tick size of 10 (the smallest unit of time the scheduler will deal with); Note that this may be any number in the range [1, 255]_

3. Schedule tasks to run

Define an entry point for each task. This function will be called whenever the task is scheduled to run (this is in contrast to more fully-featured RTOS systems like FreeRTOS which only call the entry point once). This is done to simplify the scheduler.

```C
void task1 () { /* ... */ }
void task2 () { /* ... */ }
void task3 () { /* ... */ }

csch_task_fork(&sched, task1);
csch_task_fork(&sched, task2);
csch_task_fork(&sched, task3);
```

_Uses the `fork` function schedule an initial call the associated task function_

4. Run the scheduler

Give control of the processor to the scheduler.

```C
while (1) { csch_tick(&sched, millis()); }
```

_Continuously runs the scheduler, and all required queued functions_

5. (Optional) Cyclic Tasks

The scheduler has no concept of cyclicity on its own, relying on the called tasks to reschedule themselves to be run at some later date.

```
void task1 () {
    csch_csleep(csch_cms_to_ticks(1000));

    /* ... */
}
```

_Schedule this task to run in 1000ms. As this is not a preemptive scheduler, the csleep function may be called from anywhere within the task_

