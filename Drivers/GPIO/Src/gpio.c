/**
 * @file gpio.c
 * @brief GPIO configuration for the USART2 console.
 */

#include "gpio.h"

#include "config.h"
#include "stm32h5xx_hal.h"

/**
 * @brief Configure the GPIO pins used by USART2.
 *
 * The function configures these alternate-function assignments while leaving
 * the SWD/JTAG pins in their reset configuration:
 * - PA2: USART2 TX.
 * - PA3: USART2 RX.
 *
 * @return TINY_OK after configuration completes.
 */
TinyStatus_t gpio_init(void) {

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable the GPIO port clocks used by the board configuration. */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure PA2 and PA3 for USART2 transmit and receive. */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    return TINY_OK;
}
