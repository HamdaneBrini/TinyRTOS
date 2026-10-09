
/**
 * @file scheduler.c
 * @brief Priority scheduling, time slicing, and timer-deadline processing.
 */
#include <stdbool.h>
#include <stdint.h>

#include "cmsis_gcc.h"
#include "config.h"
#include "kernel_task.h"
#include "kernel_timing.h"
#include "port.h"
#include "port_context.h"
#include "port_syscall.h"
#include "scheduler.h"
#include "stm32h5xx.h"
#include "syscall.h"
#include "timer.h"

/* Scheduler state is placed in privileged kernel RAM by the linker script. */
__attribute__((section(".kernel_bss"))) TCB_t* current_task;
__attribute__((section(".kernel_bss"))) TimerDeadline_t timeslice_end;
__attribute__((section(".kernel_user_shared"))) volatile uint32_t cycle_counter = 0;

/**
 * @brief Initialize scheduler-owned state.
 *
 * @return TINY_OK on success, or TINY_FAIL if task-system initialization fails.
 */
TinyStatus_t scheduler_init(void) {
    NVIC_SetPriority(SVCall_IRQn, SVC_EXCEPTION_PRIORITY);
    NVIC_SetPriority(PendSV_IRQn, PENDSV_EXCEPTION_PRIORITY);
    current_task = NULL;
    timeslice_end = (TimerDeadline_t){0};
    cycle_counter = 0U;
    return task_system_init();
}

/**
 * @brief Select the first ready task and start the scheduling timer.
 *
 * @return TINY_OK on success, or TINY_FAIL if no task is ready.
 */
TinyStatus_t scheduler_start(void) {
    CriticalState_t critical_state = critical_enter();
    TCB_t* first_task = get_highest_priority_ready_task();

    if (first_task == NULL) {
        critical_exit(critical_state);
        return TINY_FAIL;
    }

    current_task = first_task;
    set_task_running(current_task);
    timeslice_end = (TimerDeadline_t){.cycle_index = 0U, .offset = TASK_TIME_SLICE};
    timer_start(timeslice_end.offset);

    critical_exit(critical_state);
    return TINY_OK;
}

/** @brief Select and configure the next task eligible to run. */
static void _schedule_next_task(void) {
    CriticalState_t critical_state = critical_enter();
    TCB_t* candidate_task = get_highest_priority_ready_task();
    if (candidate_task == NULL) {
        critical_exit(critical_state);
        return;
    }

    switch (current_task->status) {

        case RUNNING: {
            if (candidate_task->priority > current_task->priority) {
                set_task_ready(current_task);
                set_task_running(candidate_task);
                current_task = candidate_task;
                task_stack_mpu_config(current_task);

            } else if (candidate_task->priority == current_task->priority) {
                /* Rotate equal-priority tasks in round-robin order. */
                task_move_to_priority_tail(current_task);
                set_task_running(candidate_task);
                current_task = candidate_task;
                task_stack_mpu_config(current_task);
            }
            break;
        }
        case SUSPENDED: {
            set_task_running(candidate_task);
            current_task = candidate_task;
            task_stack_mpu_config(current_task);
            break;
        }
        case BLOCKED: {
            set_task_running(candidate_task);
            current_task = candidate_task;
            task_stack_mpu_config(current_task);
            break;
        }
        case TERMINATED: {

            set_task_running(candidate_task);
            current_task = candidate_task;
            task_stack_mpu_config(current_task);
            break;
        }
        default: break;
    }
    critical_exit(critical_state);
}

/**
 * @brief Save the current task stack pointer and select the next context.
 *
 * @param saved_sp Saved software context of the current task.
 * @return Stack pointer of the task selected to run.
 */
uint32_t* scheduler_context_switch(uint32_t* saved_sp) {
    current_task->sp = saved_sp;
    _schedule_next_task();
    return current_task->sp;
}

/**
 * @brief Handle a compare deadline, arm the next event, and pend PendSV.
 *
 * The handler wakes every task whose current-cycle deadline has expired, then
 * chooses the earlier of the next wakeup and time-slice deadlines.
 */
void timer_event(void) {
    CriticalState_t critical_state = critical_enter();
    TimerDeadline_t now = {.cycle_index = cycle_counter, .offset = timer_now()};

    /* Wake every expired task. */
    TCB_t* task;
    while ((task = get_earliest_wakeup_task()) != NULL && timer_deadline_reached(&task->wakeup_deadline, &now)) {
        if (set_task_ready(task) != TINY_OK) {
            break;
        }
    }
    bool timeslice_expired = timer_deadline_reached(&timeslice_end, &now);
    /*
     * Schedule another time slice only when another task is ready.
     * The currently running task is not part of the ready list.
     */

    /*switch required if:
        - current task isn't RUNNING
        - current task is RUNNING && current_task's priority <= highest priority task in the current ready list
     */
    TCB_t* highest_priority_task = get_highest_priority_ready_task();
    bool switch_required = false;
    if (highest_priority_task != NULL) {
        switch_required = (current_task->status != RUNNING)
                          || ((current_task->status == RUNNING)
                              && ((current_task->priority < highest_priority_task->priority)
                                  || (current_task->priority == highest_priority_task->priority && timeslice_expired)));
    }

    if (timeslice_expired) {
        /* Advance from the previous boundary, preserving the scheduler's
         * time-slice phase. Catch up when more than one slot has elapsed. */
        do {
            timeslice_end = timer_deadline_add(timeslice_end, TASK_TIME_SLICE);
        } while (timer_deadline_reached(&timeslice_end, &now));
    }

    TimerDeadline_t next_deadline = timeslice_end;

    /* A blocked task may need to wake before the next time slice. */
    task = get_earliest_wakeup_task();

    if (task != NULL && deadline_before(&task->wakeup_deadline, &next_deadline)) {
        next_deadline = task->wakeup_deadline;
    }

    timer_set_deadline(next_deadline.offset);

    if (switch_required) {
        port_request_context_switch();
    }

    critical_exit(critical_state);
}

/**
 * @brief Advance the software timer epoch after a TIM2 counter overflow.
 *
 * Tasks whose cycle index is now active move from the future list to the
 * current-cycle wakeup list before the next compare deadline is programmed.
 */
void timer_overflow_event(void) {
    cycle_counter++;
    TCB_t* task;
    while ((task = get_earliest_future_task()) != NULL && task->wakeup_deadline.cycle_index <= cycle_counter) {
        if (remove_task_from_future_list(task) == TINY_OK) {
            (void)insert_task_by_wakeup(task);
        }
    }
    TCB_t* earliest_active_task = get_earliest_wakeup_task();
    if (earliest_active_task != NULL) {
        const TimerDeadline_t* next_deadline = deadline_before(&earliest_active_task->wakeup_deadline, &timeslice_end)
                                                   ? &earliest_active_task->wakeup_deadline
                                                   : &timeslice_end;
        timer_set_deadline(next_deadline->offset);
    }
}

/**
 * @brief Request scheduler startup through the SVC interface.
 *
 * @return Status returned by the scheduler-start system call.
 */
TinyStatus_t tiny_scheduler_start(void) {
    TinyStatus_t status = TINY_FAIL;
    PORT_SYSCALL_RET_0(status, SVC_SCHEDULER_START);
    return status;
}
