/**
 * @file string_utils.h
 * @brief Public interface for string conversion utilities.
 */

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>

/**
 * @brief Convert a signed integer to decimal characters.
 *
 * The generated characters are not terminated by a null character.
 *
 * @param value Integer value to convert.
 * @param buffer Destination buffer for the generated characters.
 * @return Number of characters written to @p buffer.
 */
size_t int_to_string(int value, char* buffer);

#endif /* STRING_UTILS_H */
