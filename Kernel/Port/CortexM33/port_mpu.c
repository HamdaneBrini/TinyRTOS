/**
 * @file port_mpu.c
 * @brief Reserved Cortex-M33 MPU port implementation unit.
 */
#include <stdint.h>

#include "config.h"
#include "mpu.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_cortex.h"
#include "port_mpu.h"

/**
 * @brief Configure and enable a Cortex-M33 MPU region.
 *
 * @param base_address Inclusive base address of the region.
 * @param limit_address Inclusive limit address of the region.
 * @param access Access permissions applied to the region.
 * @param is_executable Non-zero to allow instruction execution.
 * @param region_number MPU region slot to configure.
 * @return TINY_OK when the region has been configured.
 */
TinyStatus_t port_MPU_config(uint32_t base_address, uint32_t limit_address, MemoryAccess_t access, int is_executable,
                             MemoryRegionNumber_t region_number) {
    MPU_Attributes_InitTypeDef attributes = {0};
    MPU_Region_InitTypeDef region = {0};

    /* MPU regions must be configured while the MPU is disabled. */
    HAL_MPU_Disable();

    /* Use normal, non-cacheable SRAM attributes to remain DMA-safe. */
    attributes.Number = MPU_ATTRIBUTES_NUMBER0;
    attributes.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
    HAL_MPU_ConfigMemoryAttributes(&attributes);

    region.Enable = MPU_REGION_ENABLE;
    region.Number = region_number;
    region.BaseAddress = base_address;
    region.LimitAddress = limit_address;
    region.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;

    region.IsShareable = MPU_ACCESS_INNER_SHAREABLE;
    region.DisableExec = is_executable ? MPU_INSTRUCTION_ACCESS_ENABLE : MPU_INSTRUCTION_ACCESS_DISABLE;
    switch (access) {
        case MEMORY_NO_ACCESS: {
            region.Enable = MPU_REGION_DISABLE;
            break;
        }
        case MEMORY_READ_ONLY: {
            region.AccessPermission = MPU_REGION_ALL_RO;
            break;
        }
        case MEMORY_READ_WRITE: {
            region.AccessPermission = MPU_REGION_ALL_RW;
            break;
        }
        case MEMORY_READ_ONLY_PV: {
            region.AccessPermission = MPU_REGION_PRIV_RO;
            break;
        }
        case MEMORY_READ_WRITE_PV: {
            region.AccessPermission = MPU_REGION_PRIV_RW;
            break;
        }
        default: break;
    }

    HAL_MPU_ConfigRegion(&region);

    /* Keep the default map for privileged kernel code. Unprivileged tasks
     * require explicit code and data regions before they are started. */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

    return TINY_OK;
}
