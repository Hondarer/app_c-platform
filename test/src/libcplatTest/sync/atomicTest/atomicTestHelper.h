// atomicTest の各スイートで共有するテスト ヘルパー
#ifndef ATOMIC_TEST_HELPER_H
#define ATOMIC_TEST_HELPER_H

#include <cplat/sync/atomic.h>

#include <cstddef>

// すべてのメモリ順序。順序を変えても結果が変わらないことを検査するループに使用する。
static const cplat_memory_order kAllMemoryOrders[] = {
    CPLAT_MEMORY_ORDER_RELAXED, CPLAT_MEMORY_ORDER_ACQUIRE, CPLAT_MEMORY_ORDER_RELEASE,
    CPLAT_MEMORY_ORDER_ACQ_REL, CPLAT_MEMORY_ORDER_SEQ_CST,
};
static const std::size_t kAllMemoryOrderCount = sizeof(kAllMemoryOrders) / sizeof(kAllMemoryOrders[0]);

#endif /* ATOMIC_TEST_HELPER_H */
