#include <csch.h>

csch_t sched; // Global var to hold scheduler state
csch_proc_t sched_buf[4]; // Initialize scheduler task buffer to hold up to 4 tasks

// Declare task functions
void task1();
void task2();
void task3();

void setup() {
  Serial.begin(115200);

  sched = csch_create(10, millis, sched_buf, sizeof(sched_buf) / sizeof(*sched_buf));
  
  // Initialize all tasks
  csch_task_fork(&sched, task1);
  csch_task_fork(&sched, task2);
  csch_task_fork(&sched, task3);

  while (!Serial);

  // Run scheduler
  while(1) {
    csch_tick(&sched);
  }
}

void loop() { /* Loop Function IGNORED */}

// Counters to show concurrent behaviour
uint8_t task1_ct = 0;
uint8_t task2_ct = 0;

void task1() {
  csch_cqueue(csch_cms_to_ticks(100));
  task1_ct++;
}

void task2() {
  csch_cqueue(csch_cms_to_ticks(50 + task2_ct)); // Slow down counting as counter increases for more dynamic (non-cyclic) behaviour
  task2_ct++;
}

extern uint8_t _csch_active_pid;

// Occasionally print out counters
void task3() {
  // csch_cqueue(csch_cms_to_ticks(1000)); // Technique 1: Nonblocking queue (Flexible but harder to reason with)

  while (1) {
    Serial.print("Task1: ");
    Serial.print(task1_ct);
    Serial.print(" | Task2: ");
    Serial.println(task2_ct);
    csch_csleep(csch_cms_to_ticks(1000)); // Technique 2: Blocking loop with csleep (Rigid **single use**, but easier to reason around)
  }
}
