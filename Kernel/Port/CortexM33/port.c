/**
 * @file port.c
 * @brief Cortex-M33 critical-section implementation.
 */

#include <stdint.h>
#include "cmsis_gcc.h"
#include "port.h"

/**
 * @brief Save PRIMASK and disable maskable interrupts.
 *
 * @return Interrupt-mask state to restore with critical_exit().
 */
CriticalState_t critical_enter(void) {
    CriticalState_t state = (CriticalState_t)__get_PRIMASK();
    __disable_irq();
    __DMB();
    return state;
}

/**
 * @brief Restore the interrupt-mask state saved by critical_enter().
 *
 * @param state Previously saved PRIMASK value.
 */
void critical_exit(CriticalState_t state) {
    __DMB();
    __set_PRIMASK((uint32_t)state);
}
