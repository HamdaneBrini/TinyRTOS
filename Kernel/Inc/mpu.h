/**
 * @file mpu.h
 * @brief Memory Protection Unit configuration interface.
 */

#ifndef MPU_H
#define MPU_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"


/**
 * @brief Configure MPU regions for privileged kernel RAM, executable flash, user data, and the user heap.
 * @return TINY_OK when every linker-defined region is configured, otherwise TINY_FAIL.
 */
TinyStatus_t MPU_kernel_config(void);

#endif /* MPU_H */
