/**
 * @file timing_sys.c
 * @brief Timing system-call wrappers and time conversion helpers.
 */

#include <stdint.h>

#include "config.h"
#include "port_syscall.h"
#include "syscall.h"
#include "timer.h"
#include "timing.h"

/** @copydoc tiny_delay */
void tiny_delay(uint32_t duration_ms) { PORT_SYSCALL_VOID_1(SVC_TASK_DELAY, duration_ms); }

/** @copydoc tiny_tick */
uint32_t tiny_tick(void) {
    uint32_t result;
    PORT_SYSCALL_RET_0(result, SVC_TIMER_NOW);
    return result;
}

/** @copydoc tiny_get_now */
TinyStatus_t tiny_get_now(TinyTime_t* time) {
    if (time == NULL)
        return TINY_FAIL;

    /* The hardware timer exposes elapsed time in microseconds. */
    uint32_t now = tiny_tick();
    time->s = now / 1000000U;
    time->ms = (now % 1000000U) / 1000U;
    time->us = now % 1000U;
    return TINY_OK;
}
