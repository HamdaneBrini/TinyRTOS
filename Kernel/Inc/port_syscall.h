/**
 * @file port_syscall.h
 * @brief Cortex-M33 SVC invocation helpers for zero to two arguments.
 */

#ifndef PORT_SYSCALL_H
#define PORT_SYSCALL_H

#include <stdint.h>

/** @brief Invoke a void SVC with no register arguments. */
#define PORT_SYSCALL_VOID_0(svc_number)                                                                                \
    do {                                                                                                               \
        __asm__ volatile("svc %0" : : "I"(svc_number) : "memory");                                                     \
    } while (0)

/** @brief Invoke an SVC with no arguments and capture its R0 result. */
#define PORT_SYSCALL_RET_0(result, svc_number)                                                                         \
    do {                                                                                                               \
        register uintptr_t port_r0 __asm("r0") = (uintptr_t)(arg0);                                                    \
        __asm__ volatile("svc %1" : "=r"(port_r0) : "I"(svc_number) : "memory");                                       \
        (result) = port_r0;                                                                                            \
    } while (0)

/** @brief Invoke a void SVC with one argument in R0. */
#define PORT_SYSCALL_VOID_1(svc_number, arg0)                                                                          \
    do {                                                                                                               \
        register uintptr_t port_r0 __asm("r0") = (uintptr_t)(arg0);                                                    \
        __asm__ volatile("svc %1" : : "r"(port_r0), "I"(svc_number) : "memory");                                       \
    } while (0)

/** @brief Invoke an SVC with one R0 argument and capture its R0 result. */
#define PORT_SYSCALL_RET_1(result, svc_number, arg0)                                                                   \
    do {                                                                                                               \
        register uintptr_t port_r0 __asm("r0") = (uintptr_t)(arg0);                                                    \
        __asm__ volatile("svc %1" : "+r"(port_r0) : "I"(svc_number) : "memory");                                       \
        (result) = port_r0;                                                                                            \
    } while (0)

/** @brief Invoke a void SVC with arguments in R0 and R1. */
#define PORT_SYSCALL_VOID_2(svc_number, arg0, arg1)                                                                    \
    do {                                                                                                               \
        register uintptr_t port_r0 __asm("r0") = (uintptr_t)arg0;                                                      \
        register uintptr_t port_r1 __asm("r1") = (uintptr_t)arg1;                                                      \
        __asm__ volatile("svc %2" ::"r"(port_r0), "r"(port_r1), "I"(svc_number) : "memory");                           \
    } while (0)

/** @brief Invoke an SVC with R0/R1 arguments and capture its R0 result. */
#define PORT_SYSCALL_RET_2(result, svc_number, arg0, arg1)                                                             \
    do {                                                                                                               \
        register uintptr_t port_r0 __asm("r0") = (uintptr_t)(arg0);                                                    \
        register uintptr_t port_r1 __asm("r1") = (uintptr_t)(arg1);                                                    \
        __asm__ volatile("svc %2" : "+r"(port_r0) : "r"(port_r1), "I"(svc_number) : "memory");                         \
        (result) = port_r0;                                                                                            \
    } while (0)

#endif /* PORT_SYSCALL_H */
