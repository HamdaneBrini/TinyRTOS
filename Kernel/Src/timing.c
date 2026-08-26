
/**
 * @file timing.c
 * @brief Kernel task-delay and wrap-safe deadline implementation.
 */

#include <stdint.h>
#include <sys/_types.h>

#include "config.h"
#include "kernel_task.h"
#include "kernel_timing.h"
#include "port.h"
#include "port_context.h"
#include "scheduler.h"
#include "timer.h"

/**
 * @brief Block the current task until the requested delay expires.
 *
 * Converts the millisecond duration to the timer's microsecond time base,
 * moves the running task to the timed wakeup list, programs the earliest
 * pending deadline, and requests a context switch.
 *
 * @param duration_ms Delay duration in milliseconds.
 */
void task_delay(uint32_t duration_ms) {
    uint32_t deadline = timer_now() + duration_ms * 1000U;
    current_task->block_reason = BLOCK_DELAY;
    if (set_task_blocked(current_task) != TINY_OK)
        return;
    if (_insert_task_by_wakeup(current_task) != TINY_OK)
        return;
    if (set_task_wakeup_tick(current_task, deadline) != TINY_OK)
        return;
    port_request_context_switch();
    TCB_t* earliest_task = get_earliest_wakeup_task();
    if (earliest_task != NULL) {
        timer_set_deadline(earliest_task->wakeup_tick);
    }
}

/**
 * @brief Test whether an absolute deadline has expired.
 *
 * Signed subtraction keeps the comparison valid when the unsigned timer
 * counter wraps, provided the compared interval is less than half its range.
 *
 * @param deadline Absolute deadline tick.
 * @param now Current timer tick.
 * @return Nonzero when @p now is at or past @p deadline; otherwise zero.
 */
int deadline_reached(uint32_t deadline, uint32_t now) { return (int32_t)(now - deadline) >= 0; }

/**
 * @brief Determine whether one absolute deadline precedes another.
 *
 * @param a First absolute deadline tick.
 * @param b Second absolute deadline tick.
 * @return Nonzero when @p a occurs before @p b; otherwise zero.
 */
int deadline_before(uint32_t a, uint32_t b) { return (int32_t)(a - b) < 0; }
