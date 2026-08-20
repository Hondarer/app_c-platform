/**
 *******************************************************************************
 *  @file           hashtable-required-size.c
 *  @brief          ハッシュ テーブル構築に必要なバッファー サイズを表示します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/18
 *  @version        1.0.0
 *
 *  capacity / key_size / record_size から @ref com_util_hashtable_required_size
 *  の結果(管理領域サイズとデータ領域サイズ)を、空白区切りで標準出力へ 1 行で出します。\n
 *  既定はテーブル横断のタイムスタンプのみです。\n
 *  `--record-timestamp` を付けるとレコード単位のタイムスタンプも持ち、管理領域が増加します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/argparser/argparser.h>
#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct hashtable_required_size_options
{
    int capacity;
    int key_size;
    int record_size;
    int need_help;
    int record_timestamp;
    int pad;
} hashtable_required_size_options;

static int register_options(hashtable_required_size_options *options)
{
    com_util_argparser_default_init(
        "Print the management-region and data-region buffer sizes required to construct a hash table.");
    (void)com_util_argparser_default_register_flag("-h", "--help", "show this help", &options->need_help);
    (void)com_util_argparser_default_register_option_int("-c", "--capacity", "N", "number of slots",
                                                         COM_UTIL_ARGPARSER_REQUIRED, &options->capacity);
    (void)com_util_argparser_default_register_option_int("-k", "--key-size", "N", "bytes per key",
                                                         COM_UTIL_ARGPARSER_REQUIRED, &options->key_size);
    (void)com_util_argparser_default_register_option_int("-r", "--record-size", "N", "bytes per value",
                                                         COM_UTIL_ARGPARSER_REQUIRED, &options->record_size);
    (void)com_util_argparser_default_register_flag(NULL, "--record-timestamp",
                                                   "include per-record timestamps in the management region",
                                                   &options->record_timestamp);
    if (com_util_argparser_default_get_register_error_count() > 0)
    {
        (void)com_util_argparser_default_print_register_error_messages(stderr);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    hashtable_required_size_options options = {0};
    com_util_hashtable_config config = {0};
    size_t mgmt_size = 0;
    size_t data_size = 0;
    int ret;

    if (register_options(&options) != 0)
    {
        return EXIT_FAILURE;
    }

    ret = com_util_argparser_default_parse(argc, argv);
    if (options.need_help != 0)
    {
        (void)com_util_argparser_default_print_usage(stdout);
        return EXIT_SUCCESS;
    }
    if (ret != COM_UTIL_OK)
    {
        (void)com_util_argparser_default_print_error_messages(stderr);
        (void)com_util_argparser_default_print_usage(stderr);
        return EXIT_FAILURE;
    }

    if ((options.capacity <= 0) || (options.key_size <= 0) || (options.record_size <= 0))
    {
        (void)fprintf(stderr, "capacity, key-size, and record-size must be positive.\n");
        return EXIT_FAILURE;
    }

    config.capacity = (size_t)options.capacity;
    config.key_type = COM_UTIL_HASHTABLE_KEY_STRING;
    config.timestamp_scope = (options.record_timestamp != 0) ? COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD
                                                            : COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE;
    config.key_size = (size_t)options.key_size;
    config.record_size = (size_t)options.record_size;
    config.lifetime = 2;

    ret = com_util_hashtable_required_size(&config, &mgmt_size, &data_size);
    if (ret != COM_UTIL_OK)
    {
        (void)fprintf(stderr, "failed to compute required size: %d\n", ret);
        return EXIT_FAILURE;
    }

    if (printf("%zu %zu\n", mgmt_size, data_size) < 0)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
