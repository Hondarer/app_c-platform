/**
 *******************************************************************************
 *  @file           filter_test_trace_key_names.c
 *  @brief          filter_test_trace の文字列キーの名前解決テーブルを定義します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/26
 *  @version        0.1.0
 *
 *  名前は列挙定数を文字列化して作ります。綴りを誤るとコンパイル エラーになります。\n
 *  件数とカタログの項目数の一致は、スロットの作成時に確認されます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#include "filter_test_trace_key_names.h"

#include "gen/filter_test_trace.h"

/** 列挙定数から、名前解決テーブルの 1 件を作ります。 */
#define FILTER_TEST_TRACE_KEY_NAME(key) {#key, (key), 0U}

/** 名前解決テーブルです。 */
static const cplat_string_catalog_filter_key_name s_key_names[] = {
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_WORKER_STARTED),
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_JOB_RECEIVED),
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_JOB_PROGRESS),
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_BUFFER_ALLOCATED),
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_JOB_FAILED),
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_COMMAND_RECEIVED),
    FILTER_TEST_TRACE_KEY_NAME(FILTER_TEST_TRACE_KEY_WORKER_STOPPED),
};

/* Doxygen コメントは、ヘッダーに記載 */

const cplat_string_catalog_filter_key_name *filter_test_trace_key_names(void)
{
    return s_key_names;
}

/* Doxygen コメントは、ヘッダーに記載 */

size_t filter_test_trace_key_name_count(void)
{
    return sizeof(s_key_names) / sizeof(s_key_names[0]);
}
