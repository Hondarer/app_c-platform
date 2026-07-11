/**
 *******************************************************************************
 *  @file           argparser.h
 *  @brief          汎用コマンドライン オプション パーサー API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/11
 *  @version        1.0.0
 *
 *  コマンドライン引数 (argc / argv) を解析する汎用パーサーです。\n
 *  フラグ、値付きオプション、位置引数を事前に登録し、解析結果を
 *  登録時に指定した格納先へ書き込みます。
 *
 *  以下の構文をサポートします。
 *  - フラグ : `-v` / `--verbose` (出現回数を格納)
 *  - 値付きオプション : `-o {value}` / `--option {value}` / `--option={value}`
 *  - 位置引数 : 登録順に割り当て
 *
 *  短オプションの連結 (`-abc`) と `--` 区切りはサポートしません。
 *
 *  本 API はエラーを標準出力・標準エラーに出力しません。\n
 *  解析エラーの詳細は com_util_argparser_get_error() 系 API で取得し、
 *  表示は呼び出し側で行います。
 *
 *  @par 使用例
    @code{.c}
    #include <com_util/argparser/argparser.h>

    int main(int argc, char *argv[]) {
        int help_flag = 0;
        int count = 1;
        const char *input = NULL;
        char usage[1024];
        com_util_argparser *parser = com_util_argparser_create(NULL);

        com_util_argparser_register_flag(parser, "-h", "--help", "ヘルプを表示する", &help_flag);
        com_util_argparser_register_option_int(parser, "-c", "--count", "N", "繰り返し回数",
                                             0, &count);
        com_util_argparser_register_positional_string(parser, "input", "入力ファイル",
                                                    COM_UTIL_ARGPARSER_REQUIRED, &input);

        if (com_util_argparser_parse(parser, argc, argv) != COM_UTIL_ARGPARSER_OK) {
            char message[256];
            com_util_argparser_get_error_message(parser, message, sizeof(message));
            fprintf(stderr, "%s\n", message);
            com_util_argparser_get_usage(parser, usage, sizeof(usage), NULL);
            fprintf(stderr, "%s", usage);
            com_util_argparser_dispose(parser);
            return 1;
        }
        com_util_argparser_dispose(parser);
        return 0;
    }
    @endcode
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_ARGPARSER_H
#define COM_UTIL_ARGPARSER_H

#include <stddef.h>
#include <stdio.h>

#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 *  @brief  登録フラグ: そのオプション/位置引数を必須とします。
 */
#define COM_UTIL_ARGPARSER_REQUIRED (0x00000001u)

    /** @brief 引数解析操作の結果コード。 */
    typedef enum
    {
        COM_UTIL_ARGPARSER_OK = 0,                   /**< 成功。 */
        COM_UTIL_ARGPARSER_INVALID_ARGUMENT = 1,     /**< API 引数が不正。 */
        COM_UTIL_ARGPARSER_OUT_OF_MEMORY = 2,        /**< メモリを確保できない。 */
        COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION = 3, /**< 同名オプションが登録済み。 */
        COM_UTIL_ARGPARSER_PARSE_ERROR = 4,          /**< コマンドラインの解析エラー (詳細は
                                                          com_util_argparser_get_error() で取得)。 */
        COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL = 5      /**< 出力バッファーが不足している。 */
    } com_util_argparser_result_t;

    /** @brief 解析エラーの詳細種別。 */
    typedef enum
    {
        COM_UTIL_ARGPARSER_ERROR_NONE = 0,                 /**< エラーなし。 */
        COM_UTIL_ARGPARSER_ERROR_UNKNOWN_OPTION = 1,       /**< 未登録のオプションが出現した。 */
        COM_UTIL_ARGPARSER_ERROR_MISSING_VALUE = 2,        /**< 値付きオプションに値が指定されなかった。 */
        COM_UTIL_ARGPARSER_ERROR_INVALID_INT = 3,          /**< 整数値として解釈できない。 */
        COM_UTIL_ARGPARSER_ERROR_OUT_OF_RANGE = 4,         /**< 整数値が int の範囲を超えている。 */
        COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED = 5,     /**< 必須のオプション/位置引数が指定されなかった。 */
        COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION = 6,     /**< 単数オプションが複数回指定された。 */
        COM_UTIL_ARGPARSER_ERROR_TOO_MANY_POSITIONALS = 7, /**< 位置引数が登録数を超えた。 */
        COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES = 8, /**< 複数値オプションの出現数が容量を超えた。 */
        COM_UTIL_ARGPARSER_ERROR_UNEXPECTED_VALUE = 9      /**< フラグに値が指定された (`--flag={value}`)。 */
    } com_util_argparser_error_t;

    /**
     *  @brief  引数パーサーを操作する不透明ハンドルです。
     */
    typedef struct com_util_argparser com_util_argparser;

    /**
     *  @brief  引数パーサーの生成オプションです。
     */
    typedef struct com_util_argparser_options
    {
        /**
         *  @brief  将来拡張用のフラグです。現時点では 0 を指定してください。
         */
        unsigned int flags;

        /**
         *  @brief  構造体配置用の予約領域です。現時点では 0 を指定してください。
         */
        unsigned int reserved;

        /**
         *  @brief  usage に表示するプログラム名です。
         *          NULL の場合は com_util_argparser_parse() 時に argv[0] のベース名で補完します。
         */
        const char *program_name;

        /**
         *  @brief  usage の冒頭に表示するプログラムの説明文です。NULL も指定できます。
         */
        const char *program_description;
    } com_util_argparser_options;

    /**
     *  @brief          引数パーサー ハンドルを生成します。
     *  @param[in]      options  生成オプションです。NULL の場合は既定設定を使用します。
     *  @return         成功時は生成したハンドルを返します。メモリを確保できない場合は NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_argparser *COM_UTIL_API
    com_util_argparser_create(const com_util_argparser_options *options);

    /**
     *  @brief          プロセス共有のデフォルト パーサー ハンドルを取得します。
     *  @param[in]      options  初回呼び出し時のみ有効な生成オプションです。NULL も指定できます。
     *  @return         成功時は有効なハンドルを返します。
     *                  メモリの確保または内部の同期初期化に失敗した場合は NULL を返します。
     *
     *  本関数はプロセス内で 1 つだけ遅延生成される共有ハンドルを返します。\n
     *  初回呼び出し時に @p options を適用して生成し、以降の呼び出しでは @p options を無視して
     *  同一ハンドルを返します。\n
     *  返却するハンドルはライブラリが所有し、プロセス正常終了時に自動的に解放します。\n
     *  呼び出し側は本ハンドルを com_util_argparser_dispose() に渡さないでください。
     *  複数インスタンスを同時に扱う必要がある場合 (テストでの独立性検証など) は、
     *  引き続き com_util_argparser_create() / com_util_argparser_dispose() を使用してください。
     *
     *  @par            スレッド セーフ
     *  初回生成と同一ハンドルの取得は内部ロックによりスレッド セーフです。\n
     *  返却後のハンドルへの登録/解析呼び出し自体はスレッド セーフではありません。
     */
    COM_UTIL_EXPORT com_util_argparser *COM_UTIL_API
    com_util_argparser_default(const com_util_argparser_options *options);

    /**
     *  @brief          引数パーサー ハンドルを解放します。
     *  @param[in]      parser  com_util_argparser_create() が返したハンドルです。NULL も指定できます。
     *
     *  com_util_argparser_create() で得たハンドルは、プロセス終了時の自動解放を行いません。
     *  生成したハンドルは必ず本関数で解放してください。\n
     *  com_util_argparser_default() で得たハンドルを渡した場合は何も行いません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p parser を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_argparser_dispose(com_util_argparser *parser);

    /**
     *  @brief          フラグ (値なしオプション) を登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      short_name   短いオプション名 (`-x` 形式) です。@p long_name を指定する場合は NULL も指定できます。
     *  @param[in]      long_name    長いオプション名 (`--xxx` 形式) です。@p short_name を指定する場合は NULL も指定できます。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[out]     storage      出現回数の格納先です。NULL を渡してはなりません。\n
     *                               com_util_argparser_parse() の開始時に 0 へ初期化し、出現ごとに 1 加算します。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  フラグは同一コマンドラインで複数回指定できます (例: `-v -v` で @p storage は 2)。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API
    com_util_argparser_register_flag(com_util_argparser *parser, const char *short_name, const char *long_name,
                                     const char *description, int *storage);

    /**
     *  @brief          int 値を取るオプションを登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      short_name   短いオプション名 (`-x` 形式) です。@p long_name を指定する場合は NULL も指定できます。
     *  @param[in]      long_name    長いオプション名 (`--xxx` 形式) です。@p short_name を指定する場合は NULL も指定できます。
     *  @param[in]      value_name   usage に表示する値の名前です。NULL の場合は "VALUE" を使用します。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[in]      flags        登録フラグです。@ref COM_UTIL_ARGPARSER_REQUIRED を指定できます。
     *  @param[out]     storage      解析した値の格納先です。NULL を渡してはなりません。\n
     *                               オプションが出現した場合のみ書き込みます。既定値は解析前に呼び出し側で設定してください。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  同一コマンドラインで複数回指定された場合は解析エラー
     *  (@ref COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION) になります。\n
     *  複数回の指定を許可する場合は com_util_argparser_register_option_int_array() を使用してください。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_register_option_int(
        com_util_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
        const char *description, unsigned int flags, int *storage);

    /**
     *  @brief          文字列値を取るオプションを登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      short_name   短いオプション名 (`-x` 形式) です。@p long_name を指定する場合は NULL も指定できます。
     *  @param[in]      long_name    長いオプション名 (`--xxx` 形式) です。@p short_name を指定する場合は NULL も指定できます。
     *  @param[in]      value_name   usage に表示する値の名前です。NULL の場合は "VALUE" を使用します。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[in]      flags        登録フラグです。@ref COM_UTIL_ARGPARSER_REQUIRED を指定できます。
     *  @param[out]     storage      解析した値の格納先です。NULL を渡してはなりません。\n
     *                               オプションが出現した場合のみ書き込みます。既定値は解析前に呼び出し側で設定してください。\n
     *                               格納する文字列は argv 内の文字列を指します (コピーしません)。
     *                               寿命は argv に従います。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  同一コマンドラインで複数回指定された場合は解析エラー
     *  (@ref COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION) になります。\n
     *  複数回の指定を許可する場合は com_util_argparser_register_option_string_array() を使用してください。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_register_option_string(
        com_util_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
        const char *description, unsigned int flags, const char **storage);

    /**
     *  @brief          複数回指定できる int 値オプションを登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      short_name   短いオプション名 (`-x` 形式) です。@p long_name を指定する場合は NULL も指定できます。
     *  @param[in]      long_name    長いオプション名 (`--xxx` 形式) です。@p short_name を指定する場合は NULL も指定できます。
     *  @param[in]      value_name   usage に表示する値の名前です。NULL の場合は "VALUE" を使用します。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[in]      flags        登録フラグです。@ref COM_UTIL_ARGPARSER_REQUIRED を指定できます
     *                               (1 回以上の出現を必須とします)。
     *  @param[out]     storage      解析した値を出現順に格納する配列です。NULL を渡してはなりません。
     *  @param[in]      capacity     @p storage の要素数です。1 以上を指定してください。\n
     *                               出現数が @p capacity を超えた場合は解析エラー
     *                               (@ref COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES) になります。
     *  @param[out]     count        出現数の格納先です。NULL を渡してはなりません。\n
     *                               com_util_argparser_parse() の開始時に 0 へ初期化します。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_register_option_int_array(
        com_util_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
        const char *description, unsigned int flags, int *storage, size_t capacity, size_t *count);

    /**
     *  @brief          複数回指定できる文字列値オプションを登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      short_name   短いオプション名 (`-x` 形式) です。@p long_name を指定する場合は NULL も指定できます。
     *  @param[in]      long_name    長いオプション名 (`--xxx` 形式) です。@p short_name を指定する場合は NULL も指定できます。
     *  @param[in]      value_name   usage に表示する値の名前です。NULL の場合は "VALUE" を使用します。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[in]      flags        登録フラグです。@ref COM_UTIL_ARGPARSER_REQUIRED を指定できます
     *                               (1 回以上の出現を必須とします)。
     *  @param[out]     storage      解析した値を出現順に格納する配列です。NULL を渡してはなりません。\n
     *                               格納する文字列は argv 内の文字列を指します (コピーしません)。
     *                               寿命は argv に従います。
     *  @param[in]      capacity     @p storage の要素数です。1 以上を指定してください。\n
     *                               出現数が @p capacity を超えた場合は解析エラー
     *                               (@ref COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES) になります。
     *  @param[out]     count        出現数の格納先です。NULL を渡してはなりません。\n
     *                               com_util_argparser_parse() の開始時に 0 へ初期化します。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_register_option_string_array(
        com_util_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
        const char *description, unsigned int flags, const char **storage, size_t capacity, size_t *count);

    /**
     *  @brief          int 値の位置引数を登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      name         usage に表示する位置引数の名前です。NULL を渡してはなりません。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[in]      flags        登録フラグです。@ref COM_UTIL_ARGPARSER_REQUIRED を指定できます。
     *  @param[out]     storage      解析した値の格納先です。NULL を渡してはなりません。\n
     *                               位置引数が出現した場合のみ書き込みます。
     *                               既定値は解析前に呼び出し側で設定してください。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  位置引数は登録順にコマンドラインの非オプション トークンへ割り当てます。\n
     *  任意 (REQUIRED なし) の位置引数の後に必須の位置引数を登録した場合は
     *  @ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT を返します。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_register_positional_int(
        com_util_argparser *parser, const char *name, const char *description, unsigned int flags, int *storage);

    /**
     *  @brief          文字列値の位置引数を登録します。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      name         usage に表示する位置引数の名前です。NULL を渡してはなりません。
     *  @param[in]      description  usage に表示する説明文です。NULL も指定できます。
     *  @param[in]      flags        登録フラグです。@ref COM_UTIL_ARGPARSER_REQUIRED を指定できます。
     *  @param[out]     storage      解析した値の格納先です。NULL を渡してはなりません。\n
     *                               位置引数が出現した場合のみ書き込みます。
     *                               既定値は解析前に呼び出し側で設定してください。\n
     *                               格納する文字列は argv 内の文字列を指します (コピーしません)。
     *                               寿命は argv に従います。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  位置引数は登録順にコマンドラインの非オプション トークンへ割り当てます。\n
     *  任意 (REQUIRED なし) の位置引数の後に必須の位置引数を登録した場合は
     *  @ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT を返します。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API
    com_util_argparser_register_positional_string(com_util_argparser *parser, const char *name, const char *description,
                                                  unsigned int flags, const char **storage);

    /**
     *  @brief          コマンドラインを解析し、登録済みの格納先に結果を書き込みます。
     *  @param[in]      parser  引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      argc    argv の要素数です。1 以上を指定してください。
     *  @param[in]      argv    コマンドライン引数の配列です。NULL を渡してはなりません。\n
     *                          argv[0] はプログラム名として扱い、argv[1] 以降を解析します。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_PARSE_ERROR 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  @ref COM_UTIL_ARGPARSER_PARSE_ERROR を返した場合、エラーの詳細は
     *  com_util_argparser_get_error() 、com_util_argparser_get_error_target() 、
     *  com_util_argparser_get_error_index() 、com_util_argparser_get_error_message()
     *  で取得できます。\n
     *  解析エラー時、エラー検出より前に処理した格納先には値が書き込まれています。
     *
     *  本関数は同一ハンドルで繰り返し呼び出せます。\n
     *  呼び出しの開始時に、フラグの格納先を 0 に、複数値オプションの出現数を 0 に初期化し、
     *  前回のエラー状態をクリアします。\n
     *  値付きオプションと位置引数の格納先は出現時のみ書き込むため、
     *  既定値は毎回の解析前に呼び出し側で設定してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一 @p parser への並行呼び出しは未定義動作です。ハンドルごとに 1 スレッドから使用してください。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_parse(com_util_argparser *parser,
                                                                                      int argc, char *const *argv);

    /**
     *  @brief          直前の com_util_argparser_parse() の解析エラー種別を取得します。
     *  @param[in]      parser  引数パーサー ハンドルです。NULL の場合は
     *                          @ref COM_UTIL_ARGPARSER_ERROR_NONE を返します。
     *  @return         解析エラー種別を返します。解析が成功した場合と未解析の場合は
     *                  @ref COM_UTIL_ARGPARSER_ERROR_NONE を返します。
     */
    COM_UTIL_EXPORT com_util_argparser_error_t COM_UTIL_API
    com_util_argparser_get_error(const com_util_argparser *parser);

    /**
     *  @brief          直前の解析エラーの対象名を取得します。
     *  @param[in]      parser  引数パーサー ハンドルです。NULL の場合は NULL を返します。
     *  @return         エラーの対象を示す文字列 (オプション名、位置引数名、または該当トークン) を返します。\n
     *                  エラーがない場合と対象がない場合は NULL を返します。\n
     *                  返却する文字列はハンドルが所有します。次回の com_util_argparser_parse() または
     *                  com_util_argparser_dispose() まで有効です。
     */
    COM_UTIL_EXPORT const char *COM_UTIL_API com_util_argparser_get_error_target(const com_util_argparser *parser);

    /**
     *  @brief          直前の解析エラーが発生した argv のインデックスを取得します。
     *  @param[in]      parser  引数パーサー ハンドルです。NULL の場合は -1 を返します。
     *  @return         エラーを起こしたトークンの argv インデックスを返します。\n
     *                  エラーがない場合と、特定のトークンに対応しないエラー
     *                  (必須引数の欠落など) の場合は -1 を返します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_argparser_get_error_index(const com_util_argparser *parser);

    /**
     *  @brief          直前の解析エラーの内容を人間可読の 1 行メッセージとして組み立てます。
     *  @param[in]      parser       引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[out]     buffer       メッセージの格納先バッファーです。NULL を渡してはなりません。\n
     *                               常に NUL 終端します。
     *  @param[in]      buffer_size  @p buffer のバイト数です。1 以上を指定してください。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL のいずれかを返します。\n
     *                  @ref COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL の場合、@p buffer には
     *                  切り詰めたメッセージを格納します。
     *
     *  本 API は組み立てた文字列を返すだけで、表示は行いません。表示は呼び出し側で行ってください。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API
    com_util_argparser_get_error_message(const com_util_argparser *parser, char *buffer, size_t buffer_size);

    /**
     *  @brief          登録内容から usage 文字列を組み立てます。
     *  @param[in]      parser         引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[out]     buffer         usage 文字列の格納先バッファーです。\n
     *                                 NULL の場合は @p required_size への必要サイズの出力のみを行います
     *                                 (このとき @p required_size に NULL を渡してはなりません)。\n
     *                                 NULL でない場合は常に NUL 終端します。
     *  @param[in]      buffer_size    @p buffer のバイト数です。@p buffer が NULL の場合は無視します。
     *  @param[out]     required_size  NUL 終端を含む必要バイト数の格納先です。不要な場合は NULL も指定できます。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL のいずれかを返します。\n
     *                  @ref COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL の場合、@p buffer には
     *                  切り詰めた usage を格納し、@p required_size (NULL でない場合) に必要サイズを出力します。
     *
     *  本 API は組み立てた文字列を返すだけで、表示は行いません。\n
     *  解析の成否とは独立に、登録完了後であればいつでも呼び出せます。
     *  解析前のヘルプ表示にも、解析後に呼び出し側で行うバリデーションのエラー報告にも使用できます。
     *
     *  プログラム名は生成オプションの program_name、未指定の場合は直前の
     *  com_util_argparser_parse() が argv[0] から求めたベース名、
     *  解析前の場合は "{program}" を使用します。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API com_util_argparser_get_usage(
        const com_util_argparser *parser, char *buffer, size_t buffer_size, size_t *required_size);

    /**
     *  @brief          登録内容から組み立てた usage を指定ストリームへ出力します。
     *  @param[in]      parser  引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      stream  出力先ストリームです (stdout / stderr など)。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_ARGPARSER_OK 、@ref COM_UTIL_ARGPARSER_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ARGPARSER_OUT_OF_MEMORY のいずれかを返します。
     *
     *  内部で com_util_argparser_get_usage() を用いて usage 文字列を組み立ててから
     *  @p stream へ書き出します。固定長バッファーによる切り詰めは発生しません。\n
     *  解析の成否とは独立に、登録完了後であればいつでも呼び出せます。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API
    com_util_argparser_print_usage(const com_util_argparser *parser, FILE *stream);

    /**
     *  @brief          直前の解析エラーのメッセージを指定ストリームへ出力します。
     *  @param[in]      parser  引数パーサー ハンドルです。NULL を渡してはなりません。
     *  @param[in]      stream  出力先ストリームです (stdout / stderr など)。NULL を渡してはなりません。
     *  @return         com_util_argparser_get_error_message() の戻り値をそのまま返します。
     *
     *  内部で com_util_argparser_get_error_message() を用いてエラー メッセージを組み立ててから、
     *  "error: {メッセージ}\n" の形式で @p stream へ書き出し、続けて区切りの空行を出力します。\n
     *  エラーがない場合や対象がない場合は何も出力しません。
     */
    COM_UTIL_EXPORT com_util_argparser_result_t COM_UTIL_API
    com_util_argparser_print_error_messages(const com_util_argparser *parser, FILE *stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_ARGPARSER_H */
