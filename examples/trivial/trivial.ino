#include <csch.h>

csch_t sched; // Global var to hold scheduler state
csch_proc_t sched_buf[1]; // Initialize scheduler task buffer to hold up to 1 task

// Declare task functions
void task1();

void setup() {
  Serial.begin(115200);

  // Create scheduler
  // Runs on a 10-ms tick schedule (1tk = 10ms)
  // Pass in `millis` fn to allow for proper scheduling
  // Pass in `sched_buf` array + `sched_buf` length
  sched = csch_create(10, millis, sched_buf, sizeof(sched_buf) / sizeof(*sched_buf));
  
  // Initialize the one desired task
  csch_task_fork(&sched, task1);

  while (!Serial);

  // Run scheduler
  while(1) {
    csch_tick(&sched);
  }
}

void loop() { /* Loop Function IGNORED */}

// Occasionally print out counters
void task1() {
  csch_cqueue(csch_cms_to_ticks(1000)); // Requeue this task to run in 1000 ms = 1 s => 1 Hz operating frequency

  Serial.print("Task 1: ")
  Serial.println(mlllis());
}
