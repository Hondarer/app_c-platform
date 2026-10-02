/**
 *******************************************************************************
 *  @file           filter_test_trace_key_names.h
 *  @brief          filter_test_trace の文字列キーの名前解決テーブルを宣言します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/26
 *  @version        0.1.0
 *
 *  移行設計の第 2 段階では、カタログ定義生成器が `{module}_key_names()` を出力します。\n
 *  それまでは、同じ形の関数を手書きしています。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#ifndef FILTER_TEST_TRACE_KEY_NAMES_PRIVATE_H
#define FILTER_TEST_TRACE_KEY_NAMES_PRIVATE_H

#include <cplat/string_catalog/filter.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          名前解決テーブルの先頭を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    const cplat_string_catalog_filter_key_name *filter_test_trace_key_names(void);

    /**
     *  @brief          名前解決テーブルの要素数を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    size_t filter_test_trace_key_name_count(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FILTER_TEST_TRACE_KEY_NAMES_PRIVATE_H */
