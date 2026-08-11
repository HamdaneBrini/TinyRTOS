/**
 * @file tiny_unity.h
 * @brief Lightweight unit-test assertions and test runner for TinyRTOS.
 *
 * Test functions are registered with RUN_TEST(). A failed assertion stops the
 * current test, while the runner prints one PASS or FAIL result per test.
 */

#ifndef TINY_UNITY_H
#define TINY_UNITY_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Record and display the result of an assertion.
 *
 * @param condition Non-zero when the assertion passed.
 * @param message Description displayed when the assertion fails.
 * @param file Source file containing the assertion.
 * @param line Source line containing the assertion.
 * @param test_name Name of the test containing the assertion.
 * @return 1 if the assertion passed, otherwise 0.
 */
int tiny_assert(int condition, const char* message, const char* file, uint32_t line, const char* test_name);

/**
 * @brief Reset the test-suite counters before running tests.
 */
void tiny_test_begin(void);

/**
 * @brief Print the test-suite summary and halt the target.
 */
void tiny_test_end(void);

/**
 * @brief Fixture hook called before every test.
 *
 * Applications may provide a strong definition to override the default empty
 * implementation.
 */
void tiny_setup(void);

/**
 * @brief Fixture hook called after every test.
 *
 * Applications may provide a strong definition to override the default empty
 * implementation.
 */
void tiny_teardown(void);

/**
 * @brief Record and print the result of a completed test.
 * @param test_name Name of the completed test.
 */
void tiny_test_result(const char* test_name);

/**
 * @brief Evaluate an assertion and stop the current test if it fails.
 * @param condition Expression that must evaluate to true.
 * @param message Failure message printed when the expression is false.
 */
#define TINY_ASSERT_RESULT(condition, message)                                                                         \
    do {                                                                                                               \
        if (!tiny_assert((condition), (message), __FILE_NAME__, (uint32_t)__LINE__, __func__)) {                       \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

/** @brief Assert that a condition is true. */
#define TINY_ASSERT(condition)       TINY_ASSERT_RESULT((condition), "Expected true: " #condition)
/** @brief Assert that a condition is true. */
#define TINY_ASSERT_TRUE(condition)  TINY_ASSERT(condition)
/** @brief Assert that a condition is false. */
#define TINY_ASSERT_FALSE(condition) TINY_ASSERT_RESULT(!(condition), "Expected false: " #condition)
/** @brief Assert that two values are equal. */
#define TINY_ASSERT_EQUAL(expected, actual)                                                                            \
    TINY_ASSERT_RESULT((expected) == (actual), "Expected " #expected ", actual " #actual)
/** @brief Assert that two values are not equal. */
#define TINY_ASSERT_NOT_EQUAL(not_expected, actual)                                                                    \
    TINY_ASSERT_RESULT((not_expected) != (actual), "Did not expect " #not_expected ", actual " #actual)
/** @brief Assert that a pointer is NULL. */
#define TINY_ASSERT_NULL(pointer)     TINY_ASSERT_RESULT((pointer) == NULL, "Expected NULL: " #pointer)
/** @brief Assert that a pointer is not NULL. */
#define TINY_ASSERT_NOT_NULL(pointer) TINY_ASSERT_RESULT((pointer) != NULL, "Expected non-NULL: " #pointer)
/** @brief Assert that a value is greater than a threshold. */
#define TINY_ASSERT_GREATER_THAN(threshold, actual)                                                                    \
    TINY_ASSERT_RESULT((actual) > (threshold), "Expected " #actual " > " #threshold)
/** @brief Assert that a value is less than a threshold. */
#define TINY_ASSERT_LESS_THAN(threshold, actual)                                                                       \
    TINY_ASSERT_RESULT((actual) < (threshold), "Expected " #actual " < " #threshold)
/** @brief Assert that a value is greater than or equal to a threshold. */
#define TINY_ASSERT_GREATER_OR_EQUAL(threshold, actual)                                                                \
    TINY_ASSERT_RESULT((actual) >= (threshold), "Expected " #actual " >= " #threshold)
/** @brief Assert that a value is less than or equal to a threshold. */
#define TINY_ASSERT_LESS_OR_EQUAL(threshold, actual)                                                                   \
    TINY_ASSERT_RESULT((actual) <= (threshold), "Expected " #actual " <= " #threshold)
/** @brief Assert that every bit in a mask is set. */
#define TINY_ASSERT_BITS_SET(mask, actual)                                                                             \
    TINY_ASSERT_RESULT(((actual) & (mask)) == (mask), "Expected all bits in " #mask " set in " #actual)
/** @brief Assert that every bit in a mask is clear. */
#define TINY_ASSERT_BITS_CLEAR(mask, actual)                                                                           \
    TINY_ASSERT_RESULT(((actual) & (mask)) == 0U, "Expected all bits in " #mask " clear in " #actual)
/** @brief Unconditionally fail the current test. */
#define TINY_FAIL(message) TINY_ASSERT_RESULT(0, (message))

/**
 * @brief Run one test with fixture hooks and print its result.
 * @param test_func Test function to execute.
 */
#define RUN_TEST(test_func)                                                                                            \
    do {                                                                                                               \
        tiny_setup();                                                                                                  \
        test_func();                                                                                                   \
        tiny_test_result(#test_func);                                                                                  \
        tiny_teardown();                                                                                               \
    } while (0)

#endif /* TINY_UNITY_H */
