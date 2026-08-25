#ifndef MPU_H
#define MPU_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "stm32h5xx_hal.h"

#define MPU_REGION_ALIGNMENT 32U

/** @brief Access permissions supported by a configured MPU region. */
typedef enum {
    MEMORY_NO_ACCESS,
    MEMORY_READ_ONLY,
    MEMORY_READ_WRITE,
    MEMORY_READ_ONLY_PV,
    MEMORY_READ_WRITE_PV,
} MemoryAccess_t;

/** @brief MPU region numbers available to the kernel. */
typedef enum {
    MEMORY_REGION_NUMEBER_0,
    MEMORY_REGION_NUMEBER_1,
    MEMORY_REGION_NUMEBER_2,
    MEMORY_REGION_NUMEBER_3,
    MEMORY_REGION_NUMEBER_4,
    MEMORY_REGION_NUMEBER_5,
    MEMORY_REGION_NUMEBER_6,
} MemoryRegionNumber_t;

/** @return TINY_OK when all linker-defined kernel regions are configured. */
TinyStatus_t MPU_kernel_config(void);

/**
 * @brief Configure and enable one MPU region.
 *
 * @param base_address Inclusive region base address.
 * @param limit_address Inclusive region limit address.
 * @param access Region access permissions.
 * @param is_executable Non-zero to permit instruction execution.
 * @param region_number MPU region slot to configure.
 * @return TINY_OK after the region is configured.
 */
TinyStatus_t MPU_config(uint32_t base_address, uint32_t limit_address, MemoryAccess_t access, int is_executable,
                        MemoryRegionNumber_t region_number);

#endif /* MPU_H */
/**
 * @file mpu.h
 * @brief Memory Protection Unit configuration interface.
 */
