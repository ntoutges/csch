#include <Arduino.h>
#include <csch.h>

#define TASKS 2

csch_proc_t procBuf[TASKS];
csch_t sched;

void blinkTask() {
    csch_cqueue(csch_cms_to_ticks(500)); // Run task every 500 ms (2 Hz)

    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void printTask() {
    csch_cqueue(csch_cms_to_ticks(1000)); // Run task every 1 s (1 Hz)

    Serial.println("Hello!");
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

    csch_task_fork(&sched, blinkTask);
    csch_task_fork(&sched, printTask);
}

void loop() {
    csch_tick(&sched);
}