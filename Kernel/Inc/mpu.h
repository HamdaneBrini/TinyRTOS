#ifndef MPU_H
#define MPU_H

#include "config.h"

/* Protect the linker-defined kernel RAM region from unprivileged access. */
TinyStatus_t tiny_mpu_config(void);

#endif /* MPU_H */
