/**
 *******************************************************************************
 *  @file           argparser-sample.c
 *  @brief          com_util_argparser の動作確認用サンプル コマンドを実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/12
 *  @version        1.0.0
 *
 *  フラグ、値付きオプション、複数値オプション、位置引数を登録し、
 *  解析結果を標準出力へ出力します。\n
 *  解析エラー時はエラー メッセージと usage を標準エラー出力へ出力して EXIT_FAILURE で終了します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/argparser/argparser.h>
#include <com_util/console/console.h>
#include <stdio.h>
#include <stdlib.h>

/* 複数値オプション (--include) の最大出現数 */
#define ARGPARSER_SAMPLE_INCLUDE_MAX (4)

/* 解析結果の格納先 */
typedef struct argparser_sample_options
{
    const char *name_value;
    const char *includes[ARGPARSER_SAMPLE_INCLUDE_MAX];
    const char *input_path;
    const char *output_path;
    size_t include_count;
    int need_help;
    int verbose_count;
    int count_value;
    int pad; /* 明示パディング (-Wpadded 対応) */
} argparser_sample_options;

/**
 *  @brief          オプションをパーサーへ登録します。
 *  @param[out]     options  解析結果の格納先。
 *  @return         成功時は 0、登録に失敗した場合は -1 を返します。
 */
static int register_argparser(argparser_sample_options *options)
{
    com_util_argparser_register_flag("-h", "--help", "show this help", &options->need_help);
    com_util_argparser_register_flag("-v", "--verbose", "increase verbosity", &options->verbose_count);
    com_util_argparser_register_option_int("-c", "--count", "N", "repeat count", 0, &options->count_value);
    com_util_argparser_register_option_string("-n", "--name", "NAME", "display name", 0, &options->name_value);
    com_util_argparser_register_option_string_array("-i", "--include", "DIR", "include directory", 0, options->includes,
                                                    ARGPARSER_SAMPLE_INCLUDE_MAX, &options->include_count);
    com_util_argparser_register_positional_string("input", "input file", COM_UTIL_ARGPARSER_REQUIRED,
                                                  &options->input_path);
    com_util_argparser_register_positional_string("output", "output file", 0, &options->output_path);

    if (com_util_argparser_get_register_error_count() > 0)
    { /* オプションの登録に失敗した場合 (コーディング エラーの場合) */
        com_util_argparser_print_register_error_messages(stderr);
        return -1;
    }

    return 0;
}

/**
 *  @brief          解析結果を標準出力へ表示します。
 *  @param[in]      options  表示する解析結果。
 */
static void print_result(const argparser_sample_options *options)
{
    printf("verbose: %d\n", options->verbose_count);
    printf("count: %d\n", options->count_value);

    const char *name = options->name_value;
    if (name == NULL)
    {
        name = "(none)";
    }
    printf("name: %s\n", name);

    printf("includes: %zu\n", options->include_count);
    for (size_t i = 0; i < options->include_count; i++)
    {
        printf("  include[%zu]: %s\n", i, options->includes[i]);
    }

    printf("input: %s\n", options->input_path);

    const char *output = options->output_path;
    if (output == NULL)
    {
        output = "(none)";
    }
    printf("output: %s\n", output);
}

int main(int argc, char *argv[])
{
    com_util_console_init();

    argparser_sample_options options = {0};
    options.count_value = 1; /* 0 以外の既定値は解析前に設定する */

    com_util_argparser_init(argc, argv, "com_util_argparser sample");

    if (register_argparser(&options) != 0)
    {
        return EXIT_FAILURE;
    }

    int parse_result = com_util_argparser_parse();

    if (options.need_help != 0)
    {
        /* 必須引数が省略されていても -h, --help を優先する */
        com_util_argparser_print_usage(stdout);
        return EXIT_SUCCESS;
    }

    if (parse_result != COM_UTIL_OK)
    {
        com_util_argparser_print_error_messages(stderr);
        com_util_argparser_print_usage(stderr);
        return EXIT_FAILURE;
    }

    print_result(&options);

    return EXIT_SUCCESS;
}
