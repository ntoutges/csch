#include "csch.h"

bool _csch_valid(csch_t* csch, uint8_t pid); // Check if some csch+pid combination is valid
void _csch_task_qupdate(csch_t* csch, uint8_t pid); // Update task position within queue
uint8_t _csch_task_qnext(csch_t* csch); // Get id of next task to run
void _csch_rebase(csch_t* csch); // Rebase all task `tk_sleep` timers s.t. (csch->tk_timer = 0) to avoid overflows

// Store current running scheduler/process info
csch_t* _csch_active_scheduler = nullptr;
uint8_t _csch_active_pid = 0xFF;

void csch_create(csch_t* csch, uint8_t ms_tk, csch_proc_t* buf, uint8_t cap) {

  // Initialize csch
  csch->proc_buf = buf;
  csch->proc_cap = cap;
  csch->proc_start = 0xFF;
  csch->tk_timer = 0;
  csch->ms_tk = ms_tk;
  csch->ms_acc = 0; // Run initial tick immediately
  csch->ms_last = 0;

  // Initialize buf
  for (uint8_t i = 0; i < cap; i++) {
    buf[i].data.occupied = 0;
  }
}

uint8_t csch_task_fork(csch_t* csch, void (*task)()) {
  // Search for unoccupied PID
  uint8_t pid = 0;
  while (pid < csch->proc_cap && csch->proc_buf[pid].data.occupied)
    pid++;

  // Unable to find valid PID
  if (pid == csch->proc_cap) return 0xFF;

  // Initialize slot for process
  csch_proc_t* p = &(csch->proc_buf[pid]);
  p->data.occupied = 1;
  p->task = task;

  // Run immediately
  p->tk_sleep = csch->tk_timer;

  // Place process into start of process queue
  if (csch->proc_start != 0xFF)
    csch->proc_buf[csch->proc_start].prev = pid;
  p->next = csch->proc_start;
  p->prev = 0xFF;
  csch->proc_start = pid;

  return pid;
}

bool csch_task_kill(csch_t* csch, uint8_t pid) {
  if (!_csch_valid(csch, pid)) return false; // Ensure process is valid + active

  csch_proc_t* p = &(csch->proc_buf[pid]);
  csch_proc_t* buf = csch->proc_buf;

  // Update previous/next pointers
  if (p->next != 0xFF) buf[p->next].prev = p->prev;
  if (p->prev != 0xFF) buf[p->prev].next = p->next;

  // Update initial pointer
  if (p->prev == 0xFF) csch->proc_start = p->next;

  // Mark process slot as open
  p->data.occupied = 0;

  return true;
}

void csch_tick(csch_t* csch, uint32_t ms) {
  uint16_t delta = ms - csch->ms_last;
  uint16_t d_tk = delta / csch->ms_tk;
  uint8_t d_ms = delta % csch->ms_tk;

  // Update accumulator + tick count
  csch->tk_timer += d_tk;
  if (d_ms >= csch->ms_acc) {
    csch->ms_acc += csch->ms_tk - d_ms;
    csch->tk_timer++;
  }
  else csch->ms_acc -= d_ms;
  csch->ms_last = ms;

  // Rebase if over half the timer capacity taken up
  if (csch->tk_timer > 0x7FFF) {
    _csch_rebase(csch);
  }

  // Run tick
  _csch_active_scheduler = csch;
  _csch_active_pid = _csch_task_qnext(csch);

  while (_csch_active_pid != 0xFF) {
    csch_proc_t* p = &(csch->proc_buf[_csch_active_pid]);

    // Reached end of tasks to run; Ignore!
    if (p->tk_sleep > csch->tk_timer) break;
    
    // Hibernate task by default
    p->tk_sleep = 0xFFFF;

    // Run task
    p->data.queue_inh = 1;
    p->task();
    p->data.queue_inh = 0;

    // Update task queue position
    _csch_task_qupdate(csch, _csch_active_pid);

    _csch_active_pid = _csch_task_qnext(csch);
  }
}

bool csch_sleep(csch_t* csch, uint8_t pid, uint16_t ticks) {
  if (!_csch_valid(csch, pid)) return false; // Ensure process is valid + active

  // Clamp to a minimum of 1tk
  if (ticks == 0) ticks = 1;

  csch_proc_t* p = &(csch->proc_buf[pid]);

  // Update sleep timer
  p->tk_sleep = csch->tk_timer + ticks;

  // Update location in task queue, if required
  if (!p->data.queue_inh)
    _csch_task_qupdate(csch, pid);

  return true;
}

bool csch_csleep(uint16_t ticks) {
  return csch_sleep(_csch_active_scheduler, _csch_active_pid, ticks);
}

bool csch_hibernate(csch_t* csch, uint8_t pid) {
  if (!_csch_valid(csch, pid)) return false; // Ensure process is valid + active

  // Mark as hibernated
  csch->proc_buf[pid].tk_sleep = 0xFFFF;
}

bool csch_chibernate() {
  return csch_hibernate(_csch_active_scheduler, _csch_active_pid);
}

uint16_t csch_ms_to_ticks(csch_t* csch, uint16_t ms) {
  return ms / csch->ms_tk;
}

uint16_t csch_cms_to_ticks(uint16_t ms) {
  if (_csch_active_scheduler == nullptr) return 0; // No scheduler currently running; Return default value

  return csch_ms_to_ticks(_csch_active_scheduler, ms);
}

bool _csch_valid(csch_t* csch, uint8_t pid) {
  return csch != nullptr && pid < csch->proc_cap && csch->proc_buf[pid].data.occupied;
}

void _csch_task_qupdate(csch_t* csch, uint8_t pid) {
  csch_proc_t* buf = csch->proc_buf;

  csch_proc_t* p = &(buf[pid]);
  uint16_t timer = p->tk_sleep;

  // Remove `pid` from chain
  // Update previous/next pointers
  if (p->next != 0xFF) buf[p->next].prev = p->prev;
  if (p->prev != 0xFF) buf[p->prev].next = p->next;

  // Update initial pointer
  if (p->prev == 0xFF) csch->proc_start = p->next;

  // Search for insertion index
  uint8_t prev = 0xFF;
  uint8_t curr = csch->proc_start;

  // Climb ladder to the proper slot to insert the process
  // Round robin consideration: Newer tasks are placed further back
  while (
    curr != 0xFF &&
    timer >= buf[curr].tk_sleep
  ) {
    prev = curr;
    curr = buf[curr].next;
  }

  // Insert `pid` at start of chain
  if (prev == 0xFF) {
    if (csch->proc_start != 0xFF)
      buf[csch->proc_start].prev = pid;
    p->prev = 0xFF;
    p->next = csch->proc_start;
    csch->proc_start = pid;
  }
  else {
    // Insert `pid` in middle/end of chain
    if (curr != 0xFF)
      buf[curr].prev = pid;
    p->prev = prev;
    p->next = buf[prev].next;
    buf[prev].next = pid;
  }
}

uint8_t _csch_task_qnext(csch_t* csch) {
  if (csch->proc_start == 0xFF) return 0xFF; // No new processes to run
  return csch->proc_start;
}

void _csch_rebase(csch_t* csch) {
  // Subtract `csch->tk_timer` from all sleep timers
  // Assumes all tk_sleep >= tk_timer;

  for (uint8_t i = 0; i < csch->proc_cap; i++) {
    csch_proc_t* proc = &(csch->proc_buf[i]);
    if (
      !proc->data.occupied ||  // No task in this slot; Ignore
      proc->tk_sleep == 0xFFFF // Task is inactive; Ignore
    ) {
      continue; 
    }

    proc->tk_sleep -= csch->tk_timer;
  }

  csch->tk_timer = 0;
}
