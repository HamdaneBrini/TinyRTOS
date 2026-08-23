#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#include "config.h"
#include "kernel_task.h"

/** Task currently executing, or NULL before the scheduler starts. */
extern TCB_t* current_task;

/** Absolute timer deadline of the current scheduling time slice. */
extern uint32_t timeslice_end;

TinyStatus_t scheduler_init(void);
TinyStatus_t scheduler_start(void);
void schedule_next_task(void);
void timer_event(void);
void start_first_task(void);
TinyStatus_t tiny_scheduler_start(void);
#endif /* SCHEDULER_H */
