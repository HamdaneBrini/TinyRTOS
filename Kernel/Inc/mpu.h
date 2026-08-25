/**
 * @file mpu.h
 * @brief Memory Protection Unit configuration interface.
 */

#ifndef MPU_H
#define MPU_H

#include <stddef.h>
#include <stdint.h>
#include "config.h"


/** @return TINY_OK when all linker-defined kernel regions are configured. */
TinyStatus_t MPU_kernel_config(void);

#endif /* MPU_H */
