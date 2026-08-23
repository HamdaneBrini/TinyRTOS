/**
 * @file task_stack_allocator.h
 * @brief Public interface for allocating and releasing task stack memory.
 */

#ifndef TASK_STACK_ALLOCATOR_H
#define TASK_STACK_ALLOCATOR_H

#include <stddef.h>

/** @brief Alignment applied to task stack sizes and addresses. */
#define TASK_STACK_ALIGNMENT 32U

uint32_t get_stack_base_address(void) ;
uint32_t get_stack_end_address(void) ;
size_t get_Min_Stack_Size(void) ;
/**
 * @brief Initialize the task stack allocator.
 *
 * This function must be called before the first call to tiny_stack_malloc().
 */
void stack_allocator_init(void);

/**
 * @brief Allocate memory for a task stack.
 *
 * @param size Minimum number of bytes to allocate.
 * @return Pointer to the allocated memory, or NULL if the request cannot be
 *         satisfied.
 */
void* stack_malloc(size_t size);

/**
 * @brief Return task stack memory to the allocator.
 *
 * @param ptr Pointer previously returned by tiny_stack_malloc().
 */
void stack_free(void* ptr);

#endif /* TASK_STACK_ALLOCATOR_H */
