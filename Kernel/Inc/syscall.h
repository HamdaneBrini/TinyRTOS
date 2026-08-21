#ifndef TINY_SYSCALL_H
#define TINY_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

#include "task.h"

/*

15             8 7              0
+----------------+----------------+
|      0xDF      |  SVC number    |
+----------------+----------------+

*/
#define SVC_INST_ENCOGING 0xDF00U
#define SVC_INST_ENCODING_MSK 0xFF00U
#define SVC_NUMBER_MSK 0x00FFU

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

typedef enum {
    SVC_TASK_CREATE = 0,
    SVC_TASK_DELAY,
    SVC_TASK_YIELD,
    SVC_TASK_EXIT,
    SVC_TASK_SUSPEND,
    SVC_TASK_RESUME,
    SVC_TASK_SET_PRIORITY,
} SVC_Number_t;

typedef struct {
    TCB_t **task_handle;
    TaskFunc_t main_func;
    void *arg;
    uint32_t priority;
    void *stack_base;
    size_t stack_size;
} TaskCreateArgs_t;

void SVC_Handler_Main(ExceptionFrame_t *frame);

#endif
