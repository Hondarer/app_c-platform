#include <testfw.h>

#include <com_util/base/error.h>
#include <com_util/base/result.h>
#include <com_util/regex/regex.h>

#include <cstring>
#include <string>
#include <vector>

class regexTest : public Test
{
  protected:
    /* text のうち match が示す範囲を取り出す。 */
    std::string slice(const std::string &text, const com_util_regex_match &match)
    {
        if ((match.begin == COM_UTIL_REGEX_NPOS) || (match.end == COM_UTIL_REGEX_NPOS))
        {
            return std::string();
        }
        return text.substr(match.begin, match.end - match.begin);
    }
};

// パターンをコンパイルして捕捉グループ数を取得できることの確認
TEST_F(regexTest, create_reports_group_count)
{
    // Arrange
    com_util_regex *regex = NULL;
    int result = COM_UTIL_ERR_UNKNOWN;

    // Pre-Assert

    // Act
    result = com_util_regex_create("(a)(b)(c)", COM_UTIL_REGEX_DEFAULT, &regex,
                                   NULL); // [手順] - 捕捉グループを 3 個持つパターンをコンパイルする。

    // Assert
    ASSERT_EQ(COM_UTIL_OK, result); // [確認_正常系] - com_util_regex_create の戻り値が COM_UTIL_OK であること。
    ASSERT_NE((com_util_regex *)NULL, regex); // [確認_正常系] - ハンドルが NULL でないこと。
    EXPECT_EQ(
        (size_t)4,
        com_util_regex_get_group_count(
            regex)); // [確認_正常系] - com_util_regex_get_group_count の戻り値が捕捉グループ数 + 1 の 4 であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 部分一致の検索でマッチ範囲がバイト オフセットとして返ることの確認
TEST_F(regexTest, search_returns_byte_offsets)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match matches[1];
    int matched = -1;
    int result = COM_UTIL_ERR_UNKNOWN;
    const std::string text = "xxabcyy";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a.c", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a.c" をコンパイルしておく。

    // Pre-Assert

    // Act
    result = com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, matches, 1,
                                   &matched, NULL); // [手順] - "xxabcyy" の先頭から検索する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result);         // [確認_正常系] - com_util_regex_search の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1, matched);                  // [確認_正常系] - 一致したことを示す 1 が matched へ格納されること。
    EXPECT_EQ((size_t)2, matches[0].begin); // [確認_正常系] - マッチ開始位置が 2 であること。
    EXPECT_EQ((size_t)5, matches[0].end);   // [確認_正常系] - マッチ終了位置が 5 であること。
    EXPECT_EQ(std::string("abc"), slice(text, matches[0])); // [確認_正常系] - 切り出した部分文字列が "abc" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 一致しない場合がエラーではなく matched = 0 で表されることの確認
TEST_F(regexTest, search_reports_no_match_as_success)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match matches[1];
    int matched = -1;
    int result = COM_UTIL_ERR_UNKNOWN;
    const std::string text = "xyz";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("abc", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "abc" をコンパイルしておく。

    // Pre-Assert

    // Act
    result = com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, matches, 1,
                                   &matched, NULL); // [手順] - 一致しない入力 "xyz" を検索する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - com_util_regex_search の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, matched);          // [確認_正常系] - 一致しなかったことを示す 0 が matched へ格納されること。
    EXPECT_EQ(COM_UTIL_REGEX_NPOS,
              matches[0].begin); // [確認_正常系] - マッチ開始位置に COM_UTIL_REGEX_NPOS が格納されること。
    EXPECT_EQ(COM_UTIL_REGEX_NPOS,
              matches[0].end); // [確認_正常系] - マッチ終了位置に COM_UTIL_REGEX_NPOS が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 全体一致の判定が部分一致と区別されることの確認
TEST_F(regexTest, matches_requires_whole_text)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched_partial = -1;
    int matched_whole = -1;
    const std::string text = "xxabcyy";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a.c", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a.c" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                                 0, &matched_partial,
                                                 NULL)); // [手順] - "xxabcyy" を com_util_regex_search で照合する。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_matches(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                                  0, &matched_whole,
                                                  NULL)); // [手順] - 同じ入力を com_util_regex_matches で照合する。

    // Assert
    EXPECT_EQ(1, matched_partial); // [確認_正常系] - com_util_regex_search では一致を示す 1 が格納されること。
    EXPECT_EQ(0, matched_whole);   // [確認_正常系] - com_util_regex_matches では不一致を示す 0 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 日本語 (BMP) の 1 文字が 1 文字として扱われることの確認
TEST_F(regexTest, matches_treats_bmp_character_as_one_character)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched_three = -1;
    int matched_two = -1;
    const std::string three = u8"あいう";
    const std::string two = u8"あい";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^.{3}$", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "^.{3}$" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_matches(regex, three.data(), three.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                                  0, &matched_three,
                                                  NULL)); // [手順] - 日本語 3 文字 "あいう" を照合する。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_matches(regex, two.data(), two.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched_two,
                                     NULL)); // [手順] - 日本語 2 文字 "あい" を照合する。

    // Assert
    EXPECT_EQ(1, matched_three); // [確認_正常系] - "あいう" の照合で一致を示す 1 が格納されること。
    EXPECT_EQ(0, matched_two);   // [確認_正常系] - "あい" の照合で不一致を示す 0 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// BMP 外の 1 文字が UTF-16 コード単位 2 個として扱われることの確認
TEST_F(regexTest, matches_treats_astral_character_as_two_characters)
{
    // Arrange
    com_util_regex *one_regex = NULL;
    com_util_regex *two_regex = NULL;
    com_util_regex_match matches[1];
    int matched_one = -1;
    int matched_two = -1;
    const std::string text = u8"\U0001F600";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^.$", COM_UTIL_REGEX_DEFAULT, &one_regex,
                                                 NULL)); // [状態] - パターン "^.$" をコンパイルしておく。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^.{2}$", COM_UTIL_REGEX_DEFAULT, &two_regex,
                                                 NULL)); // [状態] - パターン "^.{2}$" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_matches(one_regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT,
                                                  NULL, 0, &matched_one,
                                                  NULL)); // [手順] - "^.$" で U+1F600 を照合する。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_matches(two_regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT,
                                                  matches, 1, &matched_two,
                                                  NULL)); // [手順] - "^.{2}$" で U+1F600 を照合する。

    // Assert
    EXPECT_EQ(0, matched_one);              // [確認_正常系] - "^.$" の照合で不一致を示す 0 が格納されること。
    EXPECT_EQ(1, matched_two);              // [確認_正常系] - "^.{2}$" の照合で一致を示す 1 が格納されること。
    EXPECT_EQ((size_t)0, matches[0].begin); // [確認_正常系] - マッチ開始位置が 0 であること。
    EXPECT_EQ((size_t)4,
              matches[0].end); // [確認_正常系] - マッチ終了位置が UTF-8 の 4 バイト境界であること。

    // Cleanup
    com_util_regex_dispose(one_regex);
    com_util_regex_dispose(two_regex);
}

// 捕捉グループの範囲が日本語を含む入力でも正しく返ることの確認
TEST_F(regexTest, search_stores_capture_groups)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match matches[3];
    int matched = -1;
    const std::string text = u8"名前=あきら";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^(.+)=(.+)$", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "^(.+)=(.+)$" をコンパイルしておく。

    // Pre-Assert
    ASSERT_EQ((size_t)3,
              com_util_regex_get_group_count(
                  regex)); // [Pre-Assert確認_正常系] - com_util_regex_get_group_count の戻り値が 3 であること。

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT,
                                                 matches, 3, &matched,
                                                 NULL)); // [手順] - "名前=あきら" を検索する。

    // Assert
    EXPECT_EQ(1, matched);                                       // [確認_正常系] - 一致を示す 1 が格納されること。
    EXPECT_EQ(text, slice(text, matches[0]));                    // [確認_正常系] - グループ 0 が入力全体であること。
    EXPECT_EQ(std::string(u8"名前"), slice(text, matches[1]));   // [確認_正常系] - グループ 1 が "名前" であること。
    EXPECT_EQ(std::string(u8"あきら"), slice(text, matches[2])); // [確認_正常系] - グループ 2 が "あきら" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 不参加の捕捉グループが COM_UTIL_REGEX_NPOS になることの確認
TEST_F(regexTest, search_marks_unmatched_group_as_npos)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match matches[3];
    int matched = -1;
    const std::string text = "b";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^(a)?(b)$", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "^(a)?(b)$" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT,
                                                 matches, 3, &matched,
                                                 NULL)); // [手順] - グループ 1 が一致しない入力 "b" を検索する。

    // Assert
    EXPECT_EQ(1, matched); // [確認_正常系] - 一致を示す 1 が格納されること。
    EXPECT_EQ(COM_UTIL_REGEX_NPOS,
              matches[1].begin); // [確認_正常系] - 不参加グループの開始位置が COM_UTIL_REGEX_NPOS であること。
    EXPECT_EQ(COM_UTIL_REGEX_NPOS,
              matches[1].end); // [確認_正常系] - 不参加グループの終了位置が COM_UTIL_REGEX_NPOS であること。
    EXPECT_EQ(std::string("b"), slice(text, matches[2])); // [確認_正常系] - グループ 2 が "b" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// matches_capacity が不足する場合に先頭から切り捨てられることの確認
TEST_F(regexTest, search_truncates_groups_to_capacity)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match matches[2];
    int matched = -1;
    const std::string text = "abc";

    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_create("(a)(b)(c)", COM_UTIL_REGEX_DEFAULT, &regex,
                                    NULL)); // [状態] - 捕捉グループを 3 個持つパターンをコンパイルしておく。

    // Pre-Assert
    ASSERT_EQ((size_t)4,
              com_util_regex_get_group_count(
                  regex)); // [Pre-Assert確認_正常系] - com_util_regex_get_group_count の戻り値が 4 であること。

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT,
                                                 matches, 2, &matched,
                                                 NULL)); // [手順] - 要素数 2 の配列を渡して検索する。

    // Assert
    EXPECT_EQ(1, matched);                                  // [確認_正常系] - 一致を示す 1 が格納されること。
    EXPECT_EQ(std::string("abc"), slice(text, matches[0])); // [確認_正常系] - グループ 0 が格納されること。
    EXPECT_EQ(std::string("a"), slice(text, matches[1]));   // [確認_正常系] - グループ 1 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// COM_UTIL_REGEX_NOSUB 指定時に捕捉グループが記録されないことの確認
TEST_F(regexTest, create_with_nosub_reports_single_group)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched = -1;
    const std::string text = "abc";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("(a)(b)(c)", COM_UTIL_REGEX_NOSUB, &regex,
                                                 NULL)); // [状態] - COM_UTIL_REGEX_NOSUB を指定してコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_matches(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                     NULL)); // [手順] - "abc" を照合する。

    // Assert
    EXPECT_EQ(1, matched); // [確認_正常系] - 一致を示す 1 が格納されること。
    EXPECT_EQ((size_t)1,
              com_util_regex_get_group_count(
                  regex)); // [確認_正常系] - com_util_regex_get_group_count の戻り値が 1 であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// COM_UTIL_REGEX_ICASE の畳み込みが ASCII 範囲に限られることの確認
TEST_F(regexTest, icase_folds_ascii_only)
{
    // Arrange
    com_util_regex *ascii_regex = NULL;
    com_util_regex *latin_regex = NULL;
    int matched_ascii = -1;
    int matched_latin = -1;
    const std::string ascii_text = "XABCX";
    const std::string latin_text = u8"ä";

    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_create("abc", COM_UTIL_REGEX_ICASE, &ascii_regex,
                                    NULL)); // [状態] - ASCII のパターン "abc" を ICASE でコンパイルしておく。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create(u8"Ä", COM_UTIL_REGEX_ICASE, &latin_regex,
                                                 NULL)); // [状態] - 非 ASCII のパターン "Ä" を ICASE で
                                                         // コンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(ascii_regex, ascii_text.data(), ascii_text.size(), 0,
                                                 COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched_ascii,
                                                 NULL)); // [手順] - パターン "abc" で "XABCX" を検索する。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(latin_regex, latin_text.data(), latin_text.size(), 0,
                                                 COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched_latin,
                                                 NULL)); // [手順] - パターン "Ä" で "ä" を検索する。

    // Assert
    EXPECT_EQ(1, matched_ascii); // [確認_正常系] - ASCII では大小が畳み込まれ、一致を示す 1 が格納されること。
    EXPECT_EQ(0, matched_latin); // [確認_正常系] - 非 ASCII では畳み込まれず、不一致を示す 0 が格納されること。

    // Cleanup
    com_util_regex_dispose(ascii_regex);
    com_util_regex_dispose(latin_regex);
}

// COM_UTIL_REGEX_EXTENDED で POSIX 拡張正規表現として解釈されることの確認
TEST_F(regexTest, create_with_extended_uses_posix_ere)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched = -1;
    const std::string text = "ababc";

    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_create("(ab)+c", COM_UTIL_REGEX_EXTENDED, &regex,
                                    NULL)); // [状態] - COM_UTIL_REGEX_EXTENDED を指定してコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_matches(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                     NULL)); // [手順] - "ababc" を照合する。

    // Assert
    EXPECT_EQ(1, matched); // [確認_正常系] - 一致を示す 1 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 文字クラスが ASCII 定義で機能することの確認
TEST_F(regexTest, search_supports_ascii_character_classes)
{
    // Arrange
    com_util_regex *posix_regex = NULL;
    com_util_regex *escape_regex = NULL;
    com_util_regex_match posix_match[1];
    com_util_regex_match escape_match[1];
    int matched_posix = -1;
    int matched_escape = -1;
    const std::string text = u8"あ123い";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("[[:digit:]]+", COM_UTIL_REGEX_DEFAULT, &posix_regex,
                                                 NULL)); // [状態] - パターン "[[:digit:]]+" をコンパイルしておく。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("\\d+", COM_UTIL_REGEX_DEFAULT, &escape_regex,
                                                 NULL)); // [状態] - パターン "\\d+" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(posix_regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT,
                                                 posix_match, 1, &matched_posix,
                                                 NULL)); // [手順] - "[[:digit:]]+" で "あ123い" を検索する。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(escape_regex, text.data(), text.size(), 0,
                                                 COM_UTIL_REGEX_MATCH_DEFAULT, escape_match, 1, &matched_escape,
                                                 NULL)); // [手順] - "\\d+" で "あ123い" を検索する。

    // Assert
    EXPECT_EQ(1, matched_posix); // [確認_正常系] - "[[:digit:]]+" の検索で一致を示す 1 が格納されること。
    EXPECT_EQ(std::string("123"),
              slice(text, posix_match[0])); // [確認_正常系] - "[[:digit:]]+" のマッチ範囲が "123" であること。
    EXPECT_EQ(1, matched_escape);           // [確認_正常系] - "\\d+" の検索で一致を示す 1 が格納されること。
    EXPECT_EQ(std::string("123"),
              slice(text, escape_match[0])); // [確認_正常系] - "\\d+" のマッチ範囲が "123" であること。

    // Cleanup
    com_util_regex_dispose(posix_regex);
    com_util_regex_dispose(escape_regex);
}

// 途中位置からの検索で行頭アンカーが誤って一致しないことの確認
TEST_F(regexTest, search_from_offset_does_not_treat_offset_as_line_start)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched_from_head = -1;
    int matched_from_offset = -1;
    const std::string text = "ab";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^b", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "^b" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                                 0, &matched_from_head,
                                                 NULL)); // [手順] - start_offset に 0 を指定して検索する。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 1, COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                                 0, &matched_from_offset,
                                                 NULL)); // [手順] - start_offset に 1 を指定して検索する。

    // Assert
    EXPECT_EQ(0, matched_from_head); // [確認_正常系] - start_offset が 0 の検索で不一致を示す 0 が格納されること。
    EXPECT_EQ(0,
              matched_from_offset); // [確認_正常系] - start_offset が 1 の検索でも不一致を示す 0 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// COM_UTIL_REGEX_MATCH_ANCHORED が照合開始位置を固定することの確認
TEST_F(regexTest, search_with_anchored_matches_only_at_start_offset)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched_default = -1;
    int matched_anchored = -1;
    const std::string text = "xxab";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("ab", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "ab" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                                 0, &matched_default,
                                                 NULL)); // [手順] - COM_UTIL_REGEX_MATCH_DEFAULT を指定して検索する。
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_ANCHORED,
                                                 NULL, 0, &matched_anchored,
                                                 NULL)); // [手順] - COM_UTIL_REGEX_MATCH_ANCHORED を指定して検索する。

    // Assert
    EXPECT_EQ(1, matched_default);  // [確認_正常系] - 既定の検索で一致を示す 1 が格納されること。
    EXPECT_EQ(0, matched_anchored); // [確認_正常系] - 固定照合では不一致を示す 0 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 不正なパターンが COM_UTIL_ERR_INVALID_PATTERN になることの確認
TEST_F(regexTest, create_rejects_invalid_pattern)
{
    // Arrange
    com_util_regex *regex = NULL;

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_PATTERN,
              com_util_regex_create("(", COM_UTIL_REGEX_DEFAULT, &regex,
                                    NULL)); // [確認_異常系] - パターン "(" に対する com_util_regex_create の
                                            // 戻り値が COM_UTIL_ERR_INVALID_PATTERN であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_PATTERN,
              com_util_regex_create("[a-", COM_UTIL_REGEX_DEFAULT, &regex,
                                    NULL)); // [確認_異常系] - パターン "[a-" に対する com_util_regex_create の
                                            // 戻り値が COM_UTIL_ERR_INVALID_PATTERN であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_PATTERN,
              com_util_regex_create("a{2,1}", COM_UTIL_REGEX_DEFAULT, &regex,
                                    NULL));   // [確認_異常系] - パターン "a{2,1}" に対する com_util_regex_create の
                                              // 戻り値が COM_UTIL_ERR_INVALID_PATTERN であること。
    EXPECT_EQ((com_util_regex *)NULL, regex); // [確認_異常系] - 失敗時にハンドルが NULL のままであること。
}

// 不正なフラグの組み合わせが COM_UTIL_ERR_INVALID_ARGUMENT になることの確認
TEST_F(regexTest, create_rejects_invalid_flags)
{
    // Arrange
    com_util_regex *regex = NULL;

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_create("a", COM_UTIL_REGEX_EXTENDED | COM_UTIL_REGEX_BASIC, &regex,
                                    NULL)); // [確認_異常系] - EXTENDED と BASIC の同時指定に対する
                                            // com_util_regex_create の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT
                                            // であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_create("a", 0x8000U, &regex,
                                    NULL)); // [確認_異常系] - 未定義ビット 0x8000 の指定に対する
                                            // com_util_regex_create の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT
                                            // であること。
}

// 不正な UTF-8 が COM_UTIL_ERR_INVALID_ENCODING になることの確認
TEST_F(regexTest, rejects_invalid_utf8)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex *invalid_regex = NULL;
    int matched = -1;
    const std::string invalid_text("\xC0\x80", 2);

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a" をコンパイルしておく。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ENCODING,
              com_util_regex_create(invalid_text.c_str(), COM_UTIL_REGEX_DEFAULT, &invalid_regex,
                                    NULL)); // [確認_異常系] - 不正な UTF-8 のパターンに対する
                                            // com_util_regex_create の戻り値が COM_UTIL_ERR_INVALID_ENCODING
                                            // であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ENCODING,
              com_util_regex_search(regex, invalid_text.data(), invalid_text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT,
                                    NULL, 0, &matched,
                                    NULL)); // [確認_異常系] - 不正な UTF-8 の入力に対する com_util_regex_search の
                                            // 戻り値が COM_UTIL_ERR_INVALID_ENCODING であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// NULL 引数が COM_UTIL_ERR_INVALID_ARGUMENT になることの確認
TEST_F(regexTest, rejects_null_arguments)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched = -1;
    const std::string text = "a";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a" をコンパイルしておく。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_create(NULL, COM_UTIL_REGEX_DEFAULT, &regex,
                                    NULL)); // [確認_異常系] - pattern が NULL のときの com_util_regex_create の
                                            // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, NULL,
                                    NULL)); // [確認_異常系] - regex_out が NULL のときの com_util_regex_create の
                                            // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_search(NULL, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                    NULL)); // [確認_異常系] - regex が NULL のときの com_util_regex_search の
                                            // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_search(regex, NULL, 0, 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                    NULL)); // [確認_異常系] - text が NULL のときの com_util_regex_search の
                                            // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_search(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, NULL,
                                    NULL)); // [確認_異常系] - matched_out が NULL のときの com_util_regex_search の
                                            // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_matches(NULL, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                     NULL)); // [確認_異常系] - regex が NULL のときの com_util_regex_matches の
                                             // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ((size_t)0,
              com_util_regex_get_group_count(
                  NULL)); // [確認_異常系] - regex が NULL のときの com_util_regex_get_group_count の戻り値が
                          // 0 であること。

    // Cleanup
    com_util_regex_dispose(regex);
    com_util_regex_dispose(NULL);
}

// 不正な start_offset が COM_UTIL_ERR_INVALID_ARGUMENT になることの確認
TEST_F(regexTest, search_rejects_invalid_start_offset)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched = -1;
    const std::string text = u8"あい";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a" をコンパイルしておく。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_search(regex, text.data(), text.size(), text.size() + 1, COM_UTIL_REGEX_MATCH_DEFAULT,
                                    NULL, 0, &matched,
                                    NULL)); // [確認_異常系] - 入力長を超える start_offset に対する
                                            // com_util_regex_search の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT
                                            // であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_search(regex, text.data(), text.size(), 1, COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                    NULL)); // [確認_異常系] - 文字の途中を指す start_offset 1 に対する
                                            // com_util_regex_search の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT
                                            // であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 空の入力を照合できることの確認
TEST_F(regexTest, matches_accepts_empty_text)
{
    // Arrange
    com_util_regex *regex = NULL;
    int matched = -1;
    const std::string text;

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("^$", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "^$" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_matches(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &matched,
                                     NULL)); // [手順] - 空の入力を照合する。

    // Assert
    EXPECT_EQ(1, matched); // [確認_正常系] - 一致を示す 1 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// detail_out が成功時にクリアされ、失敗時に直前値へ反映されることの確認
TEST_F(regexTest, reports_error_detail)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex *invalid_regex = NULL;
    com_util_error detail;
    com_util_error last;

    std::memset(&detail, 0xFF, sizeof(detail)); // [状態] - detail を未初期化に相当する値で埋めておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 &detail)); // [手順] - 正しいパターン "a" をコンパイルする。

    // Assert
    EXPECT_EQ(0, com_util_error_is_set(&detail)); // [確認_正常系] - 成功時に detail_out がクリアされること。

    // Arrange_2

    // Pre-Assert_2

    // Act_2
    ASSERT_EQ(COM_UTIL_ERR_INVALID_PATTERN,
              com_util_regex_create("(", COM_UTIL_REGEX_DEFAULT, &invalid_regex,
                                    &detail)); // [手順_2] - 不正なパターン "(" をコンパイルする。

    // Assert_2
    EXPECT_EQ(0, com_util_error_is_set(
                     &detail)); // [確認_2_正常系] - OS 由来ではない失敗のため detail_out がクリアされること。
    com_util_error_get_last(&last);
    EXPECT_EQ(0, com_util_error_is_set(&last)); // [確認_2_正常系] - 直前値もクリアされること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 一致箇所を置換できることの確認
TEST_F(regexTest, replace_substitutes_all_matches)
{
    // Arrange
    com_util_regex *regex = NULL;
    char buffer[64];
    size_t required_size = 0;
    int result = COM_UTIL_ERR_UNKNOWN;
    const std::string text = "a1b22c";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("[0-9]+", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "[0-9]+" をコンパイルしておく。

    // Pre-Assert

    // Act
    result = com_util_regex_replace(regex, text.data(), text.size(), "#", COM_UTIL_REGEX_MATCH_DEFAULT, buffer,
                                    sizeof(buffer), &required_size,
                                    NULL); // [手順] - "a1b22c" の数字列をすべて "#" へ置換する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - com_util_regex_replace の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(std::string("a#b#c"), std::string(buffer)); // [確認_正常系] - 置換結果が "a#b#c" であること。
    EXPECT_EQ((size_t)6,
              required_size); // [確認_正常系] - required_size_out に null 終端を含む 6 が格納されること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 置換文字列の後方参照が展開されることの確認
TEST_F(regexTest, replace_expands_back_references)
{
    // Arrange
    com_util_regex *regex = NULL;
    char buffer[64];
    const std::string text = "user@host";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("(\\w+)@(\\w+)", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "(\\w+)@(\\w+)" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_replace(regex, text.data(), text.size(), "$2:$1",
                                                  COM_UTIL_REGEX_MATCH_DEFAULT, buffer, sizeof(buffer), NULL,
                                                  NULL)); // [手順] - 置換文字列 "$2:$1" で置換する。

    // Assert
    EXPECT_EQ(std::string("host:user"),
              std::string(buffer)); // [確認_正常系] - 置換結果が "host:user" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 置換フラグが機能することの確認
TEST_F(regexTest, replace_honors_replace_flags)
{
    // Arrange
    com_util_regex *regex = NULL;
    char first_only[64];
    char no_copy[64];
    const std::string text = "a1b2c";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("[0-9]", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "[0-9]" をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_replace(regex, text.data(), text.size(), "#", COM_UTIL_REGEX_REPLACE_FIRST_ONLY,
                                     first_only, sizeof(first_only), NULL,
                                     NULL)); // [手順] - COM_UTIL_REGEX_REPLACE_FIRST_ONLY を指定して置換する。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_replace(regex, text.data(), text.size(), "#", COM_UTIL_REGEX_REPLACE_NO_COPY, no_copy,
                                     sizeof(no_copy), NULL,
                                     NULL)); // [手順] - COM_UTIL_REGEX_REPLACE_NO_COPY を指定して置換する。

    // Assert
    EXPECT_EQ(std::string("a#b2c"),
              std::string(first_only)); // [確認_正常系] - FIRST_ONLY では最初の 1 件のみ置換されること。
    EXPECT_EQ(std::string("##"),
              std::string(no_copy)); // [確認_正常系] - NO_COPY では非一致部分が出力されないこと。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 必要サイズの問い合わせとバッファー不足が扱えることの確認 (マルチ フェーズ テスト)
TEST_F(regexTest, replace_reports_required_size)
{
    // Arrange
    com_util_regex *regex = NULL;
    size_t required_size = 0;
    char buffer[8];
    int result = COM_UTIL_ERR_UNKNOWN;
    const std::string text = "aaa";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a" をコンパイルしておく。

    // Pre-Assert

    // Act
    result = com_util_regex_replace(regex, text.data(), text.size(), "xy", COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0,
                                    &required_size,
                                    NULL); // [手順] - result_out に NULL を渡して必要サイズを問い合わせる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - 問い合わせ時の com_util_regex_replace の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)7,
              required_size); // [確認_正常系] - "xyxyxy" と null 終端を合わせた 7 が格納されること。

    // Arrange_2
    required_size = 0;

    // Pre-Assert_2

    // Act_2
    result = com_util_regex_replace(regex, text.data(), text.size(), "xy", COM_UTIL_REGEX_MATCH_DEFAULT, buffer, 6,
                                    &required_size,
                                    NULL); // [手順_2] - 必要サイズより 1 バイト小さい 6 を指定して置換する。

    // Assert_2
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              result);                   // [確認_2_異常系] - バッファー不足時の com_util_regex_replace の戻り値が
                                         // COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ((size_t)7, required_size); // [確認_2_異常系] - required_size_out に必要な 7 が格納されること。

    // Arrange_3
    required_size = 0;

    // Pre-Assert_3

    // Act_3
    result = com_util_regex_replace(regex, text.data(), text.size(), "xy", COM_UTIL_REGEX_MATCH_DEFAULT, buffer, 7,
                                    &required_size,
                                    NULL); // [手順_3] - ちょうどの 7 を指定して置換する。

    // Assert_3
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_3_正常系] - ちょうどのサイズでの com_util_regex_replace の戻り値が COM_UTIL_OK
                       // であること。
    EXPECT_EQ(std::string("xyxyxy"), std::string(buffer)); // [確認_3_正常系] - 置換結果が "xyxyxy" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 一致箇所を順に列挙できることの確認
TEST_F(regexTest, iter_enumerates_all_matches)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    com_util_regex_match match;
    int has_match = -1;
    std::vector<std::string> found;
    const std::string text = "a1bb22ccc333";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("[0-9]+", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "[0-9]+" をコンパイルしておく。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_iter_create(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, &iter,
                                         NULL)); // [状態] - 入力に対するイテレーターを生成しておく。

    // Pre-Assert
    ASSERT_NE((com_util_regex_iter *)NULL, iter); // [Pre-Assert確認_正常系] - イテレーターが NULL でないこと。

    // Act
    for (;;)
    {
        ASSERT_EQ(COM_UTIL_OK, com_util_regex_iter_next(iter, &match, 1, &has_match,
                                                        NULL)); // [手順] - 次の一致箇所を取得する。
        if (has_match == 0)
        {
            break;
        }
        found.push_back(slice(text, match));
    }

    // Assert
    ASSERT_EQ((size_t)3, found.size());      // [確認_正常系] - 列挙された件数が 3 であること。
    EXPECT_EQ(std::string("1"), found[0]);   // [確認_正常系] - 1 件目が "1" であること。
    EXPECT_EQ(std::string("22"), found[1]);  // [確認_正常系] - 2 件目が "22" であること。
    EXPECT_EQ(std::string("333"), found[2]); // [確認_正常系] - 3 件目が "333" であること。
    EXPECT_EQ(0, has_match);                 // [確認_正常系] - 列挙終了時に has_match_out へ 0 が格納されること。

    // Cleanup
    com_util_regex_iter_dispose(iter);
    com_util_regex_dispose(regex);
}

// 空文字列に一致するパターンでも列挙が終了することの確認
TEST_F(regexTest, iter_terminates_on_empty_matches)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    int has_match = -1;
    int count = 0;
    const std::string text = "ab";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a*", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - 空一致しうるパターン "a*" をコンパイルしておく。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_iter_create(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, &iter,
                                         NULL)); // [状態] - 入力 "ab" のイテレーターを生成しておく。

    // Pre-Assert

    // Act
    for (;;)
    {
        ASSERT_EQ(COM_UTIL_OK, com_util_regex_iter_next(iter, NULL, 0, &has_match,
                                                        NULL)); // [手順] - 次の一致箇所を取得する。
        if (has_match == 0)
        {
            break;
        }
        count++;
        ASSERT_LT(count, 100); // 無限ループの検出
    }

    // Assert
    EXPECT_LT(count, 100);   // [確認_正常系] - 空一致でも列挙が有限回で終了すること。
    EXPECT_EQ(0, has_match); // [確認_正常系] - 列挙終了時に has_match_out へ 0 が格納されること。

    // Cleanup
    com_util_regex_iter_dispose(iter);
    com_util_regex_dispose(regex);
}

// イテレーターが入力文字列を複製して保持することの確認
TEST_F(regexTest, iter_copies_input_text)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    com_util_regex_match match;
    int has_match = -1;
    std::string text = "abc";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("b", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "b" をコンパイルしておく。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_regex_iter_create(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, &iter,
                                         NULL)); // [状態] - 入力 "abc" のイテレーターを生成しておく。
    text = "zzz";                                // [状態] - イテレーター生成後に元の文字列を書き換える。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_iter_next(iter, &match, 1, &has_match,
                                                    NULL)); // [手順] - 次の一致箇所を取得する。

    // Assert
    EXPECT_EQ(1, has_match);           // [確認_正常系] - 一致を示す 1 が格納されること。
    EXPECT_EQ((size_t)1, match.begin); // [確認_正常系] - 元の "abc" に対するマッチ開始位置 1 が返ること。
    EXPECT_EQ((size_t)2, match.end);   // [確認_正常系] - 元の "abc" に対するマッチ終了位置 2 が返ること。

    // Cleanup
    com_util_regex_iter_dispose(iter);
    com_util_regex_dispose(regex);
}

// 入力を分割できることの確認
TEST_F(regexTest, split_divides_text_by_matches)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match parts[8];
    size_t part_count = 0;
    const std::string text = "a,,b,c";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create(",", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - 区切りのパターン "," をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_split(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, parts,
                                                8, &part_count,
                                                NULL)); // [手順] - "a,,b,c" を上限なしで分割する。

    // Assert
    ASSERT_EQ((size_t)4, part_count);                   // [確認_正常系] - 分割件数が 4 であること。
    EXPECT_EQ(std::string("a"), slice(text, parts[0])); // [確認_正常系] - 1 件目が "a" であること。
    EXPECT_EQ(std::string(""), slice(text, parts[1]));  // [確認_正常系] - 2 件目が空文字列であること。
    EXPECT_EQ(std::string("b"), slice(text, parts[2])); // [確認_正常系] - 3 件目が "b" であること。
    EXPECT_EQ(std::string("c"), slice(text, parts[3])); // [確認_正常系] - 4 件目が "c" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 区切りが先頭と末尾にある場合に空の要素が生成されることの確認
TEST_F(regexTest, split_keeps_empty_parts_at_boundaries)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match parts[8];
    size_t part_count = 0;
    const std::string text = ",a,";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create(",", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - 区切りのパターン "," をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_split(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, parts,
                                                8, &part_count,
                                                NULL)); // [手順] - ",a," を上限なしで分割する。

    // Assert
    ASSERT_EQ((size_t)3, part_count);                   // [確認_正常系] - 分割件数が 3 であること。
    EXPECT_EQ(std::string(""), slice(text, parts[0]));  // [確認_正常系] - 1 件目が空文字列であること。
    EXPECT_EQ(std::string("a"), slice(text, parts[1])); // [確認_正常系] - 2 件目が "a" であること。
    EXPECT_EQ(std::string(""), slice(text, parts[2]));  // [確認_正常系] - 3 件目が空文字列であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 一致箇所が無い場合に入力全体が 1 件として返ることの確認
TEST_F(regexTest, split_returns_whole_text_when_no_match)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match parts[2];
    size_t part_count = 0;
    const std::string text = "abc";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create(",", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - 区切りのパターン "," をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_split(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, parts,
                                                2, &part_count,
                                                NULL)); // [手順] - 区切りを含まない "abc" を分割する。

    // Assert
    ASSERT_EQ((size_t)1, part_count);                     // [確認_正常系] - 分割件数が 1 であること。
    EXPECT_EQ(std::string("abc"), slice(text, parts[0])); // [確認_正常系] - 1 件目が入力全体であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// max_parts で分割数を打ち切れることの確認
TEST_F(regexTest, split_honors_max_parts)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match parts[8];
    size_t part_count = 0;
    const std::string text = "a,b,c,d";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create(",", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - 区切りのパターン "," をコンパイルしておく。

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_regex_split(regex, text.data(), text.size(), 2, COM_UTIL_REGEX_MATCH_DEFAULT, parts,
                                                8, &part_count,
                                                NULL)); // [手順] - max_parts に 2 を指定して分割する。

    // Assert
    ASSERT_EQ((size_t)2, part_count);                       // [確認_正常系] - 分割件数が 2 であること。
    EXPECT_EQ(std::string("a"), slice(text, parts[0]));     // [確認_正常系] - 1 件目が "a" であること。
    EXPECT_EQ(std::string("b,c,d"), slice(text, parts[1])); // [確認_正常系] - 2 件目が残り全体の "b,c,d" であること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 分割結果の件数を問い合わせられることの確認 (マルチ フェーズ テスト)
TEST_F(regexTest, split_reports_required_count)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_match parts[2];
    size_t part_count = 0;
    int result = COM_UTIL_ERR_UNKNOWN;
    const std::string text = "a,b,c";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create(",", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - 区切りのパターン "," をコンパイルしておく。

    // Pre-Assert

    // Act
    result =
        com_util_regex_split(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, &part_count,
                             NULL); // [手順] - parts_out に NULL を渡して件数を問い合わせる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - 問い合わせ時の com_util_regex_split の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)3, part_count); // [確認_正常系] - 必要な件数 3 が格納されること。

    // Arrange_2
    part_count = 0;

    // Pre-Assert_2

    // Act_2
    result =
        com_util_regex_split(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, parts, 2, &part_count,
                             NULL); // [手順_2] - 必要件数より少ない要素数 2 の配列を渡して分割する。

    // Assert_2
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              result);                // [確認_2_異常系] - 件数不足時の com_util_regex_split の戻り値が
                                      // COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ((size_t)3, part_count); // [確認_2_異常系] - part_count_out に必要な件数 3 が格納されること。
    EXPECT_EQ(std::string("a"), slice(text, parts[0])); // [確認_2_異常系] - 格納できる範囲まで書き込まれていること。

    // Cleanup
    com_util_regex_dispose(regex);
}

// 置換・列挙・分割が NULL 引数を拒否することの確認
TEST_F(regexTest, replace_iter_split_reject_null_arguments)
{
    // Arrange
    com_util_regex *regex = NULL;
    com_util_regex_iter *iter = NULL;
    char buffer[8];
    size_t part_count = 0;
    int has_match = -1;
    const std::string text = "a";

    ASSERT_EQ(COM_UTIL_OK, com_util_regex_create("a", COM_UTIL_REGEX_DEFAULT, &regex,
                                                 NULL)); // [状態] - パターン "a" をコンパイルしておく。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_replace(NULL, text.data(), text.size(), "x", COM_UTIL_REGEX_MATCH_DEFAULT, buffer,
                                     sizeof(buffer), NULL,
                                     NULL)); // [確認_異常系] - regex が NULL のときの com_util_regex_replace の
                                             // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_replace(regex, text.data(), text.size(), NULL, COM_UTIL_REGEX_MATCH_DEFAULT, buffer,
                                     sizeof(buffer), NULL,
                                     NULL)); // [確認_異常系] - replacement が NULL のときの com_util_regex_replace
                                             // の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_iter_create(regex, text.data(), text.size(), COM_UTIL_REGEX_MATCH_DEFAULT, NULL,
                                         NULL)); // [確認_異常系] - iter_out が NULL のときの
                                                 // com_util_regex_iter_create の戻り値が
                                                 // COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_iter_next(NULL, NULL, 0, &has_match,
                                       NULL)); // [確認_異常系] - iter が NULL のときの com_util_regex_iter_next の
                                               // 戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_regex_split(regex, text.data(), text.size(), 0, COM_UTIL_REGEX_MATCH_DEFAULT, NULL, 0, NULL,
                                   NULL)); // [確認_異常系] - part_count_out が NULL のときの com_util_regex_split
                                           // の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ((size_t)0, part_count);      // [確認_異常系] - part_count が変更されていないこと。

    // Cleanup
    com_util_regex_iter_dispose(iter);
    com_util_regex_dispose(regex);
}
