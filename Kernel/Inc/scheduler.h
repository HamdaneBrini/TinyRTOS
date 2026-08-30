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

/** Absolute timer deadline at which the current scheduling time slice ends. */
extern TimerDeadline_t timeslice_end;

/** Number of completed 32-bit hardware timer cycles. */
extern volatile uint32_t cycle_counter;

/** @return TINY_OK when scheduler state and task lists are initialized. */
TinyStatus_t scheduler_init(void);

/** @return TINY_OK when the first ready task and timer are selected. */
TinyStatus_t scheduler_start(void);

#ifdef UNIT_TEST
/**
 * @brief Save the current task stack pointer and select the next context.
 * @param saved_sp Saved software-context stack pointer of the current task.
 * @return Stack pointer of the task selected to run.
 */
uint32_t* scheduler_context_switch(uint32_t* saved_sp);
#endif

/** @brief Process expired compare deadlines and request a context switch. */
void timer_event(void);

/**
 * @brief Enter the scheduler through SVC and restore the first task context.
 * @return TINY_FAIL if startup fails; successful startup does not return.
 */
TinyStatus_t tiny_scheduler_start(void);

#endif /* SCHEDULER_H */
