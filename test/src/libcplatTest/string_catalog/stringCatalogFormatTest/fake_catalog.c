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

/** 2 つの引数を持つ偽のカタログ項目の引数定義です。 */
static cplat_string_catalog_argument s_two_arguments[2];

/** 1 つの引数を持つ偽のカタログ項目の引数定義です。 */
static cplat_string_catalog_argument s_one_argument[1];

/** 偽のカタログのカタログ識別オブジェクトです。インデックス表を持たず、線形探索の経路を使います。 */
static const cplat_string_catalog s_catalog = {s_entries, NULL, FAKE_CATALOG_ENTRY_COUNT, 0};

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_reset(void)
{
    static const cplat_string_catalog_argument initial_two_arguments[2] = {
        {CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING, 0, "path", "ファイルのパス。"},
        {CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT32, 0, "number", "番号。"}};
    static const cplat_string_catalog_argument initial_one_argument[1] = {
        {CPLAT_STRING_CATALOG_ARGUMENT_KIND_SIZE, 0, "limit", "上限。"}};
    static const cplat_string_catalog_entry initial_entries[FAKE_CATALOG_ENTRY_COUNT] = {
        {FAKE_CATALOG_KEY_NO_ARGUMENT,
         3, /* 分類値。ライブラリは解釈しない */
         0,
         0,
         NULL,
         "FAKE_ID_0001",
         "引数なし",
         "引数を取らない文字列です。",
         NULL,
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "started",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "開始しました。"},
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "no argument",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "引数を取らない文字列です。"}},
        {FAKE_CATALOG_KEY_TWO_ARGUMENTS,
         1, /* 分類値。ライブラリは解釈しない */
         2,
         0,
         s_two_arguments,
         "FAKE_ID_0002",
         "2 引数",
         "2 つの引数を取る文字列です。",
         NULL,
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "file {0} number {1}",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "ファイル {0} 番号 {1}",
          [CPLAT_STRING_CATALOG_LANGUAGE_ENGLISH] = "number {1} of {0}"},
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "reordered",
          [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "英語の書式が位置指定を入れ替えます。"}},
        {FAKE_CATALOG_KEY_ONE_ARGUMENT,
         2, /* 分類値。ライブラリは解釈しない */
         1,
         0,
         s_one_argument,
         "FAKE_ID_0004",
         "1 引数",
         "1 つの引数を取る文字列です。",
         NULL,
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = "limit {0}", [CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE] = "上限 {0}"},
         {[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL] = ""}}};
    int index;

    s_two_arguments[0] = initial_two_arguments[0];
    s_two_arguments[1] = initial_two_arguments[1];
    s_one_argument[0] = initial_one_argument[0];

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

void fake_catalog_set_key(const int index, const int string_key)
{
    s_entries[index].key = string_key;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_id(const int index, const char *const id)
{
    s_entries[index].id = id;
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
    if (index == FAKE_CATALOG_INDEX_TWO_ARGUMENTS)
    {
        s_two_arguments[argument_index].kind = kind;
    }
    else if (index == FAKE_CATALOG_INDEX_ONE_ARGUMENT)
    {
        s_one_argument[argument_index].kind = kind;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_brief(const int index, const char *const brief)
{
    s_entries[index].brief = brief;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_details(const int index, const char *const details)
{
    s_entries[index].details = details;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_arguments(const int index, const cplat_string_catalog_argument *const arguments)
{
    s_entries[index].arguments = arguments;
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_argument_name(const int index, const int argument_index, const char *const name)
{
    if (index == FAKE_CATALOG_INDEX_TWO_ARGUMENTS)
    {
        s_two_arguments[argument_index].name = name;
    }
    else if (index == FAKE_CATALOG_INDEX_ONE_ARGUMENT)
    {
        s_one_argument[argument_index].name = name;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void fake_catalog_set_argument_description(const int index, const int argument_index, const char *const description)
{
    if (index == FAKE_CATALOG_INDEX_TWO_ARGUMENTS)
    {
        s_two_arguments[argument_index].description = description;
    }
    else if (index == FAKE_CATALOG_INDEX_ONE_ARGUMENT)
    {
        s_one_argument[argument_index].description = description;
    }
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
