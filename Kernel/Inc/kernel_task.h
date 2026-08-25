#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "mpu.h"
#include "task.h"

typedef struct TCB_t TCB_t;
typedef int (*TCB_Compare_t)(const TCB_t* a, const TCB_t* b);

typedef enum { UNUSED = 0, BLOCKED, READY, RUNNING, SUSPENDED, TERMINATED } TaskStatus_t;

typedef enum { BLOCK_NONE, BLOCK_DELAY, BLOCK_EVENT, BLOCK_EVENT_TIMEOUT } TaskBlockReason_t;

typedef struct {
    uint32_t base;
    size_t size;
    MemoryAccess_t access_permission;
} TaskMemoryRegion_t;

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} ExceptionFrame_t;

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

typedef enum { TASK_READY_LINKS, TASK_WAKEUP_LINKS } TaskListLinks_t;

typedef struct {
    TCB_t* head;
    size_t lenght;
    TCB_Compare_t compare;
    TaskListLinks_t links;
} TaskList_t;

typedef struct {

    TaskHandle_t* task_handle;
    TaskFunc_t main_func;
    void* arg;
    uint32_t priority;
    size_t stack_size;

} TaskCreateArgs_t;

TinyStatus_t task_system_init(void);
TinyStatus_t task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                         size_t stack_size);
TinyStatus_t tiny_task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                              size_t stack_size);
TinyStatus_t task_ready_list_init(void);
TinyStatus_t task_wakeup_list_init(void);
TinyStatus_t _insert_ready_task(TCB_t* task);
TinyStatus_t _insert_task_by_wakeup(TCB_t* task);
TinyStatus_t _remove_ready_task(TCB_t* task);
TinyStatus_t _remove_task_from_wakeup_list(TCB_t* task);
TCB_t* get_highest_priority_ready_task(void);
TCB_t* get_earliest_wakeup_task(void);
TCB_t* get_next_wakeup_task(TCB_t* task);
size_t task_ready_count(void);
TinyStatus_t set_task_ready(TCB_t* task);
TinyStatus_t set_task_blocked(TCB_t* task);
TinyStatus_t set_task_running(TCB_t* task);
TinyStatus_t set_task_suspended(TCB_t* task);
TinyStatus_t set_task_terminated(TCB_t* task);
TinyStatus_t set_task_priority(TCB_t* task, uint32_t priority);
TinyStatus_t set_task_wakeup_tick(TCB_t* task, uint32_t abs_tick);
TinyStatus_t task_move_to_priority_tail(TCB_t* task);
TinyStatus_t task_stack_mpu_config(TCB_t* task);

#ifdef UNIT_TEST
const TaskList_t* task_ready_list_get(void);
const TaskList_t* task_wakeup_list_get(void);
#endif

#endif /* TASK_H */
