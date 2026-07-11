#include <testfw.h>
#include <com_util/argparser/argparser.h>
#include <mock_stdio.h>
#include <mock_stdlib.h>
#include <limits.h>
#include <string.h>
#include <string>
#include <thread>

class argparserTest : public Test
{
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
    // Arrange & Act
    com_util_argparser *parser = com_util_argparser_create(NULL);

    // Assert
    ASSERT_NE(nullptr, parser);
    com_util_argparser_dispose(parser);
    com_util_argparser_dispose(NULL);
}

// メモリ確保に失敗した場合に create が NULL を返すことの確認
TEST_F(argparserTest, create_returns_null_on_alloc_failure)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _)).WillOnce(Return(nullptr));

    // Act
    com_util_argparser *parser = com_util_argparser_create(NULL);

    // Assert
    EXPECT_EQ(nullptr, parser);
}

// default() を複数回呼び出しても同一ハンドルが返ることの確認 (dispose は呼び出さない)
TEST_F(argparserTest, default_returns_same_handle_across_calls)
{
    // Arrange & Act
    com_util_argparser *first = com_util_argparser_default(NULL);
    com_util_argparser *second = com_util_argparser_default(NULL);

    // Assert
    ASSERT_NE(nullptr, first);
    EXPECT_EQ(first, second);
}

// default() の生成オプションが初回呼び出し時のみ適用されることの確認 (dispose は呼び出さない)
TEST_F(argparserTest, default_options_applied_only_on_first_call)
{
    // Arrange
    com_util_argparser *first = com_util_argparser_default(NULL);
    ASSERT_NE(nullptr, first);

    char before[256];
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_usage(first, before, sizeof(before), NULL));

    com_util_argparser_options options = {};
    options.program_name = "should-be-ignored";

    // Act
    com_util_argparser *second = com_util_argparser_default(&options);

    char after[256];
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_usage(second, after, sizeof(after), NULL));

    // Assert
    EXPECT_EQ(first, second);
    EXPECT_STREQ(before, after);
    EXPECT_THAT(std::string(after), Not(HasSubstr("should-be-ignored")));
}

// default() の返却ハンドルを dispose() に渡しても解放されないことの確認
TEST_F(argparserTest, dispose_ignores_default_handle)
{
    // Arrange
    com_util_argparser *first = com_util_argparser_default(NULL);
    ASSERT_NE(nullptr, first);

    // Act
    com_util_argparser_dispose(first);
    com_util_argparser *second = com_util_argparser_default(NULL);

    // Assert
    EXPECT_EQ(first, second);
}

// default() の並行呼び出しが同一ハンドルを返すことの確認
TEST_F(argparserTest, default_returns_same_handle_to_concurrent_callers)
{
    // Arrange
    static constexpr size_t kThreadCount = 16;
    com_util_argparser *parsers[kThreadCount] = {};
    std::thread threads[kThreadCount];

    // Act
    for (size_t i = 0; i < kThreadCount; i++)
    {
        threads[i] = std::thread([&parsers, i]() { parsers[i] = com_util_argparser_default(NULL); });
    }
    for (size_t i = 0; i < kThreadCount; i++)
    {
        threads[i].join();
    }

    // Assert
    ASSERT_NE(nullptr, parsers[0]);
    for (size_t i = 1; i < kThreadCount; i++)
    {
        EXPECT_EQ(parsers[0], parsers[i]);
    }
}

// 登録 API が不正引数を検出することの確認
TEST_F(argparserTest, register_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    const char *string_storage = NULL;

    // Act & Assert
    // parser NULL
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_flag(NULL, "-v", "--verbose", NULL, &storage));
    // storage NULL
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, NULL));
    // 両方の名前が NULL
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_flag(parser, NULL, NULL, NULL, &storage));
    // 短い名前の形式不正
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_flag(parser, "-vv", NULL, NULL, &storage));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_register_flag(parser, "v", NULL, NULL, &storage));
    // 長い名前の形式不正
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_flag(parser, NULL, "-verbose", NULL, &storage));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_flag(parser, NULL, "--a=b", NULL, &storage));
    // 未定義の登録フラグ
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_option_int(parser, "-c", NULL, NULL, NULL, 0x100u, &storage));
    // 配列オプションの capacity 0 / count NULL
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_register_option_string_array(
                                                       parser, "-i", NULL, NULL, NULL, 0, &string_storage, 0, NULL));
    // 位置引数の名前 NULL
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_register_positional_int(parser, NULL, NULL, 0, &storage));

    com_util_argparser_dispose(parser);
}

// 同名オプションの二重登録が検出されることの確認
TEST_F(argparserTest, register_rejects_duplicate_definition)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int storage = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &storage));

    // Act & Assert
    // 短い名前の重複
    EXPECT_EQ(COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION,
              com_util_argparser_register_option_int(parser, "-v", NULL, NULL, NULL, 0, &storage));
    // 長い名前の重複
    EXPECT_EQ(COM_UTIL_ARGPARSER_DUPLICATE_DEFINITION,
              com_util_argparser_register_flag(parser, NULL, "--verbose", NULL, &storage));

    com_util_argparser_dispose(parser);
}

// 任意の位置引数の後に必須の位置引数を登録できないことの確認
TEST_F(argparserTest, register_rejects_required_positional_after_optional)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *first = NULL;
    const char *second = NULL;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_positional_string(parser, "first", NULL, 0, &first));

    // Act & Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_register_positional_string(
                                                       parser, "second", NULL, COM_UTIL_ARGPARSER_REQUIRED, &second));

    com_util_argparser_dispose(parser);
}

// 初期容量 (8) を超える登録で内部配列が拡張されることの確認
TEST_F(argparserTest, register_grows_beyond_initial_capacity)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const int spec_count = 20;
    int storages[spec_count] = {};

    // Act
    for (int i = 0; i < spec_count; i++)
    {
        char long_name[32];
        snprintf(long_name, sizeof(long_name), "--opt%02d", i);
        ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, NULL, long_name, NULL, &storages[i]));
    }

    // Assert
    ARGV(cstr("prog"), cstr("--opt00"), cstr("--opt19"));
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
    EXPECT_EQ(1, storages[0]);
    EXPECT_EQ(0, storages[1]);
    EXPECT_EQ(1, storages[19]);

    com_util_argparser_dispose(parser);
}

// フラグの出現回数が格納され、再解析で 0 に初期化されることの確認
TEST_F(argparserTest, flag_counts_occurrences_and_resets_on_reparse)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-v"), cstr("--verbose"), cstr("-v"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(3, verbose);
    }
    {
        ARGV(cstr("prog"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(0, verbose);
    }

    com_util_argparser_dispose(parser);
}

// フラグへの値指定 (--flag=value) がエラーになることの確認
TEST_F(argparserTest, flag_with_value_is_unexpected_value)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));

    // Act
    ARGV(cstr("prog"), cstr("--verbose=1"));
    com_util_argparser_result_t result = com_util_argparser_parse(parser, argc, argv);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, result);
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_UNEXPECTED_VALUE, com_util_argparser_get_error(parser));
    EXPECT_STREQ("--verbose", com_util_argparser_get_error_target(parser));
    EXPECT_EQ(1, com_util_argparser_get_error_index(parser));

    com_util_argparser_dispose(parser);
}

// int オプションの各構文 (-c 5 / --count 5 / --count=5 / 負数) の確認
TEST_F(argparserTest, option_int_accepts_all_syntaxes)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = -100;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0, &count));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-c"), cstr("5"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(5, count);
    }
    {
        ARGV(cstr("prog"), cstr("--count"), cstr("6"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(6, count);
    }
    {
        ARGV(cstr("prog"), cstr("--count=7"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(7, count);
    }
    {
        // 値位置のトークンは照合しないため負数を渡せる
        ARGV(cstr("prog"), cstr("-c"), cstr("-5"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(-5, count);
    }
    {
        // 非出現時は格納先を変更しない
        count = 42;
        ARGV(cstr("prog"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(42, count);
    }

    com_util_argparser_dispose(parser);
}

// int オプションの境界値と変換エラーの確認
TEST_F(argparserTest, option_int_boundary_and_conversion_errors)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0, &count));

    // Act & Assert
    {
        char int_max[32];
        snprintf(int_max, sizeof(int_max), "%d", INT_MAX);
        ARGV(cstr("prog"), cstr("-c"), int_max);
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(INT_MAX, count);
    }
    {
        char int_min[32];
        snprintf(int_min, sizeof(int_min), "%d", INT_MIN);
        ARGV(cstr("prog"), cstr("-c"), int_min);
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(INT_MIN, count);
    }
    {
        // INT_MAX + 1 は範囲外
        ARGV(cstr("prog"), cstr("--count=2147483648"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_OUT_OF_RANGE, com_util_argparser_get_error(parser));
    }
    {
        ARGV(cstr("prog"), cstr("--count=12a"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_INVALID_INT, com_util_argparser_get_error(parser));
        EXPECT_STREQ("--count", com_util_argparser_get_error_target(parser));
    }
    {
        // "--count=" の空値は int では変換エラー
        ARGV(cstr("prog"), cstr("--count="));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_INVALID_INT, com_util_argparser_get_error(parser));
    }

    com_util_argparser_dispose(parser);
}

// 文字列オプションが argv 内の文字列をそのまま指すことの確認
TEST_F(argparserTest, option_string_points_into_argv)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *name = NULL;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_string(parser, "-n", "--name", "NAME", NULL, 0, &name));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-n"), cstr("abc"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(argv[2], name);
        EXPECT_STREQ("abc", name);
    }
    {
        // "--name=xyz" は argv トークン内の値部分を指す
        ARGV(cstr("prog"), cstr("--name=xyz"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_STREQ("xyz", name);
    }
    {
        // "--name=" の空値は文字列では受理する
        ARGV(cstr("prog"), cstr("--name="));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_STREQ("", name);
    }

    com_util_argparser_dispose(parser);
}

// 位置引数の登録順割り当てと超過エラーの確認
TEST_F(argparserTest, positional_assignment_and_overflow)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *input = NULL;
    int level = -1;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_positional_string(
                                         parser, "input", NULL, COM_UTIL_ARGPARSER_REQUIRED, &input));
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_positional_int(parser, "level", NULL, 0, &level));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("in.txt"), cstr("3"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_STREQ("in.txt", input);
        EXPECT_EQ(3, level);
    }
    {
        // int 位置引数の変換エラー (対象は位置引数名)
        ARGV(cstr("prog"), cstr("in.txt"), cstr("abc"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_INVALID_INT, com_util_argparser_get_error(parser));
        EXPECT_STREQ("level", com_util_argparser_get_error_target(parser));
    }
    {
        // 登録数を超える位置引数
        ARGV(cstr("prog"), cstr("a"), cstr("1"), cstr("extra"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_TOO_MANY_POSITIONALS, com_util_argparser_get_error(parser));
        EXPECT_STREQ("extra", com_util_argparser_get_error_target(parser));
        EXPECT_EQ(3, com_util_argparser_get_error_index(parser));
    }
    {
        // 必須位置引数の欠落
        ARGV(cstr("prog"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED, com_util_argparser_get_error(parser));
        EXPECT_STREQ("input", com_util_argparser_get_error_target(parser));
        EXPECT_EQ(-1, com_util_argparser_get_error_index(parser));
    }

    com_util_argparser_dispose(parser);
}

// 未知のオプションが検出されることの確認 (短オプション連結を含む)
TEST_F(argparserTest, unknown_option_detection)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-x"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_UNKNOWN_OPTION, com_util_argparser_get_error(parser));
        EXPECT_STREQ("-x", com_util_argparser_get_error_target(parser));
    }
    {
        // 短オプションの連結は未サポート
        ARGV(cstr("prog"), cstr("-vv"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_UNKNOWN_OPTION, com_util_argparser_get_error(parser));
    }
    {
        ARGV(cstr("prog"), cstr("--bogus"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_STREQ("--bogus", com_util_argparser_get_error_target(parser));
    }

    com_util_argparser_dispose(parser);
}

// 末尾の値なしオプションが検出されることの確認
TEST_F(argparserTest, missing_value_at_end)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0, &count));

    // Act
    ARGV(cstr("prog"), cstr("--count"));
    com_util_argparser_result_t result = com_util_argparser_parse(parser, argc, argv);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, result);
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_MISSING_VALUE, com_util_argparser_get_error(parser));
    EXPECT_STREQ("--count", com_util_argparser_get_error_target(parser));

    com_util_argparser_dispose(parser);
}

// 単数オプションの重複出現 (short / long 混在) がエラーになることの確認
TEST_F(argparserTest, duplicate_option_occurrence)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL, 0, &count));

    // Act
    ARGV(cstr("prog"), cstr("-c"), cstr("1"), cstr("--count=2"));
    com_util_argparser_result_t result = com_util_argparser_parse(parser, argc, argv);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, result);
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION, com_util_argparser_get_error(parser));
    EXPECT_STREQ("--count", com_util_argparser_get_error_target(parser));

    com_util_argparser_dispose(parser);
}

// 配列オプションの複数出現・出現順・容量超過の確認
TEST_F(argparserTest, array_option_multiple_occurrences)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    const char *includes[2] = {};
    size_t include_count = 99;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_option_string_array(
                                         parser, "-i", "--include", "DIR", NULL, 0, includes, 2, &include_count));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-i"), cstr("dir1"), cstr("--include=dir2"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ((size_t)2, include_count);
        EXPECT_STREQ("dir1", includes[0]);
        EXPECT_STREQ("dir2", includes[1]);
    }
    {
        // 容量 (2) を超える出現
        ARGV(cstr("prog"), cstr("-i"), cstr("a"), cstr("-i"), cstr("b"), cstr("-i"), cstr("c"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES, com_util_argparser_get_error(parser));
        EXPECT_STREQ("--include", com_util_argparser_get_error_target(parser));
    }
    {
        // 非出現時は count が 0 に初期化される
        ARGV(cstr("prog"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ((size_t)0, include_count);
    }

    com_util_argparser_dispose(parser);
}

// int 配列オプションの値変換と REQUIRED の確認
TEST_F(argparserTest, array_option_int_and_required)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int ports[4] = {};
    size_t port_count = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_int_array(parser, "-p", "--port", "PORT", NULL,
                                                           COM_UTIL_ARGPARSER_REQUIRED, ports, 4, &port_count));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-p"), cstr("80"), cstr("-p"), cstr("443"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ((size_t)2, port_count);
        EXPECT_EQ(80, ports[0]);
        EXPECT_EQ(443, ports[1]);
    }
    {
        // REQUIRED の配列オプションは 1 回以上の出現が必要
        ARGV(cstr("prog"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED, com_util_argparser_get_error(parser));
        EXPECT_STREQ("--port", com_util_argparser_get_error_target(parser));
    }

    com_util_argparser_dispose(parser);
}

// 必須オプションの欠落が検出されることの確認
TEST_F(argparserTest, missing_required_option)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int count = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_option_int(parser, "-c", "--count", "N", NULL,
                                                                            COM_UTIL_ARGPARSER_REQUIRED, &count));

    // Act
    ARGV(cstr("prog"));
    com_util_argparser_result_t result = com_util_argparser_parse(parser, argc, argv);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, result);
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED, com_util_argparser_get_error(parser));
    EXPECT_STREQ("--count", com_util_argparser_get_error_target(parser));

    com_util_argparser_dispose(parser);
}

// 解析エラー後に成功した parse でエラー状態がクリアされることの確認
TEST_F(argparserTest, reparse_clears_error_state)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-x"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_UNKNOWN_OPTION, com_util_argparser_get_error(parser));
    }
    {
        ARGV(cstr("prog"), cstr("-v"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
        EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_NONE, com_util_argparser_get_error(parser));
        EXPECT_EQ(nullptr, com_util_argparser_get_error_target(parser));
        EXPECT_EQ(-1, com_util_argparser_get_error_index(parser));
    }

    com_util_argparser_dispose(parser);
}

// 複数ハンドルが独立して動作することの確認
TEST_F(argparserTest, multiple_handles_are_independent)
{
    // Arrange
    com_util_argparser *parser1 = com_util_argparser_create(NULL);
    com_util_argparser *parser2 = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser1);
    ASSERT_NE(nullptr, parser2);
    int flag1 = 0;
    int flag2 = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser1, "-a", NULL, NULL, &flag1));
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser2, "-b", NULL, NULL, &flag2));

    // Act & Assert
    {
        ARGV(cstr("prog"), cstr("-a"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser1, argc, argv));
        // parser2 には -a は未登録
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser2, argc, argv));
    }
    EXPECT_EQ(1, flag1);
    EXPECT_EQ(0, flag2);
    // エラー状態も独立している
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_NONE, com_util_argparser_get_error(parser1));
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_UNKNOWN_OPTION, com_util_argparser_get_error(parser2));

    com_util_argparser_dispose(parser1);
    com_util_argparser_dispose(parser2);
}

// エラーメッセージの組み立てとバッファー不足の確認
TEST_F(argparserTest, error_message_formatting)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));
    char message[128];

    // Act & Assert
    // 未解析時は "no error"
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_error_message(parser, message, sizeof(message)));
    EXPECT_STREQ("no error", message);

    {
        ARGV(cstr("prog"), cstr("--bogus"));
        EXPECT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));
    }
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_error_message(parser, message, sizeof(message)));
    EXPECT_STREQ("unknown option '--bogus'", message);

    // バッファー不足時は切り詰めて BUFFER_TOO_SMALL
    char small_buffer[8];
    EXPECT_EQ(COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL,
              com_util_argparser_get_error_message(parser, small_buffer, sizeof(small_buffer)));
    EXPECT_STREQ("unknown", small_buffer);

    // 不正引数
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT,
              com_util_argparser_get_error_message(NULL, message, sizeof(message)));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_get_error_message(parser, NULL, 1));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_get_error_message(parser, message, 0));

    com_util_argparser_dispose(parser);
}

// usage の組み立て内容の確認
TEST_F(argparserTest, usage_formatting)
{
    // Arrange
    com_util_argparser_options options = {};
    options.program_name = "sample";
    options.program_description = "Sample tool";
    com_util_argparser *parser = com_util_argparser_create(&options);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    int count = 0;
    const char *name = NULL;
    const char *input = NULL;
    const char *output = NULL;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_flag(parser, "-v", "--verbose", "verbose output", &verbose));
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_option_int(parser, "-c", "--count", "N", "count value",
                                                                            COM_UTIL_ARGPARSER_REQUIRED, &count));
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_option_string(parser, NULL, "--name", "NAME", "display name", 0, &name));
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_positional_string(
                                         parser, "input", "input file", COM_UTIL_ARGPARSER_REQUIRED, &input));
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_positional_string(parser, "output", "output file", 0, &output));

    // Act
    char usage[1024];
    size_t required_size = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_usage(parser, usage, sizeof(usage), &required_size));

    // Assert
    std::string usage_text(usage);
    EXPECT_EQ(strlen(usage) + 1, required_size);
    EXPECT_THAT(usage_text, HasSubstr("Sample tool\n\n"));
    EXPECT_THAT(usage_text, HasSubstr("Usage: sample [OPTIONS] <input> [output]\n"));
    EXPECT_THAT(usage_text, HasSubstr("\nPositional arguments:\n"));
    EXPECT_THAT(usage_text, HasSubstr("  input                     input file (required)\n"));
    EXPECT_THAT(usage_text, HasSubstr("  output                    output file\n"));
    EXPECT_THAT(usage_text, HasSubstr("\nOptions:\n"));
    EXPECT_THAT(usage_text, HasSubstr("  -v, --verbose             verbose output\n"));
    EXPECT_THAT(usage_text, HasSubstr("  -c, --count N             count value (required)\n"));
    EXPECT_THAT(usage_text, HasSubstr("      --name NAME           display name\n"));

    // サイズ問い合わせのみ (buffer NULL)
    size_t query_size = 0;
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_usage(parser, NULL, 0, &query_size));
    EXPECT_EQ(required_size, query_size);

    // バッファー不足時は切り詰めて BUFFER_TOO_SMALL
    char small_buffer[16];
    size_t small_required = 0;
    EXPECT_EQ(COM_UTIL_ARGPARSER_BUFFER_TOO_SMALL,
              com_util_argparser_get_usage(parser, small_buffer, sizeof(small_buffer), &small_required));
    EXPECT_EQ(required_size, small_required);
    EXPECT_EQ((size_t)15, strlen(small_buffer));

    // 不正引数
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_get_usage(NULL, usage, sizeof(usage), NULL));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_get_usage(parser, NULL, 0, NULL));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_get_usage(parser, usage, 0, NULL));

    com_util_argparser_dispose(parser);
}

// print_usage が不正引数を検出することの確認
TEST_F(argparserTest, print_usage_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);

    // Act & Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_print_usage(NULL, stdout));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_print_usage(parser, NULL));

    com_util_argparser_dispose(parser);
}

// print_usage が組み立てた usage を指定ストリームへ書き出すことの確認
TEST_F(argparserTest, print_usage_writes_to_stream)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser_options options = {};
    options.program_name = "sample";
    com_util_argparser *parser = com_util_argparser_create(&options);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK,
              com_util_argparser_register_flag(parser, "-v", "--verbose", "verbose output", &verbose));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stdout, HasSubstr("Usage: sample [OPTIONS]"))).Times(1);

    // Act
    com_util_argparser_result_t result = com_util_argparser_print_usage(parser, stdout);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, result);

    com_util_argparser_dispose(parser);
}

// print_error_messages が不正引数を検出することの確認
TEST_F(argparserTest, print_error_messages_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);

    // Act & Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_print_error_messages(NULL, stderr));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_print_error_messages(parser, NULL));

    com_util_argparser_dispose(parser);
}

// print_error_messages がエラーなしの場合に何も出力しないことの確認
TEST_F(argparserTest, print_error_messages_is_noop_without_error)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, _, _)).Times(0);

    // Act
    com_util_argparser_result_t result = com_util_argparser_print_error_messages(parser, stderr);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, result);

    com_util_argparser_dispose(parser);
}

// print_error_messages が直前の解析エラーのメッセージを指定ストリームへ書き出すことの確認
TEST_F(argparserTest, print_error_messages_writes_to_stream)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", "--verbose", NULL, &verbose));

    ARGV(cstr("prog"), cstr("--bogus"));
    ASSERT_EQ(COM_UTIL_ARGPARSER_PARSE_ERROR, com_util_argparser_parse(parser, argc, argv));

    // Pre-Assert
    // 区切りの空行の fprintf 呼び出しは検証対象外とする。
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, _)).Times(AnyNumber());
    EXPECT_CALL(mock_stdio, fprintf(_, _, _, stderr, HasSubstr("error: unknown option '--bogus'"))).Times(1);

    // Act
    com_util_argparser_result_t result = com_util_argparser_print_error_messages(parser, stderr);

    // Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_OK, result);

    com_util_argparser_dispose(parser);
}

// program_name 未指定時に argv[0] のベース名が usage に反映されることの確認
TEST_F(argparserTest, usage_program_name_resolution)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    int verbose = 0;
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_register_flag(parser, "-v", NULL, NULL, &verbose));
    char usage[256];

    // Act & Assert
    // 解析前はプレースホルダーを使用する
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_usage(parser, usage, sizeof(usage), NULL));
    EXPECT_THAT(std::string(usage), HasSubstr("Usage: {program} [OPTIONS]\n"));

    // 解析後は argv[0] のベース名を使用する
    {
        ARGV(cstr("/usr/local/bin/mytool"), cstr("-v"));
        ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_parse(parser, argc, argv));
    }
    ASSERT_EQ(COM_UTIL_ARGPARSER_OK, com_util_argparser_get_usage(parser, usage, sizeof(usage), NULL));
    EXPECT_THAT(std::string(usage), HasSubstr("Usage: mytool [OPTIONS]\n"));

    com_util_argparser_dispose(parser);
}

// parse の不正引数が検出されることの確認
TEST_F(argparserTest, parse_rejects_invalid_arguments)
{
    // Arrange
    com_util_argparser *parser = com_util_argparser_create(NULL);
    ASSERT_NE(nullptr, parser);
    ARGV(cstr("prog"));

    // Act & Assert
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_parse(NULL, argc, argv));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_parse(parser, 0, argv));
    EXPECT_EQ(COM_UTIL_ARGPARSER_INVALID_ARGUMENT, com_util_argparser_parse(parser, argc, NULL));

    // getter の NULL 安全
    EXPECT_EQ(COM_UTIL_ARGPARSER_ERROR_NONE, com_util_argparser_get_error(NULL));
    EXPECT_EQ(nullptr, com_util_argparser_get_error_target(NULL));
    EXPECT_EQ(-1, com_util_argparser_get_error_index(NULL));

    com_util_argparser_dispose(parser);
}
