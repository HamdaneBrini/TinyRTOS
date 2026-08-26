/**
 * @file memory_sys.c
 * @brief Dynamic-memory system-call wrappers.
 */

#include <stdint.h>

#include "config.h"
#include "memory.h"
#include "port_syscall.h"
#include "syscall.h"

/** @copydoc tiny_malloc */
void* tiny_malloc(size_t size) {
    uintptr_t result;
    PORT_SYSCALL_RET_1(result, SVC_MEMORY_ALLOC, size);
    return (void*)result;
}

/** @copydoc tiny_free */
TinyStatus_t tiny_free(void* ptr) {
    TinyStatus_t status;
    PORT_SYSCALL_RET_1(status, SVC_MEMORY_FREE, ptr);
    return (TinyStatus_t)status;
}
