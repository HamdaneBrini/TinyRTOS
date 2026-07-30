/**
 * @file string_utils.c
 * @brief String conversion utilities for the firmware.
 */

#include <stddef.h>
#include <stdint.h>

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
    char temp[12];
    size_t i = 0;
    size_t j = 0;

    if (value == 0) {
        buffer[0] = '0';
        return 1;
    }

    int negative = 0;

    if (value < 0) {
        negative = 1;
        value = -value;
    }

    while (value > 0) {
        temp[i++] = '0' + (value % 10); /* Convert the next decimal digit to a character. */
        value /= 10;
    }

    if (negative) {
        buffer[j++] = '-';
    }

    while (i > 0) {
        buffer[j++] = temp[--i];
    }

    return j;
}
