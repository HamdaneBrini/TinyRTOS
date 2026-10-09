/**
 * @file heap_allocator.h
 * @brief Public interface for the linker-backed system heap allocator.
 */

#ifndef HEAP_ALLOCATOR_H
#define HEAP_ALLOCATOR_H

#include <stddef.h>
#include "config.h"
#include "mem_allocator.h"

/** @brief Alignment applied to every allocation payload. */
#define MEM_ALIGNMENT _Alignof(max_align_t)

/** @brief Allocator for memory returned to user code through system calls. */
extern Allocator_CB_t user_heap;

/** @brief Allocator reserved for internal kernel allocations. */
extern Allocator_CB_t kernel_heap;

/**
 * @brief Initialize the user and kernel heap allocators.
 *
 * This function must be called once before the first user or kernel heap
 * allocation.
 *
 * @return TINY_OK when both heaps are initialized, otherwise TINY_FAIL.
 */
TinyStatus_t kernel_heap_init(void);

/**
 * @brief Allocate memory from a selected heap.
 *
 * @param size Minimum number of payload bytes to allocate.
 * @param allocator Heap allocator that services the request.
 * @return Pointer to suitably aligned memory, or NULL if @p size is zero or
 *         the request cannot be satisfied.
 */
void* kernel_malloc(size_t size, Allocator_CB_t* allocator);

/**
 * @brief Return an allocation to its heap.
 *
 * The released block is merged with adjacent free blocks when possible.
 *
 * @warning This function does not validate @p ptr except for NULL pointers. The caller must pass the
 * exact address of a live allocation returned by kernel_malloc() using the
 * same allocator. Passing
 * a foreign or interior address, or an address that has already been freed
 * results in undefined behavior.
 *
 * @param ptr Pointer previously returned by kernel_malloc().
 * @param allocator Heap allocator that owns @p ptr.
 * @return TINY_OK on success, or TINY_FAIL for an unavailable allocator or NULL pointer.
 */
TinyStatus_t kernel_free(void* ptr, Allocator_CB_t* allocator);

#endif /* HEAP_ALLOCATOR_H */
