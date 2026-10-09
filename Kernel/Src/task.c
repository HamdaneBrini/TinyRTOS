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
#include "kernel_timing.h"
#include "mpu.h"
#include "port.h"
#include "port_exception.h"
#include "port_syscall.h"
#include "scheduler.h"
#include "stm32h5xx.h"
#include "syscall.h"
#include "task.h"
#include "task_stack_allocator.h"
#include "timer.h"

#define MAX_TASKS            16U
#define IDLE_TASK_STACK_SIZE 256U
#define IDLE_TASK_PRIORITY   0U

__attribute__((section(".kernel_bss"))) static TaskList_t taskReadyList;
/** Timed tasks whose deadlines fall within the current TIM2 counter cycle. */
__attribute__((section(".kernel_bss"))) static TaskList_t active_wakeup_list;
/** Timed tasks whose deadlines fall after the current TIM2 counter cycle. */
__attribute__((section(".kernel_bss"))) static TaskList_t future_wakeup_list;
__attribute__((section(".kernel_bss"))) static TCB_t* idle_task;
__attribute__((section(".kernel_bss"))) static TCB_t TCBT[MAX_TASKS];

/* Internal ready-list operations are public only to support structural tests. */
TinyStatus_t _insert_ready_task(TCB_t* task);

TinyStatus_t _remove_ready_task(TCB_t* task);

static TinyStatus_t _task_stack_init(TCB_t* task);
static void task_exit(void);

TCB_t* task_lookup(TaskHandle_t task_handle) {
    if (task_handle >= MAX_TASKS)
        return NULL;
    return &TCBT[task_handle];
}

static int compare_priority(const TCB_t* a, const TCB_t* b) { return a->priority > b->priority; }

static int compare_wakeup(const TCB_t* a, const TCB_t* b) {
    return deadline_before(&a->wakeup_deadline, &b->wakeup_deadline);
}

static int task_has_timed_block_reason(const TCB_t* task) {
    return task->block_reason == BLOCK_DELAY || task->block_reason == BLOCK_EVENT_TIMEOUT;
}

static int task_is_in_active_wakeup_list(const TCB_t* task) {
    return active_wakeup_list.head == task || task->wakeup_prev != NULL || task->wakeup_next != NULL;
}

static int task_is_in_future_wakeup_list(const TCB_t* task) {
    return future_wakeup_list.head == task || task->future_wakeup_prev != NULL || task->future_wakeup_next != NULL;
}

static TCB_t** task_next_link(const TaskList_t* taskList, TCB_t* task) {
    switch (taskList->links) {
        case TASK_READY_LINKS: {
            return &task->next;
        }
        case TASK_WAKEUP_LINKS: {
            return &task->wakeup_next;
        }
        case TASK_FUTURE_WAKEUP_LINKS: {
            return &task->future_wakeup_next;
        }
        default: return NULL;
    }
}

static TCB_t** task_prev_link(const TaskList_t* taskList, TCB_t* task) {
    switch (taskList->links) {
        case TASK_READY_LINKS: {
            return &task->prev;
        }
        case TASK_WAKEUP_LINKS: {
            return &task->wakeup_prev;
        }
        case TASK_FUTURE_WAKEUP_LINKS: {
            return &task->future_wakeup_prev;
        }
        default: return NULL;
    }
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
    if (task_ready_list_init() != TINY_OK || active_wakeup_list_init() != TINY_OK
        || future_wakeup_list_init() != TINY_OK)
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
            new_task->status_before_suspend = READY;
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
            new_task->wakeup_deadline = (TimerDeadline_t){0};
            /* Build the task's initial software and exception frames. */
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
 * @brief Clear and configure the current-cycle wakeup list.
 *
 * @return TINY_OK after the list has been reset.
 */
TinyStatus_t active_wakeup_list_init(void) {
    active_wakeup_list.head = NULL;
    active_wakeup_list.lenght = 0;
    active_wakeup_list.compare = compare_wakeup;
    active_wakeup_list.links = TASK_WAKEUP_LINKS;
    return TINY_OK;
}

/**
 * @brief Clear and configure the future-cycle wakeup list.
 *
 * @return TINY_OK after the list has been reset.
 */
TinyStatus_t future_wakeup_list_init(void) {
    future_wakeup_list.head = NULL;
    future_wakeup_list.lenght = 0;
    future_wakeup_list.compare = compare_wakeup;
    future_wakeup_list.links = TASK_FUTURE_WAKEUP_LINKS;
    return TINY_OK;
}

#ifdef UNIT_TEST
/**
 * @brief Provide read-only access to ready-list state for unit tests.
 *
 * @return Pointer to the scheduler's ready list.
 */
const TaskList_t* task_ready_list_get(void) { return &taskReadyList; }

/** @return Pointer to the current-cycle wakeup list used by structural tests. */
const TaskList_t* task_wakeup_list_get(void) { return &active_wakeup_list; }
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

TinyStatus_t insert_task_by_wakeup(TCB_t* task) {
    if (task == NULL || task->status != BLOCKED || !task_has_timed_block_reason(task)
        || task_is_in_active_wakeup_list(task)) {
        return TINY_FAIL;
    }
    return _insert_task(&active_wakeup_list, task);
}

TinyStatus_t insert_task_in_future_list(TCB_t* task) {
    if (task == NULL || task->status != BLOCKED || !task_has_timed_block_reason(task)
        || task_is_in_future_wakeup_list(task)) {
        return TINY_FAIL;
    }
    return _insert_task(&future_wakeup_list, task);
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

TinyStatus_t remove_task_from_wakeup_list(TCB_t* task) {
    if (task == NULL || task->status != BLOCKED || !task_has_timed_block_reason(task)) {
        return TINY_FAIL;
    }
    return _remove_task(&active_wakeup_list, task);
}

TinyStatus_t remove_task_from_future_list(TCB_t* task) {
    if (task == NULL || task->status != BLOCKED || !task_has_timed_block_reason(task)) {
        return TINY_FAIL;
    }
    return _remove_task(&future_wakeup_list, task);
}

/**
 * @brief Get the next task selected by the priority scheduler.
 *
 * @return Ready-list head, or NULL when the ready list is empty.
 */
TCB_t* get_highest_priority_ready_task(void) { return taskReadyList.head; }

/** @return Task with the earliest deadline in the current timer cycle. */
TCB_t* get_earliest_wakeup_task(void) { return active_wakeup_list.head; }

/** @return Task with the earliest absolute deadline in a future timer cycle. */
TCB_t* get_earliest_future_task(void) { return future_wakeup_list.head; }

/** @return Task following @p task in the current-cycle wakeup list. */
TCB_t* get_next_wakeup_task(TCB_t* task) { return task->wakeup_next; }

/** @return Number of tasks waiting in the priority-ordered ready list. */
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
        if (remove_task_from_wakeup_list(task) != TINY_OK) {
            return TINY_FAIL;
        }
        removed_from_wakeup_list = 1;
    }

    task->status = READY;
    if (_insert_ready_task(task) != TINY_OK) {
        task->status = previous_status;
        if (removed_from_wakeup_list) {
            (void)insert_task_by_wakeup(task);
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
    if (task->status == TERMINATED || task->status == UNUSED) {
        return TINY_FAIL;
    }
    if (task->status == SUSPENDED) {
        return TINY_OK;
    }

    /* A blocked task may also be waiting for a timeout. Remove that timeout
     * before changing its status because wakeup-list removal requires BLOCKED. */
    if (task->status == BLOCKED && task_has_timed_block_reason(task)) {
        if (task_is_in_active_wakeup_list(task)) {
            if (remove_task_from_wakeup_list(task) != TINY_OK) {
                return TINY_FAIL;
            }
        } else if (task_is_in_future_wakeup_list(task)) {
            if (remove_task_from_future_list(task) != TINY_OK) {
                return TINY_FAIL;
            }
        }
    }

    task->status_before_suspend = task->status;
    TinyStatus_t result = set_task_not_ready(task, SUSPENDED);
    return result;
}

/**
 * @brief Restore a suspended task to its pre-suspension scheduling state.
 *
 * A timed blocked task is made ready if its deadline expired while suspended;
 * otherwise it is returned to the appropriate active or future wakeup list.
 *
 * @param task Suspended task to resume.
 * @return TINY_OK on success, or TINY_FAIL for an invalid state or list operation.
 */
TinyStatus_t resume_task(TCB_t* task) {
    if (task == NULL || task->status != SUSPENDED) {
        return TINY_FAIL;
    }

    switch (task->status_before_suspend) {
        case BLOCKED: {
            if (task_has_timed_block_reason(task)) {
                TimerDeadline_t now = {.cycle_index = cycle_counter, .offset = timer_now()};
                if (timer_deadline_reached(&task->wakeup_deadline, &now)) {
                    return set_task_ready(task);
                }

                task->status = BLOCKED;
                if (task->wakeup_deadline.cycle_index <= cycle_counter) {
                    if (insert_task_by_wakeup(task) != TINY_OK) {
                        task->status = SUSPENDED;
                        return TINY_FAIL;
                    }
                } else {
                    if (insert_task_in_future_list(task) != TINY_OK) {
                        task->status = SUSPENDED;
                        return TINY_FAIL;
                    }
                }

                TCB_t* earliest_task = get_earliest_wakeup_task();
                if (earliest_task != NULL) {
                    timer_set_deadline(earliest_task->wakeup_deadline.offset);
                }
            } else {
                task->status = BLOCKED;
            }

            return TINY_OK;
        }

        case READY:
        case RUNNING: return set_task_ready(task);

        default: {
            return TINY_FAIL;
        }
    }
}

/**
 * @brief Transition a task to TERMINATED.
 *
 * @param task Task whose execution has ended.
 * @return TINY_OK on success, or TINY_FAIL when the transition cannot be completed.
 */
TinyStatus_t set_task_terminated(TCB_t* task) { return set_task_not_ready(task, TERMINATED); }

/**
 * @brief Update a task priority and restore ready-list ordering when necessary.
 * @param task Task whose priority should change.
 * @param priority New scheduler priority.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or list failure.
 */
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

/**
 * @brief Store the absolute timer deadline at which a task should awaken.
 *
 * A queued task is removed and reinserted to preserve deadline ordering.
 *
 * @param task Task whose deadline should change.
 * @param cycle_index Absolute 32-bit timer-cycle index.
 * @param offset Counter offset within @p cycle_index.
 * @return TINY_OK on success, or TINY_FAIL for invalid input or list failure.
 */
TinyStatus_t set_task_wakeup_deadline(TCB_t* task, uint64_t cycle_index, uint32_t offset) {
    if (task == NULL) {
        return TINY_FAIL;
    }

    TimerDeadline_t new_deadline = {.cycle_index = cycle_index, .offset = offset};
    TimerDeadline_t old_deadline = task->wakeup_deadline;

    if (deadline_before(&new_deadline, &old_deadline) == 0 && deadline_before(&old_deadline, &new_deadline) == 0) {
        return TINY_OK;
    }

    int in_active_list = task_is_in_active_wakeup_list(task);
    int in_future_list = task_is_in_future_wakeup_list(task);

    if (!in_active_list && !in_future_list) {
        task->wakeup_deadline = new_deadline;
        return TINY_OK;
    }

    if (in_active_list && in_future_list) {
        return TINY_FAIL;
    }

    TinyStatus_t remove_status =
        in_active_list ? remove_task_from_wakeup_list(task) : remove_task_from_future_list(task);
    if (remove_status != TINY_OK) {
        return TINY_FAIL;
    }

    task->wakeup_deadline = new_deadline;
    TinyStatus_t insert_status = in_active_list ? insert_task_by_wakeup(task) : insert_task_in_future_list(task);
    if (insert_status != TINY_OK) {
        task->wakeup_deadline = old_deadline;
        (void)(in_active_list ? insert_task_by_wakeup(task) : insert_task_in_future_list(task));
        return TINY_FAIL;
    }

    return TINY_OK;
}

/**
 * @brief Map the selected task stack into the dynamic MPU region.
 * @param task Task whose stack should become accessible.
 * @return Status returned by the Cortex-M MPU port.
 */
TinyStatus_t task_stack_mpu_config(TCB_t* task) {
    uint32_t base_address = task->stack_region.base;
    uint32_t limit_address = base_address + (uint32_t)task->stack_region.size - 1U;

    return port_MPU_config(base_address, limit_address, task->stack_region.access_permission,MEMORY_TYPE_NORMAL, 0,
                           MEMORY_REGION_NUMEBER_1);
}

/** @brief Terminate a task that returns from its entry function. */
__attribute__((noreturn)) static void task_exit(void) {
    PORT_SYSCALL_VOID_0(SVC_TASK_EXIT);
    while (1)
        ;
}
