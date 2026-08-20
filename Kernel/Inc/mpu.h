#ifndef MPU_H
#define MPU_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "stm32h5xx_hal.h"

typedef enum {
    MEMORY_NO_ACCESS,
    MEMORY_READ_ONLY,
    MEMORY_READ_WRITE,
    MEMORY_READ_ONLY_PV,
    MEMORY_READ_WRITE_PV,
} MemoryAccess_t;

typedef enum {
    MEMORY_REGION_NUMEBER_0,
    MEMORY_REGION_NUMEBER_1,
    MEMORY_REGION_NUMEBER_2,
    MEMORY_REGION_NUMEBER_3,
    MEMORY_REGION_NUMEBER_4,
    MEMORY_REGION_NUMEBER_5,
    MEMORY_REGION_NUMEBER_6,
} MemoryRegionNumber_t;

/* Protect the linker-defined kernel RAM region from unprivileged access. */
TinyStatus_t MPU_kernel_config(void);
TinyStatus_t MPU_config(uint32_t base_address, uint32_t limit_address, MemoryAccess_t access, int is_executable,
                        MemoryRegionNumber_t region_number);

#endif /* MPU_H */
