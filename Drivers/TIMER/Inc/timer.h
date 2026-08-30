/**
 * @file timer.h
 * @brief Public interface for the TIM2 hardware timer driver.
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "config.h"

/** Maximum count of the 32-bit TIM2 free-running counter. */
#define COUNTER_PERIOD UINT32_MAX

/**
 * @brief Initialize TIM2 as a 1 MHz output-compare timer.
 * @return TINY_OK on success, or TINY_FAIL if HAL configuration fails.
 */
TinyStatus_t timer_init(void);

/**
 * @brief Stop TIM2 and return its interrupt, peripheral, and clock state to reset.
 * @return TINY_OK on success, or TINY_FAIL if HAL deinitialization fails.
 */
TinyStatus_t timer_deinit(void);

/** @return Current TIM2 counter value in microseconds. */
uint32_t timer_now(void);

/**
 * @brief Start the scheduler timer from counter value zero.
 * @param first_deadline Initial absolute compare deadline in microseconds from the new timer epoch.
 */
void timer_start(uint32_t first_deadline);

/**
 * @brief Arm the TIM2 Channel 1 compare interrupt for an absolute deadline.
 * @param deadline Counter value at which the next compare interrupt should occur.
 */
void timer_set_deadline(uint32_t deadline);

/** @brief Disable and clear the timer compare interrupt. */
void timer_cancel_deadline(void);

/** @brief Scheduler hook invoked when the Channel 1 compare deadline expires. */
void timer_event(void);

/** @brief Scheduler hook invoked when the 32-bit TIM2 counter overflows. */
void timer_overflow_event(void);

#ifdef UNIT_TEST
/**
 * @brief Set TIM2's counter directly for on-target timing tests.
 * @param value Counter value to write.
 */
void timer_test_set_counter(uint32_t value);
#endif

#endif /* TIMER_H */
