/**
 * @file uart.h
 * @brief Public interface for blocking UART communication.
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/**
 * @brief Initialize USART2 for console communication.
 */
void uart_init(void);

/**
 * @brief Transmit bytes through USART2.
 *
 * @param pData Pointer to the data to transmit.
 * @param Size Number of bytes to transmit.
 */
void uart_write(const uint8_t* pData, uint16_t Size);

/**
 * @brief Receive bytes through USART2.
 *
 * @param pData Pointer to the destination buffer.
 * @param Size Number of bytes to receive.
 * @param Timeout Maximum receive duration in milliseconds.
 */
void uart_read(uint8_t* pData, uint16_t Size, uint32_t Timeout);

#endif /* UART_H */
