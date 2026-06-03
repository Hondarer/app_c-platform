/**
 *******************************************************************************
 *  @file           sym_loader_init.c
 *  @brief          設定テキストファイルから com_util_sym_loader_entry エントリを読み込む。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  テキストファイルから func_key / lib_name / func_name を
 *  読み込み、_func_objects 配列の対応エントリに設定します。\n
 *
 *  ファイルフォーマット:\n
 *  @code
    # コメント行
    func_key  lib_name  func_name   # 行末コメント
 *  @endcode
 *
 *  - '#' で始まる行はコメント行として無視する。\n
 *  - 行中の '#' 以降を行末コメントとして切り捨てる。\n
 *  - com_util_sscanf で func_key / lib_name / func_name の 3 フィールドを解析する。\n
 *  - func_key が一致するキャッシュエントリの lib_name / func_name 配列に
 *    com_util_strncpy で書き込む。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/crt/stdio.h>
#include <com_util/crt/string.h>
#include <com_util/runtime/sym_loader.h>
#include <string.h>

/** fgets で読み込む行バッファの最大長 */
#define CONFIG_LINE_MAX 1024

#define STR_IMPL(x) #x
#define STR(x)      STR_IMPL(x)

#define SYMBOL_LOADER_NAME_WIDTH 255
_Static_assert(SYMBOL_LOADER_NAME_WIDTH == COM_UTIL_SYM_LOADER_NAME_MAX - 1,
               "SYMBOL_LOADER_NAME_WIDTH must be COM_UTIL_SYM_LOADER_NAME_MAX - 1");
#define CONFIG_SCAN_FIELD(width) "%" STR(width) "s"
#define CONFIG_SCAN_NAME         CONFIG_SCAN_FIELD(SYMBOL_LOADER_NAME_WIDTH)

#define CONFIG_SCAN_FMT CONFIG_SCAN_NAME " " CONFIG_SCAN_NAME " " CONFIG_SCAN_NAME

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_sym_loader_init(com_util_sym_loader_entry *const *fobj_array,
                                                           const size_t fobj_length, const char *configpath)
{
    FILE *fp;
    char line[CONFIG_LINE_MAX];
    char func_key[COM_UTIL_SYM_LOADER_NAME_MAX];
    char lib_name[COM_UTIL_SYM_LOADER_NAME_MAX];
    char func_name[COM_UTIL_SYM_LOADER_NAME_MAX];
    char *comment;
    size_t fobj_index;

    fp = com_util_fopen(configpath, "r", NULL);
    if (fp == NULL)
    {
        return;
    }

    while (com_util_fgets(line, sizeof(line), fp) != NULL)
    {
        /* '#' 以降をコメントとして切り捨てる (行頭 '#' も同様に処理される) */
        comment = strchr(line, '#');
        if (comment != NULL)
        {
            *comment = '\0';
        }

        /* func_key lib_name func_name の 3 フィールドを解析 */
        if (com_util_sscanf(line, CONFIG_SCAN_FMT, func_key, lib_name, func_name) != 3)
        {
            /* 空行・コメント行・フィールドが不足している行はスキップ */
            continue;
        }

        /* func_key が一致するキャッシュを検索し、配列に書き込む */
        for (fobj_index = 0; fobj_index < fobj_length; fobj_index++)
        {
            com_util_sym_loader_entry *cache = fobj_array[fobj_index];
            if (cache->func_key == NULL || strcmp(cache->func_key, func_key) != 0)
            {
                continue;
            }

            (void)com_util_strncpy(cache->lib_name, COM_UTIL_SYM_LOADER_NAME_MAX, lib_name, SYMBOL_LOADER_NAME_WIDTH);
            (void)com_util_strncpy(cache->func_name, COM_UTIL_SYM_LOADER_NAME_MAX, func_name, SYMBOL_LOADER_NAME_WIDTH);
            break;
        }
    }

    com_util_fclose(fp);
    return;
}
