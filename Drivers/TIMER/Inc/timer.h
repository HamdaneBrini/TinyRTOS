/**
 * @file timer.h
 * @brief Public interface for the TIM2 hardware timer driver.
 */

#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

/** @brief Initialize TIM2 as a 1 MHz output-compare timer. */
void timer_init(void);

/** @return Current TIM2 counter value in microseconds. */
uint32_t timer_now(void);

/** @param first_deadline Initial absolute compare deadline. */
void timer_start(uint32_t first_deadline);

/** @param deadline Next absolute compare deadline. */
void timer_set_deadline(uint32_t deadline);

/** @brief Disable and clear the timer compare interrupt. */
void timer_cancel_deadline(void);

/** @brief Process an expired timer deadline. */
void timer_event(void);

#endif /* TIMER_H */
