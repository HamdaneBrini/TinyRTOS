/**
 * @file task_sys.c
 * @brief Task-management system-call wrappers.
 */

#include <stdint.h>

#include "config.h"
#include "port_syscall.h"
#include "syscall.h"
#include "task.h"

/** @copydoc tiny_task_create */
TinyStatus_t tiny_task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                              size_t stack_size) {
    TinyStatus_t status = TINY_FAIL;
    TaskCreateArgs_t args = {task_handle, main_func, arg, priority, stack_size};
    PORT_SYSCALL_RET_1(status, SVC_TASK_CREATE, &args);
    return (TinyStatus_t)status;
}

/** @copydoc tiny_task_suspend */
TinyStatus_t tiny_task_suspend(TaskHandle_t* task_handle) {
    TinyStatus_t status = TINY_FAIL;
    PORT_SYSCALL_RET_1(status, SVC_TASK_SUSPEND, (uint32_t)task_handle);
    return (TinyStatus_t)status;
}

/** @copydoc tiny_task_resume */
TinyStatus_t tiny_task_resume(TaskHandle_t* task_handle) {
    TinyStatus_t status = TINY_FAIL;
    PORT_SYSCALL_RET_1(status, SVC_TASK_RESUME, (uint32_t)task_handle);
    return (TinyStatus_t)status;
}
