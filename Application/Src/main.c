/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "console.h"
#include "gpio.h"
#include "heap_allocator.h"
#include "mpu.h"
#include "uart.h"
#ifdef UNIT_TEST
#include "test_utils.h"
#endif

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void System_start(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    System_start();

#ifdef UNIT_TEST
    Test_start();
#else
    /* Initialize kernel */
    tiny_heap_init();

    tinyprint("=======Kernel started======\n\n");

    while (1) {}
#endif
}

/**
 * @brief Initialize the MCU, configured peripherals, and application services.
 */
void System_start(void) {
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Restrict the dedicated kernel SRAM to privileged accesses. */
    if (tiny_mpu_config() != TINY_OK) {
        Error_Handler();
    }

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    gpio_init();
    uart_init();

    /* Initialize services */
    console_init();
    tinyprint("=======CONSOLE INITIALIZED SUCCESSFULLY======\n\n");
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV2;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
  */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }

    /** Configure the programming delay
  */
    __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_0);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}

/**
 * @brief Provide a default unit-test entry point.
 *
 * This weak, empty implementation allows a unit-test source file to provide
 * its own strong Test_start() implementation.
 */
#ifdef UNIT_TEST
__attribute__((weak)) void Test_start(void) {
    /* Example implementation:
     *
     * tiny_test_begin();
     * RUN_TEST(test_boolean_assertions);
     * RUN_TEST(test_equality_assertions);
     * tiny_test_end();
     */
}
#endif
