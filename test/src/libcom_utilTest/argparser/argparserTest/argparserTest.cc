#include <testfw.h>
#include <com_util/argparser/argparser.h>
#include <mock_stdio.h>
#include <mock_stdlib.h>
#include <mock_com_util.h>
#include <limits.h>
#include <string.h>
#include <string>
#include <thread>

#include "argparser.inject.h"

namespace
{

com_util_shutdown_fn g_default_shutdown_callback = nullptr;
void *g_default_shutdown_context = nullptr;

} // namespace

class argparserTest : public Test
{
  protected:
    argparserTest()
    {
        // 実体への委譲で shutdown コールバックが登録されると、フィクスチャ破棄後の
        // atexit で破棄済みモックを参照してアクセス違反となるため、登録を抑止する。
        ON_CALL(mock_com_util_, com_util_shutdown_register(_, _))
            .WillByDefault(
                DoAll(SaveArg<0>(&g_default_shutdown_callback), SaveArg<1>(&g_default_shutdown_context), Return(0)));
    }

    NiceMock<Mock_com_util> mock_com_util_;
};

// テスト用の argv 生成補助 (文字列リテラルを char* に変換する)
#define ARGV(...) \
    char *argv[] = {__VA_ARGS__}; \
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]))

static char *cstr(const char *text)
{
    return const_cast<char *>(text);
}

// ハンドル生成と解放が成功し、dispose(NULL) が安全であることの確認
TEST_F(argparserTest, create_and_dispose)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_argparser *parser =
        _com_util_argparser_create(NULL); // [手順] - オプション NULL で _com_util_argparser_create を呼び出す。
    bool handle_created = parser != NULL;
    _com_util_argparser_dispose(parser); // [手順] - 生成したハンドルを dispose する。
    _com_util_argparser_dispose(NULL);   // [手順] - NULL ハンドルで dispose を呼び出す。

    // Assert
    EXPECT_TRUE(handle_created); // [確認_正常系] - _com_util_argparser_create の戻り値が NULL でないこと。
    // [確認_正常系] - dispose(NULL) がクラッシュせずに完了すること。
}

// メモリ確保に失敗した場合に create が NULL を返すことの確認
TEST_F(argparserTest, create_returns_null_on_alloc_failure)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(Return(nullptr)); // [Pre-Assert確認_異常系] - calloc が 1 回呼び出されること。
                                    // [Pre-Assert手順] - calloc から NULL を返却する。

    // Act
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [手順] - _com_util_argparser_create を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, parser); // [確認_異常系] - _com_util_argparser_create の戻り値が NULL であること。
}

// 短いオプション名の各形式を正しく判定することの確認
TEST_F(argparserTest, is_valid_short_name_rejects_invalid_names)
{
    // Arrange
    const char *name_without_dash = "a-";     // [状態] - 先頭が '-' ではない 2 文字の名前を用意する。
    const char *name_with_double_dash = "--"; // [状態] - 2 文字とも '-' である名前を用意する。

    // Pre-Assert

    // Act
    int null_name_result = test_argparser_is_valid_short_name(
        NULL); // [手順] - NULL を指定して test_argparser_is_valid_short_name を呼び出す。
    int name_without_dash_result = test_argparser_is_valid_short_name(
        name_without_dash); // [手順] - 先頭が '-' ではない名前を指定して test_argparser_is_valid_short_name を呼び出す。
    int name_with_double_dash_result = test_argparser_is_valid_short_name(
        name_with_double_dash); // [手順] - "--" を指定して test_argparser_is_valid_short_name を呼び出す。
    int valid_name_result = test_argparser_is_valid_short_name(
        "-a"); // [手順] - "-a" を指定して test_argparser_is_valid_short_name を呼び出す。

    // Assert
    EXPECT_EQ(
        0,
        null_name_result); // [確認_異常系] - NULL を指定した test_argparser_is_valid_short_name の戻り値が 0 であること。
    EXPECT_EQ(
        0,
        name_without_dash_result); // [確認_異常系] - 先頭が '-' ではない名前を指定した test_argparser_is_valid_short_name の戻り値が 0 であること。
    EXPECT_EQ(
        0,
        name_with_double_dash_result); // [確認_異常系] - "--" を指定した test_argparser_is_valid_short_name の戻り値が 0 であること。
    EXPECT_EQ(
        1,
        valid_name_result); // [確認_正常系] - "-a" を指定した test_argparser_is_valid_short_name の戻り値が 1 であること。
}

// default() の終了コールバックを並行実行しても共有ロックを破棄しないことの確認
TEST_F(argparserTest, default_shutdown_callback_keeps_process_lifetime_lock)
{
    // Arrange
    static constexpr size_t kThreadCount = 16;
    std::thread threads[kThreadCount];
    com_util_argparser *parser =
        _com_util_argparser_default(NULL); // [状態] - 終了コールバックを登録した default ハンドルを用意する。
    ASSERT_NE(nullptr, parser);
    ASSERT_NE(nullptr, g_default_shutdown_callback);
    com_util_shutdown_event event = {};
    event.reason = COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT;
    event.code_kind = COM_UTIL_SHUTDOWN_CODE_KIND_NONE;
    ON_CALL(mock_com_util_, com_util_local_lock_destroy(_)).WillByDefault([](com_util_local_lock *) {});

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_destroy(_))
        .Times(0); // [Pre-Assert確認_正常系] - default 用の共有ロックが破棄されないこと。

    // Act
    // [手順] - 16 スレッドから並行に default の終了コールバックを呼び出す。
    for (size_t i = 0; i < kThreadCount; i++)
    {
        threads[i] = std::thread([&event]() { g_default_shutdown_callback(&event, g_default_shutdown_context); });
    }
    for (size_t i = 0; i < kThreadCount; i++)
    {
        threads[i].join();
    }

    // Assert
    SUCCEED(); // [確認_正常系] - すべての終了コールバックがクラッシュせずに完了すること。
}

// default() を複数回呼び出しても同一ハンドルが返ることの確認 (dispose は呼び出さない)
TEST_F(argparserTest, default_returns_same_handle_across_calls)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_argparser *first =
        _com_util_argparser_default(NULL); // [手順] - 1 回目の _com_util_argparser_default を呼び出す。
    com_util_argparser *second =
        _com_util_argparser_default(NULL); // [手順] - 続けて 2 回目の _com_util_argparser_default を呼び出す。

    // Assert
    ASSERT_NE(nullptr, first); // [確認_正常系] - ハンドルが NULL でないこと。
    EXPECT_EQ(
        first,
        second); // [確認_正常系] - _com_util_argparser_default の戻り値として、2 回の呼び出しで同一ハンドルが返ること。
}

// default() の生成オプションが初回呼び出し時のみ適用されることの確認 (dispose は呼び出さない)
TEST_F(argparserTest, default_options_applied_only_on_first_call)
{
    // Arrange
    com_util_argparser *first =
        _com_util_argparser_default(NULL); // [状態] - 初回呼び出し済みの default ハンドルを用意する。
    ASSERT_NE(nullptr, first);

    char before[256];
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_get_usage(first, before, sizeof(before),
                                                         NULL)); // [状態] - 初回時点の usage を記録する。

    com_util_argparser_options options = {};
    options.program_name =
        "should-be-ignored"; // [状態] - 2 回目に渡す生成オプションの program_name を "should-be-ignored" とする。

    // Pre-Assert

    // Act
    com_util_argparser *second =
        _com_util_argparser_default(&options); // [手順] - 生成オプション付きで 2 回目の default を呼び出す。

    char after[256];
    int rtc_argparser_get_usage =
        _com_util_argparser_get_usage(second, after, sizeof(after), NULL); // [手順] - 2 回目時点の usage を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_usage); // [確認_正常系] - 2 回目時点の usage を取得した _com_util_argparser_get_usage の戻り値が COM_UTIL_OK であること。

    // Assert
    EXPECT_EQ(first, second);    // [確認_正常系] - _com_util_argparser_default の戻り値として、同一ハンドルが返ること。
    EXPECT_STREQ(before, after); // [確認_正常系] - usage が初回時点から変化しないこと。
    EXPECT_THAT(std::string(after),
                Not(HasSubstr("should-be-ignored"))); // [確認_正常系] - 2 回目のオプションが usage に反映されないこと。
}

// default() の返却ハンドルを dispose() に渡しても解放されないことの確認
TEST_F(argparserTest, dispose_ignores_default_handle)
{
    // Arrange
    com_util_argparser *first = _com_util_argparser_default(NULL); // [状態] - default ハンドルを取得しておく。
    ASSERT_NE(nullptr, first);

    // Pre-Assert

    // Act
    _com_util_argparser_dispose(first);                             // [手順] - default ハンドルを dispose に渡す。
    com_util_argparser *second = _com_util_argparser_default(NULL); // [手順] - 再度 default を呼び出す。

    // Assert
    EXPECT_EQ(first, second); // [確認_正常系] - dispose 後も同一ハンドルが返り、解放されないこと。
}

// default() の並行呼び出しが同一ハンドルを返すことの確認
TEST_F(argparserTest, default_returns_same_handle_to_concurrent_callers)
{
    // Arrange
    static constexpr size_t kThreadCount = 16;
    com_util_argparser *parsers[kThreadCount] = {}; // [状態] - 16 スレッド分の結果格納先を用意する。
    std::thread threads[kThreadCount];

    // Pre-Assert

    // Act
    // [手順] - 16 スレッドから並行に _com_util_argparser_default を呼び出す。
    for (size_t i = 0; i < kThreadCount; i++)
    {
        threads[i] = std::thread([&parsers, i]() { parsers[i] = _com_util_argparser_default(NULL); });
    }
    for (size_t i = 0; i < kThreadCount; i++)
    {
        threads[i].join();
    }

    // Assert
    ASSERT_NE(nullptr, parsers[0]); // [確認_正常系] - ハンドルが NULL でないこと。
    // [確認_正常系] - 16 スレッドすべてに同一ハンドルが返ること。
    for (size_t i = 1; i < kThreadCount; i++)
    {
        EXPECT_EQ(parsers[0], parsers[i]);
    }
}

// 登録 API が不正引数を検出することの確認
TEST_F(argparserTest, register_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 生成済みの parser を用意する。
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    const char *string_storage = NULL;

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_flag(
                  NULL, "-v", "--verbose", NULL,
                  &storage)); // [確認_異常系] - parser NULL の登録が INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                          NULL)); // [確認_異常系] - storage NULL の登録が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_flag(
                  parser, NULL, NULL, NULL,
                  &storage)); // [確認_異常系] - 両方の名前が NULL の登録が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_flag(
                  parser, "-vv", NULL, NULL, &storage)); // [確認_異常系] - 短い名前 "-vv" の形式不正が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_flag(parser, "v", NULL, NULL,
                                                &storage)); // [確認_異常系] - 短い名前 "v" の形式不正が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_flag(
                  parser, NULL, "-verbose", NULL,
                  &storage)); // [確認_異常系] - 長い名前 "-verbose" の形式不正が検出されること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_register_flag(parser, NULL, "--a=b", NULL,
                                          &storage)); // [確認_異常系] - 長い名前 "--a=b" の形式不正が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_option_int(
                  parser, "-c", NULL, NULL, NULL, 0x100u,
                  &storage)); // [確認_異常系] - 未定義の登録フラグ 0x100 が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_option_string_array(
                  parser, "-i", NULL, NULL, NULL, 0, &string_storage, 0,
                  NULL)); // [確認_異常系] - 配列オプションの capacity 0 / count NULL が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, _com_util_argparser_register_positional_int(
                                                 parser, NULL, NULL, 0,
                                                 &storage)); // [確認_異常系] - 位置引数の名前 NULL が検出されること。
    size_t positional_count = 0;
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_positional_int_array(
                  parser, "values", NULL, 0, NULL, 1,
                  &positional_count)); // [確認_異常系] - 可変長位置引数の storage NULL が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_positional_int_array(
                  parser, "values", NULL, 0, &storage, 0,
                  &positional_count)); // [確認_異常系] - 可変長位置引数の capacity 0 が検出されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_positional_int_array(
                  parser, "values", NULL, 0, &storage, 1,
                  NULL)); // [確認_異常系] - 可変長位置引数の count NULL が検出されること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 同名オプションの二重登録が検出されることの確認
TEST_F(argparserTest, register_rejects_duplicate_definition)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &storage)); // [状態] - "-v" / "--verbose" を登録済みとする。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_DEFINITION,
        _com_util_argparser_register_option_int(parser, "-v", NULL, NULL, NULL, 0,
                                                &storage)); // [確認_異常系] - 短い名前 "-v" の重複が検出されること。
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_DEFINITION,
        _com_util_argparser_register_flag(parser, NULL, "--verbose", NULL,
                                          &storage)); // [確認_異常系] - 長い名前 "--verbose" の重複が検出されること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 任意の位置引数の後に必須の位置引数を登録できないことの確認
TEST_F(argparserTest, register_rejects_required_positional_after_optional)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *first = NULL;
    const char *second = NULL;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string(
                               parser, "first", NULL, 0,
                               &first)); // [状態] - 任意の位置引数 "first" を登録済みとする。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_register_positional_string(
            parser, "second", NULL, COM_UTIL_ARGPARSER_REQUIRED,
            &second)); // [確認_異常系] - 任意の位置引数の後の必須位置引数 "second" の登録が INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 必須位置引数を連続して登録できることの確認
TEST_F(argparserTest, register_accepts_required_positionals_in_sequence)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *first = NULL;
    const char *second = NULL;

    // Pre-Assert

    // Act
    int first_result =
        _com_util_argparser_register_positional_string(parser, "first", NULL, COM_UTIL_ARGPARSER_REQUIRED,
                                                       &first); // [手順] - 必須位置引数 "first" を登録する。
    int second_result =
        _com_util_argparser_register_positional_string(parser, "second", NULL, COM_UTIL_ARGPARSER_REQUIRED,
                                                       &second); // [手順] - 必須位置引数 "second" を続けて登録する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, first_result); // [確認_正常系] - 1 件目の必須位置引数登録結果が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              second_result); // [確認_正常系] - 必須位置引数を連続して登録した結果が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 可変長位置引数を位置引数列の末尾に 1 件だけ登録できることの確認
TEST_F(argparserTest, register_requires_variadic_positional_to_be_last)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *values[2] = {};
    size_t value_count = 0;
    const char *trailing = NULL;
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string_array(
                               parser, "values", NULL, 0, values, 2,
                               &value_count)); // [状態] - 可変長位置引数 "values" を登録済みとする。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_positional_string(
                  parser, "trailing", NULL, 0,
                  &trailing)); // [確認_異常系] - 可変長位置引数の後に単数位置引数を登録できないこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_register_positional_string_array(
                  parser, "more", NULL, 0, values, 2,
                  &value_count)); // [確認_異常系] - 可変長位置引数を複数登録できないこと。
    EXPECT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(
                               parser, "-v", "--verbose", NULL,
                               &verbose)); // [確認_正常系] - 可変長位置引数の後でもオプションを登録できること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// register 系呼び出しで発生した複数のエラーが取りこぼしなく積み上げられることの確認
TEST_F(argparserTest, register_error_collection_accumulates_all_failures)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 生成済みの parser を用意する。
    ASSERT_NE(nullptr, parser);
    int a = 0;
    int b = 0;

    // Pre-Assert

    // Act
    int rtc_argparser_register_flag = _com_util_argparser_register_flag(
        parser, "-a", "--aa", NULL, &a); // [手順] - 成功する登録を行う (コレクションに積まれない)。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_argparser_register_flag); // [確認_正常系] - 成功する登録に対する _com_util_argparser_register_flag の戻り値が COM_UTIL_OK であること。
    int rtc_argparser_register_flag_2 = _com_util_argparser_register_flag(
        parser, "-a", "--bb", NULL, &b); // [手順] - 短い名前の重複 (1 件目のエラー) を発生させる。
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_DEFINITION,
        rtc_argparser_register_flag_2); // [確認_異常系] - _com_util_argparser_register_flag の戻り値として、短い名前の重複 (1 件目のエラー) を発生させた結果が COM_UTIL_ERR_DUPLICATE_DEFINITION であること。
    int rtc_argparser_register_flag_3 = _com_util_argparser_register_flag(
        parser, "-c", "--cc", NULL, NULL); // [手順] - 格納先 NULL (2 件目のエラー) を発生させる。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_argparser_register_flag_3); // [確認_異常系] - _com_util_argparser_register_flag の戻り値として、格納先 NULL (2 件目のエラー) を発生させた結果が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    int rtc_argparser_register_flag_4 = _com_util_argparser_register_flag(
        parser, "-d", "--aa", NULL, &b); // [手順] - 別の重複 (3 件目のエラー) を発生させる。
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_DEFINITION,
        rtc_argparser_register_flag_4); // [確認_異常系] - _com_util_argparser_register_flag の戻り値として、別の重複 (3 件目のエラー) を発生させた結果が COM_UTIL_ERR_DUPLICATE_DEFINITION であること。

    // Assert
    ASSERT_EQ((size_t)3,
              _com_util_argparser_get_register_error_count(parser)); // [確認_正常系] - エラー件数が 3 であること。
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_get_register_error(parser,
                                                     0)); // [確認_正常系] - 1 件目が DUPLICATE_DEFINITION であること。
    EXPECT_STREQ("--bb", _com_util_argparser_get_register_error_target(
                             parser, 0)); // [確認_正常系] - 1 件目の対象が "--bb" であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, _com_util_argparser_get_register_error(
                                                 parser, 1)); // [確認_正常系] - 2 件目が INVALID_ARGUMENT であること。
    EXPECT_STREQ("--cc", _com_util_argparser_get_register_error_target(
                             parser, 1)); // [確認_正常系] - 2 件目の対象が "--cc" であること。
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_get_register_error(parser,
                                                     2)); // [確認_正常系] - 3 件目が DUPLICATE_DEFINITION であること。
    EXPECT_STREQ("--aa", _com_util_argparser_get_register_error_target(
                             parser, 2)); // [確認_正常系] - 3 件目の対象が "--aa" であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// register エラーがない場合、および NULL / 範囲外アクセスが安全に既定値を返すことの確認
TEST_F(argparserTest, register_error_getters_default_when_absent_or_out_of_range)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int a = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", "--aa", NULL,
                                                             &a)); // [状態] - 正常な登録のみ行った parser とする。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ((size_t)0, _com_util_argparser_get_register_error_count(
                             parser)); // [確認_正常系] - エラーなしのとき件数が 0 であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        _com_util_argparser_get_register_error(
            parser,
            0)); // [確認_正常系] - _com_util_argparser_get_register_error の戻り値として、エラーなしのとき OK が返ること。
    EXPECT_EQ(nullptr, _com_util_argparser_get_register_error_target(
                           parser, 0)); // [確認_正常系] - エラーなしのとき対象が NULL であること。

    EXPECT_EQ(
        (size_t)0,
        _com_util_argparser_get_register_error_count(
            NULL)); // [確認_異常系] - _com_util_argparser_get_register_error_count の戻り値として、NULL ハンドルで件数 0 が返ること。
    EXPECT_EQ(
        COM_UTIL_OK,
        _com_util_argparser_get_register_error(
            NULL,
            0)); // [確認_異常系] - _com_util_argparser_get_register_error の戻り値として、NULL ハンドルで OK が返ること。
    EXPECT_EQ(
        nullptr,
        _com_util_argparser_get_register_error_target(
            NULL,
            0)); // [確認_異常系] - _com_util_argparser_get_register_error_target の戻り値として、NULL ハンドルで NULL が返ること。

    int rtc_argparser_register_flag =
        _com_util_argparser_register_flag(parser, "-a", NULL, NULL, NULL); // [手順] - エラーを 1 件発生させる。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_argparser_register_flag); // [確認_異常系] - _com_util_argparser_register_flag の戻り値として、エラーを 1 件発生させた結果が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    ASSERT_EQ((size_t)1, _com_util_argparser_get_register_error_count(parser));
    EXPECT_EQ(
        COM_UTIL_OK,
        _com_util_argparser_get_register_error(
            parser,
            1)); // [確認_異常系] - _com_util_argparser_get_register_error の戻り値として、範囲外インデックスで OK が返ること。
    EXPECT_EQ(
        nullptr,
        _com_util_argparser_get_register_error_target(
            parser,
            1)); // [確認_異常系] - _com_util_argparser_get_register_error_target の戻り値として、範囲外インデックスで NULL が返ること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// register エラーの 1 行メッセージ組み立てと不正引数の確認
TEST_F(argparserTest, register_error_message_formatting)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int a = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", "--aa", NULL, &a));
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_register_flag(parser, "-a", "--bb", NULL,
                                                &a)); // [状態] - 重複登録エラーを 1 件積んだ parser とする。
    ASSERT_EQ((size_t)1, _com_util_argparser_get_register_error_count(parser));
    char message[128];

    // Pre-Assert

    // Act
    int rtc_argparser_get_register_error_message = _com_util_argparser_get_register_error_message(
        parser, 0, message, sizeof(message)); // [手順] - 1 件目のエラー メッセージを取得する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_register_error_message); // [確認_正常系] - 1 件目のエラー メッセージを取得した _com_util_argparser_get_register_error_message の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("failed to register '--bb': duplicate definition",
                 message); // [確認_正常系] - メッセージが対象と理由を含む 1 行に組み立てられること。

    char small_buffer[8];
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        _com_util_argparser_get_register_error_message(
            parser, 0, small_buffer,
            sizeof(
                small_buffer))); // [確認_異常系] - _com_util_argparser_get_register_error_message の戻り値として、バッファー不足時は切り詰めて BUFFER_TOO_SMALL が返ること。

    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_register_error_message(
                  NULL, 0, message, sizeof(message))); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_register_error_message(
                  parser, 0, NULL, sizeof(message))); // [確認_異常系] - buffer NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_register_error_message(
                  parser, 0, message, 0)); // [確認_異常系] - サイズ 0 が INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_get_register_error_message(
            parser, 1, message, sizeof(message))); // [確認_異常系] - 範囲外インデックスが INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_register_error_messages が不正引数を検出することの確認
TEST_F(argparserTest, print_register_error_messages_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 生成済みの parser を用意する。
    ASSERT_NE(nullptr, parser);

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_print_register_error_messages(
                  NULL, stderr)); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_print_register_error_messages(
                  parser, NULL)); // [確認_異常系] - stream NULL が INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_register_error_messages がエラーなしの場合に何も出力しないことの確認
TEST_F(argparserTest, print_register_error_messages_is_noop_without_error)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - register エラーのない parser を用意する。
    ASSERT_NE(nullptr, parser);

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - fprintf が呼び出されないこと。

    // Act
    int result = _com_util_argparser_print_register_error_messages(
        parser, stderr); // [手順] - print_register_error_messages を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - _com_util_argparser_print_register_error_messages の戻り値が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_register_error_messages が積み上げた全エラーを指定ストリームへ書き出すことの確認
TEST_F(argparserTest, print_register_error_messages_writes_all_to_stream)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int a = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", "--aa", NULL, &a));
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_DEFINITION,
        _com_util_argparser_register_flag(parser, "-a", "--bb", NULL, &a)); // [状態] - 重複登録エラーを積んでおく。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_register_flag(parser, "-c", "--cc", NULL, NULL)); // [状態] - 不正引数エラーを積んでおく。

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _))
        .Times(AnyNumber()); // [Pre-Assert手順] - 区切りの空行の fprintf 呼び出しを許容する。
    EXPECT_CALL(mock_stdio,
                fprintf(_, _, _, stderr, HasSubstr("error: failed to register '--bb': duplicate definition")))
        .Times(1); // [Pre-Assert確認_正常系] - "--bb" の重複エラーが stderr へ 1 回書き出されること。
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, HasSubstr("error: failed to register '--cc': invalid argument")))
        .Times(1); // [Pre-Assert確認_正常系] - "--cc" の不正引数エラーが stderr へ 1 回書き出されること。

    // Act
    int result = _com_util_argparser_print_register_error_messages(
        parser, stderr); // [手順] - print_register_error_messages を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - _com_util_argparser_print_register_error_messages の戻り値が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 初期容量 (8) を超える登録で内部配列が拡張されることの確認
TEST_F(argparserTest, register_grows_beyond_initial_capacity)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const int spec_count = 20; // [状態] - 初期容量 8 を超える 20 個の登録数とする。
    int storages[spec_count] = {};

    // Pre-Assert

    // Act
    // [手順] - "--opt00" から "--opt19" まで 20 個のフラグを登録する。
    for (int i = 0; i < spec_count; i++)
    {
        char long_name[32];
        snprintf(long_name, sizeof(long_name), "--opt%02d", i);
        ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, NULL, long_name, NULL, &storages[i]));
    }

    // Assert
    ARGV(cstr("prog"), cstr("--opt00"), cstr("--opt19"));
    EXPECT_EQ(
        COM_UTIL_OK,
        _com_util_argparser_parse(
            parser, argc,
            argv)); // [確認_正常系] - _com_util_argparser_parse の戻り値から、拡張後も解析が成功したと判断できること。
    EXPECT_EQ(1, storages[0]);  // [確認_正常系] - 先頭の "--opt00" が解析されること。
    EXPECT_EQ(0, storages[1]);  // [確認_正常系] - 未指定の "--opt01" が 0 のままであること。
    EXPECT_EQ(1, storages[19]); // [確認_正常系] - 末尾の "--opt19" が解析されること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// フラグの出現回数が格納され、再解析で 0 に初期化されることの確認
TEST_F(argparserTest, flag_counts_occurrences_and_resets_on_reparse)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &verbose)); // [状態] - フラグ "-v" / "--verbose" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-v"), cstr("--verbose"), cstr("-v"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - フラグが 3 回出現する引数を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - フラグが 3 回出現する引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(3, verbose); // [確認_正常系] - 出現回数 3 が格納されること。
    }
    {
        ARGV(cstr("prog"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - フラグなしの引数で再解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - フラグなしの引数で再解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(0, verbose); // [確認_正常系] - 再解析で 0 に初期化されること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// フラグへの値指定 (--flag=value) がエラーになることの確認
TEST_F(argparserTest, flag_with_value_is_unexpected_value)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &verbose)); // [状態] - フラグ "-v" / "--verbose" を登録する。

    // Pre-Assert

    // Act
    ARGV(cstr("prog"), cstr("--verbose=1"));
    int result = _com_util_argparser_parse(parser, argc, argv); // [手順] - "--verbose=1" を解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNEXPECTED_VALUE,
        result); // [確認_異常系] - _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNEXPECTED_VALUE であること。
    EXPECT_EQ(COM_UTIL_ERR_UNEXPECTED_VALUE,
              _com_util_argparser_get_error(parser)); // [確認_異常系] - エラー種別が UNEXPECTED_VALUE であること。
    EXPECT_STREQ("--verbose",
                 _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が "--verbose" であること。
    EXPECT_EQ(1, _com_util_argparser_get_error_index(parser));  // [確認_異常系] - エラー位置が argv[1] であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// フラグへの値指定 (-v=value) がエラーになることの確認
TEST_F(argparserTest, flag_with_short_value_is_unexpected_value)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &verbose)); // [状態] - フラグ "-v" / "--verbose" を登録する。

    // Pre-Assert

    // Act
    ARGV(cstr("prog"), cstr("-v=1"));
    int result = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-v=1" を解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNEXPECTED_VALUE,
        result); // [確認_異常系] - _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNEXPECTED_VALUE であること。
    EXPECT_EQ(COM_UTIL_ERR_UNEXPECTED_VALUE,
              _com_util_argparser_get_error(parser)); // [確認_異常系] - エラー種別が UNEXPECTED_VALUE であること。
    EXPECT_STREQ("--verbose", _com_util_argparser_get_error_target(
                                  parser)); // [確認_異常系] - エラー対象が長い名前 "--verbose" で報告されること。
    EXPECT_EQ(1, _com_util_argparser_get_error_index(parser)); // [確認_異常系] - エラー位置が argv[1] であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// int オプションの各構文 (-c 5 / -c=5 / --count 5 / --count=5 / 負数) の確認
TEST_F(argparserTest, option_int_accepts_all_syntaxes)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = -100;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0,
                                                      &count)); // [状態] - int オプション "-c" / "--count" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-c"), cstr("5"));
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-c 5" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - "-c 5" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(5, count); // [確認_正常系] - "-c 5" で 5 が格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("--count"), cstr("6"));
        int rtc_argparser_parse_2 = _com_util_argparser_parse(parser, argc, argv); // [手順] - "--count 6" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - "--count 6" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(6, count); // [確認_正常系] - "--count 6" で 6 が格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("--count=7"));
        int rtc_argparser_parse_3 = _com_util_argparser_parse(parser, argc, argv); // [手順] - "--count=7" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_3); // [確認_正常系] - "--count=7" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(7, count); // [確認_正常系] - "--count=7" で 7 が格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("-c=8"));
        int rtc_argparser_parse_4 = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-c=8" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_4); // [確認_正常系] - "-c=8" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(8, count); // [確認_正常系] - "-c=8" で 8 が格納されること。
    }
    {
        /* 値位置のトークンは照合しないため負数を渡せる */
        ARGV(cstr("prog"), cstr("-c"), cstr("-5"));
        int rtc_argparser_parse_5 = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-c -5" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_5); // [確認_正常系] - "-c -5" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(-5, count); // [確認_正常系] - 負数 -5 が格納されること。
    }
    {
        count = 42;
        ARGV(cstr("prog"));
        int rtc_argparser_parse_6 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - オプション非出現の引数を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_6); // [確認_正常系] - オプション非出現の引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(42, count); // [確認_正常系] - 非出現時は格納先 42 が変更されないこと。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// int オプションの境界値と変換エラーの確認
TEST_F(argparserTest, option_int_boundary_and_conversion_errors)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0,
                                                      &count)); // [状態] - int オプション "-c" / "--count" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        char int_max[32];
        snprintf(int_max, sizeof(int_max), "%d", INT_MAX);
        ARGV(cstr("prog"), cstr("-c"), int_max);
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - INT_MAX を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - INT_MAX を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(INT_MAX, count); // [確認_正常系] - INT_MAX が格納されること。
    }
    {
        char int_min[32];
        snprintf(int_min, sizeof(int_min), "%d", INT_MIN);
        ARGV(cstr("prog"), cstr("-c"), int_min);
        int rtc_argparser_parse_2 = _com_util_argparser_parse(parser, argc, argv); // [手順] - INT_MIN を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - INT_MIN を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(INT_MIN, count); // [確認_正常系] - INT_MIN が格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("--count=2147483648"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - INT_MAX + 1 の "2147483648" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_OUT_OF_RANGE,
            rtc_argparser_parse_3); // [確認_異常系] - INT_MAX + 1 の "2147483648" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_OUT_OF_RANGE であること。
        EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 範囲外が OUT_OF_RANGE になること。
    }
    {
        ARGV(cstr("prog"), cstr("--count=12a"));
        int rtc_argparser_parse_4 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 数値でない "12a" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_INVALID_INTEGER,
            rtc_argparser_parse_4); // [確認_異常系] - 数値でない "12a" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_INVALID_INTEGER であること。
        EXPECT_EQ(COM_UTIL_ERR_INVALID_INTEGER,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 変換エラーが INVALID_INTEGER になること。
        EXPECT_STREQ("--count", _com_util_argparser_get_error_target(
                                    parser)); // [確認_異常系] - エラー対象が "--count" であること。
    }
    {
        ARGV(cstr("prog"), cstr("--count="));
        int rtc_argparser_parse_5 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 空値の "--count=" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_INVALID_INTEGER,
            rtc_argparser_parse_5); // [確認_異常系] - 空値の "--count=" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_INVALID_INTEGER であること。
        EXPECT_EQ(COM_UTIL_ERR_INVALID_INTEGER,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - int の空値が INVALID_INTEGER になること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 負の位置整数と短いオプションの判別を確認
TEST_F(argparserTest, positional_int_accepts_negative_value_without_hiding_unknown_options)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int value = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_positional_int(parser, "value", NULL, COM_UTIL_ARGPARSER_REQUIRED,
                                                          &value)); // [状態] - 必須の int 位置引数 "value" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-42"));
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - 負数 "-42" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - 負数 "-42" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(-42, value); // [確認_正常系] - 負数 -42 が位置引数として格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("-2147483649"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - INT_MIN - 1 の "-2147483649" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_OUT_OF_RANGE,
            rtc_argparser_parse_2); // [確認_異常系] - INT_MIN - 1 の "-2147483649" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_OUT_OF_RANGE であること。
        EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 範囲外が OUT_OF_RANGE になること。
    }
    {
        ARGV(cstr("prog"), cstr("-x"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 数値でない "-x" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse_3); // [確認_異常系] - 数値でない "-x" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNKNOWN_OPTION であること。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            _com_util_argparser_get_error(parser)); // [確認_異常系] - "-x" が UNKNOWN_OPTION として検出されること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 文字列オプションが argv 内の文字列をそのまま指すことの確認
TEST_F(argparserTest, option_string_points_into_argv)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *name = NULL;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_string(
                               parser, "-n", "--name", "NAME", NULL, 0,
                               &name)); // [状態] - 文字列オプション "-n" / "--name" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-n"), cstr("abc"));
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-n abc" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - "-n abc" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(argv[2], name);  // [確認_正常系] - 格納先が argv[2] そのものを指すこと。
        EXPECT_STREQ("abc", name); // [確認_正常系] - 値が "abc" であること。
    }
    {
        ARGV(cstr("prog"), cstr("--name=xyz"));
        int rtc_argparser_parse_2 = _com_util_argparser_parse(parser, argc, argv); // [手順] - "--name=xyz" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - "--name=xyz" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("xyz", name); // [確認_正常系] - argv トークン内の値部分 "xyz" を指すこと。
    }
    {
        ARGV(cstr("prog"), cstr("--name="));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 空値の "--name=" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_3); // [確認_正常系] - 空値の "--name=" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("", name); // [確認_正常系] - 文字列では空値 "" が受理されること。
    }
    {
        ARGV(cstr("prog"), cstr("-n=xyz"));
        int rtc_argparser_parse_4 = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-n=xyz" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_4); // [確認_正常系] - "-n=xyz" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("xyz", name); // [確認_正常系] - 短いオプションの "=" 区切りが受理されること。
    }
    {
        ARGV(cstr("prog"), cstr("-n="));
        int rtc_argparser_parse_5 = _com_util_argparser_parse(parser, argc, argv); // [手順] - 空値の "-n=" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_5); // [確認_正常系] - 空値の "-n=" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("", name); // [確認_正常系] - 短いオプションでも空値 "" が受理されること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 空白を含む値が全構文 (位置引数 / --param=v / -p=v / --param v / -p v) で同一に取得できることの確認
// シェルや MSVC CRT のクオート除去後の argv (クオートなしの 1 トークン) を模している
TEST_F(argparserTest, option_string_accepts_value_with_spaces)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *param = NULL;
    const char *input = NULL;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_string(
                               parser, "-p", "--param", "VALUE", NULL, 0,
                               &param)); // [状態] - 文字列オプション "-p" / "--param" を登録する。
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_positional_string(parser, "input", NULL, 0,
                                                             &input)); // [状態] - 位置引数 "input" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        /* command "parameter string" 相当 (位置引数) */
        input = NULL;
        ARGV(cstr("prog"), cstr("parameter string"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 空白を含む位置引数を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - 空白を含む位置引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("parameter string", input); // [確認_正常系] - 位置引数で "parameter string" が取得できること。
    }
    {
        param = NULL;
        ARGV(cstr("prog"), cstr("--param=parameter string"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - "--param=parameter string" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - "--param=parameter string" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("parameter string", param); // [確認_正常系] - "--param=" 構文で同一の値が取得できること。
    }
    {
        param = NULL;
        ARGV(cstr("prog"), cstr("-p=parameter string"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - "-p=parameter string" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_3); // [確認_正常系] - "-p=parameter string" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("parameter string", param); // [確認_正常系] - "-p=" 構文で同一の値が取得できること。
    }
    {
        param = NULL;
        ARGV(cstr("prog"), cstr("--param"), cstr("parameter string"));
        int rtc_argparser_parse_4 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - "--param" と後続トークンを解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_4); // [確認_正常系] - "--param" と後続トークンを解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("parameter string", param); // [確認_正常系] - "--param v" 構文で同一の値が取得できること。
    }
    {
        param = NULL;
        ARGV(cstr("prog"), cstr("-p"), cstr("parameter string"));
        int rtc_argparser_parse_5 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - "-p" と後続トークンを解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_5); // [確認_正常系] - "-p" と後続トークンを解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("parameter string", param); // [確認_正常系] - "-p v" 構文で同一の値が取得できること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// argv に残存したダブル クオーテーションを除去せず無加工で格納することの確認
// クオート除去はシェル / C ランタイムの責務であり、本 API は独自解釈しない (値中の正当な " を破壊しないため)
TEST_F(argparserTest, option_string_stores_argv_verbatim)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *param = NULL;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_string(
                               parser, "-p", "--param", "VALUE", NULL, 0,
                               &param)); // [状態] - 文字列オプション "-p" / "--param" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        param = NULL;
        ARGV(cstr("prog"), cstr("--param=\"quoted value\""));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 両端にクオートが残存した値を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - 両端にクオートが残存した値を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("\"quoted value\"", param); // [確認_正常系] - 両端のクオートを除去せずそのまま格納すること。
    }
    {
        param = NULL;
        ARGV(cstr("prog"), cstr("-p"), cstr("say \"hi\" now"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 値の途中にクオートを含むトークンを解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - 値の途中にクオートを含むトークンを解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("say \"hi\" now", param); // [確認_正常系] - 途中のクオートもそのまま格納すること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 位置引数の登録順割り当てと超過エラーの確認
TEST_F(argparserTest, positional_assignment_and_overflow)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *input = NULL;
    int level = -1;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_positional_string(parser, "input", NULL, COM_UTIL_ARGPARSER_REQUIRED,
                                                             &input)); // [状態] - 必須の位置引数 "input" を登録する。
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_positional_int(parser, "level", NULL, 0,
                                                          &level)); // [状態] - 任意の int 位置引数 "level" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("in.txt"), cstr("3"));
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - "in.txt 3" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - "in.txt 3" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("in.txt", input); // [確認_正常系] - 1 番目の位置引数に "in.txt" が割り当てられること。
        EXPECT_EQ(3, level);           // [確認_正常系] - 2 番目の位置引数に 3 が割り当てられること。
    }
    {
        ARGV(cstr("prog"), cstr("in.txt"), cstr("abc"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - int 位置引数に "abc" を渡して解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_INVALID_INTEGER,
            rtc_argparser_parse_2); // [確認_異常系] - int 位置引数に "abc" を渡して解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_INVALID_INTEGER であること。
        EXPECT_EQ(COM_UTIL_ERR_INVALID_INTEGER,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 変換エラーが INVALID_INTEGER になること。
        EXPECT_STREQ("level", _com_util_argparser_get_error_target(
                                  parser)); // [確認_異常系] - エラー対象が位置引数名 "level" であること。
    }
    {
        ARGV(cstr("prog"), cstr("a"), cstr("1"), cstr("extra"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 登録数を超える 3 つの位置引数を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_TOO_MANY_ARGUMENTS,
            rtc_argparser_parse_3); // [確認_異常系] - 登録数を超える 3 つの位置引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_TOO_MANY_ARGUMENTS であること。
        EXPECT_EQ(COM_UTIL_ERR_TOO_MANY_ARGUMENTS,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 超過が TOO_MANY_ARGUMENTS になること。
        EXPECT_STREQ("extra", _com_util_argparser_get_error_target(
                                  parser)); // [確認_異常系] - エラー対象が超過トークン "extra" であること。
        EXPECT_EQ(3, _com_util_argparser_get_error_index(parser)); // [確認_異常系] - エラー位置が argv[3] であること。
    }
    {
        ARGV(cstr("prog"));
        int rtc_argparser_parse_4 = _com_util_argparser_parse(parser, argc, argv); // [手順] - 位置引数なしで解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_MISSING_REQUIRED,
            rtc_argparser_parse_4); // [確認_異常系] - 位置引数なしで解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_MISSING_REQUIRED であること。
        EXPECT_EQ(COM_UTIL_ERR_MISSING_REQUIRED,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 必須欠落が MISSING_REQUIRED になること。
        EXPECT_STREQ("input",
                     _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が "input" であること。
        EXPECT_EQ(-1, _com_util_argparser_get_error_index(
                          parser)); // [確認_異常系] - エラー位置が -1 (特定位置なし) であること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 可変長文字列位置引数の割り当て、容量超過、再解析の確認
TEST_F(argparserTest, positional_string_array_assignment_and_reparse)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *input = NULL;
    const char *files[2] = {};
    size_t file_count = 99;
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string(
                               parser, "input", NULL, COM_UTIL_ARGPARSER_REQUIRED,
                               &input)); // [状態] - 必須の単数位置引数 "input" を登録する。
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string_array(
                               parser, "files", NULL, 0, files, 2,
                               &file_count)); // [状態] - 任意の可変長位置引数 "files" を容量 2 で登録する。
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(
                               parser, "-v", "--verbose", NULL,
                               &verbose)); // [状態] - 可変長位置引数の後にフラグ "--verbose" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("in.txt"), cstr("a.txt"), cstr("-v"), cstr("b.txt"));
        int rtc_argparser_parse = _com_util_argparser_parse(
            parser, argc, argv); // [手順] - 単数位置引数、可変長位置引数、フラグが混在する入力を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - 単数位置引数、可変長位置引数、フラグが混在する入力を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_STREQ("in.txt", input);    // [確認_正常系] - 先頭の位置引数が "input" に割り当てられること。
        EXPECT_EQ((size_t)2, file_count); // [確認_正常系] - 可変長位置引数の件数が 2 であること。
        EXPECT_EQ(argv[2], files[0]);     // [確認_正常系] - 1 件目の可変長位置引数が argv[2] を指すこと。
        EXPECT_EQ(argv[4], files[1]);     // [確認_正常系] - フラグを挟んだ 2 件目が argv[4] を指すこと。
        EXPECT_EQ(1, verbose);            // [確認_正常系] - 混在したフラグも解析されること。
    }
    {
        ARGV(cstr("prog"), cstr("in.txt"), cstr("a.txt"), cstr("b.txt"), cstr("c.txt"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 容量 2 を超える 3 件の可変長位置引数を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_TOO_MANY_ARGUMENTS,
            rtc_argparser_parse_2); // [確認_異常系] - 容量 2 を超える 3 件の可変長位置引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_TOO_MANY_ARGUMENTS であること。
        EXPECT_EQ(COM_UTIL_ERR_TOO_MANY_ARGUMENTS,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 容量超過が TOO_MANY_ARGUMENTS になること。
        EXPECT_STREQ(
            "c.txt",
            _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が超過した "c.txt" であること。
        EXPECT_EQ(4,
                  _com_util_argparser_get_error_index(
                      parser)); // [確認_異常系] - エラー位置が超過トークンの argv[4] であること。
    }
    {
        ARGV(cstr("prog"), cstr("in.txt"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 可変長位置引数を省略して再解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_3); // [確認_正常系] - 可変長位置引数を省略して再解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ((size_t)0,
                  file_count); // [確認_正常系] - 再解析時に可変長位置引数の件数が 0 へ初期化されること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 可変長 int 位置引数の変換、負数、必須条件の確認
TEST_F(argparserTest, positional_int_array_conversion_and_required)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int values[3] = {};
    size_t value_count = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_int_array(
                               parser, "values", NULL, COM_UTIL_ARGPARSER_REQUIRED, values, 3,
                               &value_count)); // [状態] - 必須の可変長 int 位置引数 "values" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-42"), cstr("7"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 負数と正数を可変長 int 位置引数として解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - 負数と正数を可変長 int 位置引数として解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ((size_t)2, value_count); // [確認_正常系] - 解析した値の件数が 2 であること。
        EXPECT_EQ(-42, values[0]);         // [確認_正常系] - 負数 -42 が値として格納されること。
        EXPECT_EQ(7, values[1]);           // [確認_正常系] - 正数 7 が値として格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("12a"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 整数へ変換できない "12a" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_INVALID_INTEGER,
            rtc_argparser_parse_2); // [確認_異常系] - 整数へ変換できない "12a" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_INVALID_INTEGER であること。
        EXPECT_EQ(COM_UTIL_ERR_INVALID_INTEGER,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 変換エラーが INVALID_INTEGER になること。
        EXPECT_STREQ("values",
                     _com_util_argparser_get_error_target(
                         parser)); // [確認_異常系] - 変換エラーの対象が位置引数名 "values" であること。
    }
    {
        ARGV(cstr("prog"), cstr("-2147483649"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - int の下限を下回る値を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_OUT_OF_RANGE,
            rtc_argparser_parse_3); // [確認_異常系] - int の下限を下回る値を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_OUT_OF_RANGE であること。
        EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 範囲外エラーが OUT_OF_RANGE になること。
    }
    {
        ARGV(cstr("prog"));
        int rtc_argparser_parse_4 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 必須の可変長位置引数を省略して解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_MISSING_REQUIRED,
            rtc_argparser_parse_4); // [確認_異常系] - 必須の可変長位置引数を省略して解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_MISSING_REQUIRED であること。
        EXPECT_EQ(COM_UTIL_ERR_MISSING_REQUIRED,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - 必須欠落が MISSING_REQUIRED になること。
        EXPECT_STREQ("values",
                     _com_util_argparser_get_error_target(
                         parser)); // [確認_異常系] - 必須欠落エラーの対象が位置引数名 "values" であること。
    }

    char usage[256];
    int rtc_argparser_get_usage = _com_util_argparser_get_usage(
        parser, usage, sizeof(usage), NULL); // [手順] - 必須の可変長位置引数を含む usage を組み立てる。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_usage); // [確認_正常系] - _com_util_argparser_get_usage の戻り値として、必須の可変長位置引数を含む usage を組み立てた結果が COM_UTIL_OK であること。
    EXPECT_THAT(
        std::string(usage),
        HasSubstr(
            "Usage: prog <values>...\n")); // [確認_正常系] - 必須の可変長位置引数が <values>... と表示されること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 未知のオプションが検出されることの確認 (短オプション連結を含む)
TEST_F(argparserTest, unknown_option_detection)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &verbose)); // [状態] - フラグ "-v" / "--verbose" だけを登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-x"));
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - 未登録の "-x" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse); // [確認_異常系] - 未登録の "-x" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNKNOWN_OPTION であること。
        EXPECT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - UNKNOWN_OPTION になること。
        EXPECT_STREQ("-x",
                     _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が "-x" であること。
    }
    {
        ARGV(cstr("prog"), cstr("-vv"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 短オプション連結の "-vv" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse_2); // [確認_異常系] - 短オプション連結の "-vv" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNKNOWN_OPTION であること。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            _com_util_argparser_get_error(parser)); // [確認_異常系] - 連結は未サポートで UNKNOWN_OPTION になること。
    }
    {
        ARGV(cstr("prog"), cstr("--bogus"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 未登録の "--bogus" を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse_3); // [確認_異常系] - 未登録の "--bogus" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNKNOWN_OPTION であること。
        EXPECT_STREQ("--bogus", _com_util_argparser_get_error_target(
                                    parser)); // [確認_異常系] - エラー対象が "--bogus" であること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 末尾の値なしオプションが検出されることの確認
TEST_F(argparserTest, missing_value_at_end)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0,
                                                      &count)); // [状態] - int オプション "-c" / "--count" を登録する。

    // Pre-Assert

    // Act
    ARGV(cstr("prog"), cstr("--count"));
    int result = _com_util_argparser_parse(parser, argc, argv); // [手順] - 値なしの末尾 "--count" を解析する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_MISSING_VALUE,
              result); // [確認_異常系] - _com_util_argparser_parse の戻り値が COM_UTIL_ERR_MISSING_VALUE であること。
    EXPECT_EQ(COM_UTIL_ERR_MISSING_VALUE,
              _com_util_argparser_get_error(parser)); // [確認_異常系] - エラー種別が MISSING_VALUE であること。
    EXPECT_STREQ("--count",
                 _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が "--count" であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 単数オプションの重複出現 (short / long 混在) がエラーになることの確認
TEST_F(argparserTest, duplicate_option_occurrence)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0,
                                                      &count)); // [状態] - int オプション "-c" / "--count" を登録する。

    // Pre-Assert

    // Act
    ARGV(cstr("prog"), cstr("-c"), cstr("1"), cstr("--count=2"));
    int result = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-c 1" と "--count=2" を併記して解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_OPTION,
        result); // [確認_異常系] - _com_util_argparser_parse の戻り値が COM_UTIL_ERR_DUPLICATE_OPTION であること。
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_OPTION,
              _com_util_argparser_get_error(parser)); // [確認_異常系] - エラー種別が DUPLICATE_OPTION であること。
    EXPECT_STREQ("--count",
                 _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が "--count" であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 配列オプションの複数出現・出現順・容量超過の確認
TEST_F(argparserTest, array_option_multiple_occurrences)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *includes[2] = {};
    size_t include_count = 99;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_string_array(
                  parser, "-i", "--include", "DIR", NULL, 0, includes, 2,
                  &include_count)); // [状態] - 容量 2 の文字列配列オプション "-i" / "--include" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-i"), cstr("dir1"), cstr("--include=dir2"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - "-i dir1 --include=dir2" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - "-i dir1 --include=dir2" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ((size_t)2, include_count); // [確認_正常系] - 出現数 2 が格納されること。
        EXPECT_STREQ("dir1", includes[0]);   // [確認_正常系] - 1 番目に "dir1" が出現順で格納されること。
        EXPECT_STREQ("dir2", includes[1]);   // [確認_正常系] - 2 番目に "dir2" が出現順で格納されること。
    }
    {
        ARGV(cstr("prog"), cstr("-i"), cstr("a"), cstr("-i"), cstr("b"), cstr("-i"), cstr("c"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 容量 2 を超える 3 回の出現を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_TOO_MANY_OCCURRENCES,
            rtc_argparser_parse_2); // [確認_異常系] - 容量 2 を超える 3 回の出現を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_TOO_MANY_OCCURRENCES であること。
        EXPECT_EQ(
            COM_UTIL_ERR_TOO_MANY_OCCURRENCES,
            _com_util_argparser_get_error(parser)); // [確認_異常系] - 容量超過が TOO_MANY_OCCURRENCES になること。
        EXPECT_STREQ("--include", _com_util_argparser_get_error_target(
                                      parser)); // [確認_異常系] - エラー対象が "--include" であること。
    }
    {
        ARGV(cstr("prog"));
        int rtc_argparser_parse_3 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - オプション非出現の引数を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_3); // [確認_正常系] - オプション非出現の引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ((size_t)0, include_count); // [確認_正常系] - 非出現時は count が 0 に初期化されること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// int 配列オプションの値変換と REQUIRED の確認
TEST_F(argparserTest, array_option_int_and_required)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int ports[4] = {};
    size_t port_count = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_int_array(
                               parser, "-p", "--port", "PORT", NULL, COM_UTIL_ARGPARSER_REQUIRED, ports, 4,
                               &port_count)); // [状態] - 必須の int 配列オプション "-p" / "--port" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-p"), cstr("80"), cstr("-p"), cstr("443"));
        int rtc_argparser_parse = _com_util_argparser_parse(parser, argc, argv); // [手順] - "-p 80 -p 443" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - "-p 80 -p 443" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ((size_t)2, port_count); // [確認_正常系] - 出現数 2 が格納されること。
        EXPECT_EQ(80, ports[0]);          // [確認_正常系] - 1 番目に 80 が格納されること。
        EXPECT_EQ(443, ports[1]);         // [確認_正常系] - 2 番目に 443 が格納されること。
    }
    {
        ARGV(cstr("prog"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - オプション非出現の引数を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_MISSING_REQUIRED,
            rtc_argparser_parse_2); // [確認_異常系] - オプション非出現の引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_MISSING_REQUIRED であること。
        EXPECT_EQ(COM_UTIL_ERR_MISSING_REQUIRED,
                  _com_util_argparser_get_error(
                      parser)); // [確認_異常系] - REQUIRED の配列オプション欠落が MISSING_REQUIRED になること。
        EXPECT_STREQ("--port", _com_util_argparser_get_error_target(
                                   parser)); // [確認_異常系] - エラー対象が "--port" であること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 必須オプションの欠落が検出されることの確認
TEST_F(argparserTest, missing_required_option)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_int(
                               parser, "-c", "--count", "N", NULL, COM_UTIL_ARGPARSER_REQUIRED,
                               &count)); // [状態] - 必須の int オプション "-c" / "--count" を登録する。

    // Pre-Assert

    // Act
    ARGV(cstr("prog"));
    int result = _com_util_argparser_parse(parser, argc, argv); // [手順] - オプション非出現の引数を解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_MISSING_REQUIRED,
        result); // [確認_異常系] - _com_util_argparser_parse の戻り値が COM_UTIL_ERR_MISSING_REQUIRED であること。
    EXPECT_EQ(COM_UTIL_ERR_MISSING_REQUIRED,
              _com_util_argparser_get_error(parser)); // [確認_異常系] - エラー種別が MISSING_REQUIRED であること。
    EXPECT_STREQ("--count",
                 _com_util_argparser_get_error_target(parser)); // [確認_異常系] - エラー対象が "--count" であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 解析エラー後に成功した parse でエラー状態がクリアされることの確認
TEST_F(argparserTest, reparse_clears_error_state)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &verbose)); // [状態] - フラグ "-v" / "--verbose" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-x"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 未登録の "-x" で解析エラーを発生させる。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse); // [確認_異常系] - _com_util_argparser_parse の戻り値として、未登録の "-x" で解析エラーを発生させた結果が COM_UTIL_ERR_UNKNOWN_OPTION であること。
        EXPECT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
                  _com_util_argparser_get_error(parser)); // [確認_異常系] - UNKNOWN_OPTION が記録されること。
    }
    {
        ARGV(cstr("prog"), cstr("-v"));
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 正しい引数 "-v" で再解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse_2); // [確認_正常系] - 正しい引数 "-v" で再解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        EXPECT_EQ(COM_UTIL_OK,
                  _com_util_argparser_get_error(parser)); // [確認_正常系] - エラー種別が NONE に戻ること。
        EXPECT_EQ(nullptr,
                  _com_util_argparser_get_error_target(parser));    // [確認_正常系] - エラー対象が NULL に戻ること。
        EXPECT_EQ(-1, _com_util_argparser_get_error_index(parser)); // [確認_正常系] - エラー位置が -1 に戻ること。
    }

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 複数ハンドルが独立して動作することの確認
TEST_F(argparserTest, multiple_handles_are_independent)
{
    // Arrange
    com_util_argparser *parser1 = _com_util_argparser_create(NULL);
    com_util_argparser *parser2 = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser1);
    ASSERT_NE(nullptr, parser2);
    int flag1 = 0;
    int flag2 = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser1, "-a", NULL, NULL,
                                                             &flag1)); // [状態] - parser1 に "-a" を登録する。
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser2, "-b", NULL, NULL,
                                                             &flag2)); // [状態] - parser2 に "-b" を登録する。

    // Pre-Assert

    // Act
    // Assert
    {
        ARGV(cstr("prog"), cstr("-a"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser1, argc, argv); // [手順] - parser1 で "-a" を解析する。
        EXPECT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - parser1 で "-a" を解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
        int rtc_argparser_parse_2 =
            _com_util_argparser_parse(parser2, argc, argv); // [手順] - "-a" 未登録の parser2 でも同じ引数を解析する。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse_2); // [確認_異常系] - "-a" 未登録の parser2 でも同じ引数を解析した _com_util_argparser_parse の戻り値が COM_UTIL_ERR_UNKNOWN_OPTION であること。
    }
    EXPECT_EQ(1, flag1); // [確認_正常系] - parser1 側のフラグだけが 1 になること。
    EXPECT_EQ(0, flag2); // [確認_正常系] - parser2 側のフラグが 0 のままであること。
    EXPECT_EQ(COM_UTIL_OK,
              _com_util_argparser_get_error(parser1)); // [確認_正常系] - parser1 のエラー状態が NONE であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
              _com_util_argparser_get_error(
                  parser2)); // [確認_正常系] - parser2 のエラー状態が独立して UNKNOWN_OPTION であること。

    // Cleanup
    _com_util_argparser_dispose(parser1);
    _com_util_argparser_dispose(parser2);
}

// エラー メッセージの組み立てとバッファー不足の確認
TEST_F(argparserTest, error_message_formatting)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL,
                                                &verbose)); // [状態] - フラグ "-v" / "--verbose" を登録する。
    char message[128];

    // Pre-Assert

    // Act
    int rtc_argparser_get_error_message = _com_util_argparser_get_error_message(
        parser, message, sizeof(message)); // [手順] - 未解析の状態でエラー メッセージを取得する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_error_message); // [確認_正常系] - 未解析の状態でエラー メッセージを取得した _com_util_argparser_get_error_message の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("no error", message);    // [確認_正常系] - 未解析時は "no error" が返ること。

    {
        ARGV(cstr("prog"), cstr("--bogus"));
        int rtc_argparser_parse =
            _com_util_argparser_parse(parser, argc, argv); // [手順] - 未登録の "--bogus" で解析エラーを発生させる。
        EXPECT_EQ(
            COM_UTIL_ERR_UNKNOWN_OPTION,
            rtc_argparser_parse); // [確認_異常系] - _com_util_argparser_parse の戻り値として、未登録の "--bogus" で解析エラーを発生させた結果が COM_UTIL_ERR_UNKNOWN_OPTION であること。
    }
    EXPECT_EQ(COM_UTIL_OK, _com_util_argparser_get_error_message(parser, message, sizeof(message)));
    EXPECT_STREQ("unknown option '--bogus'",
                 message); // [確認_正常系] - "unknown option '--bogus'" が組み立てられること。

    char small_buffer[8];
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        _com_util_argparser_get_error_message(
            parser, small_buffer,
            sizeof(
                small_buffer))); // [確認_異常系] - _com_util_argparser_get_error_message の戻り値として、バッファー不足時に BUFFER_TOO_SMALL が返ること。
    EXPECT_STREQ("unknown", small_buffer); // [確認_異常系] - 7 文字 + NUL に切り詰められること。

    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_error_message(
                  NULL, message, sizeof(message))); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_error_message(parser, NULL,
                                                    1)); // [確認_異常系] - buffer NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_error_message(parser, message,
                                                    0)); // [確認_異常系] - サイズ 0 が INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// usage の組み立て内容の確認
TEST_F(argparserTest, usage_formatting)
{
    // Arrange
    com_util_argparser_options options = {};
    options.program_name = "sample";
    options.program_description =
        "Sample tool"; // [状態] - program_name "sample"、説明 "Sample tool" の生成オプションとする。
    com_util_argparser *parser = _com_util_argparser_create(&options);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    int count = 0;
    const char *name = NULL;
    const char *input = NULL;
    const char *output = NULL;
    const char *files[2] = {};
    size_t file_count = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-v", "--verbose", "verbose output", &verbose));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_int(parser, "-c", "--count", "N", "count value",
                                                                   COM_UTIL_ARGPARSER_REQUIRED, &count));
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_string(parser, NULL, "--name", "NAME", "display name", 0, &name));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string(parser, "input", "input file",
                                                                          COM_UTIL_ARGPARSER_REQUIRED, &input));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string(parser, "output", "output file", 0, &output));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string_array(
                               parser, "files", "additional files", 0, files, 2,
                               &file_count)); // [状態] - フラグ、オプション、単数・可変長位置引数を一式登録する。

    // Pre-Assert

    // Act
    char usage[1024];
    size_t required_size = 0;
    int rtc_argparser_get_usage = _com_util_argparser_get_usage(
        parser, usage, sizeof(usage), &required_size); // [手順] - get_usage で usage を組み立てる。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_usage); // [確認_正常系] - _com_util_argparser_get_usage の戻り値として、get_usage で usage を組み立てた結果が COM_UTIL_OK であること。

    // Assert
    std::string usage_text(usage);
    EXPECT_EQ(strlen(usage) + 1, required_size);           // [確認_正常系] - required_size が文字列長 + 1 であること。
    EXPECT_THAT(usage_text, HasSubstr("Sample tool\n\n")); // [確認_正常系] - 説明行が含まれること。
    EXPECT_THAT(
        usage_text,
        HasSubstr(
            "Usage: sample [OPTIONS] <input> [output] [files...]\n")); // [確認_正常系] - Usage 行に単数・可変長位置引数が反映されること。
    EXPECT_THAT(usage_text,
                HasSubstr("\nPositional arguments:\n")); // [確認_正常系] - 位置引数セクションが含まれること。
    EXPECT_THAT(
        usage_text,
        HasSubstr(
            "  input                     input file (required)\n")); // [確認_正常系] - 必須位置引数に (required) が付くこと。
    EXPECT_THAT(
        usage_text,
        HasSubstr("  output                    output file\n")); // [確認_正常系] - 任意位置引数の行が含まれること。
    EXPECT_THAT(
        usage_text,
        HasSubstr(
            "  files                     additional files\n")); // [確認_正常系] - 可変長位置引数の行が含まれること。
    EXPECT_THAT(usage_text, HasSubstr("\nOptions:\n"));         // [確認_正常系] - オプション セクションが含まれること。
    EXPECT_THAT(
        usage_text,
        HasSubstr("  -v, --verbose             verbose output\n")); // [確認_正常系] - フラグの行が含まれること。
    EXPECT_THAT(
        usage_text,
        HasSubstr(
            "  -c, --count N             count value (required)\n")); // [確認_正常系] - 必須オプションに (required) が付くこと。
    EXPECT_THAT(
        usage_text,
        HasSubstr(
            "      --name NAME           display name\n")); // [確認_正常系] - 長い名前のみのオプション行が含まれること。

    size_t query_size = 0;
    int rtc_argparser_get_usage_2 = _com_util_argparser_get_usage(
        parser, NULL, 0, &query_size); // [手順] - buffer NULL でサイズ問い合わせのみ行う。
    EXPECT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_usage_2); // [確認_正常系] - _com_util_argparser_get_usage の戻り値として、buffer NULL でサイズ問い合わせのみ行った結果が COM_UTIL_OK であること。
    EXPECT_EQ(required_size, query_size); // [確認_正常系] - 問い合わせサイズが required_size と一致すること。

    char small_buffer[16];
    size_t small_required = 0;
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        _com_util_argparser_get_usage(
            parser, small_buffer, sizeof(small_buffer),
            &small_required)); // [確認_異常系] - _com_util_argparser_get_usage の戻り値として、バッファー不足時に BUFFER_TOO_SMALL が返ること。
    EXPECT_EQ(required_size, small_required);    // [確認_異常系] - 不足時も必要サイズが報告されること。
    EXPECT_EQ((size_t)15, strlen(small_buffer)); // [確認_異常系] - 15 文字 + NUL に切り詰められること。

    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_get_usage(NULL, usage, sizeof(usage),
                                            NULL)); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_get_usage(
            parser, NULL, 0, NULL)); // [確認_異常系] - buffer NULL かつ required NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_get_usage(
            parser, usage, 0, NULL)); // [確認_異常系] - サイズ 0 かつ required NULL が INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_usage が不正引数を検出することの確認
TEST_F(argparserTest, print_usage_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 生成済みの parser を用意する。
    ASSERT_NE(nullptr, parser);

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_print_usage(NULL, stdout)); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_print_usage(parser, NULL)); // [確認_異常系] - stream NULL が INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_usage が組み立てた usage を指定ストリームへ書き出すことの確認
TEST_F(argparserTest, print_usage_writes_to_stream)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser_options options = {};
    options.program_name = "sample"; // [状態] - program_name を "sample" とする。
    com_util_argparser *parser = _com_util_argparser_create(&options);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-v", "--verbose", "verbose output", &verbose));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stdout, HasSubstr("Usage: sample [OPTIONS]")))
        .Times(
            1); // [Pre-Assert確認_正常系] - "Usage: sample [OPTIONS]" を含む usage が stdout へ 1 回書き出されること。

    // Act
    int result = _com_util_argparser_print_usage(parser, stdout); // [手順] - print_usage を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - _com_util_argparser_print_usage の戻り値が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_error_messages が不正引数を検出することの確認
TEST_F(argparserTest, print_error_messages_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 生成済みの parser を用意する。
    ASSERT_NE(nullptr, parser);

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_print_error_messages(
                  NULL, stderr)); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_print_error_messages(
                  parser, NULL)); // [確認_異常系] - stream NULL が INVALID_ARGUMENT になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_error_messages がエラーなしの場合に何も出力しないことの確認
TEST_F(argparserTest, print_error_messages_is_noop_without_error)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 解析エラーのない parser を用意する。
    ASSERT_NE(nullptr, parser);

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - fprintf が呼び出されないこと。

    // Act
    int result = _com_util_argparser_print_error_messages(parser, stderr); // [手順] - print_error_messages を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - _com_util_argparser_print_error_messages の戻り値が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_error_messages が直前の解析エラーのメッセージを指定ストリームへ書き出すことの確認
TEST_F(argparserTest, print_error_messages_writes_to_stream)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));

    ARGV(cstr("prog"), cstr("--bogus"));
    ASSERT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
              _com_util_argparser_parse(parser, argc, argv)); // [状態] - "--bogus" の解析エラーを発生させた状態とする。

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _))
        .Times(AnyNumber()); // [Pre-Assert手順] - 区切りの空行の fprintf 呼び出しを許容する。
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, HasSubstr("error: unknown option '--bogus'")))
        .Times(1); // [Pre-Assert確認_正常系] - "error: unknown option '--bogus'" が stderr へ 1 回書き出されること。

    // Act
    int result = _com_util_argparser_print_error_messages(parser, stderr); // [手順] - print_error_messages を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - _com_util_argparser_print_error_messages の戻り値が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// program_name 未指定時に argv[0] のベース名が usage に反映されることの確認
TEST_F(argparserTest, usage_program_name_resolution)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - program_name 未指定で parser を生成する。
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-v", NULL, NULL, &verbose));
    char usage[256];

    // Pre-Assert

    // Act
    int rtc_argparser_get_usage =
        _com_util_argparser_get_usage(parser, usage, sizeof(usage), NULL); // [手順] - 解析前に usage を取得する。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_usage); // [確認_正常系] - 解析前に usage を取得した _com_util_argparser_get_usage の戻り値が COM_UTIL_OK であること。
    EXPECT_THAT(
        std::string(usage),
        HasSubstr(
            "Usage: {program} [OPTIONS]\n")); // [確認_正常系] - 解析前はプレースホルダー {program} が使われること。

    {
        ARGV(cstr("/usr/local/bin/mytool"), cstr("-v"));
        int rtc_argparser_parse = _com_util_argparser_parse(
            parser, argc, argv); // [手順] - argv[0] を "/usr/local/bin/mytool" として解析する。
        ASSERT_EQ(
            COM_UTIL_OK,
            rtc_argparser_parse); // [確認_正常系] - argv[0] を "/usr/local/bin/mytool" として解析した _com_util_argparser_parse の戻り値が COM_UTIL_OK であること。
    }
    int rtc_argparser_get_usage_2 =
        _com_util_argparser_get_usage(parser, usage, sizeof(usage), NULL); // [手順] - 解析後に usage を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_argparser_get_usage_2); // [確認_正常系] - 解析後に usage を取得した _com_util_argparser_get_usage の戻り値が COM_UTIL_OK であること。
    EXPECT_THAT(std::string(usage),
                HasSubstr("Usage: mytool [OPTIONS]\n")); // [確認_正常系] - argv[0] のベース名 "mytool" が使われること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// parse の不正引数が検出されることの確認
TEST_F(argparserTest, parse_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL); // [状態] - 生成済みの parser を用意する。
    ASSERT_NE(nullptr, parser);
    ARGV(cstr("prog"));

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_parse(NULL, argc, argv)); // [確認_異常系] - parser NULL が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              _com_util_argparser_parse(parser, 0, argv)); // [確認_異常系] - argc 0 が INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        _com_util_argparser_parse(parser, argc, NULL)); // [確認_異常系] - argv NULL が INVALID_ARGUMENT になること。

    EXPECT_EQ(COM_UTIL_OK,
              _com_util_argparser_get_error(NULL)); // [確認_異常系] - get_error が NULL ハンドルで NONE を返すこと。
    EXPECT_EQ(nullptr, _com_util_argparser_get_error_target(
                           NULL)); // [確認_異常系] - get_error_target が NULL ハンドルで NULL を返すこと。
    EXPECT_EQ(-1, _com_util_argparser_get_error_index(
                      NULL)); // [確認_異常系] - get_error_index が NULL ハンドルで -1 を返すこと。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 長いオプション名の各形式を正しく判定することの確認
TEST_F(argparserTest, is_valid_long_name_rejects_invalid_names)
{
    // Arrange
    const char *short_name = "--";           // [状態] - 長さが不足する長い名前を用意する。
    const char *wrong_prefix = "-name";      // [状態] - 接頭辞が不正な長い名前を用意する。
    const char *with_equal = "--name=value"; // [状態] - "=" を含む長い名前を用意する。

    // Pre-Assert

    // Act
    int null_result = test_argparser_is_valid_long_name(NULL);        // [手順] - NULL を指定して長い名前を検証する。
    int short_result = test_argparser_is_valid_long_name(short_name); // [手順] - 短すぎる名前を検証する。
    int wrong_prefix_result =
        test_argparser_is_valid_long_name(wrong_prefix);              // [手順] - 接頭辞が不正な名前を検証する。
    int equal_result = test_argparser_is_valid_long_name(with_equal); // [手順] - "=" を含む名前を検証する。
    int valid_result = test_argparser_is_valid_long_name("--name");   // [手順] - 正しい長い名前を検証する。

    // Assert
    EXPECT_EQ(0, null_result);         // [確認_異常系] - NULL の検証結果が 0 であること。
    EXPECT_EQ(0, short_result);        // [確認_異常系] - 長さ不足の検証結果が 0 であること。
    EXPECT_EQ(0, wrong_prefix_result); // [確認_異常系] - 接頭辞不正の検証結果が 0 であること。
    EXPECT_EQ(0, equal_result);        // [確認_異常系] - "=" を含む名前の検証結果が 0 であること。
    EXPECT_EQ(1, valid_result);        // [確認_正常系] - 正しい長い名前の検証結果が 1 であること。
}

// 位置引数登録の parser NULL と不正フラグが検出されることの確認
TEST_F(argparserTest, positional_register_rejects_null_parser_and_invalid_flags)
{
    // Arrange
    int storage = 0;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);

    // Pre-Assert

    // Act
    int null_parser_result = _com_util_argparser_register_positional_int(
        NULL, "value", NULL, 0u, &storage); // [手順] - parser NULL で位置引数を登録する。
    int invalid_flags_result = _com_util_argparser_register_positional_int(
        parser, "value", NULL, 0x100u, &storage); // [手順] - 未定義フラグで位置引数を登録する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_parser_result); // [確認_異常系] - parser NULL の登録結果が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              invalid_flags_result); // [確認_異常系] - 不正フラグの登録結果が INVALID_ARGUMENT であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// default parser の初期化失敗と shutdown callback の入力検証が動作することの確認
TEST_F(argparserTest, default_returns_null_when_lock_creation_fails)
{
    // Arrange
    com_util_shutdown_event event = {};
    event.reason = COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT;

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_create(_))
        .WillOnce(
            Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - default 用ロックの生成が 1 回失敗すること。

    // Act
    com_util_argparser *parser =
        _com_util_argparser_default(NULL);                  // [手順] - ロック生成失敗状態で default parser を取得する。
    test_argparser_default_dispose_on_shutdown(NULL, NULL); // [手順] - NULL event で shutdown callback を呼び出す。
    event.reason = COM_UTIL_SHUTDOWN_REASON_PROCESS_TERMINATING;
    test_argparser_default_dispose_on_shutdown(&event, NULL); // [手順] - 通常終了以外の event で callback を呼び出す。
    event.reason = COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT;
    test_argparser_default_dispose_on_shutdown(&event,
                                               NULL); // [手順] - ロック未生成状態で通常終了 callback を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, parser); // [確認_異常系] - ロック生成失敗時の default parser が NULL であること。
}

// shutdown 登録失敗時に default parser が NULL を返しロックを破棄することの確認
TEST_F(argparserTest, default_returns_null_when_shutdown_registration_fails)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_shutdown_register(_, _))
        .WillOnce(
            Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - shutdown callback 登録が 1 回失敗すること。

    // Act
    com_util_argparser *parser =
        _com_util_argparser_default(NULL); // [手順] - shutdown 登録失敗状態で default parser を取得する。

    // Assert
    EXPECT_EQ(nullptr, parser); // [確認_異常系] - shutdown 登録失敗時の default parser が NULL であること。
}

// default shutdown callback がロック取得失敗時に処理を中断することの確認
TEST_F(argparserTest, default_shutdown_callback_returns_when_locking_fails)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_default(NULL);
    ASSERT_NE(nullptr, parser);
    com_util_shutdown_event event = {};
    event.reason = COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT;

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_lock(_, _))
        .WillOnce(
            Return(COM_UTIL_ERR_BUSY)); // [Pre-Assert確認_異常系] - shutdown callback のロック取得が失敗すること。

    // Act
    test_argparser_default_dispose_on_shutdown(&event,
                                               NULL); // [手順] - ロック取得失敗状態で shutdown callback を呼び出す。

    // Assert
    SUCCEED(); // [確認_異常系] - ロック取得失敗時も shutdown callback がクラッシュせず終了すること。
}

// default parser のロック取得失敗時に NULL が返ることの確認
TEST_F(argparserTest, default_returns_null_when_locking_fails)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_lock(_, _))
        .WillOnce(Return(COM_UTIL_ERR_BUSY)); // [Pre-Assert確認_異常系] - default parser のロック取得が失敗すること。

    // Act
    com_util_argparser *parser =
        _com_util_argparser_default(NULL); // [手順] - ロック取得失敗状態で default parser を取得する。

    // Assert
    EXPECT_EQ(nullptr, parser); // [確認_異常系] - ロック取得失敗時の default parser が NULL であること。
}

// 各登録 API の引数検証と登録結果伝播が動作することの確認
TEST_F(argparserTest, register_wrappers_reject_invalid_storage_and_registration)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int int_array[1] = {};
    size_t int_count = 0;
    const char *string_array[1] = {};
    size_t string_count = 0;

    // Pre-Assert

    // Act
    int option_int_null = _com_util_argparser_register_option_int(
        parser, "-i", NULL, NULL, NULL, 0u, NULL); // [手順] - int オプションの storage NULL を検証する。
    int option_string_null = _com_util_argparser_register_option_string(
        parser, "-s", NULL, NULL, NULL, 0u, NULL); // [手順] - string オプションの storage NULL を検証する。
    int option_int_array_storage_null = _com_util_argparser_register_option_int_array(
        parser, "-a", NULL, NULL, NULL, 0u, NULL, 1u, &int_count); // [手順] - int 配列の storage NULL を検証する。
    int option_int_array_capacity_zero = _com_util_argparser_register_option_int_array(
        parser, "-b", NULL, NULL, NULL, 0u, int_array, 0u, &int_count); // [手順] - int 配列の capacity 0 を検証する。
    int option_int_array_count_null = _com_util_argparser_register_option_int_array(
        parser, "-c", NULL, NULL, NULL, 0u, int_array, 1u, NULL); // [手順] - int 配列の count NULL を検証する。
    int option_string_array_storage_null = _com_util_argparser_register_option_string_array(
        parser, "-d", NULL, NULL, NULL, 0u, NULL, 1u,
        &string_count); // [手順] - string 配列の storage NULL を検証する。
    int option_string_array_capacity_zero = _com_util_argparser_register_option_string_array(
        parser, "-e", NULL, NULL, NULL, 0u, string_array, 0u,
        &string_count); // [手順] - string 配列の capacity 0 を検証する。
    int option_string_array_count_null = _com_util_argparser_register_option_string_array(
        parser, "-f", NULL, NULL, NULL, 0u, string_array, 1u, NULL); // [手順] - string 配列の count NULL を検証する。
    int positional_int_null = _com_util_argparser_register_positional_int(
        parser, "value", NULL, 0u, NULL); // [手順] - int 位置引数の storage NULL を検証する。
    int positional_string_null = _com_util_argparser_register_positional_string(
        parser, "text", NULL, 0u, NULL); // [手順] - string 位置引数の storage NULL を検証する。
    int positional_int_array_storage_null = _com_util_argparser_register_positional_int_array(
        parser, "values", NULL, 0u, NULL, 1u, &int_count); // [手順] - int 配列位置引数の storage NULL を検証する。
    int positional_int_array_capacity_zero = _com_util_argparser_register_positional_int_array(
        parser, "values2", NULL, 0u, int_array, 0u, &int_count); // [手順] - int 配列位置引数の capacity 0 を検証する。
    int positional_int_array_count_null = _com_util_argparser_register_positional_int_array(
        parser, "values3", NULL, 0u, int_array, 1u, NULL); // [手順] - int 配列位置引数の count NULL を検証する。
    int positional_string_array_storage_null = _com_util_argparser_register_positional_string_array(
        parser, "texts", NULL, 0u, NULL, 1u, &string_count); // [手順] - string 配列位置引数の storage NULL を検証する。
    int positional_string_array_capacity_zero = _com_util_argparser_register_positional_string_array(
        parser, "texts2", NULL, 0u, string_array, 0u,
        &string_count); // [手順] - string 配列位置引数の capacity 0 を検証する。
    int positional_string_array_count_null = _com_util_argparser_register_positional_string_array(
        parser, "texts3", NULL, 0u, string_array, 1u, NULL); // [手順] - string 配列位置引数の count NULL を検証する。

    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_int_array(parser, "-x", "--x", NULL, NULL, 0u, int_array,
                                                                         1u, &int_count));
    int option_int_array_duplicate = _com_util_argparser_register_option_int_array(
        parser, "-x", "--x2", NULL, NULL, 0u, int_array, 1u, &int_count); // [手順] - int 配列の重複登録を検証する。
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_string_array(parser, "-y", "--y", NULL, NULL, 0u,
                                                                            string_array, 1u, &string_count));
    int option_string_array_duplicate =
        _com_util_argparser_register_option_string_array(parser, "-y", "--y2", NULL, NULL, 0u, string_array, 1u,
                                                         &string_count); // [手順] - string 配列の重複登録を検証する。
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string_array(parser, "tail", NULL, 0u, string_array,
                                                                                1u, &string_count));
    int positional_int_array_duplicate = _com_util_argparser_register_positional_int_array(
        parser, "tail2", NULL, 0u, int_array, 1u, &int_count); // [手順] - 可変長位置引数後の int 配列登録を検証する。
    int positional_string_array_duplicate = _com_util_argparser_register_positional_string_array(
        parser, "tail3", NULL, 0u, string_array, 1u,
        &string_count); // [手順] - 可変長位置引数後の string 配列登録を検証する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              option_int_null); // [確認_異常系] - int オプションの storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              option_string_null); // [確認_異常系] - string オプションの storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        option_int_array_storage_null); // [確認_異常系] - int 配列の storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              option_int_array_capacity_zero); // [確認_異常系] - int 配列の capacity 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              option_int_array_count_null); // [確認_異常系] - int 配列の count NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        option_string_array_storage_null); // [確認_異常系] - string 配列の storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        option_string_array_capacity_zero); // [確認_異常系] - string 配列の capacity 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        option_string_array_count_null); // [確認_異常系] - string 配列の count NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              positional_int_null); // [確認_異常系] - int 位置引数の storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_string_null); // [確認_異常系] - string 位置引数の storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_int_array_storage_null); // [確認_異常系] - int 配列位置引数の storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_int_array_capacity_zero); // [確認_異常系] - int 配列位置引数の capacity 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_int_array_count_null); // [確認_異常系] - int 配列位置引数の count NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_string_array_storage_null); // [確認_異常系] - string 配列位置引数の storage NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_string_array_capacity_zero); // [確認_異常系] - string 配列位置引数の capacity 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_string_array_count_null); // [確認_異常系] - string 配列位置引数の count NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              option_int_array_duplicate); // [確認_異常系] - int 配列の重複登録が DUPLICATE_DEFINITION であること。
    EXPECT_EQ(
        COM_UTIL_ERR_DUPLICATE_DEFINITION,
        option_string_array_duplicate); // [確認_異常系] - string 配列の重複登録が DUPLICATE_DEFINITION であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_int_array_duplicate); // [確認_異常系] - 可変長位置引数後の int 配列登録が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_string_array_duplicate); // [確認_異常系] - 可変長位置引数後の string 配列登録が INVALID_ARGUMENT であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// parse の NULL token、値不足、位置引数判定が検出されることの確認
TEST_F(argparserTest, parse_rejects_null_tokens_and_handles_negative_positionals)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    int values[2] = {};
    size_t value_count = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_int(parser, "-c", "--count", NULL, NULL, 0u, &count));
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_positional_int_array(parser, "values", NULL, 0u, values, 2u, &value_count));

    // Pre-Assert

    // Act
    char *null_token_argv[] = {cstr("prog"), NULL};
    int null_token_result =
        _com_util_argparser_parse(parser, 2, null_token_argv); // [手順] - argv 中の NULL token を解析する。
    char *long_null_value_argv[] = {cstr("prog"), cstr("--count"), NULL};
    int long_null_value_result =
        _com_util_argparser_parse(parser, 3, long_null_value_argv); // [手順] - 長形式オプションの NULL 値を解析する。
    char *short_null_value_argv[] = {cstr("prog"), cstr("-c"), NULL};
    int short_null_value_result =
        _com_util_argparser_parse(parser, 3, short_null_value_argv); // [手順] - 短形式オプションの NULL 値を解析する。
    char *short_missing_value_argv[] = {cstr("prog"), cstr("-c")};
    int short_missing_value_result =
        _com_util_argparser_parse(parser, 2, short_missing_value_argv); // [手順] - 末尾の短形式オプションを解析する。
    char *negative_array_argv[] = {cstr("prog"), cstr("-1")};
    int negative_array_result =
        _com_util_argparser_parse(parser, 2, negative_array_argv); // [手順] - 負数を可変長 int 位置引数として解析する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_token_result); // [確認_異常系] - NULL token の解析結果が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              long_null_value_result); // [確認_異常系] - 長形式の NULL 値の解析結果が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              short_null_value_result); // [確認_異常系] - 短形式の NULL 値の解析結果が INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_MISSING_VALUE,
        short_missing_value_result); // [確認_異常系] - 末尾の短形式オプションの解析結果が MISSING_VALUE であること。
    EXPECT_EQ(COM_UTIL_OK,
              negative_array_result);  // [確認_正常系] - 負数の可変長 int 位置引数の解析結果が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)1, value_count); // [確認_正常系] - 負数の可変長 int 位置引数が 1 件格納されること。
    EXPECT_EQ(-1, values[0]);          // [確認_正常系] - 負数 -1 が可変長 int 位置引数へ格納されること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 位置引数を含む未知長形式オプションと int 配列の変換失敗が検出されることの確認
TEST_F(argparserTest, parse_handles_positional_search_and_array_conversion_failure)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *text = NULL;
    int values[1] = {};
    size_t value_count = 0;
    int numbers[1] = {};
    size_t number_count = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string(parser, "text", NULL, 0u, &text));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_int_array(parser, "-n", "--number", NULL, NULL, 0u,
                                                                         numbers, 1u, &number_count));
    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_positional_int_array(parser, "values", NULL, 0u, values, 1u, &value_count));

    // Pre-Assert

    // Act
    char *unknown_long_argv[] = {cstr("prog"), cstr("--unknown")};
    int unknown_long_result = _com_util_argparser_parse(
        parser, 2, unknown_long_argv); // [手順] - 位置引数登録済み parser で未知長形式オプションを解析する。
    char *negative_string_argv[] = {cstr("prog"), cstr("-1")};
    int negative_string_result = _com_util_argparser_parse(
        parser, 2, negative_string_argv); // [手順] - 文字列位置引数へ負数形式のトークンを解析する。
    char *invalid_array_argv[] = {cstr("prog"), cstr("--number=bad")};
    int invalid_array_result =
        _com_util_argparser_parse(parser, 2, invalid_array_argv); // [手順] - int 配列へ変換できない値を解析する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN_OPTION,
        unknown_long_result); // [確認_異常系] - 位置引数を含む未知長形式オプションの解析結果が UNKNOWN_OPTION であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN_OPTION,
        negative_string_result); // [確認_異常系] - 文字列位置引数へ負数形式のトークンを渡した解析結果が UNKNOWN_OPTION であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_INTEGER,
              invalid_array_result); // [確認_異常系] - int 配列の不正値の解析結果が INVALID_INTEGER であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// argv[0] が NULL の場合と解決済みプログラム名が usage に反映されない場合の確認
TEST_F(argparserTest, parse_accepts_null_program_name)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    char *argv[] = {NULL};

    // Pre-Assert

    // Act
    int result = _com_util_argparser_parse(parser, 1, argv); // [手順] - argv[0] が NULL の argv を解析する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - argv[0] が NULL の解析結果が COM_UTIL_OK であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 短い名前のみの表示名、既定値名、長い usage ラベルが処理されることの確認
TEST_F(argparserTest, usage_handles_short_name_default_value_and_long_label)
{
    // Arrange
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int flag = 0;
    const char *value = NULL;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-v", NULL, NULL, &flag));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_option_string(parser, "-n", NULL, NULL, NULL, 0u, &value));
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_positional_string(parser, "123456789012345678901234567890",
                                                                          NULL, 0u, &value));
    char usage[512] = {};
    char *unexpected_argv[] = {cstr("prog"), cstr("-v=1")};

    // Pre-Assert

    // Act
    int unexpected_result =
        _com_util_argparser_parse(parser, 2, unexpected_argv); // [手順] - 短い名前のみのフラグへ値を指定する。
    int usage_result = _com_util_argparser_get_usage(parser, usage, sizeof(usage),
                                                     NULL); // [手順] - 既定値名と長いラベルを含む usage を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNEXPECTED_VALUE,
              unexpected_result); // [確認_異常系] - 短い名前のみのフラグの値指定が UNEXPECTED_VALUE であること。
    EXPECT_EQ(COM_UTIL_OK,
              usage_result); // [確認_正常系] - 既定値名と長いラベルを含む usage の取得結果が COM_UTIL_OK であること。
    EXPECT_THAT(std::string(usage),
                HasSubstr("-n VALUE")); // [確認_正常系] - value_name 未指定のオプションに VALUE が表示されること。
    EXPECT_THAT(
        std::string(usage),
        HasSubstr(
            "  123456789012345678901234567890\n                            \n")); // [確認_正常系] - 長いラベルの説明列が次行へ移ること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 解析エラーの全メッセージ分岐が組み立てられることの確認
TEST_F(argparserTest, error_message_formats_all_error_results)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const int results[] = {
        COM_UTIL_ERR_UNKNOWN_OPTION,     COM_UTIL_ERR_MISSING_VALUE,
        COM_UTIL_ERR_INVALID_INTEGER,    COM_UTIL_ERR_OUT_OF_RANGE,
        COM_UTIL_ERR_MISSING_REQUIRED,   COM_UTIL_ERR_DUPLICATE_OPTION,
        COM_UTIL_ERR_TOO_MANY_ARGUMENTS, COM_UTIL_ERR_TOO_MANY_OCCURRENCES,
        COM_UTIL_ERR_UNEXPECTED_VALUE,   COM_UTIL_OK,
        COM_UTIL_ERR_INVALID_ARGUMENT,
    };
    char message[128] = {};

    // Pre-Assert

    // Act
    std::vector<int> return_codes;
    for (int result : results)
    {
        test_argparser_set_last_error(parser, result);
        return_codes.push_back(_com_util_argparser_get_error_message(
            parser, message, sizeof(message))); // [手順] - 各解析結果のエラーメッセージを取得する。
    }

    // Assert
    ASSERT_EQ(sizeof(results) / sizeof(results[0]),
              return_codes.size()); // [確認_正常系] - 全解析結果のメッセージ取得件数が一致すること。
    for (int return_code : return_codes)
    {
        EXPECT_EQ(COM_UTIL_OK,
                  return_code); // [確認_正常系] - 各解析結果のメッセージ取得結果が COM_UTIL_OK であること。
    }
    EXPECT_STREQ("no error", message); // [確認_正常系] - 既定分岐のメッセージが no error であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// snprintf が失敗した場合に解析エラーメッセージが失敗することの確認
TEST_F(argparserTest, error_message_returns_invalid_argument_when_snprintf_fails)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    char message[128] = {'x'};

    // Pre-Assert
    EXPECT_CALL(mock_stdio, snprintf(_, _, _, _, _, _))
        .WillRepeatedly(Return(-1)); // [Pre-Assert確認_異常系] - エラーメッセージの snprintf が失敗すること。

    // Act
    int result = _com_util_argparser_get_error_message(
        parser, message, sizeof(message)); // [手順] - snprintf 失敗状態で解析エラーメッセージを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result);           // [確認_異常系] - snprintf 失敗時の取得結果が INVALID_ARGUMENT であること。
    EXPECT_EQ('\0', message[0]); // [確認_異常系] - snprintf 失敗時にメッセージ先頭が NUL になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 登録エラーメッセージの全分岐と対象名既定値が組み立てられることの確認
TEST_F(argparserTest, register_error_message_formats_all_results)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", "--alpha", NULL, &storage));
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_register_flag(parser, "-a", "--beta", NULL, &storage));
    ASSERT_EQ((size_t)1, _com_util_argparser_get_register_error_count(parser));
    char message[128] = {};
    const int results[] = {
        COM_UTIL_ERR_INVALID_ARGUMENT, COM_UTIL_ERR_OUT_OF_MEMORY, COM_UTIL_ERR_DUPLICATE_DEFINITION, COM_UTIL_OK,
        COM_UTIL_ERR_BUFFER_TOO_SMALL, COM_UTIL_ERR_UNKNOWN,
    };

    // Pre-Assert

    // Act
    std::vector<int> return_codes;
    for (int result : results)
    {
        test_argparser_set_register_error_result(parser, 0u, result);
        return_codes.push_back(_com_util_argparser_get_register_error_message(
            parser, 0u, message, sizeof(message))); // [手順] - 各登録結果のエラーメッセージを取得する。
    }
    test_argparser_clear_register_error_target(parser, 0u);
    int null_target_result = _com_util_argparser_get_register_error_message(
        parser, 0u, message, sizeof(message)); // [手順] - 対象名 NULL の登録エラーメッセージを取得する。

    // Assert
    EXPECT_EQ(sizeof(results) / sizeof(results[0]),
              return_codes.size()); // [確認_正常系] - 全登録結果のメッセージ取得件数が一致すること。
    for (int return_code : return_codes)
    {
        EXPECT_EQ(COM_UTIL_OK,
                  return_code); // [確認_正常系] - 各登録結果のメッセージ取得結果が COM_UTIL_OK であること。
    }
    EXPECT_EQ(COM_UTIL_OK,
              null_target_result); // [確認_正常系] - 対象名 NULL のメッセージ取得結果が COM_UTIL_OK であること。
    EXPECT_THAT(std::string(message), HasSubstr("'?'")); // [確認_正常系] - 対象名 NULL が ? として表示されること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 登録エラーメッセージの snprintf 失敗が処理されることの確認
TEST_F(argparserTest, register_error_message_returns_invalid_argument_when_snprintf_fails)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", "--alpha", NULL, &storage));
    EXPECT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_register_flag(parser, "-a", "--beta", NULL, &storage));
    char message[128] = {'x'};

    // Pre-Assert
    EXPECT_CALL(mock_stdio, snprintf(_, _, _, _, _, _))
        .WillRepeatedly(Return(-1)); // [Pre-Assert確認_異常系] - 登録エラーメッセージの snprintf が失敗すること。

    // Act
    int result = _com_util_argparser_get_register_error_message(
        parser, 0u, message, sizeof(message)); // [手順] - snprintf 失敗状態で登録エラーメッセージを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result);           // [確認_異常系] - snprintf 失敗時の取得結果が INVALID_ARGUMENT であること。
    EXPECT_EQ('\0', message[0]); // [確認_異常系] - snprintf 失敗時にメッセージ先頭が NUL になること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// 公開 API の登録、解析、取得、出力ラッパーが呼び出せることの確認
TEST_F(argparserTest, public_api_wrappers_are_callable)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    int flag = 0;
    int int_value = 0;
    const char *string_value = NULL;
    int int_values[2] = {};
    size_t int_value_count = 0;
    const char *string_values[2] = {};
    size_t string_value_count = 0;
    int positional_int = 0;
    const char *positional_string = NULL;
    int positional_int_values[2] = {};
    size_t positional_int_value_count = 0;
    const char *positional_string_values[2] = {};
    size_t positional_string_value_count = 0;
    char usage[1024] = {};
    char error_message[512] = {};
    char register_error_message[512] = {};
    size_t required_size = 0;

    // Pre-Assert

    // Act
    com_util_argparser_init("public API"); // [手順] - 公開 default parser を初期化する。
    int flag_result =
        com_util_argparser_register_flag("-f", "--flag", NULL, &flag); // [手順] - 公開 flag 登録 API を呼び出す。
    int int_result = com_util_argparser_register_option_int(
        "-i", "--integer", NULL, NULL, 0u, &int_value); // [手順] - 公開 int オプション登録 API を呼び出す。
    int string_result = com_util_argparser_register_option_string(
        "-s", "--string", NULL, NULL, 0u, &string_value); // [手順] - 公開 string オプション登録 API を呼び出す。
    int int_array_result =
        com_util_argparser_register_option_int_array("-a", "--array-int", NULL, NULL, 0u, int_values, 2u,
                                                     &int_value_count); // [手順] - 公開 int 配列登録 API を呼び出す。
    int string_array_result = com_util_argparser_register_option_string_array(
        "-b", "--array-string", NULL, NULL, 0u, string_values, 2u,
        &string_value_count); // [手順] - 公開 string 配列登録 API を呼び出す。
    int positional_int_result = com_util_argparser_register_positional_int(
        "posint", NULL, 0u, &positional_int); // [手順] - 公開 int 位置引数登録 API を呼び出す。
    int positional_string_result = com_util_argparser_register_positional_string(
        "posstr", NULL, 0u, &positional_string); // [手順] - 公開 string 位置引数登録 API を呼び出す。
    int positional_string_array_result = com_util_argparser_register_positional_string_array(
        "posstrs", NULL, 0u, positional_string_values, 2u,
        &positional_string_value_count); // [手順] - 公開 string 配列位置引数登録 API を呼び出す。
    int invalid_register_result = com_util_argparser_register_flag(
        NULL, NULL, NULL, &flag); // [手順] - 公開登録 API で登録エラーを 1 件発生させる。
    int positional_int_array_result = com_util_argparser_register_positional_int_array(
        "posints", NULL, 0u, positional_int_values, 2u,
        &positional_int_value_count); // [手順] - 公開 int 配列位置引数登録 API を呼び出す。
    char *parse_argv[] = {cstr("prog")};
    int parse_result = com_util_argparser_parse(1, parse_argv); // [手順] - 公開 parse API を引数なしで呼び出す。
    char *error_argv[] = {cstr("prog"), cstr("--unknown")};
    int error_parse_result =
        com_util_argparser_parse(2, error_argv);       // [手順] - 公開 parse API で解析エラーを発生させる。
    int error_result = com_util_argparser_get_error(); // [手順] - 公開 get_error API を呼び出す。
    const char *error_target = com_util_argparser_get_error_target(); // [手順] - 公開 get_error_target API を呼び出す。
    int error_index = com_util_argparser_get_error_index();           // [手順] - 公開 get_error_index API を呼び出す。
    int error_message_result = com_util_argparser_get_error_message(
        error_message, sizeof(error_message)); // [手順] - 公開 get_error_message API を呼び出す。
    int usage_result =
        com_util_argparser_get_usage(usage, sizeof(usage), &required_size); // [手順] - 公開 get_usage API を呼び出す。
    int print_usage_result = com_util_argparser_print_usage(stdout); // [手順] - 公開 print_usage API を呼び出す。
    int print_error_result =
        com_util_argparser_print_error_messages(stderr); // [手順] - 公開 print_error_messages API を呼び出す。
    size_t register_error_count =
        com_util_argparser_get_register_error_count(); // [手順] - 公開登録エラー件数 API を呼び出す。
    int register_error_result =
        com_util_argparser_get_register_error(0u); // [手順] - 公開登録エラー取得 API を呼び出す。
    const char *register_error_target =
        com_util_argparser_get_register_error_target(0u); // [手順] - 公開登録エラー対象 API を呼び出す。
    int register_error_message_result = com_util_argparser_get_register_error_message(
        0u, register_error_message,
        sizeof(register_error_message)); // [手順] - 公開登録エラーメッセージ API を呼び出す。
    int print_register_error_result =
        com_util_argparser_print_register_error_messages(stderr); // [手順] - 公開登録エラー出力 API を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, flag_result); // [確認_正常系] - 公開 flag 登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              int_result); // [確認_正常系] - 公開 int オプション登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              string_result); // [確認_正常系] - 公開 string オプション登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              int_array_result); // [確認_正常系] - 公開 int 配列登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              string_array_result); // [確認_正常系] - 公開 string 配列登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              positional_int_result); // [確認_正常系] - 公開 int 位置引数登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        positional_string_result); // [確認_正常系] - 公開 string 位置引数登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        positional_string_array_result); // [確認_正常系] - 公開 string 配列位置引数登録 API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        positional_int_array_result); // [確認_異常系] - 可変長位置引数後の公開 int 配列登録 API が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK, parse_result); // [確認_正常系] - 公開 parse API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              invalid_register_result); // [確認_異常系] - 公開登録 API の不正引数結果が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
              error_parse_result); // [確認_異常系] - 公開 parse API の未知オプション結果が UNKNOWN_OPTION であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
              error_result); // [確認_異常系] - 公開 get_error API の戻り値が UNKNOWN_OPTION であること。
    EXPECT_STREQ("--unknown", error_target); // [確認_異常系] - 公開 get_error_target API が未知オプション名を返すこと。
    EXPECT_EQ(1, error_index);               // [確認_異常系] - 公開 get_error_index API が 1 を返すこと。
    EXPECT_EQ(COM_UTIL_OK,
              error_message_result); // [確認_正常系] - 公開 get_error_message API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, usage_result); // [確認_正常系] - 公開 get_usage API の戻り値が COM_UTIL_OK であること。
    EXPECT_GT(required_size, (size_t)0);  // [確認_正常系] - 公開 get_usage API が正の required_size を返すこと。
    EXPECT_EQ(COM_UTIL_OK,
              print_usage_result); // [確認_正常系] - 公開 print_usage API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              print_error_result); // [確認_正常系] - 公開 print_error_messages API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)2, register_error_count); // [確認_正常系] - 公開登録エラー件数 API が 2 を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              register_error_result); // [確認_異常系] - 公開登録エラー取得 API が INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(nullptr, register_error_target); // [確認_異常系] - 公開登録エラー対象 API が NULL を返すこと。
    EXPECT_EQ(
        COM_UTIL_OK,
        register_error_message_result); // [確認_正常系] - 公開登録エラーメッセージ API の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        print_register_error_result); // [確認_正常系] - 公開登録エラー出力 API の戻り値が COM_UTIL_OK であること。
}

// usage 再計算時のバッファー不足が print_usage の戻り値へ伝播することの確認
TEST_F(argparserTest, print_usage_returns_buffer_too_small_when_usage_changes)
{
    // Arrange
    com_util_argparser_options options = {};
    options.program_description = "short";
    com_util_argparser *parser = _com_util_argparser_create(&options);
    ASSERT_NE(nullptr, parser);
    NiceMock<Mock_stdio> mock_stdio;
    NiceMock<Mock_stdlib> mock_stdlib;
    char *previous_description = NULL;
    static char long_description[] = "This description is intentionally much longer than the initial usage buffer.";

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - usage のバッファー不足時に fprintf が呼び出されないこと。
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(
            [&](const char *file, const int line, const char *function, size_t size)
            {
                previous_description = test_argparser_replace_program_description(parser, long_description);
                return delegate_real_malloc(file, line, function, size);
            }); // [Pre-Assert確認_異常系] - usage バッファー確保前に説明文が変更されること。

    // Act
    int result = _com_util_argparser_print_usage(
        parser, stdout); // [手順] - usage 組み立て中に説明文を変更して print_usage を呼び出す。
    test_argparser_replace_program_description(
        parser, previous_description); // [手順] - parser の説明文を元のポインターへ戻す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL, result); // [確認_異常系] - usage 再計算時のバッファー不足が返ること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_error_messages が長いエラーをバッファー不足として出力することの確認
TEST_F(argparserTest, print_error_messages_writes_buffer_too_small_message)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    std::string unknown_option = "--" + std::string(600u, 'x');
    char *argv[] = {cstr("prog"), const_cast<char *>(unknown_option.c_str())};
    ASSERT_EQ(
        COM_UTIL_ERR_UNKNOWN_OPTION,
        _com_util_argparser_parse(parser, 2, argv)); // [状態] - 600 文字の未知オプションで解析エラーを発生させる。

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _))
        .Times(AnyNumber()); // [Pre-Assert手順] - error message と区切り行の fprintf を許容する。

    // Act
    int result = _com_util_argparser_print_error_messages(
        parser, stderr); // [手順] - 長いエラーを print_error_messages で出力する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              result); // [確認_異常系] - 長いエラーの出力結果が BUFFER_TOO_SMALL であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_error_messages がメッセージ組み立て失敗時に改行だけ出力することの確認
TEST_F(argparserTest, print_error_messages_skips_message_when_snprintf_fails)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    char *argv[] = {cstr("prog"), cstr("--unknown")};
    ASSERT_EQ(COM_UTIL_ERR_UNKNOWN_OPTION,
              _com_util_argparser_parse(parser, 2, argv)); // [状態] - 未知オプションで解析エラーを発生させる。

    // Pre-Assert
    EXPECT_CALL(mock_stdio, snprintf(_, _, _, _, _, _))
        .WillRepeatedly(Return(-1)); // [Pre-Assert確認_異常系] - エラーメッセージの snprintf が失敗すること。
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _))
        .Times(1); // [Pre-Assert確認_正常系] - メッセージを省略した区切り改行だけが出力されること。

    // Act
    int result = _com_util_argparser_print_error_messages(
        parser, stderr); // [手順] - snprintf 失敗状態で print_error_messages を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result); // [確認_異常系] - snprintf 失敗時の出力結果が INVALID_ARGUMENT であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_register_error_messages が長い対象をバッファー不足として出力することの確認
TEST_F(argparserTest, print_register_error_messages_writes_buffer_too_small_message)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    std::string long_name = "--" + std::string(600u, 'x');
    int storage = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", long_name.c_str(), NULL, &storage));
    ASSERT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_register_flag(parser, "-b", long_name.c_str(), NULL,
                                                &storage)); // [状態] - 長い対象名の登録エラーを発生させる。

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _))
        .Times(AnyNumber()); // [Pre-Assert手順] - 登録エラーと区切り行の fprintf を許容する。

    // Act
    int result =
        _com_util_argparser_print_register_error_messages(parser, stderr); // [手順] - 長い対象の登録エラーを出力する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              result); // [確認_異常系] - 長い登録エラーの出力結果が BUFFER_TOO_SMALL であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}

// print_register_error_messages がメッセージ組み立て失敗時に改行だけ出力することの確認
TEST_F(argparserTest, print_register_error_messages_skips_message_when_snprintf_fails)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = _com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    ASSERT_EQ(COM_UTIL_OK, _com_util_argparser_register_flag(parser, "-a", "--alpha", NULL, &storage));
    ASSERT_EQ(COM_UTIL_ERR_DUPLICATE_DEFINITION,
              _com_util_argparser_register_flag(parser, "-b", "--alpha", NULL,
                                                &storage)); // [状態] - 登録エラーを 1 件発生させる。

    // Pre-Assert
    EXPECT_CALL(mock_stdio, snprintf(_, _, _, _, _, _))
        .WillRepeatedly(Return(-1)); // [Pre-Assert確認_異常系] - 登録エラーメッセージの snprintf が失敗すること。
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _))
        .Times(1); // [Pre-Assert確認_正常系] - メッセージを省略した区切り改行だけが出力されること。

    // Act
    int result = _com_util_argparser_print_register_error_messages(
        parser, stderr); // [手順] - snprintf 失敗状態で登録エラーを出力する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result); // [確認_異常系] - snprintf 失敗時の出力結果が INVALID_ARGUMENT であること。

    // Cleanup
    _com_util_argparser_dispose(parser);
}
