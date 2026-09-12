/**
 *******************************************************************************
 *  @file           fake_catalog.c
 *  @brief          テストが内容を差し替えられるカタログを提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/10
 *  @version        1.0.0
 *
 *  製品のカタログの代わりに渡し、定義が壊れた場合の経路へ到達させます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "fake_catalog.h"

#include <cplat/string_catalog/string_catalog.h>
#include <stddef.h>

/** 偽のカタログが保持する定義です。テストから書き換えます。 */
static cplat_string_catalog_entry s_entries[FAKE_CATALOG_ENTRY_COUNT];

/** 偽のカタログのカタログ識別オブジェクトです。インデックス表を持たず、線形探索の経路を使います。 */
static const cplat_string_catalog s_catalog = {s_entries, NULL, FAKE_CATALOG_ENTRY_COUNT, 0};

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_reset(void)
{
    static const cplat_string_catalog_entry initial_entries[FAKE_CATALOG_ENTRY_COUNT] = {
        {FAKE_CATALOG_ID_NO_ARGUMENT,
         3, /* 分類値。ライブラリは解釈しない */
         0,
         0,
         {0},
         "FAKE_ID_0001",
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "started",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "開始しました。"},
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "no argument",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "引数を取らない文字列です。"}},
        {FAKE_CATALOG_ID_TWO_ARGUMENTS,
         1, /* 分類値。ライブラリは解釈しない */
         2,
         0,
         {CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING, CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT32},
         "FAKE_ID_0002",
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "file {0} number {1}",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "ファイル {0} 番号 {1}",
          [CPLAT_STRING_CATALOG_LANGUAGE_ENGLISH] = "number {1} of {0}"},
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "reordered",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "英語の書式が位置指定を入れ替えます。"}},
        {FAKE_CATALOG_ID_ONE_ARGUMENT,
         2, /* 分類値。ライブラリは解釈しない */
         1,
         0,
         {CPLAT_STRING_CATALOG_ARGUMENT_KIND_SIZE},
         "FAKE_ID_0004",
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "limit {0}", [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "上限 {0}"},
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = ""}}};
    int index;

    for (index = 0; index < FAKE_CATALOG_ENTRY_COUNT; index++)
    {
        s_entries[index] = initial_entries[index];
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

const cplat_string_catalog *fake_catalog(void)
{
    return &s_catalog;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_id(const int index, const int string_id)
{
    s_entries[index].id = string_id;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_category(const int index, const int category)
{
    s_entries[index].category = category;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_argument_count(const int index, const int count)
{
    s_entries[index].argument_count = count;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_argument_kind(const int index, const int argument_index,
                                    const cplat_string_catalog_argument_kind kind)
{
    s_entries[index].arguments[argument_index] = kind;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_note(const int index, const cplat_string_catalog_language language, const char *note)
{
    s_entries[index].notes[(unsigned int)language] = note;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_text(const int index, const cplat_string_catalog_language language, const char *text)
{
    s_entries[index].texts[(unsigned int)language] = text;
}
