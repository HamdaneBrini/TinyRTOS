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

/** @copydoc tiny_delay_ms */
void tiny_delay_ms(uint32_t duration_ms) {
    if (duration_ms == 0U)
        return;
    uint64_t duration_us = (uint64_t)duration_ms * 1000ULL;

    PORT_SYSCALL_VOID_1(SVC_TASK_DELAY_US, &duration_us);
}

/** @copydoc tiny_long_delay */
void tiny_long_delay(TinyDuration_t duration) {

    if (duration.days == 0 && duration.hr == 0 && duration.min == 0 && duration.sec == 0 && duration.ms == 0)
        return;
    uint64_t duration_us = (uint64_t)duration.days * 86400000000ULL + (uint64_t)duration.hr * 3600000000ULL
                           + (uint64_t)duration.min * 60000000ULL + (uint64_t)duration.sec * 1000000ULL
                           + (uint64_t)duration.ms * 1000ULL;
    PORT_SYSCALL_VOID_1(SVC_TASK_DELAY_US, &duration_us);
}

/** @copydoc tiny_get_timestamp */
TinyStatus_t tiny_get_timestamp(TinyTimestamp_t* timestamp) {
    if (timestamp == NULL)
        return TINY_FAIL;
    uint32_t status;
    PORT_SYSCALL_RET_1(status, SVC_TIME_GET_TIMESTAMP, timestamp);
    return status;
}

/** @copydoc tiny_get_tick */
uint32_t tiny_get_tick(void) {
    TinyTimestamp_t timestamp;
    __attribute__((unused)) uint32_t status;
    PORT_SYSCALL_RET_1(status, SVC_TIME_GET_TIMESTAMP, &timestamp);
    return timestamp.tick_us;
}

/** @copydoc tiny_get_uptime */
TinyStatus_t tiny_get_uptime(TinyUptime_t* uptime) {
    if (uptime == NULL)
        return TINY_FAIL;
    TinyTimestamp_t timestamp;
    uint32_t status;
    PORT_SYSCALL_RET_1(status, SVC_TIME_GET_TIMESTAMP, &timestamp);
    if (status != TINY_OK)
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

/** Return nonzero when every uptime component is in its canonical range. */
static int uptime_is_valid(TinyUptime_t time) {
    return time.us < 1000U && time.ms < 1000U && time.sec < 60U && time.min < 60U && time.hr < 24U;
}

/** Return nonzero when every duration component is in its canonical range. */
static int duration_is_valid(TinyDuration_t duration) {
    return duration.ms < 1000U && duration.sec < 60U && duration.min < 60U && duration.hr < 24U;
}

/** Return nonzero when @p t1 occurs before @p t2. */
static int uptime_is_before(TinyUptime_t t1, TinyUptime_t t2) {
    if (t1.days != t2.days)
        return t1.days < t2.days;
    if (t1.hr != t2.hr)
        return t1.hr < t2.hr;
    if (t1.min != t2.min)
        return t1.min < t2.min;
    if (t1.sec != t2.sec)
        return t1.sec < t2.sec;
    if (t1.ms != t2.ms)
        return t1.ms < t2.ms;
    return t1.us < t2.us;
}

/** Return nonzero when @p d1 is shorter than @p d2. */
static int duration_less_than(TinyDuration_t d1, TinyDuration_t d2) {
    if (d1.days != d2.days)
        return d1.days < d2.days;
    if (d1.hr != d2.hr)
        return d1.hr < d2.hr;
    if (d1.min != d2.min)
        return d1.min < d2.min;
    if (d1.sec != d2.sec)
        return d1.sec < d2.sec;
    return d1.ms < d2.ms;
}

/** @copydoc tiny_add_uptime */
TinyStatus_t tiny_add_uptime(TinyUptime_t t1, TinyUptime_t t2, TinyUptime_t* result) {
    if (result == NULL || !uptime_is_valid(t1) || !uptime_is_valid(t2))
        return TINY_FAIL;

    TinyUptime_t sum_result;
    uint32_t sum;
    sum = (uint32_t)t1.us + (uint32_t)t2.us;
    sum_result.us = (uint16_t)(sum % 1000U);
    uint32_t carry = sum / 1000U;

    sum = (uint32_t)t1.ms + (uint32_t)t2.ms + carry;
    sum_result.ms = (uint16_t)(sum % 1000U);
    carry = sum / 1000U;

    sum = (uint32_t)t1.sec + (uint32_t)t2.sec + carry;
    sum_result.sec = (uint8_t)(sum % 60U);
    carry = sum / 60U;

    sum = (uint32_t)t1.min + (uint32_t)t2.min + carry;
    sum_result.min = (uint8_t)(sum % 60U);
    carry = sum / 60U;

    sum = (uint32_t)t1.hr + (uint32_t)t2.hr + carry;
    sum_result.hr = (uint8_t)(sum % 24U);
    carry = sum / 24U;

    uint64_t days = (uint64_t)t1.days + (uint64_t)t2.days + carry;
    if (days > UINT32_MAX)
        return TINY_FAIL;

    sum_result.days = (uint32_t)days;
    *result = sum_result;
    return TINY_OK;
}

/** @copydoc tiny_subtract_uptime */
TinyStatus_t tiny_subtract_uptime(TinyUptime_t t1, TinyUptime_t t2, TinyUptime_t* result) {
    if (result == NULL || !uptime_is_valid(t1) || !uptime_is_valid(t2))
        return TINY_FAIL;

    if (uptime_is_before(t1, t2)) {
        *result = (TinyUptime_t){0};
        return TINY_OK;
    }

    TinyUptime_t difference;
    uint32_t borrow;
    uint32_t subtrahend;

    if (t1.us < t2.us) {
        difference.us = (uint16_t)((uint32_t)t1.us + 1000U - (uint32_t)t2.us);
        borrow = 1U;
    } else {
        difference.us = (uint16_t)((uint32_t)t1.us - (uint32_t)t2.us);
        borrow = 0U;
    }

    subtrahend = (uint32_t)t2.ms + borrow;
    if ((uint32_t)t1.ms < subtrahend) {
        difference.ms = (uint16_t)((uint32_t)t1.ms + 1000U - subtrahend);
        borrow = 1U;
    } else {
        difference.ms = (uint16_t)((uint32_t)t1.ms - subtrahend);
        borrow = 0U;
    }

    subtrahend = (uint32_t)t2.sec + borrow;
    if ((uint32_t)t1.sec < subtrahend) {
        difference.sec = (uint8_t)((uint32_t)t1.sec + 60U - subtrahend);
        borrow = 1U;
    } else {
        difference.sec = (uint8_t)((uint32_t)t1.sec - subtrahend);
        borrow = 0U;
    }

    subtrahend = (uint32_t)t2.min + borrow;
    if ((uint32_t)t1.min < subtrahend) {
        difference.min = (uint8_t)((uint32_t)t1.min + 60U - subtrahend);
        borrow = 1U;
    } else {
        difference.min = (uint8_t)((uint32_t)t1.min - subtrahend);
        borrow = 0U;
    }

    subtrahend = (uint32_t)t2.hr + borrow;
    if ((uint32_t)t1.hr < subtrahend) {
        difference.hr = (uint8_t)((uint32_t)t1.hr + 24U - subtrahend);
        borrow = 1U;
    } else {
        difference.hr = (uint8_t)((uint32_t)t1.hr - subtrahend);
        borrow = 0U;
    }

    difference.days = t1.days - t2.days - borrow;
    *result = difference;
    return TINY_OK;
}

/** @copydoc tiny_add_duration */
TinyStatus_t tiny_add_duration(TinyDuration_t d1, TinyDuration_t d2, TinyDuration_t* result) {
    if (result == NULL || !duration_is_valid(d1) || !duration_is_valid(d2))
        return TINY_FAIL;

    TinyDuration_t sum_result;
    uint32_t sum = (uint32_t)d1.ms + (uint32_t)d2.ms;
    sum_result.ms = (uint16_t)(sum % 1000U);
    uint32_t carry = sum / 1000U;

    sum = (uint32_t)d1.sec + (uint32_t)d2.sec + carry;
    sum_result.sec = (uint8_t)(sum % 60U);
    carry = sum / 60U;

    sum = (uint32_t)d1.min + (uint32_t)d2.min + carry;
    sum_result.min = (uint8_t)(sum % 60U);
    carry = sum / 60U;

    sum = (uint32_t)d1.hr + (uint32_t)d2.hr + carry;
    sum_result.hr = (uint8_t)(sum % 24U);
    carry = sum / 24U;

    uint64_t days = (uint64_t)d1.days + (uint64_t)d2.days + carry;
    if (days > UINT32_MAX)
        return TINY_FAIL;

    sum_result.days = (uint32_t)days;
    *result = sum_result;
    return TINY_OK;
}

/** @copydoc tiny_subtract_duration */
TinyStatus_t tiny_subtract_duration(TinyDuration_t d1, TinyDuration_t d2, TinyDuration_t* result) {
    if (result == NULL || !duration_is_valid(d1) || !duration_is_valid(d2))
        return TINY_FAIL;

    if (duration_less_than(d1, d2)) {
        *result = (TinyDuration_t){0};
        return TINY_OK;
    }

    TinyDuration_t difference;
    uint32_t borrow;
    uint32_t subtrahend;

    if (d1.ms < d2.ms) {
        difference.ms = (uint16_t)((uint32_t)d1.ms + 1000U - (uint32_t)d2.ms);
        borrow = 1U;
    } else {
        difference.ms = (uint16_t)((uint32_t)d1.ms - (uint32_t)d2.ms);
        borrow = 0U;
    }

    subtrahend = (uint32_t)d2.sec + borrow;
    if ((uint32_t)d1.sec < subtrahend) {
        difference.sec = (uint8_t)((uint32_t)d1.sec + 60U - subtrahend);
        borrow = 1U;
    } else {
        difference.sec = (uint8_t)((uint32_t)d1.sec - subtrahend);
        borrow = 0U;
    }

    subtrahend = (uint32_t)d2.min + borrow;
    if ((uint32_t)d1.min < subtrahend) {
        difference.min = (uint8_t)((uint32_t)d1.min + 60U - subtrahend);
        borrow = 1U;
    } else {
        difference.min = (uint8_t)((uint32_t)d1.min - subtrahend);
        borrow = 0U;
    }

    subtrahend = (uint32_t)d2.hr + borrow;
    if ((uint32_t)d1.hr < subtrahend) {
        difference.hr = (uint8_t)((uint32_t)d1.hr + 24U - subtrahend);
        borrow = 1U;
    } else {
        difference.hr = (uint8_t)((uint32_t)d1.hr - subtrahend);
        borrow = 0U;
    }

    difference.days = d1.days - d2.days - borrow;
    *result = difference;
    return TINY_OK;
}
