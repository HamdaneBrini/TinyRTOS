/**
 * @file port_exception.h
 * @brief Cortex-M33 exception-frame layout and task-context interface.
 */

#ifndef PORT_EXCEPTION_H
#define PORT_EXCEPTION_H

#include <stdint.h>

/** @brief Initial xPSR value with the Thumb-state bit set. */
#define INITIAL_XPSR 0x01000000UL

/** @brief Basic exception frame automatically stacked by Cortex-M33 hardware. */
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} PortExceptionFrame_t;


/**
 * @brief Construct the initial hardware and software context of a task.
 *
 * @param sp Initial top-of-stack pointer.
 * @param main_func Address restored into PC.
 * @param task_arg Argument restored into R0.
 * @param task_exit Address restored into LR.
 * @return Updated stack pointer at the saved R4-R11 context.
 */
uint32_t* port_task_exception_frame_init(uint32_t* sp, uint32_t main_func, uint32_t task_arg, uint32_t task_exit);

#endif /* PORT_EXCEPTION_H */


