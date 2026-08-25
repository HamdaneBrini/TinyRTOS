/**
 * @file port_context.c
 * @brief Cortex-M33 task-context creation and first-task restoration.
 */

#include <stdint.h>
#include "port_exception.h"
#include "stm32h533xx.h"

/** @brief Pend the PendSV exception used for context switching. */
void port_request_context_switch(void) {
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

/**
 * @brief Restore the first task context and enter unprivileged Thread mode.
 */
__attribute__((naked, noreturn)) void port_start_first_task(uint32_t* sp __attribute__((unused))) {
    __asm__ volatile("cpsid i                       \n"
                     /* R0 = sp, provided by the caller. */
                     "ldmia r0!, {r4-r11}           \n"
                     "msr psp, r0                   \n"

                     /*
         * SVC_Handler_Main will never return, so discard its abandoned
         * MSP call frames and restore the kernel stack top.
         */
                     "ldr r1, =_estack              \n"
                     "msr msp, r1                   \n"

                     "mrs r0, control               \n"
                     "orr r0, r0, #3                \n"
                     "msr control, r0               \n"
                     "isb                           \n"

                     "cpsie i                       \n"
                     "ldr lr, =0xFFFFFFFD           \n"
                     "bx lr                         \n");
}


