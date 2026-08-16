#include <stdint.h>

#include "mpu.h"
#include "stm32h5xx_hal.h"

/* Bounds exported by STM32H533xx_FLASH.ld. */
extern uint8_t _skernel_ram;
extern uint8_t _ekernel_ram;

TinyStatus_t tiny_mpu_config(void) {
    MPU_Attributes_InitTypeDef attributes = {0};
    MPU_Region_InitTypeDef region = {0};

    /* MPU regions must be configured while the MPU is disabled. */
    HAL_MPU_Disable();

    /* Use normal, non-cacheable SRAM attributes to remain DMA-safe. */
    attributes.Number = MPU_ATTRIBUTES_NUMBER0;
    attributes.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
    HAL_MPU_ConfigMemoryAttributes(&attributes);

    region.Enable = MPU_REGION_ENABLE;
    region.Number = MPU_REGION_NUMBER0;
    region.BaseAddress = (uint32_t)&_skernel_ram;
    region.LimitAddress = (uint32_t)&_ekernel_ram - 1U;
    region.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
    region.AccessPermission = MPU_REGION_PRIV_RW;
    region.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    region.IsShareable = MPU_ACCESS_INNER_SHAREABLE;
    HAL_MPU_ConfigRegion(&region);

    /* Keep the default map for privileged kernel code. Unprivileged tasks
     * require explicit code and data regions before they are started. */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    return TINY_OK;
}
