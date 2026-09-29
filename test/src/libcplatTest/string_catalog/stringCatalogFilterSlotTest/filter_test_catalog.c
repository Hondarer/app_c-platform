/**
 *******************************************************************************
 *  @file           filter_test_catalog.c
 *  @brief          フィルター スロットのテストが使用するカタログを定義します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/30
 *  @version        1.0.0
 *
 *  指定初期化子で言語別の要素を書くため、C のソースとして定義します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "filter_test_catalog.h"

#include <stddef.h>

/** 引数を 1 個取る文字列の引数定義です。 */
static const cplat_string_catalog_argument s_number_arguments[1] = {
    {CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT32, 0, "number", "番号。"}};

/** カタログの定義です。 */
static const cplat_string_catalog_entry s_entries[1] = {{FILTER_TEST_CATALOG_KEY_NUMBER,
                                                         0,
                                                         1,
                                                         0,
                                                         s_number_arguments,
                                                         NULL,
                                                         "番号",
                                                         NULL,
                                                         NULL,
                                                         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "number {0}"},
                                                         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = ""}}};

/** カタログ識別オブジェクトです。インデックス表を持たず、線形探索の経路を使います。 */
static const cplat_string_catalog s_catalog = {s_entries, NULL, 1, 0};

/* Doxygen コメントは、ヘッダーに記載 */

const cplat_string_catalog *filter_test_catalog(void)
{
    return &s_catalog;
}
