#ifndef TINY_SYSCALL_H
#define TINY_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

#include "kernel_task.h"
#include "task.h"

/*

15             8 7              0
+----------------+----------------+
|      0xDF      |  SVC number    |
+----------------+----------------+

*/
#define SVC_INST_ENCOGING     0xDF00U
#define SVC_INST_ENCODING_MSK 0xFF00U
#define SVC_NUMBER_MSK        0x00FFU

typedef enum {
    SVC_TASK_CREATE = 0,
    SVC_SCHEDULER_START,
    SVC_TASK_DELAY,
    SVC_TASK_YIELD,
    SVC_TASK_EXIT,
    SVC_TASK_SUSPEND,
    SVC_TASK_RESUME,
    SVC_TASK_SET_PRIORITY,
} SVC_Number_t;

/**
 * @brief Decode and dispatch an SVC from its saved exception frame.
 *
 * @param frame Hardware-saved caller context.
 */
void SVC_Handler_Main(ExceptionFrame_t* frame);

#endif
/**
 * @file syscall.h
 * @brief System-call numbers and SVC dispatch interface.
 */
