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
    uint16_t us;   /**< Remaining microseconds, in the range 0-999. */
} TinyDuration_t;

/**
 * @brief Delay the calling task.
 * @param duration_ms Delay duration in milliseconds.
 */
void tiny_delay(uint32_t duration_ms);

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

#endif /* TIMING_H */
