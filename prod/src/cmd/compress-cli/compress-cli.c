/**
 *******************************************************************************
 *  @file           compress-cli.c
 *  @brief          データを圧縮および展開するコマンドを実装します。
 *
 *  コマンド ライン引数を検証し、ファイル入出力と圧縮 API の結果を終了コードへ変換します。
 *
 *******************************************************************************
 */

#include "compress-cli.h"

#include <com_util/base/platform.h>
#include <com_util/compress/compress.h>
#include <com_util/console/console.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t compress_cli_read_u32_be(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static int compress_cli_read_file_fail(FILE *file, uint8_t *data)
{
    if (file != NULL)
    {
        (void)com_util_fclose(file);
    }
    free(data);
    return -1;
}

static int compress_cli_read_file(const char *path, uint8_t **data_out, size_t *size_out)
{
    FILE *file = NULL;
    uint8_t *data = NULL;
    int open_err = 0;
    int64_t file_size_i64;
    size_t file_size;
    size_t read_count = 0u;

    if (path == NULL || data_out == NULL || size_out == NULL)
    {
        return -1;
    }

    *data_out = NULL;
    *size_out = 0u;

    file = com_util_fopen(path, "rb", &open_err);
    if (file == NULL)
    {
        fprintf(stderr, "入力ファイルを開けません: %s (errno=%d)\n", path, open_err);
        return -1;
    }

    if (com_util_fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "入力ファイルのサイズ取得に失敗しました: %s\n", path);
        return compress_cli_read_file_fail(file, data);
    }

    file_size_i64 = com_util_ftell(file);
    if (file_size_i64 < 0)
    {
        fprintf(stderr, "入力ファイルのサイズ取得に失敗しました: %s\n", path);
        return compress_cli_read_file_fail(file, data);
    }
#if SIZE_MAX < INT64_MAX
    if (file_size_i64 > (int64_t)SIZE_MAX)
    {
        fprintf(stderr, "入力ファイルが大きすぎます: %s\n", path);
        return compress_cli_read_file_fail(file, data);
    }
#endif
    file_size = (size_t)file_size_i64;

    if (com_util_fseek(file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "入力ファイルの先頭へ戻せません: %s\n", path);
        return compress_cli_read_file_fail(file, data);
    }

    if (file_size > 0u)
    {
        data = (uint8_t *)malloc(file_size);
        if (data == NULL)
        {
            fprintf(stderr, "入力バッファの確保に失敗しました。\n");
            return compress_cli_read_file_fail(file, data);
        }

        read_count = com_util_fread(data, 1u, file_size, file);
        if (read_count != file_size)
        {
            fprintf(stderr, "入力ファイルの読み込みに失敗しました: %s\n", path);
            return compress_cli_read_file_fail(file, data);
        }
    }

    if (com_util_fclose(file) != 0)
    {
        file = NULL;
        fprintf(stderr, "入力ファイルのクローズに失敗しました: %s\n", path);
        return compress_cli_read_file_fail(file, data);
    }

    *data_out = data;
    *size_out = file_size;
    return 0;
}

static int compress_cli_write_file(const char *path, const uint8_t *data, size_t size)
{
    FILE *file = NULL;
    int open_err = 0;
    size_t written = 0u;
    int close_rc = 0;

    file = com_util_fopen(path, "wb", &open_err);
    if (file == NULL)
    {
        fprintf(stderr, "出力ファイルを開けません: %s (errno=%d)\n", path, open_err);
        return -1;
    }

    if (size > 0u)
    {
        written = com_util_fwrite(data, 1u, size, file);
        if (written != size)
        {
            fprintf(stderr, "出力ファイルの書き込みに失敗しました: %s\n", path);
            close_rc = com_util_fclose(file);
            if (close_rc != 0)
            {
                fprintf(stderr, "出力ファイルのクローズに失敗しました: %s\n", path);
            }
            (void)com_util_remove(path);
            return -1;
        }
    }

    if (com_util_fclose(file) != 0)
    {
        fprintf(stderr, "出力ファイルのクローズに失敗しました: %s\n", path);
        (void)com_util_remove(path);
        return -1;
    }

    return 0;
}

static int compress_cli_resolve_paths(const compress_cli_options *options, char *input_full, size_t input_full_size,
                                      char *output_full, size_t output_full_size)
{
    int err = 0;
    int path_cmp;

    path_cmp = com_util_paths_equal(options->input_path, options->output_path, &err);
    if (path_cmp < 0)
    {
        fprintf(stderr, "入力パスと出力パスの比較に失敗しました (errno=%d)\n", err);
        return -1;
    }

    if (path_cmp != 0)
    {
        fprintf(stderr, "入力ファイルと出力ファイルに同じパスは指定できません。\n");
        return -1;
    }

    err = 0;
    if (com_util_path_get_full(input_full, input_full_size, &err, options->input_path) != 0)
    {
        fprintf(stderr, "入力パスの正規化に失敗しました: %s (errno=%d)\n", options->input_path, err);
        return -1;
    }

    err = 0;
    if (com_util_path_get_full(output_full, output_full_size, &err, options->output_path) != 0)
    {
        fprintf(stderr, "出力パスの正規化に失敗しました: %s (errno=%d)\n", options->output_path, err);
        return -1;
    }

    return 0;
}

static int compress_cli_run_compress_return(uint8_t *input_data, uint8_t *compressed_data, int rc)
{
    free(compressed_data);
    free(input_data);
    return rc;
}

static int compress_cli_run_compress(const char *input_path, const char *output_path)
{
    uint8_t *input_data = NULL;
    uint8_t *compressed_data = NULL;
    size_t input_size = 0u;
    size_t compressed_capacity;
    size_t compressed_size;
    int rc = -1;

    if (compress_cli_read_file(input_path, &input_data, &input_size) != 0)
    {
        return -1;
    }

    if (input_size == 0u)
    {
        fprintf(stderr, "空ファイルは圧縮できません: %s\n", input_path);
        return compress_cli_run_compress_return(input_data, compressed_data, rc);
    }
    /* 圧縮ヘッダーで NBO 4 バイトとして元サイズを格納するため、コーディング規範の例外として UINT32_MAX 上限を維持する。 */
    if ((uint64_t)input_size > UINT32_MAX)
    {
        fprintf(stderr, "入力ファイルが大きすぎます: %s\n", input_path);
        return compress_cli_run_compress_return(input_data, compressed_data, rc);
    }
#if SIZE_MAX <= UINT32_MAX
    if (input_size > (SIZE_MAX - COM_UTIL_COMPRESS_HEADER_SIZE) / 2u)
    {
        fprintf(stderr, "圧縮出力バッファのサイズ計算に失敗しました。\n");
        return compress_cli_run_compress_return(input_data, compressed_data, rc);
    }
#endif

    compressed_capacity = input_size * 2u + COM_UTIL_COMPRESS_HEADER_SIZE;
    if (compressed_capacity < 256u)
    {
        compressed_capacity = 256u;
    }

    compressed_data = (uint8_t *)malloc(compressed_capacity);
    if (compressed_data == NULL)
    {
        fprintf(stderr, "圧縮バッファの確保に失敗しました。\n");
        return compress_cli_run_compress_return(input_data, compressed_data, rc);
    }

    compressed_size = compressed_capacity;
    if (com_util_compress(compressed_data, &compressed_size, input_data, input_size) != 0)
    {
        fprintf(stderr, "圧縮に失敗しました: %s\n", input_path);
        return compress_cli_run_compress_return(input_data, compressed_data, rc);
    }

    if (compress_cli_write_file(output_path, compressed_data, compressed_size) != 0)
    {
        return compress_cli_run_compress_return(input_data, compressed_data, rc);
    }

    return compress_cli_run_compress_return(input_data, compressed_data, 0);
}

static int compress_cli_run_decompress_return(uint8_t *input_data, uint8_t *decompressed_data, int rc)
{
    free(decompressed_data);
    free(input_data);
    return rc;
}

static int compress_cli_run_decompress(const char *input_path, const char *output_path)
{
    uint8_t *input_data = NULL;
    uint8_t *decompressed_data = NULL;
    size_t input_size = 0u;
    size_t decompressed_size;
    uint32_t expected_size;
    int rc = -1;

    if (compress_cli_read_file(input_path, &input_data, &input_size) != 0)
    {
        return -1;
    }

    if (input_size <= COM_UTIL_COMPRESS_HEADER_SIZE)
    {
        fprintf(stderr, "展開入力が不正です。ヘッダのみ、または空です: %s\n", input_path);
        return compress_cli_run_decompress_return(input_data, decompressed_data, rc);
    }

    expected_size = compress_cli_read_u32_be(input_data);
    if (expected_size <= COM_UTIL_COMPRESS_HEADER_SIZE)
    {
        fprintf(stderr, "展開入力が不正です。ヘッダの元サイズが小さすぎます: %s\n", input_path);
        return compress_cli_run_decompress_return(input_data, decompressed_data, rc);
    }

    decompressed_data = (uint8_t *)malloc((size_t)expected_size);
    if (decompressed_data == NULL)
    {
        fprintf(stderr, "展開バッファの確保に失敗しました。\n");
        return compress_cli_run_decompress_return(input_data, decompressed_data, rc);
    }

    decompressed_size = (size_t)expected_size;
    if (com_util_decompress(decompressed_data, &decompressed_size, input_data, input_size) != 0)
    {
        fprintf(stderr, "展開に失敗しました: %s\n", input_path);
        return compress_cli_run_decompress_return(input_data, decompressed_data, rc);
    }

    if (decompressed_size != (size_t)expected_size)
    {
        fprintf(stderr, "展開後サイズがヘッダ値と一致しません: %s\n", input_path);
        return compress_cli_run_decompress_return(input_data, decompressed_data, rc);
    }

    if (compress_cli_write_file(output_path, decompressed_data, decompressed_size) != 0)
    {
        return compress_cli_run_decompress_return(input_data, decompressed_data, rc);
    }

    return compress_cli_run_decompress_return(input_data, decompressed_data, 0);
}

void compress_cli_options_init(compress_cli_options *options)
{
    if (options == NULL)
    {
        return;
    }

    options->mode = COMPRESS_CLI_MODE_NONE;
    options->input_path = NULL;
    options->output_path = NULL;
}

int compress_cli_parse_args(int argc, char *argv[], compress_cli_options *options)
{
    if (argc != 4 || argv == NULL || options == NULL)
    {
        return -1;
    }

    compress_cli_options_init(options);

    if (strcmp(argv[1], "--compress") == 0)
    {
        options->mode = COMPRESS_CLI_MODE_COMPRESS;
    }
    else if (strcmp(argv[1], "--decompress") == 0)
    {
        options->mode = COMPRESS_CLI_MODE_DECOMPRESS;
    }
    else
    {
        return -1;
    }

    options->input_path = argv[2];
    options->output_path = argv[3];

    if (options->input_path == NULL || options->output_path == NULL || options->input_path[0] == '\0' ||
        options->output_path[0] == '\0')
    {
        return -1;
    }

    return 0;
}

void compress_cli_print_usage(const char *argv0)
{
    const char *name;
    if (argv0 != NULL)
    {
        name = argv0;
    }
    else
    {
        name = "compress-cli";
    }

    fprintf(stderr, "使用方法: %s --compress <input> <output>\n", name);
    fprintf(stderr, "          %s --decompress <input> <output>\n", name);
}

int main(int argc, char *argv[])
{
    compress_cli_options options;
    char input_full[PLATFORM_PATH_MAX];
    char output_full[PLATFORM_PATH_MAX];

    com_util_console_init();

    if (compress_cli_parse_args(argc, argv, &options) != 0)
    {
        const char *print_usage_arg;
        if (argc > 0)
        {
            print_usage_arg = argv[0];
        }
        else
        {
            print_usage_arg = "compress-cli";
        }
        compress_cli_print_usage(print_usage_arg);
        return EXIT_FAILURE;
    }

    if (compress_cli_resolve_paths(&options, input_full, sizeof(input_full), output_full, sizeof(output_full)) != 0)
    {
        return EXIT_FAILURE;
    }

    if (options.mode == COMPRESS_CLI_MODE_COMPRESS)
    {
        if (compress_cli_run_compress(input_full, output_full) == 0)
        {
            return EXIT_SUCCESS;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }

    if (options.mode == COMPRESS_CLI_MODE_DECOMPRESS)
    {
        if (compress_cli_run_decompress(input_full, output_full) == 0)
        {
            return EXIT_SUCCESS;
        }
        else
        {
            return EXIT_FAILURE;
        }
    }

    const char *print_usage_arg;
    if (argc > 0)
    {
        print_usage_arg = argv[0];
    }
    else
    {
        print_usage_arg = "compress-cli";
    }
    compress_cli_print_usage(print_usage_arg);
    return EXIT_FAILURE;
}
