/**
 * @file kernel_timing.h
 * @brief Internal timing and deadline helpers.
 */

#ifndef KERNEL_TIMING_H
#define KERNEL_TIMING_H

#include <stdint.h>

/** @brief Absolute timer deadline split into an overflow-cycle index and counter offset. */
typedef struct {
    uint64_t cycle_index; /**< Number of complete 32-bit counter cycles. */
    uint32_t offset;      /**< Counter value within the selected cycle. */
} TimerDeadline_t;

/**
 * @brief Block the current task for a duration.
 * @param duration_us Delay duration in microseconds.
 */
void task_delay_us(uint64_t duration_us);

/**
 * @brief Add a duration to an absolute timer deadline.
 * @param deadline Starting absolute deadline.
 * @param duration_us Duration to add, in microseconds.
 * @return Normalized deadline with 32-bit counter wrap propagated to the cycle index.
 */
TimerDeadline_t timer_deadline_add(TimerDeadline_t deadline, uint64_t duration_us);

/**
 * @brief Test whether an absolute timer deadline has been reached.
 * @param deadline Absolute deadline to test.
 * @param now Current absolute timer position.
 * @return Nonzero when @p now is at or past @p deadline; otherwise zero.
 */
int timer_deadline_reached(const TimerDeadline_t* deadline, const TimerDeadline_t* now);

/**
 * @brief Compare two absolute deadlines by cycle index and counter offset.
 * @param a First absolute deadline.
 * @param b Second absolute deadline.
 * @return Nonzero when @p a occurs before @p b; otherwise zero.
 */
int deadline_before(const TimerDeadline_t* a, const TimerDeadline_t* b);

#endif /* KERNEL_TIMING_H */
