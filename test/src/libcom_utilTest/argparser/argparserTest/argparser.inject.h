/* テスト対象ソース ファイルの注入用追加ヘッダー
 * このヘッダーをテスト プログラムが参照することで
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef ARGPARSER_INJECT_H
#define ARGPARSER_INJECT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* argparser.c のファイル内 static 関数 argparser_is_valid_short_name へのアクセサー。
       判定関数の各条件を直接確認するため、inject 経由で呼び出す。 */
    extern int test_argparser_is_valid_short_name(const char *name);

    /* argparser.c のファイル内 static 関数 argparser_is_valid_long_name へのアクセサー。 */
    extern int test_argparser_is_valid_long_name(const char *name);

    /* argparser.c のファイル内 static 関数 argparser_default_dispose_on_shutdown へのアクセサー。 */
    extern void test_argparser_default_dispose_on_shutdown(const com_util_shutdown_event *event, void *context);

    /* usage の再計算でバッファー不足を発生させるためのテスト用置換関数。 */
    extern char *test_argparser_replace_program_description(com_util_argparser *parser, char *description);

    /* 登録エラーの各メッセージ分岐を確認するためのテスト用設定関数。 */
    extern void test_argparser_set_register_error_result(com_util_argparser *parser, size_t index, int result);

    /* 解析エラーの各メッセージ分岐を確認するためのテスト用設定関数。 */
    extern void test_argparser_set_last_error(com_util_argparser *parser, int result);

    /* 登録エラー対象の既定値分岐を確認するためのテスト用設定関数。 */
    extern void test_argparser_clear_register_error_target(com_util_argparser *parser, size_t index);

    /* 登録結果記録の早期終了条件を直接確認するためのテスト用関数。 */
    extern void test_argparser_record_register_result(com_util_argparser *parser, int result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ARGPARSER_INJECT_H */
