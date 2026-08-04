#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * Linker-defined symbols. `_end` marks the first byte available to the heap,
 * while the address of `_Min_Heap_Size` represents the configured heap size.
 */
extern uint32_t _end, _Min_Heap_Size;

/* Header stored immediately before every user-visible allocation. */
typedef struct MemBlock {
    uint32_t is_empty;     /* Non-zero when the block can be allocated. */
    size_t size;           /* Payload size; the header is not included. */
    struct MemBlock* next; /* Next physical block in the heap. */
} MemBlock;

/* Amount of heap space occupied by a block's metadata. */

#define HEAP_ALIGNMENT _Alignof(max_align_t)

_Static_assert(HEAP_ALIGNMENT <= 8, "Linker heap alignment is insufficient");

/* Keep every payload and block header suitably aligned for any C object. */
static inline size_t _align_up(size_t size) { return (size + HEAP_ALIGNMENT - 1U) & ~(HEAP_ALIGNMENT - 1U); }
#define BLOCK_HEADER_SIZE (_align_up(sizeof(MemBlock)))
/* Wrapper for the heap's linked list of blocks. */
typedef struct Heap {
    MemBlock* head;
} Heap;

/* Allocator state is private to this translation unit. */
static Heap heap;

/**
 * @brief Initialize the heap as one large free block.
 *
 * The linker script reserves the heap directly after `_end`. The initial
 * block consumes that whole region, less the metadata stored at its start.
 */
void tiny_heap_init(void) {
    heap.head = (MemBlock*)&_end;
    heap.head->is_empty = 1;
    heap.head->size = (size_t)&_Min_Heap_Size - BLOCK_HEADER_SIZE;
    heap.head->next = NULL;
}

/** @brief Get the address of the data part of the memory block
 *
 * @param block pointer to memory block
 * @return Pointer to the start of the data part
 */
static inline void* _get_data_ptr(MemBlock* block) { return (void*)((size_t)block + BLOCK_HEADER_SIZE); }

/** @brief Get the address of the end of the data part of the memory block
 *
 * @param block pointer to memory block
 * @return Pointer to the end of the data part
 */
static inline void* _get_end_block(MemBlock* block) { return (void*)((size_t)block + BLOCK_HEADER_SIZE + block->size); }

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
static inline int _split_block(MemBlock* block, size_t size) {
    if (block == NULL)
        return 0;
    if (size == 0)
        return 1;
    /* Equality would leave a header followed by a zero-byte free block. Since
     * all sizes are aligned, any positive remainder is also suitably aligned. */
    if (size + BLOCK_HEADER_SIZE >= block->size)
        return 0;

    /* The second block owns all bytes left after its new header. */
    size_t new_block_size = block->size - size - BLOCK_HEADER_SIZE;
    block->size = size;
    MemBlock* new_block = (MemBlock*)_get_end_block(block);
    new_block->is_empty = 1;
    new_block->size = new_block_size;
    new_block->next = block->next;
    block->next = new_block;
    return 1;
}

/**
 * @brief Allocate at least @p size bytes from the static heap.
 *
 * The list is searched for an exact match or the smallest free block that is
 * large enough (best fit). An oversized selected block is split when possible.
 *
 * @param size Number of payload bytes requested.
 * @return Pointer to the payload, or NULL when the request cannot be served.
 */
void* tiny_malloc(size_t size) {
    size_t al_size = _align_up(size);
    /* Reject empty requests and requests larger than the entire payload area. */
    if (al_size == 0 || al_size > (size_t)&_Min_Heap_Size - BLOCK_HEADER_SIZE)
        return NULL;

    MemBlock* current_block = heap.head;
    /* No block is a candidate until a sufficiently large free block is found. */
    MemBlock* best_block = NULL;
    size_t best_size = (size_t)&_Min_Heap_Size - BLOCK_HEADER_SIZE;
    while (current_block != NULL) {
        if (current_block->is_empty) {
            if (current_block->size == al_size) {
                /* An exact match needs neither further searching nor splitting. */
                current_block->is_empty = 0;
                return _get_data_ptr(current_block);
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
        _split_block(best_block, al_size);
        return _get_data_ptr(best_block);
    } else {
        return NULL;
    }
}
