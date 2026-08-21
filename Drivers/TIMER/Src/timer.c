
#include "timer.h"

#include "main.h"
#include "stm32h5xx_hal.h"

TIM_HandleTypeDef htim2;


static uint32_t _timer_get_pclk1_freq(void)
{
    RCC_ClkInitTypeDef clock_config;
    uint32_t flash_latency;

    HAL_RCC_GetClockConfig(&clock_config, &flash_latency);
    if (clock_config.APB1CLKDivider == RCC_HCLK_DIV1)
    {
        return HAL_RCC_GetPCLK1Freq();
    }
    return 2U * HAL_RCC_GetPCLK1Freq();
}

void timer_init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;

    /*
     * Timer configuration
     *
     * Timer frequency:
     *     TIM2_CLK / (Prescaler + 1)
     */

    htim2.Init.Prescaler =
        (_timer_get_pclk1_freq() / 1000000U) - 1U;

    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;

    /*
     * TIM2 is 32-bit on STM32H533.
     */
    htim2.Init.Period = 0xFFFFFFFFU;

    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    htim2.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Channel 1 = Output Compare
     * No physical pin output is needed.
     */
    sConfigOC.OCMode = TIM_OCMODE_TIMING;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;

    if (HAL_TIM_OC_ConfigChannel(
            &htim2,
            &sConfigOC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * TIM2 interrupt
     */
    HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

uint32_t timer_now(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

void timer_start(uint32_t first_deadline)
{
    /*
     * Program the first compare value BEFORE starting
     * the output-compare interrupt.
     */
    __HAL_TIM_SET_COMPARE(
        &htim2,
        TIM_CHANNEL_1,
        first_deadline);

    if (HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

}

void timer_set_deadline(uint32_t deadline)
{

    __HAL_TIM_SET_COMPARE(
        &htim2,
        TIM_CHANNEL_1,
        deadline);
}
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2 &&
        htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        timer_event();
    }
}
__attribute__((weak)) void timer_event(void){

}
