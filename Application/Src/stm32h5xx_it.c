/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32h5xx_it.c
  * @brief   Interrupt Service Routines.
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <sys/cdefs.h>
#include "cmsis_gcc.h"
#include "main.h"
#include "scheduler.h"
#include "stm32h5xx_it.h"

extern TIM_HandleTypeDef htim2;

/* Private includes ----------------------------------------------------------*/

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void) {
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
    while (1) {}
    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void) {
    /* USER CODE BEGIN HardFault_IRQn 0 */

    /* USER CODE END HardFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void) {
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */
    uint32_t fault_pc = ((uint32_t*)__get_PSP())[6];

    MPU->RNR = 2; /* Flash region. */
    uint32_t flash_rbar = MPU->RBAR;
    uint32_t flash_rlar = MPU->RLAR;
    /* USER CODE END MemoryManagement_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void) {
    volatile void* tim_instance = htim2.Instance;
    /* USER CODE BEGIN BusFault_IRQn 0 */
    uint32_t cfsr = SCB->CFSR;
    uint32_t hfsr = SCB->HFSR;
    uint32_t bfar = SCB->BFAR;
    uint32_t psp = __get_PSP();
    uint32_t msp = __get_MSP();
    volatile uint32_t lr;
    __asm__ volatile("mov %0, lr" : "=r"(lr));
    /* USER CODE END BusFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void) {
    /* USER CODE BEGIN UsageFault_IRQn 0 */

    /* USER CODE END UsageFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
__attribute__((naked)) void SVC_Handler(void) {
    __asm__ volatile("tst   lr, #4      \n"
                     "ite   eq          \n"
                     "mrseq r0, msp     \n"
                     "mrsne r0, psp     \n"
                     "b     SVC_Handler_Main \n");
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void) {
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm__ volatile("cpsid i                 \n"
                     "mrs r0, psp             \n"
                     "stmdb r0!, {r4-r11}     \n"
                     "ldr r1, =current_task   \n"
                     "ldr r2, [r1]            \n"
                     "str r0, [r2]            \n"
                     "push {r3,lr}            \n"
                     "bl schedule_next_task   \n"
                     "pop {r3,lr}             \n"
                     "ldr r1, =current_task   \n"
                     "ldr r2, [r1]            \n"
                     "ldr r0, [r2]            \n"
                     "ldmia r0!, {r4-r11}     \n"
                     "msr psp, r0             \n"
                     "cpsie i                       \n"
                     "bx lr                         \n");
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void) {
    /* USER CODE BEGIN SysTick_IRQn 0 */

    /* USER CODE END SysTick_IRQn 0 */
    HAL_IncTick();
    /* USER CODE BEGIN SysTick_IRQn 1 */

    /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32H5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h5xx.s).                    */
/******************************************************************************/

void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }
