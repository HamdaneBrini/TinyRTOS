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

/**
 * @brief Return an allocation to the static heap.
 *
 * The released block is merged with adjacent free blocks when possible.
 *
 * @warning This function does not validate @p ptr except for NULL pointers. The caller must pass the
 * exact address of a live allocation returned by tiny_malloc(). Passing
 * a foreign or interior address, or an address that has already been freed
 * results in undefined behavior.
 *
 * @param ptr Pointer previously returned by tiny_malloc().
 */
void tiny_free(void* ptr);

#endif /* HEAP_ALLOCATOR_H */

