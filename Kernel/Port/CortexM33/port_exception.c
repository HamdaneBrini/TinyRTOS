/**
 * @file port_exception.c
 * @brief Cortex-M33 SVC and PendSV exception entry points.
 */

#include <stdint.h>
#include "port.h"
#include "stm32h533xx.h"

/**
  * @brief This function handles System service call via SWI instruction.
  */
__attribute__((naked)) void SVC_Handler(void) {
    __asm__ volatile("tst   lr, #4      \n"
                     "ite   eq          \n"
                     "mrseq r0, msp     \n"
                     "mrsne r0, psp     \n"
                     "b     SVC_Handler_Main \n");
}

/**
  * @brief This function handles Pendable request for system service.
  */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm__ volatile("cpsid i                     \n"
                     "mrs r0, psp                 \n"
                     "stmdb r0!, {r4-r11}         \n"
                     "push {r3,lr}                \n"
                     "bl scheduler_context_switch \n"
                     "pop {r3,lr}                 \n"
                     "ldmia r0!, {r4-r11}         \n"
                     "msr psp, r0                 \n"
                     "cpsie i                     \n"
                     "bx lr                       \n");
}
/**
 * @brief Build the initial exception frame and software-saved task context.
 *
 * @param sp Initial top-of-stack address.
 * @param main_func Address of the task entry function.
 * @param task_arg Argument passed to the task entry function through R0.
 * @param task_exit Address executed when the task entry function returns.
 * @return Updated stack pointer at the beginning of the saved context.
 */
uint32_t* port_task_exception_frame_init(uint32_t* sp, uint32_t main_func, uint32_t task_arg, uint32_t task_exit) {
    *(--sp) = INITIAL_XPSR; /* xPSR */
    *(--sp) = main_func;    /* PC */
    *(--sp) = task_exit;    /* LR */
    *(--sp) = 0U;           /* R12 */
    *(--sp) = 0U;           /* R3 */
    *(--sp) = 0U;           /* R2 */
    *(--sp) = 0U;           /* R1 */
    *(--sp) = task_arg;     /* R0 */

    /* Software-saved context: R4-R11. */
    for (int i = 0; i < 8; i++) {
        *(--sp) = 0U;
    }

    return sp;
}