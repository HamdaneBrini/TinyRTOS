/**
 * @file timing_sys.c
 * @brief Timing system-call wrappers and time conversion helpers.
 */

#include <stdint.h>

#include "config.h"
#include "port_syscall.h"
#include "scheduler.h"
#include "syscall.h"
#include "timer.h"
#include "timing.h"

/** @copydoc tiny_delay */
void tiny_delay(uint32_t duration_ms) { PORT_SYSCALL_VOID_1(SVC_TASK_DELAY_MS, duration_ms); }

/** @copydoc tiny_get_timestamp */
TinyStatus_t tiny_get_timestamp(TinyTimestamp_t* timestamp) {
    if (timestamp == NULL)
        return TINY_FAIL;
    uint32_t result;
    PORT_SYSCALL_RET_1(result, SVC_TIME_GET_TIMESTAMP, timestamp);
    return result;
}

/** @copydoc tiny_get_tick */
uint32_t tiny_get_tick(void) {
    TinyTimestamp_t timestamp;
    tiny_get_timestamp(&timestamp);
    return timestamp.tick_us;
}

/** @copydoc tiny_get_uptime */
TinyStatus_t tiny_get_uptime(TinyUptime_t* uptime) {
    if (uptime == NULL)
        return TINY_FAIL;
    TinyTimestamp_t timestamp;
    if (tiny_get_timestamp(&timestamp) != TINY_OK)
        return TINY_FAIL;
    uint64_t global_time_us = ((uint64_t)timestamp.cycle_count << 32) + (uint64_t)timestamp.tick_us;

    uptime->days = (uint32_t)(global_time_us / 86400000000ULL);

    uint64_t remaining_us = global_time_us % 86400000000ULL;

    uptime->hr = (uint8_t)(remaining_us / 3600000000ULL);
    remaining_us %= 3600000000ULL;

    uptime->min = (uint8_t)(remaining_us / 60000000ULL);
    remaining_us %= 60000000ULL;

    uptime->sec = (uint8_t)(remaining_us / 1000000ULL);
    remaining_us %= 1000000ULL;

    uptime->ms = (uint16_t)(remaining_us / 1000ULL);
    uptime->us = (uint16_t)(remaining_us % 1000ULL);

    return TINY_OK;
}
