/**
 * @file kernel_task.h
 * @brief Internal task control blocks, lists, and state-transition interface.
 */

#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "kernel_timing.h"
#include "mpu.h"
#include "port_mpu.h"
#include "task.h"

/** @brief Opaque task control block type. */
typedef struct TCB_t TCB_t;

/** @brief Comparison callback used to order task lists. */
typedef int (*TCB_Compare_t)(const TCB_t* a, const TCB_t* b);

/** @brief Lifecycle states maintained for every task. */
typedef enum { UNUSED = 0, BLOCKED, READY, RUNNING, SUSPENDED, TERMINATED } TaskStatus_t;

/** @brief Reasons for which a task can remain blocked. */
typedef enum { BLOCK_NONE, BLOCK_DELAY, BLOCK_EVENT, BLOCK_EVENT_TIMEOUT } TaskBlockReason_t;

/** @brief MPU-visible memory region allocated to a task stack. */
typedef struct {
    uint32_t base; /**< Inclusive base address. */
    size_t size; /**< Region capacity in bytes. */
    MemoryAccess_t access_permission; /**< MPU access permission. */
} TaskMemoryRegion_t;

/** @brief Scheduler-owned state and saved context of one task. */
struct TCB_t {
    uint32_t* sp; /**< Saved software-context stack pointer. */
    TaskFunc_t main_func; /**< Task entry point. */
    void* arg; /**< Argument supplied to @ref main_func. */
    TaskMemoryRegion_t stack_region; /**< Allocated stack and MPU attributes. */
    TCB_t* next; /**< Next task in the ready list. */
    TCB_t* prev; /**< Previous task in the ready list. */
    TCB_t* wakeup_next; /**< Next task in the current-cycle wakeup list. */
    TCB_t* wakeup_prev; /**< Previous task in the current-cycle wakeup list. */
    TCB_t* future_wakeup_next; /**< Next task in the future-cycle wakeup list. */
    TCB_t* future_wakeup_prev; /**< Previous task in the future-cycle wakeup list. */
    TaskStatus_t status; /**< Current lifecycle state. */
    TaskStatus_t status_before_suspend; /**< State restored when the task resumes. */
    TaskBlockReason_t block_reason; /**< Condition currently blocking the task. */
    uint32_t priority; /**< Scheduler priority; larger values run first. */
    TimerDeadline_t wakeup_deadline; /**< Absolute deadline for a timed block. */
};

/** @brief Selects which linkage fields a generic task list operates on. */
typedef enum { TASK_READY_LINKS, TASK_WAKEUP_LINKS, TASK_FUTURE_WAKEUP_LINKS } TaskListLinks_t;

/** @brief Ordered intrusive list of task control blocks. */
typedef struct {
    TCB_t* head; /**< First task in list order. */
    size_t lenght; /**< Number of tasks in the list. */
    TCB_Compare_t compare; /**< Ordering predicate. */
    TaskListLinks_t links; /**< TCB linkage pair used by this list. */
} TaskList_t;

/** @brief Arguments transferred by the task-create system call. */
typedef struct {
    TaskHandle_t* task_handle; /**< Optional destination for the new handle. */
    TaskFunc_t main_func; /**< Task entry point. */
    void* arg; /**< Argument supplied to the task entry point. */
    uint32_t priority; /**< Scheduler priority assigned to the task. */
    size_t stack_size; /**< Requested stack capacity in bytes. */
} TaskCreateArgs_t;

/**
 * @brief Initialize task storage, task lists, stack allocation, and the idle task.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t task_system_init(void);

/**
 * @brief Create and enqueue a kernel task.
 * @param task_handle Optional destination for the assigned handle.
 * @param main_func Task entry point.
 * @param arg Argument passed to @p main_func.
 * @param priority Scheduler priority; larger values run first.
 * @param stack_size Requested stack capacity in bytes.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                         size_t stack_size);

/**
 * @brief Reset the priority-ordered ready list.
 * @return TINY_OK after the list is reset.
 */
TinyStatus_t task_ready_list_init(void);

/**
 * @brief Reset the deadline-ordered wakeup list for the current timer cycle.
 * @return TINY_OK after the list is reset.
 */
TinyStatus_t active_wakeup_list_init(void);

/**
 * @brief Reset the deadline-ordered wakeup list for future timer cycles.
 * @return TINY_OK after the list is reset.
 */
TinyStatus_t future_wakeup_list_init(void);

/**
 * @brief Insert a READY task into the ready list.
 * @param task Task to insert.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t _insert_ready_task(TCB_t* task);

/**
 * @brief Insert a timed BLOCKED task into the current-cycle wakeup list.
 * @param task Task to insert.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t insert_task_by_wakeup(TCB_t* task);

/**
 * @brief Insert a timed BLOCKED task into the future-cycle wakeup list.
 * @param task Task to insert.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t insert_task_in_future_list(TCB_t* task);

/**
 * @brief Remove a READY task from the ready list.
 * @param task Task to remove.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t _remove_ready_task(TCB_t* task);

/**
 * @brief Remove a timed BLOCKED task from the current-cycle wakeup list.
 * @param task Task to remove.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t remove_task_from_wakeup_list(TCB_t* task);

/**
 * @brief Remove a timed BLOCKED task from the future-cycle wakeup list.
 * @param task Task to remove.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t remove_task_from_future_list(TCB_t* task);

/** @return Highest-priority task currently ready to run. */
TCB_t* get_highest_priority_ready_task(void);

/** @return Blocked task with the earliest deadline in the current timer cycle. */
TCB_t* get_earliest_wakeup_task(void);

/** @return Blocked task with the earliest absolute deadline in a future timer cycle. */
TCB_t* get_earliest_future_task(void);

/**
 * @param task Task whose successor is requested.
 * @return Task following @p task in the current-cycle wakeup list.
 */
TCB_t* get_next_wakeup_task(TCB_t* task);

/** @return Number of tasks currently stored in the ready list. */
size_t task_ready_count(void);

/**
 * @brief Transition a task to READY and insert it by priority.
 * @param task Task whose state changes.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_ready(TCB_t* task);

/**
 * @brief Transition a task to BLOCKED.
 * @param task Task whose state changes.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_blocked(TCB_t* task);

/**
 * @brief Transition a task to RUNNING.
 * @param task Task whose state changes.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_running(TCB_t* task);

/**
 * @brief Transition a task to SUSPENDED while preserving its prior state.
 * @param task Task whose state changes.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_suspended(TCB_t* task);

/**
 * @brief Transition a task to TERMINATED and unlink it from scheduler lists.
 * @param task Task whose state changes.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_terminated(TCB_t* task);

/**
 * @brief Change a task priority and preserve ready-list ordering.
 * @param task Task to update.
 * @param priority New scheduler priority.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_priority(TCB_t* task, uint32_t priority);

/**
 * @brief Set the cycle and counter offset at which a blocked task should awaken.
 * @param task Timed blocked task to update.
 * @param cycle_index Absolute timer-cycle index.
 * @param offset Counter value within that timer cycle.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t set_task_wakeup_deadline(TCB_t* task, uint64_t cycle_index, uint32_t offset);

/**
 * @brief Move a task behind all ready tasks of equal priority.
 * @param task Task to rotate within its priority group.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t task_move_to_priority_tail(TCB_t* task);

/**
 * @brief Configure the dynamic MPU region for a task stack.
 * @param task Task whose stack becomes accessible.
 * @return TINY_OK on success, otherwise TINY_FAIL.
 */
TinyStatus_t task_stack_mpu_config(TCB_t* task);

/**
 * @brief Resolve a public task handle to its kernel control block.
 * @param task_handle Handle to resolve.
 * @return Matching task control block, or NULL when the handle is invalid.
 */
TCB_t* task_lookup(TaskHandle_t task_handle);

/**
 * @brief Resume a suspended task and restore its previous scheduling state.
 * @param task Task to resume.
 * @return TINY_OK on success; otherwise TINY_FAIL.
 */
TinyStatus_t resume_task(TCB_t* task);
#ifdef UNIT_TEST
/** @return Read-only ready-list state for structural tests. */
const TaskList_t* task_ready_list_get(void);

/** @return Read-only current-cycle wakeup-list state for structural tests. */
const TaskList_t* task_wakeup_list_get(void);
#endif

#endif /* KERNEL_TASK_H */
