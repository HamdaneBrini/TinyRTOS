/**
 * @file task_stack_allocator.c
 * @brief Task-stack-specific wrapper around the generic memory allocator.
 */

#include <stddef.h>
#include <stdint.h>
#include "task_stack_allocator.h"
#include "mem_allocator.h"

/*
 * Linker-defined symbols. `_suser_stack` marks the first byte available for
 * task stacks, while the address of `_Min_Stack_Size` represents the size of
 * the reserved stack memory.
 */
extern uint32_t _suser_stack, _Min_Stack_Size;

/* The linker script aligns the beginning of the global stack reserved memory to an 8-byte boundary. */
_Static_assert(MEM_ALIGNMENT <= 8, "Linker stack alignment is insufficient");

/* Allocator state is private to this translation unit. */
static Allocator_CB_t task_stack;

/**
 * @brief Initialize the task stack allocator as one large free block.
 *
 * The initial block consumes the stack region reserved by the linker script,
 * less the metadata stored at its start.
 */
void tiny_stack_init(void) {
    memory_allocator_init((uint32_t)&_suser_stack, (size_t)&_Min_Stack_Size, &task_stack);
}

/**
 * @brief Allocate memory for a task stack.
 *
 * @param size Minimum number of stack bytes to allocate.
 * @return Pointer to the allocated stack memory, or NULL if the request
 *         cannot be satisfied.
 */
void* tiny_stack_malloc(size_t size) {
    return memory_allocator_alloc(size, &task_stack);
}

/**
 * @brief Return task stack memory to the allocator.
 *
 * @param ptr Pointer previously returned by tiny_stack_malloc(); NULL is
 *            accepted.
 */
void tiny_stack_free(void* ptr) {
    memory_allocator_free(ptr, &task_stack);
}
