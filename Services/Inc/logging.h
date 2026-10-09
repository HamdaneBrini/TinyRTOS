/**
 * @file logging.h
 * @brief Level-based formatted logging interface.
 */

#ifndef LOGGING_H
#define LOGGING_H

/**
 * @brief Write an informational log message.
 *
 * @param format Null-terminated format string accepted by print().
 * @param ... Values corresponding to the conversions in @p format.
 */
void tiny_info(const char* format, ...);

/**
 * @brief Write a warning log message.
 *
 * @param format Null-terminated format string accepted by print().
 * @param ... Values corresponding to the conversions in @p format.
 */
void tiny_warning(const char* format, ...);

/**
 * @brief Write an error log message.
 *
 * @param format Null-terminated format string accepted by print().
 * @param ... Values corresponding to the conversions in @p format.
 */
void tiny_error(const char* format, ...);

#endif /* LOGGING_H */
