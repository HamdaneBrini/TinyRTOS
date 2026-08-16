#ifndef SCHEDULER_H
#define SCHEDULER_H

/**
 * @file scheduler.h
 * @brief Task-control and ready-list interface for the TinyRTOS scheduler.
 */

#include <stddef.h>
#include <stdint.h>

#include "config.h"

/** @brief Entry-point signature used by a scheduled task. */
typedef void (*TaskFunc_t)(void* arg);

/** @brief States through which a task can transition during its lifetime. */
typedef enum {
    UNUSED = 0, /**< TCB slot is available for allocation. */
    BLOCKED,    /**< Task is waiting for an event or timeout. */
    READY,      /**< Task is eligible to be scheduled. */
    RUNNING,    /**< Task is currently executing. */
    SUSPENDED,  /**< Task was explicitly suspended. */
    TERMINATED, /**< Task has completed execution. */
} TaskStatus_t;

/** @brief Task Control Block containing task context and scheduler metadata. */
typedef struct TCB_t {
    TaskStatus_t status;   /**< Current lifecycle state. */
    uint32_t priority;     /**< Scheduling priority; larger values run first. */
    uint32_t wakeup_tick;  /**< Tick at which a blocked task becomes ready. */
    TaskFunc_t main_func;  /**< Task entry point. */
    void* arg;             /**< Argument passed to the entry point. */
    size_t stack_size;     /**< Stack capacity in bytes. */
    void* stack_base;      /**< Base address of the task stack. */
    void* sp;              /**< Saved stack pointer. */
    struct TCB_t* next;    /**< Next task in the ready list. */
    struct TCB_t* prev;    /**< Previous task in the ready list. */
} TCB_t;

/** @brief Priority-ordered, doubly linked list of ready tasks. */
typedef struct {
    TCB_t* head;  /**< Highest-priority ready task, or NULL when empty. */
    size_t lenght; /**< Number of tasks in the list. */
} TaskReadyList_t;

/**
 * @brief Allocate and initialize a task, then insert it into the ready list.
 *
 * @param task_handle Optional output pointer receiving the allocated TCB.
 * @param main_func Task entry point.
 * @param arg Argument passed to @p main_func.
 * @param priority Task scheduling priority.
 * @param stack_base Base address of the task stack.
 * @param stack_size Stack capacity in bytes.
 * @return TINY_OK on success, or TINY_FAIL when no TCB is available or insertion fails.
 */
TinyStatus_t tiny_task_create(TCB_t** task_handle, TaskFunc_t main_func, void* arg, uint32_t priority, void* stack_base,
                              size_t stack_size);

/**
 * @brief Reset the ready list to an empty state.
 * @return TINY_OK after the list has been reset.
 */
TinyStatus_t task_ready_list_init(void);

/**
 * @brief Insert a READY task in descending-priority order.
 *
 * @param task Task to insert.
 * @return TINY_OK on success, or TINY_FAIL for NULL or non-READY tasks.
 */
TinyStatus_t _insert_ready_task(TCB_t* task);

/**
 * @brief Unlink a READY task that belongs to the ready list.
 *
 * @param task Task to remove.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or an empty list.
 * @note The caller must ensure that @p task belongs to the ready list.
 */
TinyStatus_t _remove_ready_task(TCB_t* task);

/**
 * @brief Return the highest-priority task that is ready to run.
 *
 * @return Pointer to the ready-list head, or NULL when no task is ready.
 */
TCB_t* get_highest_priority_ready_task(void);

/**
 * @brief Initialize scheduler-owned data structures.
 *
 * @return TINY_OK on success, or TINY_FAIL when initialization fails.
 */
TinyStatus_t scheduler_init(void);

/**
 * @brief Transition a task to READY and add it to the ready list.
 *
 * @param task Task to make ready.
 * @return TINY_OK on success, or TINY_FAIL for NULL input or insertion failure.
 * @note Calling this function for an already READY task has no effect.
 */
TinyStatus_t set_task_ready(TCB_t* task);

/**
 * @brief Transition a task to BLOCKED and remove it from the ready list.
 *
 * @param task Task to block.
 * @return TINY_OK on success, or TINY_FAIL for NULL input or removal failure.
 */
TinyStatus_t set_task_blocked(TCB_t* task);

/**
 * @brief Transition a task to RUNNING and remove it from the ready list.
 *
 * @param task Task selected to run.
 * @return TINY_OK on success, or TINY_FAIL for NULL input or removal failure.
 */
TinyStatus_t set_task_running(TCB_t* task);

/**
 * @brief Transition a task to SUSPENDED and remove it from the ready list.
 *
 * @param task Task to suspend.
 * @return TINY_OK on success, or TINY_FAIL for NULL input or removal failure.
 */
TinyStatus_t set_task_suspended(TCB_t* task);

/**
 * @brief Transition a task to TERMINATED and remove it from the ready list.
 *
 * @param task Task whose execution has ended.
 * @return TINY_OK on success, or TINY_FAIL for NULL input or removal failure.
 */
TinyStatus_t set_task_terminated(TCB_t* task);

/**
 * @brief Change a task's scheduling priority.
 *
 * A READY task is reinserted to preserve ready-list ordering.
 *
 * @param task Task whose priority is changing.
 * @param priority New scheduling priority.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or list failure.
 */
TinyStatus_t set_task_priority(TCB_t* task, uint32_t priority);

/**
 * @brief Set the absolute tick at which a task should wake.
 * @param task Task whose wake-up time is changing.
 * @param abs_tick Absolute scheduler tick.
 * @return TINY_OK on success, or TINY_FAIL for NULL input.
 */
TinyStatus_t set_task_wakeup_tick(TCB_t* task, uint32_t abs_tick);

#ifdef UNIT_TEST
/**
 * @brief Expose the ready list for structural unit-test assertions.
 *
 * @return Read-only pointer to the scheduler's ready list.
 */
const TaskReadyList_t* task_ready_list_get(void);
#endif

#endif /* SCHEDULER_H */
