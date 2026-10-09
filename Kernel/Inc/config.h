/**
 * @file config.h
 * @brief Common TinyRTOS status and context constants.
 */

#ifndef CONFIG_H
#define CONFIG_H

/** @brief TIM2 interrupt priority; lower values have higher Cortex-M priority. */
#define TIMER_IRQ_PRIORITY         4U
/** @brief Supervisor-call exception priority. */
#define SVC_EXCEPTION_PRIORITY     5U
/** @brief USART2 interrupt priority. */
#define UART_IRQ_PRIORITY          6U
/** @brief PendSV priority, kept lowest so context switching is deferred. */
#define PENDSV_EXCEPTION_PRIORITY 15U

#if TIMER_IRQ_PRIORITY >= SVC_EXCEPTION_PRIORITY || SVC_EXCEPTION_PRIORITY >= UART_IRQ_PRIORITY \
    || UART_IRQ_PRIORITY >= PENDSV_EXCEPTION_PRIORITY
#error "Interrupt priorities must satisfy TIMER > SVC > UART > PendSV"
#endif /* CONFIG_H */

/** @brief Status returned by TinyRTOS operations. */
typedef enum {
    TINY_OK = 1,
    TINY_FAIL = 0,

} TinyStatus_t;

#endif
