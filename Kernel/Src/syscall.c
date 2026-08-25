/**
 * @file syscall.c
 * @brief TinyRTOS system-call validation and dispatch implementation.
 */

#include <stdint.h>
#include <sys/_types.h>
#include "config.h"
#include "kernel_task.h"
#include "port.h"
#include "scheduler.h"
#include "syscall.h"
#include "task.h"

/** @return Non-zero when @p instruction contains an SVC opcode. */
static inline int is_SVC_instruction(uint16_t instruction) {
    return (instruction & SVC_INST_ENCODING_MSK) == SVC_INST_ENCOGING;
}

/**
 * @brief Decode and dispatch a system call from its hardware exception frame.
 *
 * @param frame Hardware-saved caller context.
 */
void SVC_Handler_Main(PortExceptionFrame_t* frame) {

    /*
  * Stack contains:
  * r0, r1, r2, r3, r12, r14, the return address and xPSR
  * First argument (r0) is frame->r0
  */
    CriticalState_t critical_state = critical_enter();
    uint16_t instruction = *(const uint16_t*)(frame->pc - 2U);
    if (!is_SVC_instruction(instruction)) {
        frame->r0 = TINY_FAIL;
        critical_exit(critical_state);
        return;
    }

    uint32_t svc_number = instruction & SVC_NUMBER_MSK;

    switch (svc_number) {
        case SVC_SCHEDULER_START: {
            scheduler_start();
            if (current_task == NULL) {
                critical_exit(critical_state);
                return;
            }

            task_stack_mpu_config(current_task);
            port_start_first_task(current_task->sp);

            break;
        }
        case SVC_TASK_CREATE: {
            TaskCreateArgs_t* args = (TaskCreateArgs_t*)(frame->r0);
            if (args == NULL) {
                frame->r0 = TINY_FAIL;
            }
            if (args->priority <= 0) {
                frame->r0 = TINY_FAIL;
            }
            frame->r0 = task_create(args->task_handle, args->main_func, args->arg, args->priority, args->stack_size);

            break;
        }
        case SVC_TASK_EXIT: {
            set_task_terminated(current_task);
            port_request_context_switch();
            break;
        }
        default: /* unknown SVC */ break;
    }
    critical_exit(critical_state);
}
