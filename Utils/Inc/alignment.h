/**
 * @file alignment.h
 * @brief Common helpers for aligning sizes and addresses.
 */

#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <stddef.h>

/**
 * @brief Round a value up to an alignment boundary.
 *
 * @param value Value to align.
 * @param alignment Required power-of-two alignment.
 * @return The smallest aligned value greater than or equal to @p value.
 */
static inline size_t align_up(size_t value, size_t alignment) {
    return (value + alignment - 1U) & ~(alignment - 1U);
}

#endif /* ALIGNMENT_H */
