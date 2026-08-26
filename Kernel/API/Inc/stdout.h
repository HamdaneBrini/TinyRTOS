/**
 * @file stdout.h
 * @brief User-facing formatted console output API.
 */

#ifndef TINY_STDOUT_H
#define TINY_STDOUT_H

/**
 * @brief Write a formatted string to the privileged console service.
 *
 * Supported conversion specifiers are @c %d, @c %u, @c %s, and @c %c.
 *
 * @param format Null-terminated format string.
 * @param ... Values corresponding to the conversion specifiers.
 */
void tiny_print(const char* format, ...);

#endif /* TINY_STDOUT_H */
