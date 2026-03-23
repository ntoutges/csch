#ifndef _CSCH_H // Cooperatie SCHeduler
#define _CSCH_H

#include <stdbool.h>
#include <stdint.h>

// Make nullptr accessible in C
#ifndef __cplusplus
#define nullptr ((void*)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Keep track of tasks
typedef struct {
  struct {
    uint8_t occupied: 1; // Whether this task slot is occupied or not
    uint8_t queue_inh: 1; // Inhibit queue updates due to tk_timer updates
    uint8_t unused: 6;
  } data;
  uint8_t next;      // Next task to run; If 0xFF: End of chain
  uint8_t prev;      // Previous task in the chain; If 0xFF: Start of chain
  uint16_t tk_sleep; // Tick to wake up; If 0xFFFF: Inactive
  void (*task)();    // Task cb function to run
} csch_proc_t;

typedef struct {

  // Process buffer data
  csch_proc_t* proc_buf;
  uint8_t proc_cap;

  uint8_t proc_start; // First task to run; If 0xFF: End of chain

  uint8_t ms_acc; // Number of MS until the next TK
  uint8_t ms_tk; // Convert from MS to TK
  uint16_t tk_timer; // Time since last tick rebase
  uint32_t ms_last; // Previous tick
} csch_t;

/**
 * @param csch  The scheduler to initialize
 * @param buf   The buffer to hold the initialized tasks
 * @param ms_tk The number of ms in a single tick (recommended: 1)
 * @param cap   The size of the buffer
 */
void csch_create(csch_t* csch, uint8_t ms_tk, csch_proc_t* buf, uint8_t cap);

/**
 * @param csch  The scheduler to modify
 * @param task  The task to run. Note that this task will be queued to run immediately upon the ntext `csch_tick` call
 * @returns     The PID of the task. Note that this may fail if the task is pool is full. If this happens, 0xFF is returned.
 */
uint8_t csch_task_fork(csch_t* csch, void (*task)());

/**
 * @param csch  The scheduler to modify
 * @param pid   The id of the task to remove
 * @returns     `true` if the task existed and was removed, `false` otherwise
 */
bool csch_task_kill(csch_t* csch, uint8_t pid);

/**
 * Run all available tasks
 * @param csch  The scheduler to run
 * @param ms    The current time
 */
void csch_tick(csch_t* csch, uint32_t ms);

/**
 * Tell a process to sleep for some amount of ticks
 . Note: Minimum sleep time is 1tk to prevent infinite blocking loops
 * @param csch  The scheduler whose tasks will be modified
 * @param pid   The process to put to sleep
 * @param ticks The length of time to sleep the task for
 * @returns `true` if the task could be found and put to sleep; False otherwise
 */
bool csch_sleep(csch_t* csch, uint16_t pid, uint16_t ticks);

/**
 * Put the current running process to sleep. Note: Minimum sleep time is 1tk to prevent infinite blocking loops
 * @param ticks The length of time to sleep the task for
 * @returns `true` if the task could be found and put to sleep; False otherwise
 */
bool csch_csleep(uint16_t ticks);

/**
 * Tell a process to sleep for an infinite time
 * @param csch  The scheduler whose tasks will be modified
 * @param pid   The process to hibernate
 * @returns `true` if the task could be found and hiberated; False otherwise
 */
bool csch_hibernate(csch_t* csch, uint16_t pid);

/**
 * Put the current process to sleep for an infinite time
 * @returns `true` if the task could be found and hiberated; False otherwise
 */
bool csch_chibernate();

/**
 * Convert some `ms` to ticks under the given `csch`'s tick setting
 * @param csch  The scheduler whose tick time will be used
 * @param ms    The ms value to convert to ticks
 * @returns     The converted time represented in ticks. Note that this value will be rounded _down_ to the nearest integer
 */
uint16_t csch_ms_to_ticks(csch_t* csch, uint16_t ms);

/**
 * Convert some `ms` to ticks under the current `csch`'s tick setting
 * @param ms    The ms value to convert to ticks
 * @returns     The converted time represented in ticks. Note that this value will be rounded _down_ to the nearest integer
 * If no scheduler is currently running, the value `0` is returned.
 */
uint16_t csch_cms_to_ticks(uint16_t ms);

#ifdef __cplusplus
}
#endif

#endif