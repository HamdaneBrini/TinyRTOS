
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
 * moves the running task to the current- or future-cycle wakeup list, programs
 * the earliest active deadline, and requests a context switch.
 *
 * @param duration_ms Delay duration in milliseconds.
 */
void task_delay(uint32_t duration_ms) {
    if (duration_ms == 0U) {
        return;
    }

    uint64_t duration_us = (uint64_t)duration_ms * 1000ULL;
    uint64_t relative_deadline = (uint64_t)timer_now() + duration_us;
    uint64_t cycle_index = (uint64_t)cycle_counter + (relative_deadline >> 32);
    uint32_t offset = (uint32_t)relative_deadline;

    current_task->block_reason = BLOCK_DELAY;
    if (set_task_blocked(current_task) != TINY_OK) {
        current_task->block_reason = BLOCK_NONE;
        return;
    }

    if (set_task_wakeup_deadline(current_task, cycle_index, offset) != TINY_OK) {
        goto insertion_failed;
    }

    if (cycle_index == (uint64_t)cycle_counter) {
        if (insert_task_by_wakeup(current_task) != TINY_OK) {
            goto insertion_failed;
        }
    } else if (insert_task_in_future_list(current_task) != TINY_OK) {
        goto insertion_failed;
    }

    TCB_t* earliest_task = get_earliest_wakeup_task();
    if (earliest_task != NULL) {
        timer_set_deadline(earliest_task->wakeup_deadline.offset);
    }
    port_request_context_switch();
    return;

insertion_failed:
    /* Restore the caller if its deadline could not be queued. */
    current_task->block_reason = BLOCK_NONE;
    (void)set_task_running(current_task);
}

/**
 * @brief Test whether an absolute timer deadline has been reached.
 *
 * Comparing the overflow-cycle index before the counter offset removes the
 * half-range limitation of signed 32-bit offset subtraction.
 *
 * @param deadline Absolute deadline to test.
 * @param now Current absolute timer position.
 * @return Nonzero when @p now is at or past @p deadline; otherwise zero.
 */
int timer_deadline_reached(const TimerDeadline_t* deadline, const TimerDeadline_t* now) {
    return !deadline_before(now, deadline);
}

/**
 * @brief Determine whether one absolute deadline precedes another.
 *
 * @param a First absolute deadline.
 * @param b Second absolute deadline.
 * @return Nonzero when @p a occurs before @p b; otherwise zero.
 */
int deadline_before(const TimerDeadline_t* a, const TimerDeadline_t* b) {
    if (a->cycle_index != b->cycle_index) {
        return a->cycle_index < b->cycle_index;
    }

    return a->offset < b->offset;
}
