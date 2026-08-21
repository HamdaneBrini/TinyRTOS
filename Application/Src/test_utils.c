#include <stddef.h>
#include <stdint.h>

#include "logging.h"
#include "tiny_unity.h"

static void test_boolean_assertions(void) {
    TINY_ASSERT(1);
    TINY_ASSERT_TRUE(2 + 2 == 4);
    TINY_ASSERT_FALSE(2 + 2 == 5);
}

static void test_equality_assertions(void) {
    TINY_ASSERT_EQUAL(11, 5 + 6);
    TINY_ASSERT_NOT_EQUAL(11, 5 - 6);
}

static void test_ordering_assertions(void) {
    TINY_ASSERT_GREATER_THAN(5, 6);
    TINY_ASSERT_LESS_THAN(6, 5);
    TINY_ASSERT_GREATER_OR_EQUAL(5, 5);
    TINY_ASSERT_GREATER_OR_EQUAL(5, 6);
    TINY_ASSERT_LESS_OR_EQUAL(5, 5);
    TINY_ASSERT_LESS_OR_EQUAL(6, 5);
}

static void test_pointer_assertions(void) {
    int value = 42;
    int* pointer = &value;
    int* null_pointer = NULL;

    TINY_ASSERT_NOT_NULL(pointer);
    TINY_ASSERT_NULL(null_pointer);
}

static void test_bit_assertions(void) {
    const uint32_t value = 0xA5U;

    TINY_ASSERT_BITS_SET(0x81U, value);
    TINY_ASSERT_BITS_CLEAR(0x5AU, value);
}

static void test_assertion_arguments_are_evaluated_once(void) {
    int expected = 3;
    int actual = 3;

    TINY_ASSERT_EQUAL(expected++, actual++);
    TINY_ASSERT_EQUAL(4, expected);
    TINY_ASSERT_EQUAL(4, actual);
}

void Test_start(void) {
    tiny_test_begin();
    RUN_TEST(test_boolean_assertions);
    RUN_TEST(test_equality_assertions);
    RUN_TEST(test_ordering_assertions);
    RUN_TEST(test_pointer_assertions);
    RUN_TEST(test_bit_assertions);
    RUN_TEST(test_assertion_arguments_are_evaluated_once);
    tiny_test_end();
}
