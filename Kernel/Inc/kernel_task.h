/**
 * @file kernel_task.h
 * @brief Internal task control blocks, lists, and state-transition interface.
 */

#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "mpu.h"
#include "task.h"
#include "port_mpu.h"

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
    uint32_t base;
    size_t size;
    MemoryAccess_t access_permission;
} TaskMemoryRegion_t;

/** @brief Scheduler-owned state and saved context of one task. */
struct TCB_t {
    uint32_t* sp;
    TaskFunc_t main_func;
    void* arg;
    TaskMemoryRegion_t stack_region;
    TCB_t* next;
    TCB_t* prev;
    TCB_t* wakeup_next;
    TCB_t* wakeup_prev;
    TaskStatus_t status;
    TaskBlockReason_t block_reason;
    uint32_t priority;
    uint32_t wakeup_tick;
};

/** @brief Selects which linkage fields a generic task list operates on. */
typedef enum { TASK_READY_LINKS, TASK_WAKEUP_LINKS } TaskListLinks_t;

/** @brief Ordered intrusive list of task control blocks. */
typedef struct {
    TCB_t* head;
    size_t lenght;
    TCB_Compare_t compare;
    TaskListLinks_t links;
} TaskList_t;

/** @brief Arguments transferred by the task-create system call. */
typedef struct {

    TaskHandle_t* task_handle;
    TaskFunc_t main_func;
    void* arg;
    uint32_t priority;
    size_t stack_size;

} TaskCreateArgs_t;

/** @brief Initialize task storage, task lists, stack allocation, and the idle task. */
TinyStatus_t task_system_init(void);

/** @brief Create and enqueue a kernel task. */
TinyStatus_t task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                         size_t stack_size);

/** @brief Create a task through the SVC interface. */
TinyStatus_t tiny_task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                              size_t stack_size);

/** @brief Reset the priority-ordered ready list. */
TinyStatus_t task_ready_list_init(void);

/** @brief Reset the deadline-ordered wakeup list. */
TinyStatus_t task_wakeup_list_init(void);

/** @brief Insert a READY task into the ready list. */
TinyStatus_t _insert_ready_task(TCB_t* task);

/** @brief Insert a timed BLOCKED task into the wakeup list. */
TinyStatus_t _insert_task_by_wakeup(TCB_t* task);

/** @brief Remove a READY task from the ready list. */
TinyStatus_t _remove_ready_task(TCB_t* task);

/** @brief Remove a timed BLOCKED task from the wakeup list. */
TinyStatus_t _remove_task_from_wakeup_list(TCB_t* task);

/** @return Highest-priority task currently ready to run. */
TCB_t* get_highest_priority_ready_task(void);

/** @return Blocked task with the earliest wakeup deadline. */
TCB_t* get_earliest_wakeup_task(void);

/** @return Task following @p task in the wakeup list. */
TCB_t* get_next_wakeup_task(TCB_t* task);

/** @return Number of tasks currently stored in the ready list. */
size_t task_ready_count(void);

/** @brief Transition @p task to READY. */
TinyStatus_t set_task_ready(TCB_t* task);

/** @brief Transition @p task to BLOCKED. */
TinyStatus_t set_task_blocked(TCB_t* task);

/** @brief Transition @p task to RUNNING. */
TinyStatus_t set_task_running(TCB_t* task);

/** @brief Transition @p task to SUSPENDED. */
TinyStatus_t set_task_suspended(TCB_t* task);

/** @brief Transition @p task to TERMINATED. */
TinyStatus_t set_task_terminated(TCB_t* task);

/** @brief Change a task priority and preserve ready-list ordering. */
TinyStatus_t set_task_priority(TCB_t* task, uint32_t priority);

/** @brief Set the absolute wakeup deadline of a blocked task. */
TinyStatus_t set_task_wakeup_tick(TCB_t* task, uint32_t abs_tick);

/** @brief Move a task behind all ready tasks of equal priority. */
TinyStatus_t task_move_to_priority_tail(TCB_t* task);

/** @brief Configure the dynamic MPU region for a task stack. */
TinyStatus_t task_stack_mpu_config(TCB_t* task);

#ifdef UNIT_TEST
/** @return Read-only ready-list state for structural tests. */
const TaskList_t* task_ready_list_get(void);

/** @return Read-only wakeup-list state for structural tests. */
const TaskList_t* task_wakeup_list_get(void);
#endif

#endif /* TASK_H */
