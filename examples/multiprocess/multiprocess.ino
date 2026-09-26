#include <Arduino.h>
#include <csch.h>

#define TASKS 2

csch_proc_t procBuf[TASKS];
csch_t sched;
uint8_t p_ledOff;

void t_ledOff() {
    // Perform task action
    digitalWrite(LED_BUILTIN, LOW);
}

void t_ledOn() {
    csch_cqueue(csch_cms_to_ticks(1000)); // Run this task every 1 s (1 Hz)

    csch_queue(&sched, p_ledOff, csch_cms_to_ticks(200)); // Run task to turn LED off in 200ms

    // Perform task action
    digitalWrite(LED_BUILTIN, HIGH);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);

    sched = csch_create(
        1,
        millis,
        procBuf,
        TASKS
    );

    csch_task_fork(&sched, t_LedOn);
    
    // Create unscheduled task
    p_ledOff = csch_task_fork(&sched, t_ledOff);
    csch_hibernate(&sched, p_ledOff);
}

void loop() {
    csch_tick(&sched);
}