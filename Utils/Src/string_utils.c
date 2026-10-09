/**
 * @file string_utils.c
 * @brief String conversion utilities for the firmware.
 */

#include <stddef.h>

#include "string_utils.h"

/** Return the magnitude of a signed integer without overflowing for INT_MIN. */
static unsigned int _int_magnitude(int value) { return value < 0 ? 0U - (unsigned int)value : (unsigned int)value; }

/** @copydoc uint_to_string_len */
size_t uint_to_string_len(unsigned int value) {
    size_t length = 1U;

    while (value >= 10U) {
        value /= 10U;
        length++;
    }

    return length;
}

/** @copydoc uint_to_string */
size_t uint_to_string(unsigned int value, char* buffer) {
    size_t length = uint_to_string_len(value);

    for (size_t index = length; index > 0U; index--) {
        buffer[index - 1U] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    return length;
}

/**
 * @brief Convert a signed integer to its decimal character representation.
 *
 * The generated characters are written to @p buffer without a terminating
 * null character.
 *
 * @param value Integer value to convert.
 * @param buffer Destination buffer for the generated characters. It must have
 *               enough capacity for the result.
 * @return Number of characters written to @p buffer.
 */
size_t int_to_string(int value, char* buffer) {
    size_t offset = 0U;
    if (value < 0) {
        buffer[offset++] = '-';
    }

    return offset + uint_to_string(_int_magnitude(value), &buffer[offset]);
}

/** @copydoc int_to_string_len */
size_t int_to_string_len(int value) {
    size_t sign_length = value < 0 ? 1U : 0U;
    return sign_length + uint_to_string_len(_int_magnitude(value));
}
