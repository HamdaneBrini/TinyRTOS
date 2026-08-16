#include <stddef.h>

#include "scheduler.h"
#include "tiny_time.h"
#include "tiny_unity.h"
#include "console.h"

static TCB_t make_ready_task(uint32_t priority) {
    TCB_t task = {0};

    task.status = READY;
    task.priority = priority;
    return task;
}

static void dummy_task(void* arg) { (void)arg; }

void tiny_setup(void) { TINY_ASSERT_EQUAL(TINY_OK, task_ready_list_init()); }

static void test_ready_list_init_creates_empty_list(void) {
    const TaskReadyList_t* list = task_ready_list_get();

    TINY_ASSERT_NULL(list->head);
    TINY_ASSERT_EQUAL(0U, list->lenght);
}

static void test_ready_list_rejects_null_task(void) {
    TINY_ASSERT_EQUAL(TINY_FAIL, _insert_ready_task(NULL));
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_ready_list_rejects_task_that_is_not_ready(void) {
    TCB_t task = make_ready_task(1U);
    task.status = BLOCKED;

    TINY_ASSERT_EQUAL(TINY_FAIL, _insert_ready_task(&task));
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_ready_list_inserts_first_task_as_head(void) {
    TCB_t task = make_ready_task(2U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(&task, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(1U, task_ready_list_get()->lenght);
    TINY_ASSERT_NULL(task.next);
}

static void test_ready_list_orders_tasks_by_descending_priority(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t high = make_ready_task(5U);
    TCB_t medium = make_ready_task(3U);
    const TaskReadyList_t* list = task_ready_list_get();

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&high));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&medium));

    TINY_ASSERT_EQUAL(3U, list->lenght);
    TINY_ASSERT_EQUAL(&high, list->head);
    TINY_ASSERT_EQUAL(&medium, high.next);
    TINY_ASSERT_EQUAL(&low, medium.next);
    TINY_ASSERT_NULL(low.next);
}

static void test_ready_list_init_resets_populated_list(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, task_ready_list_init());
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_ready_list_remove_rejects_null_task(void) {
    TINY_ASSERT_EQUAL(TINY_FAIL, _remove_ready_task(NULL));
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_ready_list_remove_rejects_task_that_is_not_ready(void) {
    TCB_t task = make_ready_task(1U);
    task.status = BLOCKED;

    TINY_ASSERT_EQUAL(TINY_FAIL, _remove_ready_task(&task));
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_ready_list_remove_rejects_task_from_empty_list(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_FAIL, _remove_ready_task(&task));
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_ready_list_removes_only_task(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, _remove_ready_task(&task));
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
    TINY_ASSERT_NULL(task.next);
    TINY_ASSERT_NULL(task.prev);
}

static void test_ready_list_removes_head_task(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t high = make_ready_task(3U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&high));
    TINY_ASSERT_EQUAL(TINY_OK, _remove_ready_task(&high));
    TINY_ASSERT_EQUAL(&low, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(1U, task_ready_list_get()->lenght);
    TINY_ASSERT_NULL(low.prev);
    TINY_ASSERT_NULL(high.next);
    TINY_ASSERT_NULL(high.prev);
}

static void test_ready_list_removes_middle_task(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t medium = make_ready_task(2U);
    TCB_t high = make_ready_task(3U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&high));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&medium));
    TINY_ASSERT_EQUAL(TINY_OK, _remove_ready_task(&medium));
    TINY_ASSERT_EQUAL(&high, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(2U, task_ready_list_get()->lenght);
    TINY_ASSERT_EQUAL(&low, high.next);
    TINY_ASSERT_EQUAL(&high, low.prev);
    TINY_ASSERT_NULL(medium.next);
    TINY_ASSERT_NULL(medium.prev);
}

static void test_ready_list_removes_tail_task(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t high = make_ready_task(3U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&high));
    TINY_ASSERT_EQUAL(TINY_OK, _remove_ready_task(&low));
    TINY_ASSERT_EQUAL(&high, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(1U, task_ready_list_get()->lenght);
    TINY_ASSERT_NULL(high.next);
    TINY_ASSERT_NULL(low.next);
    TINY_ASSERT_NULL(low.prev);
}

static void test_set_task_ready_inserts_task_by_priority(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t high = make_ready_task(3U);
    high.status = BLOCKED;

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_ready(&high));
    TINY_ASSERT_EQUAL(READY, high.status);
    TINY_ASSERT_EQUAL(&high, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(2U, task_ready_list_get()->lenght);
}

static void test_set_task_ready_is_idempotent(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_ready(&task));
    TINY_ASSERT_EQUAL(&task, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(1U, task_ready_list_get()->lenght);
}

static void test_set_task_blocked_removes_ready_task(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_blocked(&task));
    TINY_ASSERT_EQUAL(BLOCKED, task.status);
    TINY_ASSERT_NULL(task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(0U, task_ready_list_get()->lenght);
}

static void test_set_task_running_removes_ready_task(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_running(&task));
    TINY_ASSERT_EQUAL(RUNNING, task.status);
    TINY_ASSERT_NULL(task_ready_list_get()->head);
}

static void test_set_task_suspended_removes_ready_task(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_suspended(&task));
    TINY_ASSERT_EQUAL(SUSPENDED, task.status);
    TINY_ASSERT_NULL(task_ready_list_get()->head);
}

static void test_set_task_terminated_removes_ready_task(void) {
    TCB_t task = make_ready_task(1U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&task));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_terminated(&task));
    TINY_ASSERT_EQUAL(TERMINATED, task.status);
    TINY_ASSERT_NULL(task_ready_list_get()->head);
}

static void test_set_task_functions_reject_null(void) {
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_ready(NULL));
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_blocked(NULL));
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_running(NULL));
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_suspended(NULL));
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_terminated(NULL));
}

static void test_task_create_initializes_tcb_and_adds_it_to_ready_list(void) {
    uint8_t stack[64];
    uint32_t argument = 42U;
    TCB_t* task = NULL;

    TINY_ASSERT_EQUAL(TINY_OK, tiny_task_create(&task, dummy_task, &argument, 7U, stack, sizeof(stack)));
    TINY_ASSERT_NOT_NULL(task);
    TINY_ASSERT_EQUAL(READY, task->status);
    TINY_ASSERT_EQUAL(7U, task->priority);
    TINY_ASSERT_EQUAL(dummy_task, task->main_func);
    TINY_ASSERT_EQUAL(&argument, task->arg);
    TINY_ASSERT_EQUAL(stack, task->stack_base);
    TINY_ASSERT_EQUAL(sizeof(stack), task->stack_size);
    TINY_ASSERT_NULL(task->next);
    TINY_ASSERT_NULL(task->prev);
    TINY_ASSERT_EQUAL(task, get_highest_priority_ready_task());
}

static void test_task_create_accepts_null_handle(void) {
    uint8_t stack[32];

    TINY_ASSERT_EQUAL(TINY_OK, tiny_task_create(NULL, dummy_task, NULL, 1U, stack, sizeof(stack)));
    TINY_ASSERT_NOT_NULL(get_highest_priority_ready_task());
}

static void test_get_highest_priority_ready_task_tracks_list_head(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t high = make_ready_task(4U);

    TINY_ASSERT_NULL(get_highest_priority_ready_task());
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(&low, get_highest_priority_ready_task());
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&high));
    TINY_ASSERT_EQUAL(&high, get_highest_priority_ready_task());
}

static void test_set_task_priority_reorders_ready_task(void) {
    TCB_t low = make_ready_task(1U);
    TCB_t high = make_ready_task(3U);

    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&low));
    TINY_ASSERT_EQUAL(TINY_OK, _insert_ready_task(&high));
    TINY_ASSERT_EQUAL(TINY_OK, set_task_priority(&low, 5U));
    TINY_ASSERT_EQUAL(5U, low.priority);
    TINY_ASSERT_EQUAL(&low, task_ready_list_get()->head);
    TINY_ASSERT_EQUAL(&high, low.next);
    TINY_ASSERT_EQUAL(&low, high.prev);
    TINY_ASSERT_EQUAL(2U, task_ready_list_get()->lenght);
}

static void test_set_task_priority_updates_non_ready_task(void) {
    TCB_t task = {0};
    task.status = BLOCKED;
    task.priority = 1U;

    TINY_ASSERT_EQUAL(TINY_OK, set_task_priority(&task, 6U));
    TINY_ASSERT_EQUAL(6U, task.priority);
    TINY_ASSERT_NULL(task_ready_list_get()->head);
}

static void test_set_task_priority_rejects_null(void) {
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_priority(NULL, 1U));
}

static void test_set_task_wakeup_tick_updates_task(void) {
    TCB_t task = {0};

    TINY_ASSERT_EQUAL(TINY_OK, set_task_wakeup_tick(&task, 1234U));
    TINY_ASSERT_EQUAL(1234U, task.wakeup_tick);
    TINY_ASSERT_EQUAL(TINY_FAIL, set_task_wakeup_tick(NULL, 1234U));
}

static void test_scheduler_init_creates_only_idle_task(void) {
    TINY_ASSERT_EQUAL(TINY_OK, scheduler_init());
    TINY_ASSERT_NOT_NULL(get_highest_priority_ready_task());
    TINY_ASSERT_EQUAL(READY, get_highest_priority_ready_task()->status);
    TINY_ASSERT_EQUAL(0U, get_highest_priority_ready_task()->priority);
    TINY_ASSERT_EQUAL(1U, task_ready_list_get()->lenght);
    TINY_ASSERT_NULL(get_highest_priority_ready_task()->next);
}

void Test_start(void) {
    tiny_test_begin();

    RUN_TEST(test_ready_list_init_creates_empty_list);
    RUN_TEST(test_ready_list_rejects_null_task);
    RUN_TEST(test_ready_list_rejects_task_that_is_not_ready);
    RUN_TEST(test_ready_list_inserts_first_task_as_head);
    RUN_TEST(test_ready_list_orders_tasks_by_descending_priority);
    RUN_TEST(test_ready_list_init_resets_populated_list);
    RUN_TEST(test_ready_list_remove_rejects_null_task);
    RUN_TEST(test_ready_list_remove_rejects_task_that_is_not_ready);
    RUN_TEST(test_ready_list_remove_rejects_task_from_empty_list);
    RUN_TEST(test_ready_list_removes_only_task);
    RUN_TEST(test_ready_list_removes_head_task);
    RUN_TEST(test_ready_list_removes_tail_task);
    RUN_TEST(test_ready_list_removes_middle_task);
    RUN_TEST(test_set_task_ready_inserts_task_by_priority);
    RUN_TEST(test_set_task_ready_is_idempotent);
    RUN_TEST(test_set_task_blocked_removes_ready_task);
    RUN_TEST(test_set_task_running_removes_ready_task);
    RUN_TEST(test_set_task_suspended_removes_ready_task);
    RUN_TEST(test_set_task_terminated_removes_ready_task);
    RUN_TEST(test_set_task_functions_reject_null);
    RUN_TEST(test_task_create_initializes_tcb_and_adds_it_to_ready_list);
    RUN_TEST(test_task_create_accepts_null_handle);
    RUN_TEST(test_get_highest_priority_ready_task_tracks_list_head);
    RUN_TEST(test_set_task_priority_reorders_ready_task);
    RUN_TEST(test_set_task_priority_updates_non_ready_task);
    RUN_TEST(test_set_task_priority_rejects_null);
    RUN_TEST(test_set_task_wakeup_tick_updates_task);
    RUN_TEST(test_scheduler_init_creates_only_idle_task);
    tiny_test_end();
}
