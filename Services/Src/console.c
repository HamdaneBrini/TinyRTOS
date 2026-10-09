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
#include <sys/_types.h>

#include "console.h"
#include "heap_allocator.h"
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

/** Return the number of bytes produced by the supported format specifiers. */
static size_t _formatted_length(const char* format, va_list args) {
    size_t buffer_length = 0;
    while (*format != '\0') {
        if (*format != '%') {
            buffer_length++;
            format++;
            continue;
        }
        format++;
        if (*format == '\0')
            break;
        switch (*format) {
            case 'd': {
                int value = va_arg(args, int);
                buffer_length = buffer_length + int_to_string_len(value);
                break;
            }
            case 'u': {
                buffer_length = buffer_length + uint_to_string_len(va_arg(args, unsigned int));
                break;
            }
            case 's': {
                buffer_length = buffer_length + strlen(va_arg(args, char*));
                break;
            }
            case 'c': {
                (void)va_arg(args, int);
                buffer_length++;
                break;
            }
            default: buffer_length++;
        }
        format++;
    }
    return buffer_length;
}

/**
 * @brief Write a minimally formatted string using an existing argument list.
 *
 * This is the shared formatting engine used by print() and the logging
 * service. The caller retains ownership of @p args.
 *
 * @param format Null-terminated format string.
 * @param args Arguments corresponding to the conversions in @p format.
 */
void vprint(const char* format, va_list args) {
    va_list length_args;
    va_copy(length_args, args);
    size_t buffer_length = _formatted_length(format, length_args);
    va_end(length_args);

    char* buffer = kernel_malloc(buffer_length, &kernel_heap);
    if (buffer == NULL) {
        /* The UART backend keeps each formatted buffer until the asynchronous
         * transfer completes. A burst of log messages can therefore exhaust
         * the kernel heap; never format through a NULL allocation. */
        return;
    }
    size_t buffer_index = 0;

    while (*format != '\0') {
        if (*format != '%') {
            buffer[buffer_index++] = *format;
            format++;
            continue;
        }
        format++;
        if (*format == '\0')
            break;
        switch (*format) {
            case 'd': {
                buffer_index = buffer_index + int_to_string(va_arg(args, int), &buffer[buffer_index]);
                break;
            }
            case 'u': {
                buffer_index = buffer_index + uint_to_string(va_arg(args, unsigned int), &buffer[buffer_index]);
                break;
            }
            case 's': {
                char* value = va_arg(args, char*);
                size_t value_length = strlen(value);
                memcpy(&buffer[buffer_index], value, value_length);
                buffer_index = buffer_index + value_length;
                break;
            }
            case 'c': {
                char value = (char)va_arg(args, int);
                buffer[buffer_index++] = value;

                break;
            }
            default: {
                buffer[buffer_index++] = *format;

                break;
            }
        }
        format++;
    }
    console_write(buffer, buffer_length);
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
void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprint(format, args);
    va_end(args);
}
