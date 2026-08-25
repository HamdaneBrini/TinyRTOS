/**
 * @file task.c
 * @brief Task creation, state transitions, and ordered task-list management.
 */

#include <stddef.h>
#include <stdint.h>
#include "alignment.h"
#include "cmsis_gcc.h"
#include "config.h"
#include "kernel_task.h"
#include "mpu.h"
#include "port.h"
#include "port_exception.h"
#include "stm32h5xx.h"
#include "syscall.h"
#include "task.h"
#include "task_stack_allocator.h"

#define MAX_TASKS            16U
#define IDLE_TASK_STACK_SIZE 256U
#define IDLE_TASK_PRIORITY   0U

__attribute__((section(".kernel_bss"))) static TaskList_t taskReadyList;
__attribute__((section(".kernel_bss"))) static TaskList_t taskWakeupList;
__attribute__((section(".kernel_bss"))) static TCB_t* idle_task;
__attribute__((section(".kernel_bss"))) static TCB_t TCBT[MAX_TASKS];

/* Internal ready-list operations are public only to support structural tests. */
TinyStatus_t _insert_ready_task(TCB_t* task);
TinyStatus_t _insert_task_by_wakeup(TCB_t* task);
TinyStatus_t _remove_ready_task(TCB_t* task);
TinyStatus_t _remove_task_from_wakeup_list(TCB_t* task);
static TinyStatus_t _task_stack_init(TCB_t* task);
static void task_exit(void);

static inline TCB_t* task_lookup(TaskHandle_t task_handle) {
    if (task_handle >= MAX_TASKS)
        return NULL;
    return &TCBT[task_handle];
}

static int compare_priority(const TCB_t* a, const TCB_t* b) { return a->priority > b->priority; }

static int deadline_before(uint32_t a, uint32_t b) { return (int32_t)(a - b) < 0; }

static int compare_wakeup(const TCB_t* a, const TCB_t* b) { return deadline_before(a->wakeup_tick, b->wakeup_tick); }

static int task_has_timed_block_reason(const TCB_t* task) {
    return task->block_reason == BLOCK_DELAY || task->block_reason == BLOCK_EVENT_TIMEOUT;
}

static int task_is_in_wakeup_list(const TCB_t* task) {
    return taskWakeupList.head == task || task->wakeup_prev != NULL || task->wakeup_next != NULL;
}

static TCB_t** task_next_link(const TaskList_t* taskList, TCB_t* task) {
    return taskList->links == TASK_READY_LINKS ? &task->next : &task->wakeup_next;
}

static TCB_t** task_prev_link(const TaskList_t* taskList, TCB_t* task) {
    return taskList->links == TASK_READY_LINKS ? &task->prev : &task->wakeup_prev;
}

__attribute__((noreturn)) static void idle_func(void* arg) {
    (void)arg;
    while (1) {
        __WFI();
    }
}

/**
 * @brief Build the initial hardware and software context for a task.
 *
 * @param task Task whose stack is initialized.
 * @return TINY_OK on success, or TINY_FAIL for a NULL task.
 */
static TinyStatus_t _task_stack_init(TCB_t* task) {

    if (task == NULL)
        return TINY_FAIL;
    uint32_t* sp = (uint32_t*)(task->stack_region.base + (uint32_t)task->stack_region.size);
    task->sp = port_task_exception_frame_init(sp, (uint32_t)task->main_func, (uint32_t)task->arg, (uint32_t)task_exit);
    return TINY_OK;
}

TinyStatus_t task_system_init(void) {
    for (size_t i = 0; i < MAX_TASKS; i++) {
        TCBT[i] = (TCB_t){0};
    }
    stack_allocator_init();
    if (task_ready_list_init() != TINY_OK || task_wakeup_list_init() != TINY_OK)
        return TINY_FAIL;
    TaskHandle_t handle = 0;
    if (task_create(&handle, (TaskFunc_t)idle_func, NULL, IDLE_TASK_PRIORITY, IDLE_TASK_STACK_SIZE) != TINY_OK)
        return TINY_FAIL;
    idle_task = task_lookup(handle);
    return TINY_OK;
}

/**
 * @brief Allocate a TCB, initialize it, and make the task ready to run.
 *
 * @param task_handle Optional output pointer receiving the allocated TCB.
 * @param main_func Task entry point.
 * @param arg Argument supplied to the task entry point.
 * @param priority Scheduling priority; larger values have higher priority.
 * @param stack_size Stack capacity in bytes.
 * @return TINY_OK on success, or TINY_FAIL if allocation or insertion fails.
 */
TinyStatus_t task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                         size_t stack_size) {

    /* Reuse the first unallocated entry in the fixed-size TCB table. */
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (TCBT[i].status == UNUSED) {
            size_t aligned_stack_size = align_up(stack_size, TASK_STACK_ALIGNMENT);
            void* stack_base = stack_malloc(aligned_stack_size);
            if (stack_base == NULL) {
                return TINY_FAIL;
            }
            TCB_t* new_task = &TCBT[i];
            new_task->status = READY;
            new_task->block_reason = BLOCK_NONE;
            new_task->main_func = main_func;
            new_task->arg = arg;
            new_task->priority = priority;
            new_task->stack_region.base = (uint32_t)stack_base;
            new_task->stack_region.size = aligned_stack_size;
            new_task->stack_region.access_permission = MEMORY_READ_WRITE;
            new_task->next = NULL;
            new_task->prev = NULL;
            new_task->wakeup_next = NULL;
            new_task->wakeup_prev = NULL;
            new_task->wakeup_tick = 0;
            /* Initialize the task's stack*/
            _task_stack_init(new_task);
            if (task_handle != NULL) {
                *task_handle = i;
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
    taskReadyList.compare = compare_priority;
    taskReadyList.links = TASK_READY_LINKS;
    return TINY_OK;
}

/**
 * @brief Clear and configure the scheduler's wakeup list.
 *
 * @return TINY_OK after the list has been reset.
 */
TinyStatus_t task_wakeup_list_init(void) {
    taskWakeupList.head = NULL;
    taskWakeupList.lenght = 0;
    taskWakeupList.compare = compare_wakeup;
    taskWakeupList.links = TASK_WAKEUP_LINKS;
    return TINY_OK;
}

#ifdef UNIT_TEST
/**
 * @brief Provide read-only access to ready-list state for unit tests.
 *
 * @return Pointer to the scheduler's ready list.
 */
const TaskList_t* task_ready_list_get(void) { return &taskReadyList; }

const TaskList_t* task_wakeup_list_get(void) { return &taskWakeupList; }
#endif

/**
 * @brief Insert a task into a list using the list's comparison function.
 *
 * @param taskList Destination list.
 * @param task Task to insert.
 * @return TINY_OK on success, or TINY_FAIL for invalid arguments.
 */
static TinyStatus_t _insert_task(TaskList_t* taskList, TCB_t* task) {
    if (taskList == NULL || task == NULL || taskList->compare == NULL)
        return TINY_FAIL;

    TCB_t** task_next = task_next_link(taskList, task);
    TCB_t** task_prev = task_prev_link(taskList, task);
    TCB_t* cursor = taskList->head;
    /* The first task becomes the list head. */
    if (cursor == NULL) {
        taskList->head = task;
        *task_next = NULL;
        *task_prev = NULL;
        taskList->lenght++;
        return TINY_OK;
    }
    if (taskList->compare(task, cursor)) {
        taskList->head = task;
        *task_next = cursor;
        *task_prev = NULL;
        *task_prev_link(taskList, cursor) = task;
        taskList->lenght++;
        return TINY_OK;
    }

    TCB_t** current_next = task_next_link(taskList, cursor);
    while (*current_next != NULL && !taskList->compare(task, *current_next)) {
        cursor = *current_next;
        current_next = task_next_link(taskList, cursor);
    }

    TCB_t* next_task = *current_next;
    *task_next = next_task;
    *task_prev = cursor;
    if (next_task != NULL) {
        *task_prev_link(taskList, next_task) = task;
    }
    *current_next = task;
    taskList->lenght++;
    return TINY_OK;
}

TinyStatus_t _insert_ready_task(TCB_t* task) {
    if (task == NULL || task->status != READY) {
        return TINY_FAIL;
    }
    return _insert_task(&taskReadyList, task);
}

TinyStatus_t _insert_task_by_wakeup(TCB_t* task) {
    if (task == NULL || task->status != BLOCKED || !task_has_timed_block_reason(task) || task_is_in_wakeup_list(task)) {
        return TINY_FAIL;
    }
    return _insert_task(&taskWakeupList, task);
}

/**
 * @brief Remove a task from a list and repair neighboring links.
 *
 * @param taskList List containing the task.
 * @param task Task to remove.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or membership.
 */
static TinyStatus_t _remove_task(TaskList_t* taskList, TCB_t* task) {
    if (taskList == NULL || task == NULL || taskList->head == NULL)
        return TINY_FAIL;

    TCB_t* current = taskList->head;
    while (current != NULL && current != task) {
        current = *task_next_link(taskList, current);
    }
    if (current == NULL) {
        return TINY_FAIL;
    }

    TCB_t** task_prev = task_prev_link(taskList, task);
    TCB_t** task_next = task_next_link(taskList, task);
    TCB_t* previous = *task_prev;
    TCB_t* next = *task_next;
    if (previous != NULL) {
        *task_next_link(taskList, previous) = next;
    } else {
        taskList->head = next;
    }
    if (next != NULL) {
        *task_prev_link(taskList, next) = previous;
    }

    taskList->lenght--;
    *task_next = NULL;
    *task_prev = NULL;
    return TINY_OK;
}

TinyStatus_t _remove_ready_task(TCB_t* task) {
    if (task == NULL || task->status != READY) {
        return TINY_FAIL;
    }
    return _remove_task(&taskReadyList, task);
}

TinyStatus_t _remove_task_from_wakeup_list(TCB_t* task) {
    if (task == NULL || task->status != BLOCKED || !task_has_timed_block_reason(task)) {
        return TINY_FAIL;
    }
    return _remove_task(&taskWakeupList, task);
}

/**
 * @brief Get the next task selected by the priority scheduler.
 *
 * @return Ready-list head, or NULL when the ready list is empty.
 */
TCB_t* get_highest_priority_ready_task(void) { return taskReadyList.head; }

/** Return the task whose wake-up deadline comes first. */
TCB_t* get_earliest_wakeup_task(void) { return taskWakeupList.head; }

TCB_t* get_next_wakeup_task(TCB_t* task) { return task->wakeup_next; }

size_t task_ready_count(void) { return taskReadyList.lenght; }

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
    int removed_from_wakeup_list = 0;

    /* Removal must happen while the task still satisfies the BLOCKED invariant. */
    if (previous_status == BLOCKED && task_has_timed_block_reason(task)) {
        if (_remove_task_from_wakeup_list(task) != TINY_OK) {
            return TINY_FAIL;
        }
        removed_from_wakeup_list = 1;
    }

    task->status = READY;
    if (_insert_ready_task(task) != TINY_OK) {
        task->status = previous_status;
        if (removed_from_wakeup_list) {
            (void)_insert_task_by_wakeup(task);
        }
        return TINY_FAIL;
    }

    task->block_reason = BLOCK_NONE;
    return TINY_OK;
}

/**
 * @brief Move a running or ready task behind tasks of the same priority.
 *
 * @param task Task to place at the end of its priority group.
 * @return TINY_OK on success, or TINY_FAIL if the task cannot be queued.
 */
TinyStatus_t task_move_to_priority_tail(TCB_t* task) {
    if (task == NULL || (task->status != RUNNING && task->status != READY)) {
        return TINY_FAIL;
    }

    if (task->status == RUNNING) {
        return set_task_ready(task);
    }

    if (_remove_ready_task(task) != TINY_OK) {
        return TINY_FAIL;
    }

    return _insert_ready_task(task);
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
TinyStatus_t set_task_suspended(TCB_t* task) {
    if (task == NULL) {
        return TINY_FAIL;
    }
    if (task->status == SUSPENDED) {
        return TINY_OK;
    }

    /* A blocked task may also be waiting for a timeout. Remove that timeout
     * before changing its status because wakeup-list removal requires BLOCKED. */
    if (task->status == BLOCKED && task_has_timed_block_reason(task)
        && _remove_task_from_wakeup_list(task) != TINY_OK) {
        return TINY_FAIL;
    }
    TinyStatus_t result = set_task_not_ready(task, SUSPENDED);
    if (result == TINY_OK) {
        task->block_reason = BLOCK_NONE;
    }
    return result;
}

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

    if (task->status == READY) {

        int ordered_after_previous = task->prev == NULL || task->prev->priority >= priority;
        int ordered_before_next = task->next == NULL || priority >= task->next->priority;

        if (ordered_after_previous && ordered_before_next) {
            task->priority = priority;
            return TINY_OK;
        }
        if (_remove_ready_task(task) != TINY_OK) {
            return TINY_FAIL;
        }
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
    if (abs_tick == task->wakeup_tick) {
        return TINY_OK;
    }

    if (task->status != BLOCKED || !task_has_timed_block_reason(task) || !task_is_in_wakeup_list(task)) {
        task->wakeup_tick = abs_tick;
        return TINY_OK;
    }

    int ordered_after_previous =
        task->wakeup_prev == NULL || !deadline_before(abs_tick, task->wakeup_prev->wakeup_tick);
    int ordered_before_next = task->wakeup_next == NULL || !deadline_before(task->wakeup_next->wakeup_tick, abs_tick);

    if (ordered_after_previous && ordered_before_next) {
        task->wakeup_tick = abs_tick;
        return TINY_OK;
    }

    if (_remove_task_from_wakeup_list(task) != TINY_OK) {
        return TINY_FAIL;
    }
    task->wakeup_tick = abs_tick;
    if (_insert_task_by_wakeup(task) != TINY_OK) {
        return TINY_FAIL;
    }
    return TINY_OK;
}

TinyStatus_t task_stack_mpu_config(TCB_t* task) {
    uint32_t base_address = task->stack_region.base;
    uint32_t limit_address = base_address + (uint32_t)task->stack_region.size - 1U;

    return MPU_config(base_address, limit_address, task->stack_region.access_permission, 0, MEMORY_REGION_NUMEBER_1);
}

TinyStatus_t tiny_task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                              size_t stack_size) {

    TaskCreateArgs_t args = {task_handle, main_func, arg, priority, stack_size};
    register uintptr_t r0 __asm("r0") = (uintptr_t)&args;
    __asm__ volatile("svc %1" : "+r"(r0) : "I"(SVC_TASK_CREATE) : "memory");

    return (TinyStatus_t)r0;
}

/** @brief Terminate a task that returns from its entry function. */
__attribute__((noreturn)) static void task_exit(void) {
    __asm__ volatile("svc %0" : : "I"(SVC_TASK_EXIT) : "memory");
    while (1)
        ;
}
