/**
 *******************************************************************************
 *  @file           filter_test_context.c
 *  @brief          トレースのコンテキスト引数として付加する値の取得関数を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/26
 *  @version        0.1.0
 *
 *  条件式フィルターのテストで、app が定義するコンテキスト引数の照合を確かめるための巡回連番です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#include "filter_test_context.h"

#include <cplat/sync/atomic.h>

#include <stdint.h>

/** 呼び出しごとに増加するカウンターです。計数だけが目的のため、順序の保証がない軽量な加算 (RELAXED) で更新します。 */
static cplat_atomic_u32 s_sequence_number = CPLAT_ATOMIC_INIT(0U);

/* Doxygen コメントは、ヘッダーに記載 */

int32_t filter_test_next_sequence_number(void)
{
    /* 加算前の値が返るため、1 を足して加算後の値にそろえる。 */
    const uint32_t current = cplat_atomic_fetch_add_u32(&s_sequence_number, 1U, CPLAT_MEMORY_ORDER_RELAXED) + 1U;

    return (int32_t)((current - 1U) % (uint32_t)FILTER_TEST_SEQUENCE_NUMBER_MAX) + 1;
}
