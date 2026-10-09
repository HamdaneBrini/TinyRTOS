/**
 * @file gpio.h
 * @brief Public interface for board GPIO initialization.
 */

#ifndef GPIO_H
#define GPIO_H

#include "config.h"

/**
 * @brief Initialize the GPIO pins used by the firmware.
 * @return TINY_OK after the pins have been configured.
 */
TinyStatus_t gpio_init(void);

#endif /* GPIO_H */
