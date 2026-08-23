#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"

typedef uint32_t TaskHandle_t;
typedef void (*TaskFunc_t)(void* arg);

TinyStatus_t tiny_task_create(TaskHandle_t* task_handle, TaskFunc_t main_func, void* arg, uint32_t priority,
                              size_t stack_size);

#endif /* TASK_H */
