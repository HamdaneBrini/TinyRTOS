/**
 * @file port_mpu.c
 * @brief Cortex-M33 MPU region configuration.
 */
#include <stdint.h>

#include "config.h"
#include "mpu.h"
#include "port_mpu.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_hal_cortex.h"

/**
 * @brief Configure and enable a Cortex-M33 MPU region.
 *
 * @param base_address Inclusive base address of the region.
 * @param limit_address Inclusive limit address of the region.
 * @param access Access permissions applied to the region.
 * @param memory_type Normal-memory or device-memory attributes.
 * @param is_executable Non-zero to allow instruction execution.
 * @param region_number MPU region slot to configure.
 * @return TINY_OK when configured, or TINY_FAIL for unsupported attributes.
 */
TinyStatus_t port_MPU_config(uint32_t base_address, uint32_t limit_address, MemoryAccess_t access,
                             MemoryType_t memory_type, int is_executable, MemoryRegionNumber_t region_number) {
    MPU_Attributes_InitTypeDef attributes = {0};
    MPU_Region_InitTypeDef region = {0};

    /* MPU regions must be configured while the MPU is disabled. */
    HAL_MPU_Disable();

    switch (memory_type) {
        case MEMORY_TYPE_NORMAL:
            attributes.Number = MPU_ATTRIBUTES_NUMBER0;
            attributes.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
            break;

        case MEMORY_TYPE_DEVICE:
            /* Device memory must never be executable. */
            if (is_executable) {
                return TINY_FAIL;
            }

            attributes.Number = MPU_ATTRIBUTES_NUMBER1;
            attributes.Attributes = MPU_DEVICE_nGnRnE;
            break;

        default: return TINY_FAIL;
    }

    HAL_MPU_ConfigMemoryAttributes(&attributes);

    region.AttributesIndex = attributes.Number;

    region.Enable = MPU_REGION_ENABLE;
    region.Number = region_number;
    region.BaseAddress = base_address;
    region.LimitAddress = limit_address;

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

    /* Unit tests inspect kernel state directly, so leave MPU enforcement
     * disabled instead of applying the production privilege boundaries. */
#ifdef UNIT_TEST
    HAL_MPU_Disable();
#else
    /* Keep the default map for privileged kernel code. Unprivileged tasks
     * require explicit code and data regions before they are started. */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
#endif

    return TINY_OK;
}
