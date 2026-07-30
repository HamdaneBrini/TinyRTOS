/**
 * @file gpio.c
 * @brief GPIO configuration for the firmware peripherals.
 */

#include "gpio.h"

#include "stm32h5xx_hal.h"

/**
 * @brief Configure the GPIO pins used by USART2 and the debug interface.
 *
 * The configured pin assignments are:
 * - PA2: USART2 TX.
 * - PA3: USART2 RX.
 * - PA13: SWDIO.
 * - PA14: SWCLK.
 * - PA15: JTDI.
 * - PB3: SWO.
 */
void gpio_init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  #define GPIO_INITIALIZED
}


