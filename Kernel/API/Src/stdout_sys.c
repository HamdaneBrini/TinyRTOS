/**
 * @file stdout_sys.c
 * @brief Formatted console output system-call wrapper.
 */

#include <stdarg.h>

#include "port_syscall.h"
#include "stdout.h"
#include "syscall.h"

/**
 * @brief Write a formatted string through the privileged console service.
 *
 * @param format Null-terminated format string.
 * @param ... Values corresponding to the conversion specifiers.
 */
void tiny_print(const char* format, ...) {
    va_list args;

    va_start(args, format);
    PORT_SYSCALL_VOID_2(SVC_CONSOLE_OUT, format, &args);
    va_end(args);
}
