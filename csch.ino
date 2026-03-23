#include "csch.h"

csch_t sched; // Global var to hold scheduler state
csch_proc_t sched_buf[4]; // Initialize scheduler task buffer to hold up-to 4 tasks

// Declare task functions
void task1();
void task2();
void task3();

void setup() {
  csch_create(&sched, 10, sched_buf, sizeof(sched_buf) / sizeof(*sched_buf));
  
  // Initialize all tasks
  csch_task_fork(&csch, task1);
  csch_task_fork(&csch, task2);
  csch_task_fork(&csch, task3);

  // Run scheduler
  while(1) {
    csch_tick(&csch, millis());
  }
}

void loop() {}

void task1() {
  csch_csleep(100);
}

void task2() {

}

void task3() {

}
