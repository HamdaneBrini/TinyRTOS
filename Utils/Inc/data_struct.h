/**
 * @file data_struct.h
 * @brief Fixed-capacity ring-buffer types and operations.
 */

#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stddef.h>
#include <stdbool.h>

#include "config.h"

/** @brief One queued byte-buffer descriptor. */
typedef struct {
    void* buffer; /**< Address of the queued data; the data itself is not copied. */
    size_t length; /**< Number of valid bytes starting at buffer. */
    bool is_full; /**< True while this slot contains an unread descriptor. */
} Ring_node_t;

/** @brief State of a fixed-capacity FIFO of @ref Ring_node_t descriptors. */
typedef struct {
    volatile Ring_node_t* buffer; /**< Caller-owned descriptor array. */
    size_t max_length; /**< Number of descriptors in the array. */
    size_t read_index; /**< Index of the next descriptor to remove. */
    size_t write_index; /**< Index at which the next descriptor is inserted. */

#ifdef DEBUG
    size_t rejected_puts_count; /**< Number of insertions rejected because the FIFO was full. */
#endif
} Ring_buffer_t;

/**
 * @brief Initialize an empty ring buffer over caller-provided storage.
 * @param ring_buffer Ring-buffer state to initialize.
 * @param buffer Array of @ref Ring_node_t elements used as storage.
 * @param max_length Number of elements available in @p buffer.
 * @return TINY_OK on success, or TINY_FAIL for a NULL pointer or zero capacity.
 */
TinyStatus_t ring_buffer_init(volatile Ring_buffer_t* ring_buffer, volatile Ring_node_t* buffer, size_t max_length);

/**
 * @brief Append a descriptor to a ring buffer.
 * @param ring_buffer Initialized destination ring buffer.
 * @param value Descriptor to append; its pointed-to data is not copied.
 * @return TINY_OK on success, or TINY_FAIL when the next slot is still occupied.
 */
TinyStatus_t ring_buffer_put(volatile Ring_buffer_t* ring_buffer, Ring_node_t value);

/**
 * @brief Remove the oldest descriptor from a ring buffer.
 * @param ring_buffer Initialized source ring buffer.
 * @param output Destination receiving the removed descriptor.
 * @return TINY_OK on success, or TINY_FAIL when the ring buffer is empty.
 */
TinyStatus_t ring_buffer_get(volatile Ring_buffer_t* ring_buffer, Ring_node_t* output);

#endif /* DATA_STRUCT_H */
