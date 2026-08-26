/**
 * @file kernel_timing.h
 * @brief Internal timing and deadline helpers.
 */

#ifndef KERNEL_TIMING_H
#define KERNEL_TIMING_H

#include <stdint.h>

/**
 * @brief Block the current task for a duration.
 * @param duration_ms Delay duration in milliseconds.
 */
void task_delay(uint32_t duration_ms);

/**
 * @brief Test whether a deadline has been reached using wrap-safe arithmetic.
 * @param deadline Absolute deadline tick.
 * @param now Current tick value.
 * @return Nonzero when @p now is at or past @p deadline; otherwise zero.
 */
int deadline_reached(uint32_t deadline, uint32_t now);

/**
 * @brief Compare two deadlines using wrap-safe arithmetic.
 * @param a First absolute deadline tick.
 * @param b Second absolute deadline tick.
 * @return Nonzero when @p a occurs before @p b; otherwise zero.
 */
int deadline_before(uint32_t a, uint32_t b);

#endif /* KERNEL_TIMING_H */
