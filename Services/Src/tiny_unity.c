/**
 * @file tiny_unity.c
 * @brief Runtime state and reporting for the TinyRTOS unit-test framework.
 */

#include <stdint.h>

#include "console.h"
#include "stm32h5xx.h"
#include "tiny_unity.h"

/**
 * @brief Internal state accumulated while a test suite is running.
 */
typedef struct {
    uint32_t failed_count;        /**< Number of tests that failed. */
    uint32_t test_count;          /**< Number of tests that completed. */
    uint32_t current_test_failed; /**< Non-zero if an assertion failed in the active test. */
} Test_status;

/** @brief State of the currently running test suite. */
static Test_status test_status;

/**
 * @brief Evaluate an assertion and store its failure state.
 *
 * Successful assertions remain silent. Failed assertions print diagnostic
 * information; the final test result is printed by tiny_test_result().
 */
int tiny_assert(int condition, const char* message, const char* file, uint32_t line, const char* test_name) {
    if (condition) {
        return 1;
    }

    test_status.current_test_failed = 1U;
    print("  %s:%u: assertion failed in %s\n", file, line, test_name);
    print("    %s\n", message);
    return 0;
}

void tiny_test_begin(void) {
    test_status.failed_count = 0;
    test_status.test_count = 0;
    test_status.current_test_failed = 0;
}

void tiny_test_result(const char* test_name) {
    test_status.test_count++;
    if (test_status.current_test_failed != 0U) {
        test_status.failed_count++;
        print("%s: FAIL\n", test_name);
    } else {
        print("%s: PASS\n", test_name);
    }
    test_status.current_test_failed = 0U;
}

void tiny_test_end(void) {
    print("\n-----------------------\n");
    print("%u Tests %u Failures\n", test_status.test_count, test_status.failed_count);
    print(test_status.failed_count == 0U ? "OK\n" : "FAIL\n");

    __disable_irq();
    /** Avoid causing a HardFault when no debugger is connected. */
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U) {
        __BKPT(0);
    }
    while (1) {
        __WFI();
    }
}

/** @brief Default empty setup hook, overridable by the application. */
__attribute__((weak)) void tiny_setup(void) {}

/** @brief Default empty teardown hook, overridable by the application. */
__attribute__((weak)) void tiny_teardown(void) {}
