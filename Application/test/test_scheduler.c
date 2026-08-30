/**
 * @file test_scheduler.c
 * @brief Scheduler policy, context-selection, and timer-overflow tests.
 */

#include <stdint.h>

#include "kernel_task.h"
#include "memory.h"
#include "mpu.h"
#include "scheduler.h"
#include "stm32h5xx.h"
#include "task.h"
#include "timer.h"
#include "timing.h"
#include "tiny_unity.h"

#define OVERFLOW_TEST_DELAY_MS         10U
#define OVERFLOW_TEST_DELAY_US         (OVERFLOW_TEST_DELAY_MS * 1000U)
#define OVERFLOW_TEST_TICKS_TO_WRAP    5000U
#define OVERFLOW_TEST_TOLERANCE_US     77U
#define OVERFLOW_TEST_TASK_STACK_SIZE  512U
#define SCHEDULER_TEST_TASK_STACK_SIZE 256U

void tiny_setup(void) {
    /* Reapply TIM2 configuration after the preceding test's teardown. The
     * first call is harmless because system_start() already initialized it. */
    (void)timer_init();
    (void)MPU_kernel_config();
    (void)scheduler_init();
}

void tiny_teardown(void) { (void)timer_deinit(); }

/** No-op entry point used to create schedulable test tasks. */
static void scheduler_test_task(void* argument) { (void)argument; }

/** Create a task and resolve its public handle to the internal test TCB. */
static TCB_t* create_scheduler_test_task(uint32_t priority) {
    TaskHandle_t handle;

    if (task_create(&handle, scheduler_test_task, NULL, priority, SCHEDULER_TEST_TASK_STACK_SIZE) != TINY_OK) {
        return NULL;
    }
    return task_lookup(handle);
}

static void test_scheduler_start_rejects_empty_ready_list(void) {
    TINY_ASSERT_EQUAL(TINY_OK, task_ready_list_init());

    TINY_ASSERT_EQUAL(TINY_FAIL, scheduler_start());
    TINY_ASSERT_NULL(current_task);
}

static void test_scheduler_start_selects_highest_priority_task(void) {
    TCB_t* low = create_scheduler_test_task(1U);
    TCB_t* high = create_scheduler_test_task(3U);

    TINY_ASSERT_NOT_NULL(low);
    TINY_ASSERT_NOT_NULL(high);
    TINY_ASSERT_EQUAL(TINY_OK, scheduler_start());

    /* These tests inspect scheduler state on MSP rather than entering the
     * selected task. Prevent the live timer from requesting PendSV. */
    NVIC_DisableIRQ(TIM2_IRQn);

    TINY_ASSERT_EQUAL(high, current_task);
    TINY_ASSERT_EQUAL(RUNNING, high->status);
    TINY_ASSERT_EQUAL(READY, low->status);
    TINY_ASSERT_EQUAL(low, get_highest_priority_ready_task());
    TINY_ASSERT_EQUAL(0U, timeslice_end.cycle_index);
    TINY_ASSERT_GREATER_THAN(0U, timeslice_end.offset);
}

static void test_higher_priority_ready_task_preempts_current_task(void) {
    uint32_t saved_context[8] = {0};
    TCB_t* low = create_scheduler_test_task(1U);
    TCB_t* high = create_scheduler_test_task(3U);

    TINY_ASSERT_NOT_NULL(low);
    TINY_ASSERT_NOT_NULL(high);
    TINY_ASSERT_EQUAL(TINY_OK, set_task_running(low));
    current_task = low;
    uint32_t* high_context = high->sp;

    TINY_ASSERT_EQUAL(high_context, scheduler_context_switch(saved_context));
    TINY_ASSERT_EQUAL(saved_context, low->sp);
    TINY_ASSERT_EQUAL(READY, low->status);
    TINY_ASSERT_EQUAL(RUNNING, high->status);
    TINY_ASSERT_EQUAL(high, current_task);
}

static void test_lower_priority_ready_task_does_not_preempt_current_task(void) {
    uint32_t saved_context[8] = {0};
    TCB_t* low = create_scheduler_test_task(1U);
    TCB_t* high = create_scheduler_test_task(3U);

    TINY_ASSERT_NOT_NULL(low);
    TINY_ASSERT_NOT_NULL(high);
    TINY_ASSERT_EQUAL(TINY_OK, set_task_running(high));
    current_task = high;

    TINY_ASSERT_EQUAL(saved_context, scheduler_context_switch(saved_context));
    TINY_ASSERT_EQUAL(saved_context, high->sp);
    TINY_ASSERT_EQUAL(RUNNING, high->status);
    TINY_ASSERT_EQUAL(READY, low->status);
    TINY_ASSERT_EQUAL(high, current_task);
}

static void test_equal_priority_tasks_rotate_round_robin(void) {
    uint32_t first_saved_context[8] = {0};
    uint32_t second_saved_context[8] = {0};
    TCB_t* first = create_scheduler_test_task(2U);
    TCB_t* second = create_scheduler_test_task(2U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(second);
    TINY_ASSERT_EQUAL(TINY_OK, set_task_running(first));
    current_task = first;
    uint32_t* second_initial_context = second->sp;

    TINY_ASSERT_EQUAL(second_initial_context, scheduler_context_switch(first_saved_context));
    TINY_ASSERT_EQUAL(second, current_task);
    TINY_ASSERT_EQUAL(RUNNING, second->status);
    TINY_ASSERT_EQUAL(READY, first->status);
    TINY_ASSERT_EQUAL(first, get_highest_priority_ready_task());

    TINY_ASSERT_EQUAL(first_saved_context, scheduler_context_switch(second_saved_context));
    TINY_ASSERT_EQUAL(first, current_task);
    TINY_ASSERT_EQUAL(RUNNING, first->status);
    TINY_ASSERT_EQUAL(READY, second->status);
    TINY_ASSERT_EQUAL(second, get_highest_priority_ready_task());
}

static void test_blocked_current_task_switches_to_ready_task(void) {
    uint32_t saved_context[8] = {0};
    TCB_t* low = create_scheduler_test_task(1U);
    TCB_t* high = create_scheduler_test_task(3U);

    TINY_ASSERT_NOT_NULL(low);
    TINY_ASSERT_NOT_NULL(high);
    TINY_ASSERT_EQUAL(TINY_OK, set_task_running(high));
    current_task = high;
    high->block_reason = BLOCK_EVENT;
    TINY_ASSERT_EQUAL(TINY_OK, set_task_blocked(high));
    uint32_t* low_context = low->sp;

    TINY_ASSERT_EQUAL(low_context, scheduler_context_switch(saved_context));
    TINY_ASSERT_EQUAL(BLOCKED, high->status);
    TINY_ASSERT_EQUAL(RUNNING, low->status);
    TINY_ASSERT_EQUAL(low, current_task);
}

static void test_timer_deadline_comparison_uses_full_counter_range(void) {
    TimerDeadline_t end_of_cycle = {.cycle_index = 4U, .offset = UINT32_MAX};
    TimerDeadline_t start_of_same_cycle = {.cycle_index = 4U, .offset = 0U};
    TimerDeadline_t exact_deadline = end_of_cycle;
    TimerDeadline_t start_of_next_cycle = {.cycle_index = 5U, .offset = 0U};
    TimerDeadline_t future_cycle = {.cycle_index = 5U, .offset = 2U};

    /* Offsets spanning the complete uint32_t range remain ordered because
     * the cycle index removes the ambiguity at counter wrap. */
    TINY_ASSERT_FALSE(timer_deadline_reached(&end_of_cycle, &start_of_same_cycle));
    TINY_ASSERT_TRUE(timer_deadline_reached(&end_of_cycle, &exact_deadline));
    TINY_ASSERT_TRUE(timer_deadline_reached(&end_of_cycle, &start_of_next_cycle));
    TINY_ASSERT_FALSE(timer_deadline_reached(&future_cycle, &end_of_cycle));
}

static void test_absolute_deadline_comparison_uses_cycle_then_offset(void) {
    TimerDeadline_t early_cycle = {.cycle_index = 4U, .offset = UINT32_MAX};
    TimerDeadline_t late_cycle = {.cycle_index = 5U, .offset = 0U};
    TimerDeadline_t early_offset = {.cycle_index = 7U, .offset = 10U};
    TimerDeadline_t late_offset = {.cycle_index = 7U, .offset = 20U};

    TINY_ASSERT_TRUE(deadline_before(&early_cycle, &late_cycle));
    TINY_ASSERT_FALSE(deadline_before(&late_cycle, &early_cycle));
    TINY_ASSERT_TRUE(deadline_before(&early_offset, &late_offset));
    TINY_ASSERT_FALSE(deadline_before(&late_offset, &early_offset));
    TINY_ASSERT_FALSE(deadline_before(&early_offset, &early_offset));
}

static void verify_task_delay_with_overflow(void) {
    /* Place the counter 5 ms before wrap, then request a 10 ms delay. */
    timer_test_set_counter(UINT32_MAX - OVERFLOW_TEST_TICKS_TO_WRAP);
    uint32_t start_offset = timer_now();
    uint32_t start_cycle = cycle_counter;

    tiny_delay(OVERFLOW_TEST_DELAY_MS);

    uint32_t elapsed_us = timer_now() - start_offset;
    uint32_t elapsed_cycles = cycle_counter - start_cycle;

    TINY_ASSERT_EQUAL(1U, elapsed_cycles);
    TINY_ASSERT_GREATER_OR_EQUAL(OVERFLOW_TEST_DELAY_US, elapsed_us);
    TINY_ASSERT_LESS_OR_EQUAL(OVERFLOW_TEST_DELAY_US + OVERFLOW_TEST_TOLERANCE_US, elapsed_us);
}

static void overflow_delay_task(void* argument) {
    (void)argument;

    verify_task_delay_with_overflow();

    /* tiny_scheduler_start() does not return after entering task mode, so finish
     * reporting from the test task. */
    tiny_test_result("test_task_delay_with_overflow");
    tiny_test_end();
}

static void test_task_delay_with_overflow(void) {
    TaskHandle_t test_task;

    TINY_ASSERT_EQUAL(TINY_OK,
                      tiny_task_create(&test_task, overflow_delay_task, NULL, 1U, OVERFLOW_TEST_TASK_STACK_SIZE));

    /* The scheduler switches to overflow_delay_task and does not return on
     * success. A return therefore indicates startup failure. */
    TINY_ASSERT_EQUAL(TINY_OK, tiny_scheduler_start());
}

void Test_start(void) {
    tiny_test_begin();
    RUN_TEST(test_scheduler_start_rejects_empty_ready_list);
    RUN_TEST(test_scheduler_start_selects_highest_priority_task);
    RUN_TEST(test_higher_priority_ready_task_preempts_current_task);
    RUN_TEST(test_lower_priority_ready_task_does_not_preempt_current_task);
    RUN_TEST(test_equal_priority_tasks_rotate_round_robin);
    RUN_TEST(test_blocked_current_task_switches_to_ready_task);
    RUN_TEST(test_timer_deadline_comparison_uses_full_counter_range);
    RUN_TEST(test_absolute_deadline_comparison_uses_cycle_then_offset);

    /* This integration test enters task mode and therefore must remain last. */
    RUN_TEST(test_task_delay_with_overflow);
    tiny_test_end();
}
