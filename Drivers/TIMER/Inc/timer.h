/**
 * @file timer.h
 * @brief Public interface for the TIM2 hardware timer driver.
 */

#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
/** @brief Initialize TIM2 as a 1 MHz output-compare timer. */
void timer_init(void);
uint32_t timer_now(void);
void timer_start(uint32_t first_deadline);
void timer_set_deadline(uint32_t deadline);
void timer_cancel_deadline(void);
void timer_event(void);
#endif /* TIMER_H */
