/**
 * @file mpu.c
 * @brief Linker-region and generic MPU configuration implementation.
 */

#include <stdint.h>

#include "config.h"
#include "mpu.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_cortex.h"
#include "port_mpu.h"

/* Bounds exported by STM32H533xx_FLASH.ld. */
extern uint8_t _skernel_ram;
extern uint8_t _ekernel_ram;
extern uint8_t _sFlash;
extern uint8_t _eFlash;
extern uint8_t _sdata;
extern uint8_t _ebss;
extern uint8_t _suser_heap;
extern uint8_t _euser_heap;

TinyStatus_t MPU_kernel_config(void) {
    /* Configure Kernel ram */
    if (port_MPU_config((uint32_t)&_skernel_ram, (uint32_t)&_ekernel_ram - 1U, MEMORY_READ_WRITE_PV, 0,
                   MEMORY_REGION_NUMEBER_0)
        != TINY_OK)
        return TINY_FAIL;
    /* Configure flash */
    if (port_MPU_config((uint32_t)&_sFlash, (uint32_t)&_eFlash - 1U, MEMORY_READ_ONLY, 1, MEMORY_REGION_NUMEBER_2)
        != TINY_OK)
        return TINY_FAIL;
    /* Configure user data and bss*/
    if (port_MPU_config((uint32_t)&_sdata, (uint32_t)&_ebss - 1U, MEMORY_READ_WRITE, 0, MEMORY_REGION_NUMEBER_3) != TINY_OK)
        return TINY_FAIL;
    /* Configure user heap */
    if (port_MPU_config((uint32_t)&_suser_heap, (uint32_t)&_euser_heap - 1U, MEMORY_READ_WRITE, 0, MEMORY_REGION_NUMEBER_4)
        != TINY_OK)
        return TINY_FAIL;
    return TINY_OK;
}


