/**
 * @file mem_allocator.c
 * @brief Generic best-fit memory allocator implementation.
 */

#include <stddef.h>
#include <stdint.h>

#include "mem_allocator.h"

/**
 * @brief Initialize a memory region as one free block.
 *
 * @param base_address Address of the first byte in the memory region.
 * @param mem_size Total size of the memory region in bytes.
 * @param allocator_CB Allocator state to initialize.
 */
void memory_allocator_init(uint32_t base_address, size_t mem_size, size_t mem_aligment, Allocator_CB_t* allocator_CB) {
    allocator_CB->head = (MemBlock*)base_address;
    allocator_CB->head->is_empty = 1;
    allocator_CB->mem_aligment = mem_aligment;
    allocator_CB->block_header_size = align_up(sizeof(MemBlock), mem_aligment);
    allocator_CB->head->size = mem_size - allocator_CB->block_header_size;
    allocator_CB->head->next = NULL;
    allocator_CB->head->prev = NULL;
    allocator_CB->mem_size = mem_size;
    allocator_CB->initialized = 1;
}

/**
 * @brief Get the first byte of a block's payload.
 *
 * @param block Block whose payload address is required.
 * @return Pointer immediately after the aligned block header.
 */
static inline void* _get_data_ptr(MemBlock* block, Allocator_CB_t* allocator_CB) {
    return (void*)((size_t)block + allocator_CB->block_header_size);
}

/**
 * @brief Get the address immediately following a block's payload.
 *
 * This is also the location at which a remainder block's header is created
 * when the current block is split.
 *
 * @param block Block whose end address is required.
 * @return Pointer one byte past the block's payload.
 */
static inline void* _get_end_block(MemBlock* block, Allocator_CB_t* allocator_CB) {
    return (void*)((size_t)block + allocator_CB->block_header_size + block->size);
}

/**
 * @brief Split a block into an allocated-size prefix and a free remainder.
 *
 * The new remainder header is placed directly after the first block's
 * payload. Splitting is possible only when the original payload can hold the
 * requested payload plus another block header and a non-empty remainder.
 *
 * @param block Block to split.
 * @param size Payload size of the first block.
 * @return 1 on success, otherwise 0.
 */
static inline int _split_block(MemBlock* block, size_t size, Allocator_CB_t* allocator_CB) {
    if (block == NULL)
        return 0;
    if (size == 0)
        return 1;
    /* Equality would leave a header followed by a zero-byte free block. Since
     * all sizes are aligned, any positive remainder is also suitably aligned. */
    if (size + allocator_CB->block_header_size >= block->size)
        return 0;

    /* The second block owns all bytes left after its new header. */
    size_t new_block_size = block->size - size - allocator_CB->block_header_size;
    block->size = size;
    MemBlock* new_block = (MemBlock*)_get_end_block(block, allocator_CB);
    new_block->is_empty = 1;
    new_block->size = new_block_size;
    new_block->next = block->next;
    block->next = new_block;
    new_block->prev = block;
    return 1;
}

/**
 * @brief Allocate at least @p size bytes from the memory region.
 *
 * The list is searched for an exact match or the smallest free block that is
 * large enough (best fit). An oversized selected block is split when possible.
 *
 * @param size Number of payload bytes requested.
 * @param allocator_CB Allocator used for the allocation.
 * @return Pointer to the payload, or NULL when the request cannot be served.
 */
void* memory_allocator_alloc(size_t size, Allocator_CB_t* allocator_CB) {
    if (!allocator_CB->initialized)
        return NULL;
    size_t al_size = align_up(size, allocator_CB->mem_aligment);
    /* Reject empty requests and requests larger than the entire payload area. */
    if (al_size == 0 || al_size > allocator_CB->mem_size - allocator_CB->block_header_size)
        return NULL;

    MemBlock* current_block = allocator_CB->head;
    /* No block is a candidate until a sufficiently large free block is found. */
    MemBlock* best_block = NULL;
    size_t best_size = allocator_CB->mem_size - allocator_CB->block_header_size;
    while (current_block != NULL) {
        if (current_block->is_empty) {
            if (current_block->size == al_size) {
                /* An exact match needs neither further searching nor splitting. */
                current_block->is_empty = 0;
                return _get_data_ptr(current_block, allocator_CB);
            } else if (current_block->size > al_size) {
                /* Retain the smallest usable block seen so far. */
                if (current_block->size <= best_size) {
                    best_block = current_block;
                    best_size = current_block->size;
                }
            }
        }
        /* Move to the next block */
        current_block = current_block->next;
    }

    if (best_block != NULL) {
        /* `_split_block` leaves the block intact if no header fits after it. */
        best_block->is_empty = 0;
        best_block->size = best_size;
        _split_block(best_block, al_size, allocator_CB);
        return _get_data_ptr(best_block, allocator_CB);
    } else {
        return NULL;
    }
}

/**
 * @brief Merge a free block with its free physical neighbors.
 *
 * The following block is absorbed first. The resulting block is then
 * absorbed into the preceding block if that block is also free. Recovered
 * header bytes become part of the merged payload area.
 *
 * @param block Block from which coalescing starts; NULL is accepted.
 */
static void _mem_coalescing(MemBlock* block, Allocator_CB_t* allocator_CB) {
    if (block == NULL)
        return;
    if (block->is_empty) {
        MemBlock* next_block = block->next;
        MemBlock* prev_block = block->prev;
        if (next_block != NULL) {
            if (next_block->is_empty) {
                block->size += next_block->size + allocator_CB->block_header_size;
                block->next = next_block->next;
                if (block->next != NULL)
                    block->next->prev = block;
            }
        }
        if (prev_block != NULL) {
            if (prev_block->is_empty) {
                prev_block->size += block->size + allocator_CB->block_header_size;
                prev_block->next = block->next;
                if (prev_block->next != NULL)
                    prev_block->next->prev = prev_block;
            }
        }
    }
}

/**
 * @brief Mark an allocation as free and coalesce adjacent free blocks.
 *
 * The allocation header is recovered from its fixed position immediately
 * before the user-visible payload.
 *
 * @warning No pointer validation is performed except for NULL pointers. The caller must provide the
 * exact address of a live allocation returned by memory_allocator_alloc(); any other
 * value results in undefined behavior.
 *
 * @param ptr Pointer to the payload being released.
 * @param allocator_CB Allocator that owns the allocation.
 */
void memory_allocator_free(void* ptr, Allocator_CB_t* allocator_CB) {
    if (!allocator_CB->initialized)
        return;
    if (ptr == NULL)
        return;
    MemBlock* current_block = (MemBlock*)((size_t)ptr - allocator_CB->block_header_size);
    current_block->is_empty = 1;
    _mem_coalescing(current_block, allocator_CB);
}
