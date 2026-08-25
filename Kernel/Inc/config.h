#ifndef CONFIG_H
#define CONFIG_H

/** @brief Status returned by TinyRTOS operations. */
typedef enum {
    TINY_OK = 1,
    TINY_FAIL = 0,

} TinyStatus_t;

#define INITIAL_XPSR 0x01000000UL

#endif
/**
 * @file config.h
 * @brief Common TinyRTOS status and context constants.
 */
