#include "csch.h"

csch_t sched; // Global var to hold scheduler state
csch_proc_t sched_buf[4]; // Initialize scheduler task buffer to hold up-to 4 tasks

// Declare task functions
void task1();
void task2();
void task3();

void setup() {
  Serial.begin(115200);

  csch_create(&sched, 10, sched_buf, sizeof(sched_buf) / sizeof(*sched_buf));
  
  // Initialize all tasks
  csch_task_fork(&sched, task1);
  csch_task_fork(&sched, task2);
  csch_task_fork(&sched, task3);

  // Run scheduler
  while(1) {
    csch_tick(&sched, millis());
  }
}

void loop() { /* Loop Function IGNORED */}

// Counters to show concurrent behaviour
uint8_t task1_ct = 0;
uint8_t task2_ct = 0;

void task1() {
  csch_csleep(csch_cms_to_ticks(100));
  task1_ct++;
}

void task2() {
  csch_csleep(csch_cms_to_ticks(50 + task2_ct)); // Slow down counting as counter increases for more dynamic (non-cyclic) behaviour
  task2_ct++;
}

// Occasionally print out counters
void task3() {
  csch_csleep(csch_cms_to_ticks(1000));

  Serial.print("Task1: ");
  Serial.print(task1_ct);
  Serial.print(" | Task2: ");  
  Serial.println(task2_ct);
}
