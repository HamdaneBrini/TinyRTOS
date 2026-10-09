
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
#include "timing.h"

/**
 * @brief Capture a wrap-aware snapshot of the TIM2 time base.
 * @return Current absolute deadline representation.
 */
static inline TimerDeadline_t get_current_time(void) {
    TimerDeadline_t now;
    now.cycle_index = cycle_counter;
    now.offset = timer_now();

    if (timer_overflow_pending()) {
        now.cycle_index++;
        now.offset = timer_now();
    }

    return now;
}

/** @copydoc timer_deadline_add */
TimerDeadline_t timer_deadline_add(TimerDeadline_t deadline, uint64_t duration_us) {
    uint64_t offset_sum = (uint64_t)deadline.offset + (uint64_t)(uint32_t)duration_us;

    deadline.cycle_index += (duration_us >> 32) + (offset_sum >> 32);
    deadline.offset = (uint32_t)offset_sum;
    return deadline;
}

/** @copydoc task_delay_us */
void task_delay_us(uint64_t duration_us) {
    TimerDeadline_t now = get_current_time();
    TimerDeadline_t wakeup_deadline = timer_deadline_add(now, duration_us);

    current_task->block_reason = BLOCK_DELAY;
    if (set_task_blocked(current_task) != TINY_OK) {
        current_task->block_reason = BLOCK_NONE;
        return;
    }

    if (set_task_wakeup_deadline(current_task, wakeup_deadline.cycle_index, wakeup_deadline.offset) != TINY_OK) {
        goto insertion_failed;
    }

    if (wakeup_deadline.cycle_index == now.cycle_index) {
        if (insert_task_by_wakeup(current_task) != TINY_OK) {
            goto insertion_failed;
        }
    } else if (insert_task_in_future_list(current_task) != TINY_OK) {
        goto insertion_failed;
    }
    /* A task that blocks before the current boundary does not consume another
     * time slot. Only catch up when the existing boundary has really expired,
     * preserving the scheduler's fixed time grid. */
    if (timer_deadline_reached(&timeslice_end, &now)) {
        do {
            timeslice_end = timer_deadline_add(timeslice_end, TASK_TIME_SLICE);
        } while (timer_deadline_reached(&timeslice_end, &now));
    }

    TimerDeadline_t next_deadline = timeslice_end;

    TCB_t* earliest_task = get_earliest_wakeup_task();
    if (earliest_task != NULL) {

        if (deadline_before(&(earliest_task->wakeup_deadline), &next_deadline)) {
            next_deadline = earliest_task->wakeup_deadline;
        }
    }

    timer_set_deadline(next_deadline.offset);

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
