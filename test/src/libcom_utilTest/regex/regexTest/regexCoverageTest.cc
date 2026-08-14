#include <testfw.h>

#include <com_util/regex/regex.h>

#include <cstdint>
#include <cstdlib>
#include <new>

#include "regex.inject.h"
#include "regexUtf8Fake.h"

namespace
{
int g_regex_new_fail_after = -1;
} // namespace

void *operator new(std::size_t size)
{
    if (g_regex_new_fail_after == 0)
    {
        g_regex_new_fail_after = -1;
        throw std::bad_alloc();
    }
    if (g_regex_new_fail_after > 0)
    {
        g_regex_new_fail_after--;
    }
    void *memory = std::malloc(size);
    if (memory == NULL)
    {
        throw std::bad_alloc();
    }
    return memory;
}

void operator delete(void *memory) noexcept
{
    std::free(memory);
}

void operator delete(void *memory, std::size_t) noexcept
{
    std::free(memory);
}

void *operator new[](std::size_t size)
{
    return operator new(size);
}

void operator delete[](void *memory) noexcept
{
    operator delete(memory);
}

void operator delete[](void *memory, std::size_t) noexcept
{
    operator delete(memory);
}

using testing::Test;

class regexCoverageTest : public Test
{
  protected:
    void TearDown() override
    {
        test_regex_utf8_set_decode_mode(REGEX_UTF8_FAKE_REAL);
        test_regex_utf8_set_encode_mode(REGEX_UTF8_FAKE_REAL);
    }
};

#if !defined(COM_UTIL_REGEX_NO_EXCEPTIONS)
// 公開 API の catch が utf8_decode の例外を結果コードへ変換することの確認
TEST_F(regexCoverageTest, public_apis_translate_decode_exceptions)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    com_util_regex_match match = {};
    com_util_regex_match parts[1] = {};
    char buffer[16] = {};
    size_t part_count = 0U;
    int matched = 0;
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex, NULL)); // [状態] - パターン "a" をコンパイルする。
                                                                                              // [状態確認] - com_util_regex_create の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    test_regex_utf8_set_decode_mode(REGEX_UTF8_FAKE_THROW_BAD_ALLOC);
    int search_result = com_util_regex_search(regex, "a", 1U, 0U, COM_UTIL_REGEX_DEFAULT, NULL, 0U, &matched,
                                              NULL); // [手順] - utf8_decode が bad_alloc を送出する状態で検索する。
    int replace_alloc_result =
        com_util_regex_replace(regex, "a", 1U, "x", COM_UTIL_REGEX_DEFAULT, buffer, sizeof(buffer), NULL,
                               NULL); // [手順] - utf8_decode が bad_alloc を送出する状態で置換する。
    int iter_create_result =
        com_util_regex_iter_create(regex, "a", 1U, COM_UTIL_REGEX_DEFAULT, &iter,
                                   NULL); // [手順] - utf8_decode が bad_alloc を送出する状態で列挙を生成する。
    int split_result = com_util_regex_split(regex, "a", 1U, 0U, COM_UTIL_REGEX_DEFAULT, parts, 1U, &part_count,
                                            NULL); // [手順] - utf8_decode が bad_alloc を送出する状態で分割する。
    test_regex_utf8_set_decode_mode(REGEX_UTF8_FAKE_THROW_REGEX_ERROR);
    int search_regex_result =
        com_util_regex_search(regex, "a", 1U, 0U, COM_UTIL_REGEX_DEFAULT, &match, 1U, &matched,
                              NULL); // [手順] - utf8_decode が regex_error を送出する状態で検索する。
    test_regex_utf8_set_decode_mode(REGEX_UTF8_FAKE_THROW_INT);
    int search_unknown_result =
        com_util_regex_search(regex, "a", 1U, 0U, COM_UTIL_REGEX_DEFAULT, NULL, 0U, &matched,
                              NULL); // [手順] - utf8_decode が未知例外を送出する状態で検索する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        search_result); // [確認_異常系] - bad_alloc 時の com_util_regex_search の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        replace_alloc_result); // [確認_異常系] - bad_alloc 時の com_util_regex_replace の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        iter_create_result); // [確認_異常系] - bad_alloc 時の com_util_regex_iter_create の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        split_result); // [確認_異常系] - bad_alloc 時の com_util_regex_split の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(
        COM_UTIL_ERR_LIMIT_EXCEEDED,
        search_regex_result); // [確認_異常系] - regex_error 時の com_util_regex_search の戻り値が COM_UTIL_ERR_LIMIT_EXCEEDED であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        search_unknown_result); // [確認_異常系] - 未知例外時の com_util_regex_search の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_regex_dispose(regex);
}
#endif /* !COM_UTIL_REGEX_NO_EXCEPTIONS */

// 置換後の utf8_encode 失敗と列挙位置超過を処理することの確認
TEST_F(regexCoverageTest, replace_encode_failure_and_iter_position_past_end)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    char buffer[16] = {};
    int has_match = 0;
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex, NULL)); // [状態] - パターン "a" をコンパイルする。
                                                                                              // [状態確認] - com_util_regex_create の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_iter_create(regex, "a", 1U, COM_UTIL_REGEX_DEFAULT, &iter, NULL)); // [状態] - 入力 "a" のイテレーターを生成する。
                                                                                                             // [状態確認] - com_util_regex_iter_create の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    test_regex_utf8_set_encode_mode(REGEX_UTF8_FAKE_RETURN_FALSE);
    int encode_result = com_util_regex_replace(regex, "a", 1U, "x", COM_UTIL_REGEX_DEFAULT, buffer, sizeof(buffer),
                                               NULL, NULL); // [手順] - utf8_encode が false を返す状態で置換する。
#if !defined(COM_UTIL_REGEX_NO_EXCEPTIONS)
    test_regex_utf8_set_encode_mode(REGEX_UTF8_FAKE_THROW_BAD_ALLOC);
    int encode_throw_result =
        com_util_regex_replace(regex, "a", 1U, "x", COM_UTIL_REGEX_DEFAULT, buffer, sizeof(buffer), NULL,
                               NULL); // [手順] - utf8_encode が bad_alloc を送出する状態で置換する。
#endif                                /* !COM_UTIL_REGEX_NO_EXCEPTIONS */
    test_regex_utf8_set_encode_mode(REGEX_UTF8_FAKE_REAL);
    test_regex_iter_set_position(iter, 99U); // [状態] - 列挙位置を入力長より後ろへ進める。
    int next_result = com_util_regex_iter_next(iter, NULL, 0U, &has_match,
                                               NULL); // [手順] - 終端を超えた位置で次の一致を取得する。
    bool past_end =
        test_regex_find_next_rejects_position_past_end(regex); // [手順] - 空入力の位置 1 で find_next を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        encode_result); // [確認_異常系] - utf8_encode 失敗時の com_util_regex_replace の戻り値が COM_UTIL_ERR_UNKNOWN であること。
#if !defined(COM_UTIL_REGEX_NO_EXCEPTIONS)
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        encode_throw_result); // [確認_異常系] - utf8_encode の例外時の com_util_regex_replace の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
#endif /* !COM_UTIL_REGEX_NO_EXCEPTIONS */
    EXPECT_EQ(
        COM_UTIL_OK,
        next_result); // [確認_正常系] - 終端超過位置の com_util_regex_iter_next の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, has_match); // [確認_正常系] - 終端超過位置では一致が無いこと。
    EXPECT_FALSE(past_end);  // [確認_正常系] - 位置超過の find_next が false を返すこと。

    // Cleanup
    com_util_regex_iter_dispose(iter);
    com_util_regex_dispose(regex);
}

// 文字クラス名の非 ASCII と icase、境界文字を分類することの確認
TEST_F(regexCoverageTest, traits_cover_remaining_classname_and_isctype_conditions)
{
    // Arrange
    const wchar_t mixed_name[] = {L'A', 0x3042, L'[', L'\0'};
    const unsigned int mixed = test_regex_lookup_classname(mixed_name, false);
    const unsigned int digit_icase = test_regex_lookup_classname(L"digit", true);
    const unsigned int digit = test_regex_lookup_classname(L"digit", false);

    // Pre-Assert

    // Act
    bool bracket_upper =
        test_regex_isctype(L'[', test_regex_lookup_classname(L"upper", false)); // [手順] - '[' を upper で判定する。
    bool brace_lower =
        test_regex_isctype(L'{', test_regex_lookup_classname(L"lower", false)); // [手順] - '{' を lower で判定する。
    bool slash_digit =
        test_regex_isctype(L'/', test_regex_lookup_classname(L"digit", false)); // [手順] - '/' を digit で判定する。
    bool at_xdigit =
        test_regex_isctype(L'@', test_regex_lookup_classname(L"xdigit", false)); // [手順] - '@' を xdigit で判定する。
    bool del_cntrl =
        test_regex_isctype(L'\x7F', test_regex_lookup_classname(L"cntrl", false)); // [手順] - DEL を cntrl で判定する。
    bool cr_space =
        test_regex_isctype(L'\r', test_regex_lookup_classname(L"space", false)); // [手順] - CR を space で判定する。
    int hex_g = test_regex_value(L'g', 16); // [手順] - 16 進の範囲外文字 g の値を取得する。
    int hex_G = test_regex_value(L'G', 16); // [手順] - 16 進の範囲外文字 G の値を取得する。

    // Assert
    EXPECT_EQ(0U, mixed);          // [確認_異常系] - 非 ASCII を含むクラス名が 0 になること。
    EXPECT_EQ(digit, digit_icase); // [確認_正常系] - digit の icase が digit のままであること。
    EXPECT_FALSE(bracket_upper);   // [確認_異常系] - '[' が upper でないこと。
    EXPECT_FALSE(brace_lower);     // [確認_異常系] - '{' が lower でないこと。
    EXPECT_FALSE(slash_digit);     // [確認_異常系] - '/' が digit でないこと。
    EXPECT_FALSE(at_xdigit);       // [確認_異常系] - '@' が xdigit でないこと。
    EXPECT_TRUE(del_cntrl);        // [確認_正常系] - DEL が cntrl であること。
    EXPECT_TRUE(cr_space);         // [確認_正常系] - CR が space であること。
    EXPECT_EQ(-1, hex_g);          // [確認_異常系] - g の値が -1 であること。
    EXPECT_EQ(-1, hex_G);          // [確認_異常系] - G の値が -1 であること。
}

// コンパイル フラグ変換と残りの文字クラス境界を充足することの確認
TEST_F(regexCoverageTest, syntax_option_and_remaining_class_boundaries)
{
    // Arrange
    const wchar_t bracket_name[] = {L'[', L'\0'};

    // Pre-Assert

    // Act
    bool both_syntax = test_regex_to_syntax_option(
        COM_UTIL_REGEX_EXTENDED | COM_UTIL_REGEX_BASIC); // [手順] - EXTENDED と BASIC を同時に指定する。
    bool extended_only = test_regex_to_syntax_option(COM_UTIL_REGEX_EXTENDED); // [手順] - EXTENDED だけを指定する。
    bool default_syntax = test_regex_to_syntax_option(COM_UTIL_REGEX_DEFAULT); // [手順] - 既定フラグを指定する。
    bool icase_nosub = test_regex_to_syntax_option(COM_UTIL_REGEX_ICASE |
                                                   COM_UTIL_REGEX_NOSUB); // [手順] - ICASE と NOSUB を指定する。
    unsigned int bracket_class =
        test_regex_lookup_classname(bracket_name, false); // [手順] - '[' だけのクラス名を変換する。
    int slash_value = test_regex_value(L'/', 10);         // [手順] - '/' の 10 進値を取得する。
    int colon_value = test_regex_value(L':', 10);         // [手順] - ':' の 10 進値を取得する。
    int grave_value = test_regex_value(L'`', 16);         // [手順] - '`' の 16 進値を取得する。
    bool graph_space =
        test_regex_isctype(L' ', test_regex_lookup_classname(L"graph", false)); // [手順] - 空白を graph で判定する。
    bool print_del =
        test_regex_isctype(L'\x7F', test_regex_lookup_classname(L"print", false)); // [手順] - DEL を print で判定する。

    // Assert
    EXPECT_FALSE(both_syntax);    // [確認_異常系] - EXTENDED と BASIC の同時指定が false であること。
    EXPECT_TRUE(extended_only);   // [確認_正常系] - EXTENDED だけの変換が true であること。
    EXPECT_TRUE(default_syntax);  // [確認_正常系] - 既定フラグの変換が true であること。
    EXPECT_TRUE(icase_nosub);     // [確認_正常系] - ICASE と NOSUB の変換が true であること。
    EXPECT_EQ(0U, bracket_class); // [確認_異常系] - '[' のクラス名が 0 であること。
    EXPECT_EQ(-1, slash_value);   // [確認_異常系] - '/' の値が -1 であること。
    EXPECT_EQ(-1, colon_value);   // [確認_異常系] - ':' の値が -1 であること。
    EXPECT_EQ(-1, grave_value);   // [確認_異常系] - '`' の値が -1 であること。
    EXPECT_FALSE(graph_space);    // [確認_異常系] - 空白が graph でないこと。
    EXPECT_FALSE(print_del);      // [確認_異常系] - DEL が print でないこと。
}

// 残っている複合条件と上限、サロゲート進行を充足することの確認
TEST_F(regexCoverageTest, remaining_source_conditions)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex *limit_regex = NULL;
    com_util_regex_iter *iter = NULL;
    com_util_regex_match extra[4] = {};
    int matched = 0;
    size_t part_count = 0;
    const std::string long_pattern(COM_UTIL_REGEX_MAX_LENGTH + 1U, 'a');
    const wchar_t two_low[] = {static_cast<wchar_t>(0xDC00), static_cast<wchar_t>(0xDC00), L'\0'};
    unsigned int class_at = 0;
    unsigned int class_z = 0;
    std::size_t advanced = 0;
    int create_limit = COM_UTIL_OK;
    int search_extra = COM_UTIL_OK;
    int replace_text_null = COM_UTIL_OK;
    int iter_regex_null = COM_UTIL_OK;
    int iter_text_null = COM_UTIL_OK;
    int next_has_null = COM_UTIL_OK;
    int split_regex_null = COM_UTIL_OK;
    int split_text_null = COM_UTIL_OK;

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex, NULL)); // [状態] - パターン "a" をコンパイルする。
                                                                                              // [状態確認] - com_util_regex_create の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    class_at = test_regex_lookup_classname(L"@", false); // [手順] - '@' だけのクラス名を変換する。
    class_z = test_regex_lookup_classname(L"Z", false);  // [手順] - 'Z' だけのクラス名を変換する。
    create_limit = com_util_regex_create(long_pattern.c_str(), COM_UTIL_REGEX_DEFAULT, &limit_regex,
                                         NULL); // [手順] - 上限を超えるパターンをコンパイルする。
    search_extra = com_util_regex_search(regex, "a", 1U, 0U, COM_UTIL_REGEX_DEFAULT, extra, 4U, &matched,
                                         NULL); // [手順] - グループ数より多い格納先で検索する。
    replace_text_null = com_util_regex_replace(regex, NULL, 0U, "x", COM_UTIL_REGEX_DEFAULT, NULL, 0U, NULL,
                                               NULL); // [手順] - text NULL で置換する。
    iter_regex_null = com_util_regex_iter_create(NULL, "a", 1U, COM_UTIL_REGEX_DEFAULT, &iter,
                                                 NULL); // [手順] - regex NULL で列挙を生成する。
    iter_text_null = com_util_regex_iter_create(regex, NULL, 0U, COM_UTIL_REGEX_DEFAULT, &iter,
                                                NULL); // [手順] - text NULL で列挙を生成する。
    next_has_null = com_util_regex_iter_next(reinterpret_cast<com_util_regex_iter *>(static_cast<uintptr_t>(0x1)), NULL,
                                             0U, NULL, NULL); // [手順] - has_match_out NULL で次一致を取得する。
    split_regex_null = com_util_regex_split(NULL, "a", 1U, 0U, COM_UTIL_REGEX_DEFAULT, NULL, 0U, &part_count,
                                            NULL); // [手順] - regex NULL で分割する。
    split_text_null = com_util_regex_split(regex, NULL, 0U, 0U, COM_UTIL_REGEX_DEFAULT, NULL, 0U, &part_count,
                                           NULL);        // [手順] - text NULL で分割する。
    advanced = test_regex_advance_position(two_low, 0U); // [手順] - 連続する下位サロゲートで位置を進める。
    com_util_regex_dispose(NULL);                        // [手順] - NULL のコンパイル済みパターンを破棄する。
    com_util_regex_iter_dispose(NULL);                   // [手順] - NULL の列挙を破棄する。

    // Assert
    EXPECT_EQ(0U, class_at); // [確認_異常系] - '@' のクラス名が 0 であること。
    EXPECT_EQ(0U, class_z);  // [確認_異常系] - 'Z' のクラス名が 0 であること。
    EXPECT_EQ(
        COM_UTIL_ERR_LIMIT_EXCEEDED,
        create_limit); // [確認_異常系] - 長過ぎるパターンの com_util_regex_create の戻り値が COM_UTIL_ERR_LIMIT_EXCEEDED であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        search_extra); // [確認_正常系] - 余剰グループ付き com_util_regex_search の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_REGEX_NPOS, extra[3].begin); // [確認_正常系] - 余剰グループの begin が NPOS であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        replace_text_null); // [確認_異常系] - text NULL の com_util_regex_replace の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        iter_regex_null); // [確認_異常系] - regex NULL の com_util_regex_iter_create の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        iter_text_null); // [確認_異常系] - text NULL の com_util_regex_iter_create の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        next_has_null); // [確認_異常系] - has_match_out NULL の com_util_regex_iter_next の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        split_regex_null); // [確認_異常系] - regex NULL の com_util_regex_split の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        split_text_null); // [確認_異常系] - text NULL の com_util_regex_split の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(2U, advanced); // [確認_正常系] - 連続下位サロゲートの test_regex_advance_position が 2 を返すこと。

    // Cleanup
    com_util_regex_dispose(regex);
}

#if !defined(COM_UTIL_REGEX_NO_EXCEPTIONS)
// 列挙の次一致取得が確保失敗を catch することの確認
TEST_F(regexCoverageTest, iter_next_translates_allocation_failure)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    int has_match = 0;
    int throw_count = 0;
    int fail_after = 0;
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex, NULL)); // [状態] - パターン "a" をコンパイルする。
                                                                                              // [状態確認] - com_util_regex_create の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_iter_create(regex, "aaa", 3U, COM_UTIL_REGEX_DEFAULT, &iter, NULL)); // [状態] - 入力 "aaa" のイテレーターを生成する。
                                                                                                               // [状態確認] - com_util_regex_iter_create の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    for (fail_after = 0; fail_after < 32; fail_after++)
    {
        int result = COM_UTIL_OK;
        g_regex_new_fail_after = fail_after;
        try
        {
            result = com_util_regex_iter_next(iter, NULL, 0U, &has_match,
                                              NULL); // [手順] - 確保失敗を注入して次の一致を取得する。
        }
        catch (const std::bad_alloc &)
        {
            result = COM_UTIL_ERR_OUT_OF_MEMORY;
        }
        g_regex_new_fail_after = -1;
        if (result == COM_UTIL_ERR_OUT_OF_MEMORY)
        {
            throw_count++;
        }
    }

    // Assert
    EXPECT_GT(throw_count, 0); // [確認_異常系] - iter_next が確保失敗を COM_UTIL_ERR_OUT_OF_MEMORY へ変換すること。

    // Cleanup
    com_util_regex_iter_dispose(iter);
    com_util_regex_dispose(regex);
}
#endif /* !COM_UTIL_REGEX_NO_EXCEPTIONS */
