/**
 *******************************************************************************
 *  @file           string_catalog_format.c
 *  @brief          文字列の組み立てとメタデータ参照の公開 API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/10
 *  @version        1.0.0
 *
 *  公開 API は、引数スキーマの取得、可変長引数の取り出し、書式の展開の 3 つの処理段階を順に呼び出します。\n
 *  可変長引数を事前に値の配列へ格納するため、言語ごとの語順の違いは書式の展開処理のみで対応できます。
 *
 *  出力する言語はプロセスの設定から取得します。言語の保持は `string_catalog_language.c`、
 *  カタログの保持は `string_catalog_catalog.c` が担います。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "string_catalog.h"

#include <cplat/base/result.h>
#include <cplat/string_catalog/catalog_internal.h>
#include <cplat/string_catalog/string_catalog.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/** 引数種別が公開列挙の範囲内であることを確認します。 */
static bool is_valid_argument_kind(const cplat_string_catalog_argument_kind kind)
{
    return (kind >= CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING) &&
           (kind <= CPLAT_STRING_CATALOG_ARGUMENT_KIND_ERROR_CODE);
}

/** 先に定義された項目と key が重複していないことを確認します。 */
static bool is_unique_key(const cplat_string_catalog *const catalog, const int entry_index)
{
    const cplat_string_catalog_entry *entry = &catalog->entries[entry_index];
    int previous_index;

    for (previous_index = 0; previous_index < entry_index; previous_index++)
    {
        if ((catalog->entries[previous_index].key != NULL) &&
            (strcmp(entry->key, catalog->entries[previous_index].key) == 0))
        {
            return false;
        }
    }

    return true;
}

/**
 *  @brief          言語別リソースから、指定した言語の要素を選びます。
 *  @param[in]      localized 言語をインデックスとするリソースの配列。NULL を渡してはなりません。
 *  @param[in]      language  出力する言語。範囲内の値を渡してください。
 *  @return         使用するリソースです。ニュートラル言語の要素も未設定の場合は NULL を返します。
 *
 *  指定した言語のリソースが未設定の場合は、ニュートラル言語の要素を代替として使用します。\n
 *  翻訳が完了していない言語であっても、文字列を出力できます。
 */
static const char *select_localized(const char *const *localized, const cplat_string_catalog_language language)
{
    const char *text = localized[(unsigned int)language];

    if (text == NULL)
    {
        text = localized[CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL];
    }

    return text;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_vformat(const cplat_string_catalog *const catalog, char *dest, const size_t dest_size,
                                 const int string_id, va_list args)
{
    const cplat_string_catalog_entry *entry;
    const cplat_string_catalog_language language = cplat_string_catalog_get_language();
    const char *text;
    string_catalog_argument_value values[CPLAT_STRING_CATALOG_ARGUMENT_MAX] = {0};
    int ret;

    if ((dest == NULL) || (dest_size == 0U))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    dest[0] = '\0';

    if (!cplat_internal_string_catalog_is_usable(catalog))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    entry = cplat_internal_string_catalog_find_entry(catalog, string_id);
    if (entry == NULL)
    {
        return CPLAT_ERR_NOT_FOUND;
    }

    if ((entry->argument_count < 0) || (entry->argument_count > CPLAT_STRING_CATALOG_ARGUMENT_MAX) ||
        ((entry->argument_count > 0) && (entry->arguments == NULL)))
    {
        return CPLAT_ERR_MALFORMED_DEFINITION;
    }

    text = select_localized(entry->texts, language);
    if (text == NULL)
    {
        return CPLAT_ERR_MALFORMED_DEFINITION;
    }

    ret = string_catalog_collect_arguments(entry, args, values);
    if (ret != CPLAT_OK)
    {
        return ret;
    }

    return string_catalog_render_text(dest, dest_size, text, values, entry->argument_count);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_format(const cplat_string_catalog *const catalog, char *dest, const size_t dest_size,
                                const int string_id, ...)
{
    va_list args;
    int ret;

    va_start(args, string_id);
    ret = cplat_string_catalog_vformat(catalog, dest, dest_size, string_id, args);
    va_end(args);

    return ret;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_verify(const cplat_string_catalog *const catalog, int *string_id_out,
                                cplat_string_catalog_language *language_out)
{
    int entry_count;
    int entry_index;

    if (!cplat_internal_string_catalog_is_usable(catalog))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    entry_count = cplat_internal_string_catalog_entry_count(catalog);

    for (entry_index = 0; entry_index < entry_count; entry_index++)
    {
        const cplat_string_catalog_entry *entry;
        int language_index;
        int argument_index;

        entry = cplat_internal_string_catalog_entry_at(catalog, entry_index);

        /* 分類値はライブラリが解釈しないため、範囲は確認しない */
        if ((entry->argument_count < 0) || (entry->argument_count > CPLAT_STRING_CATALOG_ARGUMENT_MAX) ||
            ((entry->argument_count > 0) && (entry->arguments == NULL)) || (entry->key == NULL) ||
            (entry->brief == NULL) || !is_unique_key(catalog, entry_index) ||
            (cplat_internal_string_catalog_find_entry(catalog, entry->id) != entry))
        {
            if (string_id_out != NULL)
            {
                *string_id_out = entry->id;
            }
            if (language_out != NULL)
            {
                /* 引数個数とインデックス表の不正は言語に依存しないため、言語ではない値を格納する */
                *language_out = CPLAT_STRING_CATALOG_LANGUAGE_COUNT;
            }
            return CPLAT_ERR_MALFORMED_DEFINITION;
        }

        for (argument_index = 0; argument_index < entry->argument_count; argument_index++)
        {
            const cplat_string_catalog_argument *argument = &entry->arguments[argument_index];

            if (!is_valid_argument_kind(argument->kind) || (argument->name == NULL) ||
                (argument->description == NULL))
            {
                if (string_id_out != NULL)
                {
                    *string_id_out = entry->id;
                }
                if (language_out != NULL)
                {
                    *language_out = CPLAT_STRING_CATALOG_LANGUAGE_COUNT;
                }
                return CPLAT_ERR_MALFORMED_DEFINITION;
            }
        }

        for (language_index = 0; language_index < (int)CPLAT_STRING_CATALOG_LANGUAGE_COUNT; language_index++)
        {
            const char *text = select_localized(entry->texts, (cplat_string_catalog_language)language_index);
            int ret = CPLAT_OK;

            if ((text == NULL) ||
                (select_localized(entry->notes, (cplat_string_catalog_language)language_index) == NULL))
            {
                ret = CPLAT_ERR_MALFORMED_DEFINITION;
            }
            else
            {
                ret = string_catalog_validate_text(text, entry->argument_count);
            }

            if (ret != CPLAT_OK)
            {
                if (string_id_out != NULL)
                {
                    *string_id_out = entry->id;
                }
                if (language_out != NULL)
                {
                    *language_out = (cplat_string_catalog_language)language_index;
                }
                return ret;
            }
        }
    }

    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_get_category(const cplat_string_catalog *const catalog, const int string_id)
{
    const cplat_string_catalog_entry *entry;

    entry = cplat_internal_string_catalog_find_entry(catalog, string_id);
    if (entry == NULL)
    {
        return 0;
    }

    return entry->category;
}

/* Doxygen コメントは、ヘッダーに記載 */

const char *cplat_string_catalog_get_key(const cplat_string_catalog *const catalog, const int string_id)
{
    const cplat_string_catalog_entry *entry;

    entry = cplat_internal_string_catalog_find_entry(catalog, string_id);
    if (entry == NULL)
    {
        return NULL;
    }

    return entry->key;
}

/* Doxygen コメントは、ヘッダーに記載 */

const char *cplat_string_catalog_get_note(const cplat_string_catalog *const catalog, const int string_id)
{
    const cplat_string_catalog_entry *entry;

    entry = cplat_internal_string_catalog_find_entry(catalog, string_id);
    if (entry == NULL)
    {
        return NULL;
    }

    return select_localized(entry->notes, cplat_string_catalog_get_language());
}
