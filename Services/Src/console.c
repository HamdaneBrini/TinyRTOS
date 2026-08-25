/**
 * @file console.c
 * @brief Lightweight formatted console output over UART.
 *
 * This module provides basic console initialization and output functions,
 * together with a small printf-like formatter intended for embedded systems.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "console.h"
#include "string_utils.h"
#include "uart.h"

/**
 * @brief Initialize the console communication interface.
 *
 * Initializes the UART peripheral used by the console.
 */
void console_init(void) { uart_init(); }

/**
 * @brief Write a buffer to the console.
 *
 * @param buffer Pointer to the character buffer to transmit.
 * @param len Number of bytes to transmit.
 */
void console_write(const char* buffer, size_t len) { uart_write((const uint8_t*)buffer, (uint16_t)len); }

/**
 * @brief Write a minimally formatted string using an existing argument list.
 *
 * This is the shared formatting engine used by tinyprint() and the logging
 * service. The caller retains ownership of @p args.
 *
 * @param format Null-terminated format string.
 * @param args Arguments corresponding to the conversions in @p format.
 */
void tiny_vprint(const char* format, va_list args) {
    char tmp_int[12];
    char tmp_uint[12];

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd': {
                    int value = va_arg(args, int);
                    size_t len_int = int_to_string(value, tmp_int);
                    console_write(tmp_int, len_int);
                    break;
                }

                case 'u': {
                    unsigned int value = va_arg(args, unsigned int);
                    size_t len_uint = int_to_string(value, tmp_uint);
                    console_write(tmp_uint, len_uint);
                    break;
                }

                case 's': {
                    char* value = va_arg(args, char*);
                    console_write(value, strlen(value));
                    break;
                }

                case 'c': {
                    int value = va_arg(args, int);
                    console_write((char*)&value, 1);
                    break;
                }
            }
        } else {
            console_write(format, 1);
        }
        format++;
    }
}

/**
 * @brief Write a minimally formatted string to the console.
 *
 * Supported conversion specifiers are:
 * - @c %d for signed decimal integers.
 * - @c %u for unsigned decimal integers.
 * - @c %s for null-terminated strings.
 * - @c %c for characters.
 *
 * @param format Null-terminated format string.
 * @param ... Values corresponding to the conversion specifiers in @p format.
 */
void tinyprint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    tiny_vprint(format, args);
    va_end(args);
}
