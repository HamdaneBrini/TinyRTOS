/**
 * @file timing.h
 * @brief Public task-delay and system-time interface.
 */

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include "config.h"

/** @brief Time since scheduler startup split into human-readable units. */
typedef struct {
    uint32_t days; /**< Complete days since scheduler startup. */
    uint8_t hr;    /**< Hours remaining after complete days, in the range 0-23. */
    uint8_t min;   /**< Minutes remaining after complete hours, in the range 0-59. */
    uint8_t sec;   /**< Seconds remaining after complete minutes, in the range 0-59. */
    uint16_t ms;   /**< Milliseconds remaining after complete seconds, in the range 0-999. */
    uint16_t us;   /**< Microseconds remaining after complete milliseconds, in the range 0-999. */
} TinyUptime_t;

/** @brief Absolute timer position represented by overflow cycles and counter offset. */
typedef struct {
    uint32_t cycle_count; /**< Number of complete 32-bit TIM2 counter cycles. */
    uint32_t tick_us;     /**< TIM2 counter offset within the current cycle, in microseconds. */
} TinyTimestamp_t;

/** @brief Elapsed duration split into human-readable units. */
typedef struct {
    uint32_t days; /**< Complete days in the duration. */
    uint8_t hr;    /**< Remaining hours, in the range 0-23. */
    uint8_t min;   /**< Remaining minutes, in the range 0-59. */
    uint8_t sec;   /**< Remaining seconds, in the range 0-59. */
    uint16_t ms;   /**< Remaining milliseconds, in the range 0-999. */
} TinyDuration_t;

/**
 * @brief Delay the calling task.
 * @param duration_ms Delay duration in milliseconds.
 */
void tiny_delay_ms(uint32_t duration_ms);

/**
 * @brief Delay the calling task for a decomposed duration.
 *
 * The duration components must use their canonical ranges: hours 0-23,
 * minutes and seconds 0-59, and milliseconds 0-999.
 *
 * @param duration Duration for which the task remains blocked.
 */
void tiny_long_delay(TinyDuration_t duration);
/**
 * @brief Read an atomic snapshot of the system timer.
 * @param timestamp Destination for the overflow-cycle count and counter offset.
 * @return TINY_OK on success, or TINY_FAIL when @p timestamp is NULL.
 */
TinyStatus_t tiny_get_timestamp(TinyTimestamp_t* timestamp);

/**
 * @brief Read the current raw 32-bit timer counter.
 * @return Current counter offset in microseconds.
 */
uint32_t tiny_get_tick(void);

/**
 * @brief Read time since scheduler startup in human-readable units.
 * @param uptime Destination for the decomposed uptime.
 * @return TINY_OK on success, or TINY_FAIL when the uptime cannot be read.
 */
TinyStatus_t tiny_get_uptime(TinyUptime_t* uptime);

/**
 * @brief Add two normalized uptime values.
 * @param t1 First operand.
 * @param t2 Second operand.
 * @param result Destination for the normalized sum.
 * @return TINY_OK on success, or TINY_FAIL for invalid input, a NULL destination, or day overflow.
 */
TinyStatus_t tiny_add_uptime(TinyUptime_t t1, TinyUptime_t t2, TinyUptime_t* result);

/**
 * @brief Subtract two normalized uptime values, saturating at zero.
 * @param t1 Minuend.
 * @param t2 Subtrahend.
 * @param result Destination for the normalized difference.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or a NULL destination.
 */
TinyStatus_t tiny_subtract_uptime(TinyUptime_t t1, TinyUptime_t t2, TinyUptime_t* result);

/**
 * @brief Add two normalized duration values.
 * @param d1 First operand.
 * @param d2 Second operand.
 * @param result Destination for the normalized sum.
 * @return TINY_OK on success, or TINY_FAIL for invalid input, a NULL destination, or day overflow.
 */
TinyStatus_t tiny_add_duration(TinyDuration_t d1, TinyDuration_t d2, TinyDuration_t* result);

/**
 * @brief Subtract two normalized duration values, saturating at zero.
 * @param d1 Minuend.
 * @param d2 Subtrahend.
 * @param result Destination for the normalized difference.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or a NULL destination.
 */
TinyStatus_t tiny_subtract_duration(TinyDuration_t d1, TinyDuration_t d2, TinyDuration_t* result);

#endif /* TIMING_H */
