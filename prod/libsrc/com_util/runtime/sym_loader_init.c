/**
 *******************************************************************************
 *  @file           sym_loader_init.c
 *  @brief          JSON 設定ファイルから com_util_sym_loader_entry エントリを読み込みます。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  JSON ファイルから func_key / lib / func を読み込み、fobj_array 配列の
 *  対応エントリに設定します。\n
 *
 *  ファイル フォーマット:\n
    @code
    // 行コメントと C 形式のブロック コメントを利用できる
    {
      "func_key": {
        "lib": "lib_name",
        "func": "func_name"
      }
    }
    @endcode
 *
 *  - ルートは JSON object であること。\n
 *  - 各プロパティ名が func_key に対応する。\n
 *  - 値は object で、文字列フィールド "lib" と "func" を持つ。\n
 *  - 行コメント (//) および C 形式のブロック コメントは
 *    cJSON_Minify により解析前に除去する。\n
 *  - func_key が一致するキャッシュ エントリの lib_name / func_name 配列に
 *    com_util_strncpy で書き込みます。\n
 *  - ファイル未存在、読取失敗、JSON 解析失敗、必須フィールド欠落は黙って無視する。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/crt/stdio.h>
#include <com_util/crt/string.h>
#include <com_util/runtime/sym_loader.h>
#include <cJSON.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** 設定ファイル全体の最大バイト数 (NUL 終端を含まない)。 */
#define CONFIG_FILE_MAX ((int64_t)(1024 * 1024))

#define SYMBOL_LOADER_NAME_WIDTH 255
_Static_assert(SYMBOL_LOADER_NAME_WIDTH == COM_UTIL_SYM_LOADER_NAME_MAX - 1,
               "SYMBOL_LOADER_NAME_WIDTH must be COM_UTIL_SYM_LOADER_NAME_MAX - 1");

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_sym_loader_init(com_util_sym_loader_entry *const *fobj_array, const size_t fobj_length,
                              const char *configpath)
{
    FILE *fp;
    char *buffer;
    cJSON *root;
    int64_t size;
    size_t file_size;
    size_t read_count;
    const cJSON *entry;

    if (configpath == NULL || configpath[0] == '\0')
    {
        return;
    }

    fp = com_util_fopen(configpath, "rb", NULL);
    if (fp == NULL)
    {
        return;
    }

    buffer = NULL;
    root = NULL;

    if (com_util_fseek(fp, 0, SEEK_END) != 0)
    {
        goto out_close;
    }

    size = com_util_ftell(fp);
    if ((size <= 0) || (size > CONFIG_FILE_MAX))
    {
        goto out_close;
    }

    if (com_util_fseek(fp, 0, SEEK_SET) != 0)
    {
        goto out_close;
    }

    file_size = (size_t)size;
    buffer = (char *)malloc(file_size + 1u);
    if (buffer == NULL)
    {
        goto out_close;
    }

    read_count = com_util_fread(buffer, 1u, file_size, fp, NULL);
    if (read_count != file_size)
    {
        goto out_free_buffer;
    }
    buffer[file_size] = '\0';

    /* 行コメントと C 形式ブロック コメントを除去する (JSONC 相当)。バッファーを破壊的に短縮する。 */
    cJSON_Minify(buffer);

    root = cJSON_Parse(buffer);
    if ((root == NULL) || (cJSON_IsObject(root) == 0))
    {
        goto out_free_json;
    }

    for (entry = root->child; entry != NULL; entry = entry->next)
    {
        const cJSON *lib_item;
        const cJSON *func_item;
        const char *func_key;
        const char *lib_name;
        const char *func_name;
        size_t lib_len;
        size_t func_len;
        size_t fobj_index;

        if (cJSON_IsObject(entry) == 0)
        {
            continue;
        }

        func_key = entry->string;
        /* cJSON の object 子要素ではプロパティ名 string が必ず設定される。 */
        if (func_key[0] == '\0')
        {
            continue;
        }

        lib_item = cJSON_GetObjectItemCaseSensitive(entry, "lib");
        func_item = cJSON_GetObjectItemCaseSensitive(entry, "func");
        if ((cJSON_IsString(lib_item) == 0) || (cJSON_IsString(func_item) == 0))
        {
            continue;
        }

        lib_name = cJSON_GetStringValue(lib_item);
        func_name = cJSON_GetStringValue(func_item);
        if ((lib_name == NULL) || (func_name == NULL))
        {
            continue;
        }

        lib_len = strlen(lib_name);
        func_len = strlen(func_name);
        if ((lib_len == 0u) || (func_len == 0u))
        {
            continue;
        }
        if ((lib_len > (size_t)SYMBOL_LOADER_NAME_WIDTH) || (func_len > (size_t)SYMBOL_LOADER_NAME_WIDTH))
        {
            continue;
        }

        if (fobj_array == NULL)
        {
            continue;
        }

        for (fobj_index = 0; fobj_index < fobj_length; fobj_index++)
        {
            com_util_sym_loader_entry *cache = fobj_array[fobj_index];

            if ((cache == NULL) || (cache->func_key == NULL))
            {
                continue;
            }
            if (strcmp(cache->func_key, func_key) != 0)
            {
                continue;
            }

            (void)com_util_strncpy(cache->lib_name, COM_UTIL_SYM_LOADER_NAME_MAX, lib_name, SYMBOL_LOADER_NAME_WIDTH);
            (void)com_util_strncpy(cache->func_name, COM_UTIL_SYM_LOADER_NAME_MAX, func_name, SYMBOL_LOADER_NAME_WIDTH);
            break;
        }
    }

out_free_json:
    cJSON_Delete(root);
    root = NULL;
out_free_buffer:
    free(buffer);
    buffer = NULL;
out_close:
    (void)com_util_fclose(fp, NULL);
    return;
}
