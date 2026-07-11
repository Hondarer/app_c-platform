/**
 *******************************************************************************
 *  @file           argparser-sample.c
 *  @brief          com_util_argparser の動作確認用サンプル コマンドを実装します。
 *
 *  フラグ、値付きオプション、複数値オプション、位置引数を登録し、
 *  解析結果を標準出力へ表示します。\n
 *  解析エラー時はエラー メッセージと usage を標準エラーへ表示して 1 で終了します。
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
    int help_flag;
    int verbose_count;
    int count_value;
    int pad; /* 明示パディング (-Wpadded 対応) */
} argparser_sample_options;

/**
 *  @brief          サンプルの全オプションをパーサーへ登録します。
 *  @param[in]      parser   登録先のパーサー ハンドル。
 *  @param[out]     options  解析結果の格納先。
 *  @return         成功時は 0、登録に失敗した場合は -1 を返します。
 */
static int argparser_sample_regist(com_util_argparser *parser, argparser_sample_options *options)
{
    com_util_argparser_result_t results[7];

    results[0] = com_util_argparser_register_flag(parser, "-h", "--help", "show this help", &options->help_flag);
    results[1] =
        com_util_argparser_register_flag(parser, "-v", "--verbose", "increase verbosity", &options->verbose_count);
    results[2] =
        com_util_argparser_register_option_int(parser, "-c", "--count", "N", "repeat count", 0, &options->count_value);
    results[3] = com_util_argparser_register_option_string(parser, "-n", "--name", "NAME", "display name", 0,
                                                           &options->name_value);
    results[4] = com_util_argparser_register_option_string_array(parser, "-i", "--include", "DIR", "include directory",
                                                                 0, options->includes, ARGPARSER_SAMPLE_INCLUDE_MAX,
                                                                 &options->include_count);
    results[5] = com_util_argparser_register_positional_string(parser, "input", "input file",
                                                               COM_UTIL_ARGPARSER_REQUIRED, &options->input_path);
    results[6] =
        com_util_argparser_register_positional_string(parser, "output", "output file", 0, &options->output_path);

    for (size_t i = 0; i < (sizeof(results) / sizeof(results[0])); i++)
    {
        if (results[i] != COM_UTIL_ARGPARSER_OK)
        {
            fprintf(stderr, "failed to register option (index=%zu, result=%d)\n", i, (int)results[i]);
            return -1;
        }
    }

    return 0;
}

/**
 *  @brief          解析結果を標準出力へ表示します。
 *  @param[in]      options  表示する解析結果。
 */
static void argparser_sample_print_result(const argparser_sample_options *options)
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
    options.count_value = 1; /* 既定値は解析前に設定する */

    com_util_argparser_options create_options = {0};
    create_options.program_description = "com_util_argparser sample";

    /* プロセス共有のデフォルト ハンドルを使用する。プロセス正常終了時に自動解放されるため、
       明示的な com_util_argparser_dispose() の呼び出しは不要。 */
    com_util_argparser *parser = com_util_argparser_default(&create_options);
    if (parser == NULL)
    {
        fprintf(stderr, "failed to create parser\n");
        return 1;
    }

    if (argparser_sample_regist(parser, &options) != 0)
    {
        return 1;
    }

    com_util_argparser_result_t result = com_util_argparser_parse(parser, argc, argv);

    if (options.help_flag != 0)
    {
        /* 必須引数が省略されていても --help を優先する */
        com_util_argparser_print_usage(parser, stdout);
        return 0;
    }

    if (result != COM_UTIL_ARGPARSER_OK)
    {
        com_util_argparser_print_error_messages(parser, stderr);
        com_util_argparser_print_usage(parser, stderr);
        return 1;
    }

    argparser_sample_print_result(&options);

    return 0;
}
