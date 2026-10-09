/**
 * @file uart.c
 * @brief Interrupt-driven USART2 transmission and blocking reception implementation.
 */

#include "uart.h"
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "data_struct.h"
#include "heap_allocator.h"
#include "io_utils.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_def.h"
#include "stm32h5xx_hal_uart.h"

typedef enum {
    IDLE,
    BUSY,
} Tx_state_t;

#define TX_BUFFER_SIZE 512U
/** @brief HAL handle used to manage the USART2 peripheral. */
UART_HandleTypeDef huart2;
static _IO Ring_node_t tx_buffer[TX_BUFFER_SIZE];
static _IO Ring_buffer_t ring_buffer;
static _IO Tx_state_t tx_active = IDLE;
static _IO Ring_node_t active_tx = {0};

static void _uart_transmit(void);

/**
 * @brief Initialize USART2 with the console communication settings.
 *
 * USART2 is configured for 115200 baud, 8 data bits, one stop bit, no parity,
 * no hardware flow control, and both transmit and receive operation.
 *
 * @return TINY_OK on success, or TINY_FAIL if HAL initialization fails.
 */
TinyStatus_t uart_init(void) {
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
        return TINY_FAIL;
    }
    HAL_NVIC_SetPriority(USART2_IRQn, UART_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    ring_buffer_init(&ring_buffer, tx_buffer, TX_BUFFER_SIZE);
    return TINY_OK;
}

/**
 * @brief Transmit a sequence of bytes through USART2.
 *
 * The buffer descriptor is queued without copying the payload. If no transfer
 * is active, this function starts an interrupt-driven transmission immediately.
 *
 * @param pData Pointer to the data to transmit.
 * @param size Number of bytes to transmit.
 */
void uart_write(const uint8_t* pData, uint16_t size) {
    Ring_node_t data = {.buffer = (char*)pData, .length = (size_t)size};
    if (ring_buffer_put(&ring_buffer, data) != TINY_OK){
        return;
    }

    if (tx_active == IDLE) {
        active_tx = data;
        tx_active = BUSY;
        _uart_transmit();
    }
}

/**
 * @brief Start transmitting the oldest queued buffer.
 *
 * The transmitter becomes idle when no queued descriptor remains.
 */
void _uart_transmit(void) {
    Ring_node_t output;
    if (ring_buffer_get(&ring_buffer, &output) != TINY_OK) {
        tx_active = IDLE;
        return;
    }
    active_tx=output;
    HAL_UART_Transmit_IT(&huart2, output.buffer, (uint16_t)output.length);
}

/** @copydoc uart_read */
void uart_read(uint8_t* pData, uint16_t size, uint32_t timeout) { HAL_UART_Receive(&huart2, pData, size, timeout); }

/**
 * @brief Release the completed console buffer and start the next queued transfer.
 * @param huart HAL UART handle that completed transmission.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART2) {

        kernel_free(active_tx.buffer, &kernel_heap);
        _uart_transmit();
    }
}
