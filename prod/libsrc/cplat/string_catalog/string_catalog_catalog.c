/**
 *******************************************************************************
 *  @file           string_catalog_catalog.c
 *  @brief          カタログの検索を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/10
 *  @version        1.0.0
 *
 *  カタログはライブラリ側では保持せず、利用側で用意します。\n
 *  ライブラリが規定するのは、言語、引数種別、分類、書式の構文です。\n
 *  利用側で用意するのは、文字列 ID の列挙とカタログ項目の配列です。
 *
 *  本ファイルは内部状態を持ちません。呼び出し元から渡されたカタログのみを参照します。\n
 *  1 つのプロセスで複数のカタログを扱え、カタログ同士は互いに独立しています。
 *
 *  カタログは利用側で静的初期化されるため、関数の入口で構造の妥当性を確認します。\n
 *  文字列 ID からカタログ項目を検索する際は、インデックス表が指定されていればインデックス参照を、指定がなければ線形探索を使用します。\n
 *  インデックス表の内容も利用側で用意されるため、参照前に有効範囲内であることを確認します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <cplat/string_catalog/catalog_internal.h>
#include <cplat/string_catalog/string_catalog.h>
#include <stdbool.h>
#include <stddef.h>

/* Doxygen コメントは、ヘッダーに記載 */

bool cplat_internal_string_catalog_is_usable(const cplat_string_catalog *const catalog)
{
    if (catalog == NULL)
    {
        return false;
    }

    if ((catalog->entries == NULL) || (catalog->entry_count < 0) || (catalog->id_index_count < 0))
    {
        return false;
    }

    return true;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_internal_string_catalog_entry_count(const cplat_string_catalog *const catalog)
{
    if (!cplat_internal_string_catalog_is_usable(catalog))
    {
        return 0;
    }

    return catalog->entry_count;
}

/* Doxygen コメントは、ヘッダーに記載 */

const cplat_string_catalog_entry *cplat_internal_string_catalog_entry_at(const cplat_string_catalog *const catalog,
                                                                         const int index)
{
    if (!cplat_internal_string_catalog_is_usable(catalog))
    {
        return NULL;
    }

    if ((index < 0) || (index >= catalog->entry_count))
    {
        return NULL;
    }

    return &catalog->entries[index];
}

/* Doxygen コメントは、ヘッダーに記載 */

const cplat_string_catalog_entry *cplat_internal_string_catalog_find_entry(const cplat_string_catalog *const catalog,
                                                                           const int string_id)
{
    int index;

    if (!cplat_internal_string_catalog_is_usable(catalog))
    {
        return NULL;
    }

    if ((catalog->id_index != NULL) && (string_id >= 0) && (string_id < catalog->id_index_count))
    {
        /* インデックス表の内容は利用側で用意されるため、参照前に有効範囲内であることを確認する */
        const int entry_index = catalog->id_index[string_id];

        if ((entry_index < 0) || (entry_index >= catalog->entry_count))
        {
            return NULL;
        }

        return &catalog->entries[entry_index];
    }

    for (index = 0; index < catalog->entry_count; index++)
    {
        if (catalog->entries[index].id == string_id)
        {
            return &catalog->entries[index];
        }
    }

    return NULL;
}
