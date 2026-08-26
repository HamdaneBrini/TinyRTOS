/**
 * @file timing.h
 * @brief Public task-delay and system-time interface.
 */

#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

#include "config.h"

/** @brief System time split into common units. */
typedef struct {
    uint32_t us;
    uint32_t ms;
    uint32_t s;
} TinyTime_t;

/**
 * @brief Delay the calling task.
 * @param duration_ms Delay duration in milliseconds.
 */
void tiny_delay(uint32_t duration_ms);

/**
 * @brief Read the current system time.
 * @param time Destination for the current time.
 * @return Status returned by the timer system call.
 */
TinyStatus_t tiny_get_now(TinyTime_t* time);

/** @return Current system tick value. */
uint32_t tiny_tick(void);

#endif /* TIMING_H */
