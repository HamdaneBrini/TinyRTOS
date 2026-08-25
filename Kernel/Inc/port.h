/**
 * @file port.h
 * @brief Cortex-M33 kernel-port services shared with the generic kernel.
 */

#ifndef PORT_H
#define PORT_H

#include <stdint.h>
#include <sys/types.h>

/** @brief Saved interrupt-mask state used by nested critical sections. */
typedef uint32_t CriticalState_t;

/** @return PRIMASK value that was active before interrupts were disabled. */
CriticalState_t critical_enter(void);

/** @param state PRIMASK value returned by critical_enter(). */
void critical_exit(CriticalState_t state);



#endif /* PORT_H */
