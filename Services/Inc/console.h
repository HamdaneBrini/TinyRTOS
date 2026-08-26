/**
 * @file console.h
 * @brief Public interface for lightweight console output.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>
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
 * @brief Write a minimally formatted string using an existing argument list.
 *
 * Supported conversion specifiers are @c %d, @c %u, @c %s, and @c %c.
 * The caller owns @p args and is responsible for calling va_end().
 *
 * @param format Null-terminated format string.
 * @param args Arguments corresponding to the conversions in @p format.
 */
void vprint(const char* format, va_list args);

/**
 * @brief Write a minimally formatted string to the console.
 *
 * Supported conversion specifiers are @c %d, @c %u, @c %s, and @c %c.
 *
 * @param format Null-terminated format string.
 * @param ... Values corresponding to the conversion specifiers in @p format.
 */
void print(const char* format, ...);

#endif /* CONSOLE_H */
