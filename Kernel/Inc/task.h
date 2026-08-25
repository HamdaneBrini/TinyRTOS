/**
 * @file task.h
 * @brief Public task creation interface.
 */

#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"

typedef uint32_t TaskHandle_t;

/** @brief Task entry-point signature. */
typedef void (*TaskFunc_t)(void* arg);

/**
 * @brief Create a task through the system-call interface.
 *
 * @param task_handle Optional destination for the created task handle.
 * @param main_func Task entry point.
 * @param arg Argument passed to @p main_func.
 * @param priority Task priority; larger values have higher priority.
 * @param stack_size Requested task stack size in bytes.
 * @return Status returned by the task-create system call.
 */
TinyStatus_t tiny_task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                              size_t stack_size);

#endif /* TASK_H */
