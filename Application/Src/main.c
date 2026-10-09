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
#include <stdint.h>
#include "config.h"
#include "console.h"
#include "gpio.h"
#include "heap_allocator.h"
#include "io_utils.h"
#include "mpu.h"
#include "scheduler.h"
#include "stdout.h"
#include "task.h"
#include "timer.h"
#include "timing.h"
#include "uart.h"

#ifdef UNIT_TEST
#include "test_utils.h"
#endif

/* Private function prototypes -----------------------------------------------*/
static void system_clock_config(void);
static void system_start(void);
static void task1(void* arg) __attribute__((unused));
static void task2(void* arg) __attribute__((unused));

/** Counters proving that both equal-priority load tasks received CPU time. */
static volatile uint32_t task2_iterations;
static volatile uint32_t task3_iterations;
static volatile uint32_t task4_iterations;
static volatile uint32_t task5_iterations;
static volatile uint32_t task6_iterations;
/**
 * @brief Exercise task execution and shared initialized data access.
 *
 * @param arg Unused task argument.
 */
static void task1(void* arg) {
    (void)arg;

    /* Measure a long delay while two CPU-bound tasks of the same priority run. */
    TinyTimestamp_t start;
    TinyTimestamp_t end;
    const TinyDuration_t long_delay = {.sec = 5U};

    tiny_print("Equal-priority test: waiting 5 sec with 5 load tasks...\n");
    if (tiny_get_timestamp(&start) != TINY_OK) {
        tiny_print("Failed to read the start timestamp\n");
        return;
    }

    tiny_long_delay(long_delay);

    if (tiny_get_timestamp(&end) != TINY_OK) {
        tiny_print("Failed to read the end timestamp\n");
        return;
    }

    uint64_t start_us = ((uint64_t)start.cycle_count << 32) + start.tick_us;
    uint64_t end_us = ((uint64_t)end.cycle_count << 32) + end.tick_us;
    uint64_t elapsed_us = end_us - start_us;
    uint32_t elapsed_sec = (uint32_t)(elapsed_us / 1000000ULL);
    uint32_t remaining_us = (uint32_t)(elapsed_us % 1000000ULL);

    tiny_print("Measured delay = %u sec, %u ms, %u us\n", elapsed_sec, remaining_us / 1000U,
               remaining_us % 1000U);
    tiny_print("Load counters: task 2 = %u, task 3 = %u, task 4 = %u, task 5 = %u, task 6 = %u\n", task2_iterations, task3_iterations,task4_iterations,task5_iterations,task6_iterations);
    tiny_print(elapsed_us >= 5000000ULL ? "Equal-priority delay test: PASS\n" : "Equal-priority delay test: FAIL\n");
}

/**
 * @brief Exercise task execution and shared zero-initialized data access.
 *
 * @param arg Unused task argument.
 */
static void task2(void* arg) {
    volatile uint32_t* iterations = (volatile uint32_t*)arg;
    for (;;) {
        (*iterations)++;
        tiny_delay_ms(1);
    }
}


/**
  * @brief Application entry point.
  * @return This function does not return during normal firmware execution.
  */
int main(void) {
    system_start();

#ifdef UNIT_TEST
    Test_start();
#else
    /* Establish kernel memory protection before creating unprivileged tasks. */
    if (MPU_kernel_config() != TINY_OK) {
        Error_Handler();
    }
    if (scheduler_init() != TINY_OK) {
        Error_Handler();
    }
    print("-----Scheduler initialized------\n");
    /* All three tasks use the same priority to exercise round-robin scheduling. */
    TaskHandle_t task1_handle;
    TaskHandle_t task2_handle;
    TaskHandle_t task3_handle;
    TaskHandle_t task4_handle;
    TaskHandle_t task5_handle;
    TaskHandle_t task6_handle;

    if (tiny_task_create(&task1_handle, task1, NULL, 3U, 512U) != TINY_OK ||
         tiny_task_create(&task2_handle, task2, (void*)&task2_iterations, 3U, 512U) != TINY_OK
        || tiny_task_create(&task3_handle, task2, (void*)&task3_iterations, 3U, 512U) != TINY_OK
        || tiny_task_create(&task4_handle, task2, (void*)&task4_iterations, 3U, 512U) != TINY_OK
        || tiny_task_create(&task5_handle, task2, (void*)&task5_iterations, 3U, 512U) != TINY_OK
        || tiny_task_create(&task6_handle, task2, (void*)&task6_iterations, 3U, 512U) != TINY_OK) {
        Error_Handler();
    }

    if (tiny_scheduler_start() != TINY_OK) {
        Error_Handler();
    }

    for (;;) {}
#endif
}

/**
 * @brief Initialize the MCU, configured peripherals, and application services.
 */
static void system_start(void) {
    /* MCU Configuration--------------------------------------------------------*/

    /* Reset peripherals and initialize the Flash interface and SysTick. */
    HAL_Init();

    /* Configure the system clock */
    system_clock_config();
    /* Route configurable bus errors to BusFault instead of escalating them to HardFault. */
    SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
    __DSB();
    __ISB();

    /* Initialize all configured peripherals. */
    if (timer_init() != TINY_OK || uart_init() != TINY_OK || gpio_init() != TINY_OK) {
        Error_Handler();
    }

    /* Initialize services */

    if (kernel_heap_init() != TINY_OK) {
        Error_Handler();
    }
    console_init();
    print("Heaps initialized\n");
    print("Console initialized\n");
}

/**
  * @brief Configure the system clock tree and flash programming delay.
  */
static void system_clock_config(void) {
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
  * @brief Disable interrupts and stop execution after a fatal initialization error.
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
