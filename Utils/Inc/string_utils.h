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

/**
 * @brief Return the number of decimal characters needed for a signed integer.
 *
 * The returned length includes the minus sign for negative values and does
 * not include space for a null terminator.
 *
 * @param value Integer whose formatted length is required.
 * @return Number of characters produced by int_to_string().
 */
size_t int_to_string_len(int value);

/**
 * @brief Convert an unsigned integer to decimal characters.
 *
 * The generated characters are not terminated by a null character.
 *
 * @param value Unsigned integer to convert.
 * @param buffer Destination buffer for the generated characters.
 * @return Number of characters written to @p buffer.
 */
size_t uint_to_string(unsigned int value, char* buffer);

/**
 * @brief Return the number of decimal characters needed for an unsigned integer.
 *
 * The returned length does not include space for a null terminator.
 *
 * @param value Unsigned integer whose formatted length is required.
 * @return Number of characters produced by uint_to_string().
 */
size_t uint_to_string_len(unsigned int value);

#endif /* STRING_UTILS_H */
