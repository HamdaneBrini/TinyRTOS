
/**
 * @file scheduler.c
 * @brief Task creation and priority-ordered ready-list implementation.
 */
#include "scheduler.h"
#include <stdint.h>
#include <sys/types.h>
#include "cmsis_gcc.h"
#include "config.h"
#include "console.h"
#include "kernel_task.h"
#include "port.h"
#include "stm32h5xx.h"
#include "syscall.h"
#include "timer.h"

/** Task maximum execution time slice in microseconds. */
#define TASK_TIME_SLICE 1000 /* 1ms */

/* Scheduler state is placed in privileged kernel RAM by the linker script. */
__attribute__((section(".kernel_bss"))) TCB_t* current_task;
__attribute__((section(".kernel_bss"))) uint32_t timeslice_end;

static int _deadline_before(uint32_t a, uint32_t b) { return (int32_t)(a - b) < 0; }

/**
 * @brief Initialize scheduler-owned state.
 *
 * @return TINY_OK on success, or TINY_FAIL if ready-list initialization fails.
 */
TinyStatus_t scheduler_init(void) {
    NVIC_SetPriority(PendSV_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
    current_task = NULL;
    timeslice_end = 0U;
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
    timeslice_end = TASK_TIME_SLICE;
    timer_start(timeslice_end);
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
                /* Round robin if equal priorities*/
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

static int _deadline_reached(uint32_t deadline, uint32_t now) { return (int32_t)(now - deadline) >= 0; }

/** @brief Wake expired tasks, program the next deadline, and pend PendSV. */
void timer_event(void) {
    uint32_t now = timer_now();
    CriticalState_t critical_state = critical_enter();
    /* Wake every expired task. */
    TCB_t* task;
    while ((task = get_earliest_wakeup_task()) != NULL && _deadline_reached(task->wakeup_tick, now)) {
        if (set_task_ready(task) != TINY_OK) {
            break;
        }
    }

    uint32_t next_deadline = 0U;
    int deadline_available = 0;

    /*
     * Schedule another time slice only when another task is ready.
     * The currently running task is not part of the ready list.
     */
    if (task_ready_count() > 0U) {
        if (_deadline_reached(timeslice_end, now)) {
            timeslice_end = now + TASK_TIME_SLICE;
        }

        next_deadline = timeslice_end;
        deadline_available = 1;
    }

    /* A blocked task may need to wake before the next time slice. */
    task = get_earliest_wakeup_task();

    if (task != NULL && (!deadline_available || _deadline_before(task->wakeup_tick, next_deadline))) {
        next_deadline = task->wakeup_tick;
        deadline_available = 1;
    }

    if (deadline_available) {
        timer_set_deadline(next_deadline);

    } else {
        timer_cancel_deadline();
    }

    port_request_context_switch();
    critical_exit(critical_state);
}

/**
 * @brief Request scheduler startup through the SVC interface.
 *
 * @return Status returned by the scheduler-start system call.
 */
TinyStatus_t tiny_scheduler_start(void) {
    register uintptr_t r0 __asm("r0");
    __asm__ volatile("svc %1" : "=r"(r0) : "I"(SVC_SCHEDULER_START) : "memory");
    return (TinyStatus_t)r0;
}
