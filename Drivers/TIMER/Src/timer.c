/**
 * @file timer.c
 * @brief TIM2 output-compare timer implementation.
 */

#include "timer.h"

#include <stdint.h>

#include "config.h"
#include "main.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_tim.h"

/** HAL state for the TIM2 scheduler timer. */
TIM_HandleTypeDef htim2;

/**
 * @brief Return the effective TIM2 input-clock frequency.
 *
 * STM32 timer clocks run at twice PCLK1 when the APB1 prescaler divides HCLK.
 */
static uint32_t _timer_get_pclk1_freq(void) {
    RCC_ClkInitTypeDef clock_config;
    uint32_t flash_latency;

    HAL_RCC_GetClockConfig(&clock_config, &flash_latency);
    if (clock_config.APB1CLKDivider == RCC_HCLK_DIV1) {
        return HAL_RCC_GetPCLK1Freq();
    }
    return 2U * HAL_RCC_GetPCLK1Freq();
}

/** @copydoc timer_init */
TinyStatus_t timer_init(void) {
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();
    /* Freeze TIM2 whenever a debugger halts the CPU. */
    __HAL_DBGMCU_FREEZE_TIM2();

    htim2.Instance = TIM2;

    /* Configure a 1 MHz counter: TIM2_CLK / (Prescaler + 1). */

    htim2.Init.Prescaler = (_timer_get_pclk1_freq() / 1000000U) - 1U;

    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;

    /* TIM2 is a 32-bit timer on STM32H533. */
    htim2.Init.Period = COUNTER_PERIOD;

    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_OC_Init(&htim2) != HAL_OK) {
        return TINY_FAIL;
    }

    /* Use Channel 1 for an internal compare deadline; no output pin is needed. */
    sConfigOC.OCMode = TIM_OCMODE_TIMING;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;

    if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        return TINY_FAIL;
    }

    /* Configure the interrupt used for compare deadlines and counter overflow. */
    HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    return TINY_OK;
}

/** @copydoc timer_deinit */
TinyStatus_t timer_deinit(void) {
    HAL_NVIC_DisableIRQ(TIM2_IRQn);

    if (HAL_TIM_OC_Stop_IT(&htim2, TIM_CHANNEL_1) != HAL_OK) {
        return TINY_FAIL;
    }

    /* OC_Stop_IT disables CC1, but not the overflow interrupt. */
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1 | TIM_FLAG_UPDATE);

    NVIC_ClearPendingIRQ(TIM2_IRQn);

    if (HAL_TIM_OC_DeInit(&htim2) != HAL_OK) {
        return TINY_FAIL;
    }

    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();
    __HAL_RCC_TIM2_CLK_DISABLE();

    return TINY_OK;
}

/** @copydoc timer_now */
uint32_t timer_now(void) { return __HAL_TIM_GET_COUNTER(&htim2); }

#ifdef UNIT_TEST
/** @copydoc timer_test_set_counter */
void timer_test_set_counter(uint32_t value) { __HAL_TIM_SET_COUNTER(&htim2, value); }
#endif

/** @copydoc timer_start */
void timer_start(uint32_t first_deadline) {
    /* Keep the counter stable while establishing a deterministic time origin,
     * then program the first compare before enabling its interrupt. */
    __HAL_TIM_DISABLE(&htim2);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, first_deadline);

    if (HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    /* Track complete 32-bit timer cycles in the scheduler. */
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
}

/** @copydoc timer_set_deadline */
void timer_set_deadline(uint32_t deadline) {
    /*
     * timer_cancel_deadline() masks CC1, so every newly programmed deadline
     * must arm the compare interrupt again. Clear a stale match before
     * updating CCR1 to avoid servicing a cancelled deadline.
     */
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, deadline);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
}

/** @copydoc timer_cancel_deadline */
void timer_cancel_deadline(void) {
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);
}

/** Forward a TIM2 Channel 1 compare event to the scheduler hook. */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        timer_event();
    }
}

/** Forward a TIM2 counter-overflow event to the scheduler hook. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    if (htim->Instance == TIM2) {
        timer_overflow_event();
    }
}

/** Default overflow hook for builds that do not provide a scheduler handler. */
__attribute__((weak)) void timer_overflow_event(void) {}

/** Default compare hook for builds that do not provide a scheduler handler. */
__attribute__((weak)) void timer_event(void) {}
