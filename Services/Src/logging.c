/**
 * @file logging.c
 * @brief Level-based formatted logging over the system console.
 */

#include <stdarg.h>

#include "console.h"
#include "logging.h"

/**
 * @brief Write a level prefix followed by a formatted message.
 *
 * The caller owns @p args and remains responsible for calling va_end().
 *
 * @param level Null-terminated level label.
 * @param format Null-terminated format string accepted by vprint().
 * @param args Arguments corresponding to the conversions in @p format.
 */
static void _tiny_vlog(const char* level, const char* format, va_list args) {
    print("[%s]: ", level);
    vprint(format, args);
    print("\n");
}

/** @brief Write an informational formatted log message. */
void tiny_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    _tiny_vlog("INFO", format, args);
    va_end(args);
}

/** @brief Write a warning formatted log message. */
void tiny_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    _tiny_vlog("WARNING", format, args);
    va_end(args);
}

/** @brief Write an error formatted log message. */
void tiny_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    _tiny_vlog("ERROR", format, args);
    va_end(args);
}
