
/**
 * @file scheduler.c
 * @brief Task creation and priority-ordered ready-list implementation.
 */
#include "stm32h5xx.h"
#include "scheduler.h"
#include <stdint.h>
#include "config.h"

/** Maximum number of task control blocks owned by the scheduler. */
#define MAX_TASKS 16
/** Idle-task stack size in bytes. */
#define IDLE_STACK_SIZE 256

/* Scheduler state is placed in privileged kernel RAM by the linker script. */
__attribute__((section(".kernel_bss"))) static TaskReadyList_t taskReadyList;
__attribute__((section(".kernel_bss"))) static TCB_t TCBT[MAX_TASKS];
__attribute__((section(".kernel_bss"))) static TCB_t* idle_task;
__attribute__((section(".kernel_bss"), aligned(8))) static uint8_t idle_task_stack[IDLE_STACK_SIZE];

/* Internal ready-list operations are public only to support structural tests. */
TinyStatus_t _insert_ready_task(TCB_t* task);
TinyStatus_t _remove_ready_task(TCB_t* task);
/**
 * @brief Allocate a TCB, initialize it, and make the task ready to run.
 *
 * @param task_handle Optional output pointer receiving the allocated TCB.
 * @param main_func Task entry point.
 * @param arg Argument supplied to the task entry point.
 * @param priority Scheduling priority; larger values have higher priority.
 * @param stack_base Base address of the task stack.
 * @param stack_size Stack capacity in bytes.
 * @return TINY_OK on success, or TINY_FAIL if allocation or insertion fails.
 */
TinyStatus_t tiny_task_create(TCB_t** task_handle, TaskFunc_t main_func, void* arg, uint32_t priority, void* stack_base,
                              size_t stack_size) {

    /* Reuse the first unallocated entry in the fixed-size TCB table. */
    for (int i = 0; i < MAX_TASKS; i++) {
        if (TCBT[i].status == UNUSED) {
            TCB_t* new_task = &TCBT[i];
            new_task->status = READY;
            new_task->main_func = main_func;
            new_task->arg = arg;
            new_task->priority = priority;
            new_task->stack_base = stack_base;
            new_task->stack_size = stack_size;
            new_task->next = NULL;
            new_task->prev = NULL;
            if (task_handle != NULL) {
                *task_handle = new_task;
            }
            if (_insert_ready_task(new_task) != TINY_OK) {
                return TINY_FAIL;
            }
            return TINY_OK;
        }
    }
    return TINY_FAIL;
}

/**
 * @brief Clear the scheduler's ready list.
 *
 * @return TINY_OK after the list has been reset.
 */
TinyStatus_t task_ready_list_init(void) {
    taskReadyList.head = NULL;
    taskReadyList.lenght = 0;
    return TINY_OK;
}

#ifdef UNIT_TEST
/**
 * @brief Provide read-only access to ready-list state for unit tests.
 *
 * @return Pointer to the scheduler's ready list.
 */
const TaskReadyList_t* task_ready_list_get(void) { return &taskReadyList; }
#endif

/**
 * @brief Insert a task into the ready list in descending-priority order.
 *
 * @param task READY task to insert.
 * @return TINY_OK on success, or TINY_FAIL for NULL or non-READY tasks.
 */
TinyStatus_t _insert_ready_task(TCB_t* task) {
    if (task == NULL)
        return TINY_FAIL;
    if (task->status != READY)
        return TINY_FAIL;
    TCB_t* current_task = taskReadyList.head;
    /* The first ready task becomes the list head. */
    if (current_task == NULL) {
        taskReadyList.head = task;
        task->next = NULL;
        task->prev = NULL;
        taskReadyList.lenght++;
        return TINY_OK;
    }
    /* A task with greater priority becomes the new head. */
    if (current_task->priority < task->priority) {
        taskReadyList.head = task;
        task->next = current_task;
        task->prev = NULL;
        current_task->prev = task;
        taskReadyList.lenght++;
        return TINY_OK;
    }
    /* Find the insertion point while preserving descending priority order. */
    while (current_task->next != NULL && task->priority < current_task->next->priority) {
        current_task = current_task->next;
    }
    task->next = current_task->next;
    task->prev = current_task;
    if (task->next != NULL) {
        task->next->prev = task;
    }
    current_task->next = task;
    taskReadyList.lenght++;
    return TINY_OK;
}

/**
 * @brief Remove a task from the ready list and repair neighboring links.
 *
 * @param task READY task to remove.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or an empty list.
 * @note The caller must ensure that @p task belongs to taskReadyList.
 */
TinyStatus_t _remove_ready_task(TCB_t* task) {
    if (task == NULL)
        return TINY_FAIL;
    if (task->status != READY)
        return TINY_FAIL;
    if (taskReadyList.head == NULL)
        return TINY_FAIL;
    /* Removing the only task leaves an empty list. */
    if (task->prev == NULL && task->next == NULL) {
        taskReadyList.head = NULL;
        taskReadyList.lenght = 0;
        return TINY_OK;
    }
    /* Promote the second task when removing the head. */
    if (task->prev == NULL) {
        taskReadyList.head = task->next;
        if (task->next != NULL) {
            task->next->prev = NULL;
        }
        taskReadyList.lenght--;
        task->next = NULL;
        return TINY_OK;
    }
    /* Detach the final task without changing the head. */
    if (task->next == NULL) {
        task->prev->next = NULL;
        taskReadyList.lenght--;
        task->prev = NULL;
        return TINY_OK;
    }
    /* Bridge both neighbors when removing a task from the middle. */
    task->prev->next = task->next;
    task->next->prev = task->prev;
    taskReadyList.lenght--;
    task->next = NULL;
    task->prev = NULL;
    return TINY_OK;
}
/** Run whenever no application task is ready. */
static void idle_task_func(void* arg) {
    (void)arg;
    while (1) {
        __WFI();
    }
}

/** Create the lowest-priority task that sleeps until an interrupt occurs. */
static TinyStatus_t idle_task_init(void) {
    return tiny_task_create(&idle_task, idle_task_func, NULL, 0U, idle_task_stack, sizeof(idle_task_stack));
}

/**
 * @brief Get the next task selected by the priority scheduler.
 *
 * @return Ready-list head, or NULL when the ready list is empty.
 */
TCB_t* get_highest_priority_ready_task(void) { return taskReadyList.head; }

/**
 * @brief Initialize scheduler-owned state.
 *
 * @return TINY_OK on success, or TINY_FAIL if ready-list initialization fails.
 */
TinyStatus_t scheduler_init(void) {
    if (task_ready_list_init() != TINY_OK) {
        return TINY_FAIL;
    }
    return idle_task_init();
}

/**
 * @brief Move a task out of the ready list and assign a non-READY state.
 *
 * @param task Task whose state is changing.
 * @param status New state; must not be READY.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or removal failure.
 */
static TinyStatus_t set_task_not_ready(TCB_t* task, TaskStatus_t status) {
    if (task == NULL || status == READY) {
        return TINY_FAIL;
    }
    if (task->status == status) {
        return TINY_OK;
    }
    if (task->status == READY && _remove_ready_task(task) != TINY_OK) {
        return TINY_FAIL;
    }
    task->status = status;
    return TINY_OK;
}

/**
 * @brief Transition a task to READY and insert it by priority.
 *
 * @param task Task to make ready.
 * @return TINY_OK on success, or TINY_FAIL for NULL input or insertion failure.
 */
TinyStatus_t set_task_ready(TCB_t* task) {
    if (task == NULL) {
        return TINY_FAIL;
    }
    if (task->status == READY) {
        return TINY_OK;
    }

    TaskStatus_t previous_status = task->status;
    task->status = READY;
    if (_insert_ready_task(task) != TINY_OK) {
        task->status = previous_status;
        return TINY_FAIL;
    }
    return TINY_OK;
}

/**
 * @brief Transition a task to BLOCKED.
 *
 * @param task Task to block.
 * @return TINY_OK on success, or TINY_FAIL when the transition cannot be completed.
 */
TinyStatus_t set_task_blocked(TCB_t* task) { return set_task_not_ready(task, BLOCKED); }

/**
 * @brief Transition a task to RUNNING.
 *
 * @param task Task selected to run.
 * @return TINY_OK on success, or TINY_FAIL when the transition cannot be completed.
 */
TinyStatus_t set_task_running(TCB_t* task) { return set_task_not_ready(task, RUNNING); }

/**
 * @brief Transition a task to SUSPENDED.
 *
 * @param task Task to suspend.
 * @return TINY_OK on success, or TINY_FAIL when the transition cannot be completed.
 */
TinyStatus_t set_task_suspended(TCB_t* task) { return set_task_not_ready(task, SUSPENDED); }

/**
 * @brief Transition a task to TERMINATED.
 *
 * @param task Task whose execution has ended.
 * @return TINY_OK on success, or TINY_FAIL when the transition cannot be completed.
 */
TinyStatus_t set_task_terminated(TCB_t* task) { return set_task_not_ready(task, TERMINATED); }


/** Update priority and restore ready-list ordering when necessary. */
TinyStatus_t set_task_priority(TCB_t* task, uint32_t priority) {
    if (task == NULL) {
        return TINY_FAIL;
    }
    if (task->priority == priority) {
        return TINY_OK;
    }
    if (task->status == READY && _remove_ready_task(task) != TINY_OK) {
        return TINY_FAIL;
    }
    task->priority = priority;
    if (task->status == READY && _insert_ready_task(task) != TINY_OK) {
        return TINY_FAIL;
    }
    return TINY_OK;
}

/** Store the absolute tick at which a blocked task should awaken. */
TinyStatus_t set_task_wakeup_tick(TCB_t* task, uint32_t abs_tick) {
    if (task == NULL) {
        return TINY_FAIL;
    }
    task->wakeup_tick = abs_tick;
    return TINY_OK;
}
