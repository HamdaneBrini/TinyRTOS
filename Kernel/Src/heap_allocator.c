/**
 * @file heap_allocator.c
 * @brief Heap-specific wrapper around the generic memory allocator.
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include "heap_allocator.h"
#include "mem_allocator.h"

/*
 * Linker-defined symbols. `_end` marks the first byte available to the heap,
 * while the address of `_Min_Heap_Size` represents the configured heap size.
 */
extern uint32_t _end, _Min_Heap_Size;

/* The linker script aligns the beginning of the heap to an 8-byte boundary. */
_Static_assert(MEM_ALIGNMENT <= 8, "Linker heap alignment is insufficient");

/* Allocator state is private to this translation unit. */
static Allocator_CB_t heap;

/**
 * @brief Initialize the heap as one large free block.
 *
 * The linker script reserves the heap directly after `_end`. The initial
 * block consumes that whole region, less the metadata stored at its start.
 */
void tiny_heap_init(void) {
    memory_allocator_init((uint32_t)&_end, (size_t)&_Min_Heap_Size, &heap);
}

/**
 * @brief Allocate memory from the system heap.
 *
 * @param size Minimum number of payload bytes to allocate.
 * @return Pointer to the allocated payload, or NULL if the request cannot be
 *         satisfied.
 */
void* tiny_malloc(size_t size) {
    return memory_allocator_alloc(size, &heap);
}

/**
 * @brief Return an allocation to the system heap.
 *
 * @param ptr Pointer previously returned by tiny_malloc(); NULL is accepted.
 */
void tiny_free(void* ptr) {
    memory_allocator_free(ptr, &heap);
}
