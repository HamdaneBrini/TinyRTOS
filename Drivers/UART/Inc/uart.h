/**
 * @file uart.h
 * @brief Public interface for interrupt-driven transmission and blocking reception over USART2.
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "config.h"

/**
 * @brief Initialize USART2 for console communication.
 * @return TINY_OK on success, or TINY_FAIL if HAL initialization fails.
 */
TinyStatus_t uart_init(void);

/**
 * @brief Transmit bytes through USART2.
 *
 * The descriptor is queued and transmission proceeds asynchronously. The data
 * is not copied, so @p pData must remain valid until the transmission-complete
 * callback runs. The current console backend transfers ownership of a buffer
 * allocated from @ref kernel_heap to this function.
 *
 * @param pData Pointer to the data to transmit.
 * @param size Number of bytes to transmit.
 */
void uart_write(const uint8_t* pData, uint16_t size);

/**
 * @brief Receive bytes through USART2.
 *
 * Reception is blocking until all requested bytes arrive or @p timeout expires.
 *
 * @param pData Pointer to the destination buffer.
 * @param size Number of bytes to receive.
 * @param timeout Maximum receive duration in milliseconds.
 */
void uart_read(uint8_t* pData, uint16_t size, uint32_t timeout);

#endif /* UART_H */
