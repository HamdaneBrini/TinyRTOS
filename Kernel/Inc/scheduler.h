/**
 * @file scheduler.h
 * @brief Public interface for task scheduling and context switching.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#include "config.h"
#include "kernel_task.h"

/** Task currently executing, or NULL before the scheduler starts. */
extern TCB_t* current_task;

/** Absolute timer deadline of the current scheduling time slice. */
extern uint32_t timeslice_end;

/** @return TINY_OK when scheduler state and task lists are initialized. */
TinyStatus_t scheduler_init(void);

/** @return TINY_OK when the first ready task and timer are selected. */
TinyStatus_t scheduler_start(void);

/**@brief Save the current task stack pointer and select the next context. */
uint32_t* scheduler_switch_context(uint32_t* saved_sp);

/** @brief Process timer deadlines and request a context switch. */
void timer_event(void);

/** @brief Restore the initial task context and enter Thread mode. */
void start_first_task(void);

/** @return Status returned by the scheduler-start system call. */
TinyStatus_t tiny_scheduler_start(void);

#endif /* SCHEDULER_H */
