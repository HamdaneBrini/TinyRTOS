/**
 * @file ring_buffer.c
 * @brief Fixed-capacity ring-buffer implementation.
 */

#include "data_struct.h"
#include "config.h"

/** @copydoc ring_buffer_init */
TinyStatus_t ring_buffer_init(volatile Ring_buffer_t* ring_buffer, volatile Ring_node_t* buffer, size_t max_length) {
    if (ring_buffer == NULL || buffer == NULL || max_length == 0)
        return TINY_FAIL;

    ring_buffer->buffer = buffer;
    ring_buffer->max_length = max_length;
    ring_buffer->read_index = 0;
    ring_buffer->write_index = 0;
#ifdef DEBUG
    ring_buffer->rejected_puts_count = 0;
#endif
    return TINY_OK;
}

/** @copydoc ring_buffer_put */
TinyStatus_t ring_buffer_put(volatile Ring_buffer_t* ring_buffer, Ring_node_t value) {

    if (!ring_buffer->buffer[ring_buffer->write_index].is_full) {
        ring_buffer->buffer[ring_buffer->write_index] = value;
        ring_buffer->buffer[ring_buffer->write_index].is_full=1;
        ring_buffer->write_index = (ring_buffer->write_index + 1) % ring_buffer->max_length;
    } else {
#ifdef DEBUG
        ring_buffer->rejected_puts_count++;
#endif
        return TINY_FAIL;
    }
    return TINY_OK;
}

/** @copydoc ring_buffer_get */
TinyStatus_t ring_buffer_get(volatile Ring_buffer_t* ring_buffer, Ring_node_t* output) {
    Ring_node_t node = ring_buffer->buffer[ring_buffer->read_index];
    if (!node.is_full) return TINY_FAIL;
    *output = node;
    ring_buffer->buffer[ring_buffer->read_index] = (Ring_node_t){0};
    ring_buffer->read_index = (ring_buffer->read_index + 1 )% ring_buffer->max_length;
    return TINY_OK;
}
    
