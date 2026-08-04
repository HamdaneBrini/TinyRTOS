#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include <stddef.h>

/**
 * @brief Initialize the static heap allocator.
 *
 * This function must be called once before the first call to tiny_malloc().
 */
void tiny_heap_init(void);

/**
 * @brief Allocate memory from the static heap.
 *
 * @param size Minimum number of payload bytes to allocate.
 * @return Pointer to suitably aligned memory, or NULL if @p size is zero or
 *         the request cannot be satisfied.
 */
void* tiny_malloc(size_t size);

#endif

