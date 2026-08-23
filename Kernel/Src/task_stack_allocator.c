/**
 * @file task_stack_allocator.c
 * @brief Task-stack-specific wrapper around the generic memory allocator.
 */

#include <stddef.h>
#include <stdint.h>
#include "mem_allocator.h"
#include "task_stack_allocator.h"

/*
 * Linker-defined symbols. `_suser_stack` marks the first byte available for
 * task stacks, while the address of `_Min_Stack_Size` represents the size of
 * the reserved stack memory.
 */
extern uint32_t _suser_stack, _Min_Stack_Size, _euser_stack;

/* The linker script aligns the beginning of the global stack reserved memory to an 32-byte boundary. */
_Static_assert(TASK_STACK_ALIGNMENT == 32U, "Task payloads must be 32-byte aligned");

/* Allocator state is private to this translation unit. */
static Allocator_CB_t task_stack;

uint32_t get_stack_base_address(void) { return (uint32_t)&_suser_stack; }

uint32_t get_stack_end_address(void) { return (uint32_t)&_euser_stack; }

size_t get_Min_Stack_Size(void) { return (size_t)&_Min_Stack_Size; }

/**
 * @brief Initialize the task stack allocator as one large free block.
 *
 * The initial block consumes the stack region reserved by the linker script,
 * less the metadata stored at its start.
 */
void stack_allocator_init(void) {
    memory_allocator_init((uint32_t)&_suser_stack, (size_t)&_Min_Stack_Size, TASK_STACK_ALIGNMENT, &task_stack);
}

/**
 * @brief Allocate memory for a task stack.
 *
 * @param size Minimum number of stack bytes to allocate.
 * @return Pointer to the allocated stack memory, or NULL if the request
 *         cannot be satisfied.
 */
void* stack_malloc(size_t size) { return memory_allocator_alloc(size, &task_stack); }

/**
 * @brief Return task stack memory to the allocator.
 *
 * @param ptr Pointer previously returned by tiny_stack_malloc(); NULL is
 *            accepted.
 */
void stack_free(void* ptr) { memory_allocator_free(ptr, &task_stack); }
