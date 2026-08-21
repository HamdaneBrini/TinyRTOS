#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#include "task.h"

/** Task currently executing, or NULL before the scheduler starts. */
extern TCB_t* current_task;

/** Absolute timer deadline of the current scheduling time slice. */
extern uint32_t timeslice_end;

TinyStatus_t scheduler_init(void);
void scheduler_start(void);
void schedule_next_task(void);
void timer_event(void);

#endif /* SCHEDULER_H */
