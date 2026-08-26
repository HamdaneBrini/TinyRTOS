/**
 * @file memory.h
 * @brief Public dynamic-memory allocation interface.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

#include "config.h"

/**
 * @brief Allocate memory from the kernel heap.
 * @param size Number of bytes requested.
 * @return Pointer to the allocated memory, or NULL if allocation fails.
 */
void* tiny_malloc(size_t size);

/**
 * @brief Return a previously allocated block to the kernel heap.
 * @param ptr Pointer returned by tiny_malloc().
 * @return Status returned by the memory-free system call.
 */
TinyStatus_t tiny_free(void* ptr);

#endif /* MEMORY_H */
