#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

/* Header stored immediately before every user-visible allocation. */
typedef struct MemBlock {
    uint32_t is_empty;     /* Non-zero when the block can be allocated. */
    size_t size;           /* Payload size; the header is not included. */
    struct MemBlock* next; /* Next physical block in the memory region. */
    struct MemBlock* prev; /* Previous physical block in the memory region. */
} MemBlock;

/* Wrapper for the allocator's linked list of blocks. */
typedef struct Allocator_CB {
    MemBlock* head;
    size_t mem_size;
    int initialized;
} Allocator_CB_t;

/* Payloads must satisfy the strictest alignment required by a C object. */
#define MEM_ALIGNMENT _Alignof(max_align_t)

/* Round a byte count up to the next payload-alignment boundary. */
static inline size_t _align_up(size_t size) { return (size + MEM_ALIGNMENT - 1U) & ~(MEM_ALIGNMENT - 1U); }

/* Metadata size including any padding required before the payload. */
#define BLOCK_HEADER_SIZE (_align_up(sizeof(MemBlock)))

void memory_allocator_init(uint32_t base_address, size_t mem_size, Allocator_CB_t* allocator_CB);
void* memory_allocator_alloc(size_t size, Allocator_CB_t* allocator_CB);
void memory_allocator_free(void* ptr, Allocator_CB_t* allocator_CB);

#endif /* MEM_ALLOCATOR_H */
