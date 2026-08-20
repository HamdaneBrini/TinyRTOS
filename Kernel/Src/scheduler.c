
/**
 * @file scheduler.c
 * @brief Task creation and priority-ordered ready-list implementation.
 */
#include "scheduler.h"
#include <stdint.h>
#include "stm32h5xx.h"
#include "timer.h"
/** Task max execution time slice in microseconds*/
#define TASK_TIME_SLICE 1000 /* 1ms */


/* Scheduler state is placed in privileged kernel RAM by the linker script. */
__attribute__((section(".kernel_bss"))) TCB_t* current_task;
__attribute__((section(".kernel_bss"))) uint32_t timeslice_end;

static int deadline_before(uint32_t a, uint32_t b) { return (int32_t)(a - b) < 0; }


/**
 * @brief Initialize scheduler-owned state.
 *
 * @return TINY_OK on success, or TINY_FAIL if ready-list initialization fails.
 */
TinyStatus_t scheduler_init(void) {
    current_task = NULL;
    timeslice_end = 0U;
    return task_system_init();
}



void scheduler_start(void) {
    timer_start(TASK_TIME_SLICE + timer_now());
    TCB_t* first_task = get_highest_priority_ready_task();
    if (first_task != NULL) {
        current_task = first_task;
        timeslice_end = TASK_TIME_SLICE;
        set_task_running(current_task);
    }
}

void schedule_next_task(void) {
    TCB_t* candidate_task = get_highest_priority_ready_task();
    if (candidate_task == NULL)
        return;
    switch (current_task->status) {

        case RUNNING: {
            if (candidate_task->priority > current_task->priority) {
                set_task_ready(current_task);
                set_task_running(candidate_task);
                current_task = candidate_task;

            } else if (candidate_task->priority == current_task->priority) {
                current_task->status = READY;
                /**
            TODO:
            insert task at the end of the priority part (round robin)
            
            */
                set_task_running(candidate_task);
                current_task = candidate_task;
            }
            break;
        }
        case SUSPENDED: {
            set_task_running(candidate_task);
            current_task = candidate_task;
            break;
        }
        case BLOCKED: {
            set_task_running(candidate_task);
            current_task = candidate_task;
            break;
        }
        case TERMINATED: {
            set_task_running(candidate_task);
            current_task = candidate_task;
            break;
        }
        default: break;
    }
    if (task_ready_count() > 0U) {
        timeslice_end += TASK_TIME_SLICE;
        timer_set_deadline(timeslice_end);
    }
}

void timer_event(void) {
    uint32_t now = timer_now();
    TCB_t* task;
    while ((task = get_earliest_wakeup_task()) != NULL
           && (deadline_before(task->wakeup_tick, now) || task->wakeup_tick == now)) {
        if (set_task_ready(task) != TINY_OK) {
            break;
        }
    }

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

