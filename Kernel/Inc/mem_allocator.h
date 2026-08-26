/**
 * @file mem_allocator.h
 * @brief Generic best-fit allocator for caller-provided memory regions.
 */

#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include "alignment.h"
#include "config.h"

/** @brief Metadata stored immediately before every user-visible allocation. */
typedef struct MemBlock {
    uint32_t is_empty;     /* Non-zero when the block can be allocated. */
    size_t size;           /* Payload size; the header is not included. */
    struct MemBlock* next; /* Next physical block in the memory region. */
    struct MemBlock* prev; /* Previous physical block in the memory region. */
} MemBlock;

/** @brief State associated with one independently managed memory region. */
typedef struct Allocator_CB {
    MemBlock* head;  /**< First physical block in the memory region. */
    size_t mem_size; /**< Total size of the managed memory region in bytes. */
    int initialized; /**< Non-zero after successful initialization. */
    size_t block_header_size;
    size_t mem_aligment;
} Allocator_CB_t;

/**
 * @brief Initialize an allocator over a caller-provided memory region.
 *
 * The region is initialized as one free block. Its base address must satisfy
 * @ref MEM_ALIGNMENT and its size must be greater than @ref BLOCK_HEADER_SIZE.
 *
 * @param base_address Address of the first byte in the memory region.
 * @param mem_size Total size of the memory region in bytes.
 * @param mem_aligment Required payload alignment in bytes.
 * @param allocator_CB Allocator state to initialize.
 */
TinyStatus_t memory_allocator_init(uint32_t base_address, size_t mem_size, size_t mem_aligment,
                                   Allocator_CB_t* allocator_CB);

/**
 * @brief Allocate memory from a generic allocator.
 *
 * @param size Minimum number of payload bytes to allocate.
 * @param allocator_CB Initialized allocator used for the request.
 * @return Pointer to the allocated payload, or NULL if the request cannot be
 *         satisfied.
 */
void* memory_allocator_alloc(size_t size, Allocator_CB_t* allocator_CB);

/**
 * @brief Return an allocation to a generic allocator.
 *
 * Adjacent free blocks are coalesced when possible.
 *
 * @warning Except for NULL, @p ptr is not validated. It must be the exact
 * address returned by memory_allocator_alloc() for @p allocator_CB.
 *
 * @param ptr Pointer to the payload being released; NULL is accepted.
 * @param allocator_CB Allocator that owns the allocation.
 */
TinyStatus_t memory_allocator_free(void* ptr, Allocator_CB_t* allocator_CB);

#endif /* MEM_ALLOCATOR_H */
