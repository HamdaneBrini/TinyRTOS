/**
 * @file port_context.h
 * @brief Cortex-M33 task-context switching interface.
 */

#ifndef PORT_CONTEXT_H
#define PORT_CONTEXT_H

#include <stdint.h>

/**
 * @brief Restore the first task context and enter unprivileged Thread mode.
 *
 * @param sp Pointer to the first word of the saved software context.
 */
void port_start_first_task(uint32_t* sp __attribute__((unused)));

/** @brief Pend a context switch through the Cortex-M33 exception controller. */
void port_request_context_switch(void);

#endif /* PORT_CONTEXT_H */
