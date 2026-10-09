/**
 * @file heap_allocator.c
 * @brief Heap-specific wrapper around the generic memory allocator.
 */

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "heap_allocator.h"
#include "mem_allocator.h"
#include "port.h"

/** Linker-defined bounds for the user and kernel heap regions. */
extern uint8_t _suser_heap, _euser_heap;
extern uint8_t _skernel_heap, _ekernel_heap;

/* The linker script aligns the beginning of the heap to an 8-byte boundary. */
_Static_assert(MEM_ALIGNMENT <= 8, "Linker heap alignment is insufficient");

/* Keep allocator metadata in privileged kernel memory. */
__attribute__((section(".kernel_bss"))) Allocator_CB_t user_heap;
__attribute__((section(".kernel_bss"))) Allocator_CB_t kernel_heap;

/**
 * @brief Initialize the user and kernel heaps as independent free blocks.
 *
 * The linker script provides the bounds of both reserved regions. Each initial
 * block consumes its whole region, less the metadata stored at its start.
 */
TinyStatus_t kernel_heap_init(void) {
    size_t user_heap_size = (size_t)((uintptr_t)&_euser_heap - (uintptr_t)&_suser_heap);
    size_t kernel_heap_size = (size_t)((uintptr_t)&_ekernel_heap - (uintptr_t)&_skernel_heap);

    if (memory_allocator_init((uint32_t)&_suser_heap, user_heap_size, MEM_ALIGNMENT, &user_heap) != TINY_OK) {
        return TINY_FAIL;
    }

    return memory_allocator_init((uint32_t)&_skernel_heap, kernel_heap_size, MEM_ALIGNMENT, &kernel_heap);
}

/**
 * @brief Allocate memory from a selected heap.
 *
 * @param size Minimum number of payload bytes to allocate.
 * @param allocator Heap allocator that services the request.
 * @return Pointer to the allocated payload, or NULL if the request cannot be
 *         satisfied.
 */
void* kernel_malloc(size_t size, Allocator_CB_t* allocator) {
    /* Kernel allocations are shared by SVC handlers and peripheral ISRs (the
     * asynchronous UART releases transmitted buffers from its IRQ). Protect
     * allocator metadata against nested exception access. */
    CriticalState_t critical_state = critical_enter();
    void* allocation = memory_allocator_alloc(size, allocator);
    critical_exit(critical_state);
    return allocation;
}

/**
 * @brief Return an allocation to its heap.
 *
 * @param ptr Pointer previously returned by kernel_malloc().
 * @param allocator Heap allocator that owns @p ptr.
 */
TinyStatus_t kernel_free(void* ptr, Allocator_CB_t* allocator) {
    CriticalState_t critical_state = critical_enter();
    TinyStatus_t status = memory_allocator_free(ptr, allocator);
    critical_exit(critical_state);
    return status;
}
