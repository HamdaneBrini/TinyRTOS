/**
 * @file console.h
 * @brief Public interface for lightweight console output.
 */

#ifndef CONSOLE_H 
#define CONSOLE_H

#include <stddef.h>

/**
 * @brief Initialize the console communication interface.
 */
void console_init(void);

/**
 * @brief Write a buffer to the console.
 *
 * @param buffer Pointer to the character buffer to transmit.
 * @param len Number of bytes to transmit.
 */
void console_write(const char* buffer, size_t len);

/**
 * @brief Write a minimally formatted string to the console.
 *
 * Supported conversion specifiers are @c %d, @c %u, @c %s, and @c %c.
 *
 * @param format Null-terminated format string.
 * @param ... Values corresponding to the conversion specifiers in @p format.
 */
void tinyprint(const char* format, ...);

#endif /* CONSOLE_H */
