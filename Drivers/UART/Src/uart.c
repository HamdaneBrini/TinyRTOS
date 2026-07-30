/**
 * @file uart.c
 * @brief Blocking USART2 driver implementation.
 */

#include "uart.h"

#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_def.h"
#include "stm32h5xx_hal_uart.h"

/** @brief HAL handle used to manage the USART2 peripheral. */
static UART_HandleTypeDef huart2;

/**
 * @brief Initialize USART2 with the console communication settings.
 *
 * USART2 is configured for 115200 baud, 8 data bits, one stop bit, no parity,
 * no hardware flow control, and both transmit and receive operation.
 *
 * @note This function does not return if HAL initialization fails.
 */
void uart_init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    huart2.Instance = USART2;

    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK) {
        while (1)
            ;
    }

}

/**
 * @brief Transmit a sequence of bytes through USART2.
 *
 * This function uses a blocking transfer with no timeout limit.
 *
 * @param pData Pointer to the data to transmit.
 * @param Size Number of bytes to transmit.
 */
void uart_write(const uint8_t* pData, uint16_t Size) {

    HAL_UART_Transmit(&huart2, pData, Size, HAL_MAX_DELAY);
}

/**
 * @brief Receive a sequence of bytes through USART2.
 *
 * @param pData Pointer to the destination buffer.
 * @param Size Number of bytes to receive.
 * @param Timeout Maximum receive duration in milliseconds.
 */
void uart_read(uint8_t* pData, uint16_t Size, uint32_t Timeout) {

    HAL_UART_Receive(&huart2, pData, Size, Timeout);
}
