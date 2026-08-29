/**
 *******************************************************************************
 *  @file           hashtable_key.c
 *  @brief          キーのハッシュ値と比較、および入力フィールドの寸法を求めます。
 *  @author         Tetsuo Honda
 *  @date           2026/08/29
 *  @version        1.0.0
 *
 *  バケット番号の算出、格納済みキーとの一致判定、キーが設定に収まるかの判定を担います。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "hashtable.h"

#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

size_t hashtable_hash_key(const cplat_hashtable *ht, const void *key)
{
    uint64_t hash = 5381;
    const unsigned char *p = (const unsigned char *)key;

    if (ht->hdr->config.key_type != CPLAT_HASHTABLE_FIELD_FIXED_BINARY)
    {
        int c;

        /* 文字列は先頭の NUL までだけを混ぜ、残り 0 埋めはハッシュに入れません。 */
        while ((c = *p++) != 0)
        {
            hash = ((hash << 5) + hash) + (uint64_t)c;
        }
    }
    else
    {
        size_t i;

        for (i = 0; i < ht->hdr->config.key_size; i++)
        {
            hash = ((hash << 5) + hash) + p[i];
        }
    }
    return (size_t)hash % ht->hdr->config.capacity;
}

/* Doxygen コメントは、ヘッダーに記載 */

int hashtable_key_equal(const cplat_hashtable *ht, const char *stored_key, const void *key)
{
    if (ht->hdr->config.key_type != CPLAT_HASHTABLE_FIELD_FIXED_BINARY)
    {
        return strcmp(stored_key, (const char *)key) == 0;
    }
    return memcmp(stored_key, key, ht->hdr->config.key_size) == 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int hashtable_key_fits(const cplat_hashtable *ht, const void *key)
{
    if (ht->hdr->config.key_type == CPLAT_HASHTABLE_FIELD_FIXED_BINARY)
    {
        return 1;
    }
    if (ht->hdr->config.key_type == CPLAT_HASHTABLE_FIELD_VARIABLE_STRING)
    {
        return strlen((const char *)key) < ht->hdr->config.key_storage_size;
    }
    return memchr(key, '\0', ht->hdr->config.key_size) != NULL;
}

/* Doxygen コメントは、ヘッダーに記載 */

size_t hashtable_field_input_size(cplat_hashtable_field_type type, size_t fixed_size, const void *value)
{
    if (type == CPLAT_HASHTABLE_FIELD_VARIABLE_STRING)
    {
        return strlen((const char *)value) + 1u;
    }
    return fixed_size;
}

/* Doxygen コメントは、ヘッダーに記載 */

int hashtable_fixed_string_fits(cplat_hashtable_field_type type, size_t fixed_size, const void *value)
{
    return (type != CPLAT_HASHTABLE_FIELD_FIXED_STRING) || (memchr(value, '\0', fixed_size) != NULL);
}
