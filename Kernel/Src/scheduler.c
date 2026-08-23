
/**
 * @file scheduler.c
 * @brief Task creation and priority-ordered ready-list implementation.
 */
#include "scheduler.h"
#include <stdint.h>
#include <sys/types.h>
#include "cmsis_gcc.h"
#include "config.h"
#include "stm32h5xx.h"
#include "kernel_task.h"
#include "syscall.h"
#include "timer.h"
/** Task max execution time slice in microseconds*/
#define TASK_TIME_SLICE 1000 /* 1ms */

/* Scheduler state is placed in privileged kernel RAM by the linker script. */
__attribute__((section(".kernel_bss"))) TCB_t* current_task;
__attribute__((section(".kernel_bss"))) uint32_t timeslice_end;

static int deadline_before(uint32_t a, uint32_t b) { return (int32_t)(a - b) < 0; }

__attribute__((naked, noreturn)) void start_first_task(void) {
    __asm__ volatile("ldr r0, =current_task \n"
                     "ldr r0, [r0] \n" /* r0 = current_task */
                     "ldr r0, [r0] \n" /* r0 = current_task->sp */
                     "ldmia r0!, {r4-r11} \n"
                     "msr psp, r0 \n"

                     /* Switch to thread mode and use PSP for execution*/
                     "mrs r0, CONTROL \n"
                     "orrs r0, r0, #3 \n"
                     "msr CONTROL, r0 \n"
                     "isb \n"
                     "ldr lr, =0xFFFFFFFD \n"
                     "bx lr \n"

    );
}

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

TinyStatus_t scheduler_start(void) {

    TCB_t* first_task = get_highest_priority_ready_task();
   
    if (first_task == NULL)
        return TINY_FAIL;
    current_task = first_task;
    set_task_running(current_task);
    timeslice_end = TASK_TIME_SLICE;
    timer_start(timeslice_end);
    return TINY_OK;
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
    TCB_t* task = get_earliest_wakeup_task();
    while (task != NULL) {
        if (deadline_before(task->wakeup_tick, now) || task->wakeup_tick == now) {
            if (set_task_ready(task) != TINY_OK) {
                break;
            }
            task = get_next_wakeup_task(task);
        } else
            break;
    }

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

TinyStatus_t tiny_scheduler_start(void){
    register uintptr_t r0 __asm("r0");
    __asm__ volatile (
        "svc %1"
        :"=r"(r0)
        : "I"(SVC_SCHEDULER_START)
        : "memory"
    );
    return (TinyStatus_t)r0;
}
