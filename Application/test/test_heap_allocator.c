/**
 * @file test_heap_allocator.c
 * @brief Unit tests for the linker-backed heap allocator.
 */

#include <stddef.h>
#include <stdint.h>

#include "heap_allocator.h"
#include "tiny_unity.h"

extern uint32_t _Min_Heap_Size;

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static size_t heap_size(void) { return (size_t)&_Min_Heap_Size; }

static size_t largest_allocation_size(void) {
    size_t size = heap_size();

    while (size > 0U) {
        tiny_heap_init();
        if (tiny_malloc(size) != NULL)
            return size;
        --size;
    }
    return 0U;
}

void tiny_setup(void) { tiny_heap_init(); }

static void test_malloc_before_init_returns_null(void) { TINY_ASSERT_NULL(tiny_malloc(16U)); }

static void test_zero_size_allocation_returns_null(void) { TINY_ASSERT_NULL(tiny_malloc(0U)); }

static void test_oversized_allocation_returns_null(void) { TINY_ASSERT_NULL(tiny_malloc(heap_size())); }

static void test_size_max_allocation_returns_null(void) { TINY_ASSERT_NULL(tiny_malloc(SIZE_MAX)); }

static void test_allocation_is_max_align_t_aligned(void) {
    void* allocation = tiny_malloc(1U);

    TINY_ASSERT_NOT_NULL(allocation);
    TINY_ASSERT_EQUAL(0U, (uintptr_t)allocation % _Alignof(max_align_t));
}

static void test_odd_sized_allocations_are_aligned_and_distinct(void) {
    const size_t sizes[] = {1U, 3U, 7U, 17U};
    void* allocations[ARRAY_SIZE(sizes)];

    for (size_t i = 0U; i < ARRAY_SIZE(sizes); ++i) {
        allocations[i] = tiny_malloc(sizes[i]);
        TINY_ASSERT_NOT_NULL(allocations[i]);
        TINY_ASSERT_EQUAL(0U, (uintptr_t)allocations[i] % _Alignof(max_align_t));
        if (i > 0U)
            TINY_ASSERT_NOT_EQUAL(allocations[i - 1U], allocations[i]);
    }
}

static void test_multiple_allocations_are_distinct_and_writable(void) {
    uint8_t* first = tiny_malloc(32U);
    uint8_t* second = tiny_malloc(32U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(second);
    TINY_ASSERT_NOT_EQUAL(first, second);

    for (size_t i = 0U; i < 32U; ++i) {
        first[i] = (uint8_t)i;
        second[i] = (uint8_t)(0xFFU - i);
    }
    for (size_t i = 0U; i < 32U; ++i) {
        TINY_ASSERT_EQUAL((uint8_t)i, first[i]);
        TINY_ASSERT_EQUAL((uint8_t)(0xFFU - i), second[i]);
    }
}

static void test_free_null_is_safe(void) {
    tiny_free(NULL);
    TINY_ASSERT_NOT_NULL(tiny_malloc(16U));
}

static void test_freed_block_is_reused(void) {
    void* first = tiny_malloc(64U);
    void* guard = tiny_malloc(32U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(guard);
    tiny_free(first);
    TINY_ASSERT_EQUAL(first, tiny_malloc(64U));
}

static void test_free_does_not_corrupt_live_neighbors(void) {
    uint8_t* first = tiny_malloc(32U);
    void* middle = tiny_malloc(32U);
    uint8_t* last = tiny_malloc(32U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(middle);
    TINY_ASSERT_NOT_NULL(last);
    for (size_t i = 0U; i < 32U; ++i) {
        first[i] = 0xA5U;
        last[i] = 0x5AU;
    }

    tiny_free(middle);

    for (size_t i = 0U; i < 32U; ++i) {
        TINY_ASSERT_EQUAL(0xA5U, first[i]);
        TINY_ASSERT_EQUAL(0x5AU, last[i]);
    }
}

static void test_best_fit_uses_smallest_suitable_free_block(void) {
    void* large = tiny_malloc(128U);
    void* guard_one = tiny_malloc(16U);
    void* small = tiny_malloc(64U);
    void* guard_two = tiny_malloc(16U);

    TINY_ASSERT_NOT_NULL(large);
    TINY_ASSERT_NOT_NULL(guard_one);
    TINY_ASSERT_NOT_NULL(small);
    TINY_ASSERT_NOT_NULL(guard_two);
    tiny_free(large);
    tiny_free(small);
    TINY_ASSERT_EQUAL(small, tiny_malloc(48U));
}

static void test_best_fit_remains_correct_after_repeated_reuse(void) {
    void* large = tiny_malloc(128U);
    void* guard_one = tiny_malloc(16U);
    void* medium = tiny_malloc(96U);
    void* guard_two = tiny_malloc(16U);

    TINY_ASSERT_NOT_NULL(large);
    TINY_ASSERT_NOT_NULL(guard_one);
    TINY_ASSERT_NOT_NULL(medium);
    TINY_ASSERT_NOT_NULL(guard_two);
    tiny_free(large);
    tiny_free(medium);
    TINY_ASSERT_EQUAL(medium, tiny_malloc(80U));
    TINY_ASSERT_EQUAL(large, tiny_malloc(112U));
}

static void test_exhausted_heap_returns_null(void) {
    void* allocations[64];
    size_t count = 0U;

    while (count < ARRAY_SIZE(allocations)) {
        allocations[count] = tiny_malloc(64U);
        if (allocations[count] == NULL)
            break;
        ++count;
    }

    TINY_ASSERT_GREATER_THAN(0U, count);
    TINY_ASSERT_LESS_THAN(ARRAY_SIZE(allocations), count);
    TINY_ASSERT_NULL(tiny_malloc(64U));
}

static void test_allocation_succeeds_after_fragmented_blocks_are_freed(void) {
    void* allocations[8];

    for (size_t i = 0U; i < ARRAY_SIZE(allocations); ++i) {
        allocations[i] = tiny_malloc(32U);
        TINY_ASSERT_NOT_NULL(allocations[i]);
    }
    for (size_t i = 0U; i < ARRAY_SIZE(allocations); i += 2U)
        tiny_free(allocations[i]);

    for (size_t i = 0U; i < ARRAY_SIZE(allocations); i += 2U)
        TINY_ASSERT_EQUAL(allocations[i], tiny_malloc(32U));
}

static void test_exact_fit_consumes_the_remaining_heap(void) {
    size_t maximum = largest_allocation_size();

    TINY_ASSERT_GREATER_THAN(0U, maximum);
    tiny_heap_init();
    TINY_ASSERT_NOT_NULL(tiny_malloc(maximum));
    TINY_ASSERT_NULL(tiny_malloc(1U));
}

static void test_request_larger_than_exact_fit_returns_null(void) {
    size_t maximum = largest_allocation_size();

    TINY_ASSERT_GREATER_THAN(0U, maximum);
    tiny_heap_init();
    TINY_ASSERT_NULL(tiny_malloc(maximum + 1U));
}

static void test_adjacent_blocks_coalesce_when_freed_left_to_right(void) {
    void* first = tiny_malloc(64U);
    void* second = tiny_malloc(64U);
    void* guard = tiny_malloc(64U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(second);
    TINY_ASSERT_NOT_NULL(guard);
    tiny_free(first);
    tiny_free(second);
    TINY_ASSERT_EQUAL(first, tiny_malloc(128U));
}

static void test_adjacent_blocks_coalesce_when_freed_right_to_left(void) {
    void* first = tiny_malloc(64U);
    void* second = tiny_malloc(64U);
    void* guard = tiny_malloc(64U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(second);
    TINY_ASSERT_NOT_NULL(guard);
    tiny_free(second);
    tiny_free(first);
    TINY_ASSERT_EQUAL(first, tiny_malloc(128U));
}

static void test_three_blocks_coalesce_when_middle_is_freed_last(void) {
    void* first = tiny_malloc(64U);
    void* middle = tiny_malloc(64U);
    void* third = tiny_malloc(64U);
    void* guard = tiny_malloc(64U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(middle);
    TINY_ASSERT_NOT_NULL(third);
    TINY_ASSERT_NOT_NULL(guard);
    tiny_free(first);
    tiny_free(third);
    tiny_free(middle);
    TINY_ASSERT_EQUAL(first, tiny_malloc(192U));
}

static void test_entire_heap_is_reusable_after_freeing_all_blocks(void) {
    void* first = tiny_malloc(64U);
    void* second = tiny_malloc(64U);
    void* third = tiny_malloc(64U);

    TINY_ASSERT_NOT_NULL(first);
    TINY_ASSERT_NOT_NULL(second);
    TINY_ASSERT_NOT_NULL(third);
    tiny_free(second);
    tiny_free(first);
    tiny_free(third);
    TINY_ASSERT_NOT_NULL(tiny_malloc(heap_size() / 2U));
}

static void test_heap_init_resets_allocator_state(void) {
    void* original = tiny_malloc(64U);

    TINY_ASSERT_NOT_NULL(original);
    TINY_ASSERT_NOT_NULL(tiny_malloc(64U));
    tiny_heap_init();
    TINY_ASSERT_EQUAL(original, tiny_malloc(64U));
}

void Test_start(void) {
    tiny_test_begin();

    /* This must run first because later fixtures initialize the heap. */
    test_malloc_before_init_returns_null();
    tiny_test_result("test_malloc_before_init_returns_null");

    RUN_TEST(test_zero_size_allocation_returns_null);
    RUN_TEST(test_oversized_allocation_returns_null);
    RUN_TEST(test_size_max_allocation_returns_null);
    RUN_TEST(test_allocation_is_max_align_t_aligned);
    RUN_TEST(test_odd_sized_allocations_are_aligned_and_distinct);
    RUN_TEST(test_multiple_allocations_are_distinct_and_writable);
    RUN_TEST(test_free_null_is_safe);
    RUN_TEST(test_freed_block_is_reused);
    RUN_TEST(test_free_does_not_corrupt_live_neighbors);
    RUN_TEST(test_best_fit_uses_smallest_suitable_free_block);
    RUN_TEST(test_best_fit_remains_correct_after_repeated_reuse);
    RUN_TEST(test_exhausted_heap_returns_null);
    RUN_TEST(test_allocation_succeeds_after_fragmented_blocks_are_freed);
    RUN_TEST(test_exact_fit_consumes_the_remaining_heap);
    RUN_TEST(test_request_larger_than_exact_fit_returns_null);
    RUN_TEST(test_adjacent_blocks_coalesce_when_freed_left_to_right);
    RUN_TEST(test_adjacent_blocks_coalesce_when_freed_right_to_left);
    RUN_TEST(test_three_blocks_coalesce_when_middle_is_freed_last);
    RUN_TEST(test_entire_heap_is_reusable_after_freeing_all_blocks);
    RUN_TEST(test_heap_init_resets_allocator_state);

    tiny_test_end();
}
