#include <testfw.h>
#include <com_util/crt/string.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int call_com_util_vsscanf(const char *buffer, const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = com_util_vsscanf(buffer, format, args);
    va_end(args);
    return result;
}

class crt_stringTest : public Test
{
};

// 文字列がコピーされることの確認
TEST_F(crt_stringTest, strcpy_success)
{
    // Arrange
    char buf[16]; // [状態] - 16 バイトのコピー先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret = com_util_strcpy(buf, sizeof(buf), "abc"); // [手順] - "abc" を指定して com_util_strcpy を呼び出す。

    // Assert
    EXPECT_EQ(0, ret);        // [確認_正常系] - com_util_strcpy の戻り値が 0 であること。
    EXPECT_STREQ("abc", buf); // [確認_正常系] - バッファーに "abc" がコピーされること。
}

// コピー元が NULL の場合に EINVAL を返すことの確認
TEST_F(crt_stringTest, strcpy_null_argument)
{
    // Arrange
    char buf[16]; // [状態] - 16 バイトのコピー先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret = com_util_strcpy(buf, sizeof(buf), NULL); // [手順] - コピー元に NULL を渡して com_util_strcpy を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_strcpy の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// バッファー不足の場合に ERANGE を返しバッファーが空になることの確認
TEST_F(crt_stringTest, strcpy_buffer_shortage)
{
    // Arrange
    char buf[3] = "xx"; // [状態] - コピー元より小さい 3 バイトのバッファーを "xx" で初期化する。

    // Pre-Assert

    // Act
    int ret =
        com_util_strcpy(buf, sizeof(buf), "abcd"); // [手順] - 4 文字の "abcd" を指定して com_util_strcpy を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              ret);         // [確認_異常系] - com_util_strcpy の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_STREQ("", buf);  // [確認_異常系] - バッファーが空文字列にクリアされること。
}

// count 指定で文字列が切り詰めてコピーされることの確認
TEST_F(crt_stringTest, strncpy_truncates_to_count)
{
    // Arrange
    char buf[8]; // [状態] - 8 バイトのコピー先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret = com_util_strncpy(buf, sizeof(buf), "abcdef",
                               3); // [手順] - "abcdef" と count=3 を指定して com_util_strncpy を呼び出す。

    // Assert
    EXPECT_EQ(0, ret);        // [確認_正常系] - com_util_strncpy の戻り値が 0 であること。
    EXPECT_STREQ("abc", buf); // [確認_正常系] - 先頭 3 文字 "abc" だけがコピーされること。
}

// 文字列が連結されることの確認
TEST_F(crt_stringTest, strcat_success)
{
    // Arrange
    char buf[16] = "abc"; // [状態] - 連結先バッファーを "abc" で初期化する。

    // Pre-Assert

    // Act
    int ret = com_util_strcat(buf, sizeof(buf), "def"); // [手順] - "def" を指定して com_util_strcat を呼び出す。

    // Assert
    EXPECT_EQ(0, ret);           // [確認_正常系] - com_util_strcat の戻り値が 0 であること。
    EXPECT_STREQ("abcdef", buf); // [確認_正常系] - バッファーが "abcdef" になること。
}

// count 指定で連結元が切り詰められることの確認
TEST_F(crt_stringTest, strncat_truncates_to_count)
{
    // Arrange
    char buf[16] = "abc"; // [状態] - 連結先バッファーを "abc" で初期化する。

    // Pre-Assert

    // Act
    int ret = com_util_strncat(buf, sizeof(buf), "defgh", 2u); // [手順] - "defgh" の先頭 2 文字を連結する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_strncat の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("abcde", buf);  // [確認_正常系] - バッファーが "abcde" になること。
}

// count が連結元の長さを超える場合に全体が連結されることの確認
TEST_F(crt_stringTest, strncat_copies_whole_source_when_count_exceeds_length)
{
    // Arrange
    char buf[16] = "abc"; // [状態] - 連結先バッファーを "abc" で初期化する。

    // Pre-Assert

    // Act
    int ret = com_util_strncat(buf, sizeof(buf), "de", 10u); // [手順] - 長さ 2 の "de" に count 10 を指定して連結する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_strncat の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("abcde", buf);  // [確認_正常系] - バッファーが "abcde" になること。
}

// 連結結果がちょうど収まる場合に成功することの確認
TEST_F(crt_stringTest, strncat_exact_fit_succeeds)
{
    // Arrange
    char buf[6] = "abc"; // [状態] - 終端を含めて "abcde" がちょうど収まる 6 バイトのバッファーを "abc" で初期化する。

    // Pre-Assert

    // Act
    int ret = com_util_strncat(buf, sizeof(buf), "de", 2u); // [手順] - 残り容量ちょうどの "de" を連結する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_strncat の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("abcde", buf);  // [確認_正常系] - バッファーが "abcde" になること。
}

// 連結結果が収まらない場合に連結先が変更されないことの確認
TEST_F(crt_stringTest, strncat_buffer_shortage_keeps_dest)
{
    // Arrange
    char buf[6] = "abc"; // [状態] - 6 バイトのバッファーを "abc" で初期化する。

    // Pre-Assert

    // Act
    int ret = com_util_strncat(buf, sizeof(buf), "defg", 4u); // [手順] - 残り容量を超える 4 文字を連結させる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              ret);           // [確認_異常系] - com_util_strncat の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_STREQ("abc", buf); // [確認_異常系] - 連結先が "abc" のまま変更されないこと。
}

// 連結先が null 終端していない場合にバッファー不足を返すことの確認
TEST_F(crt_stringTest, strncat_unterminated_dest_returns_buffer_too_small)
{
    // Arrange
    char buf[4] = {'a', 'b', 'c', 'd'}; // [状態] - null 終端を持たない 4 バイトのバッファーを用意する。

    // Pre-Assert

    // Act
    int ret = com_util_strncat(buf, sizeof(buf), "e", 1u); // [手順] - 非終端のバッファーへ "e" を連結させる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              ret);        // [確認_異常系] - com_util_strncat の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_STREQ("", buf); // [確認_異常系] - バッファーが空文字列にクリアされること。
}

// 連結元が NULL の場合に引数エラーになることの確認
TEST_F(crt_stringTest, strncat_null_argument)
{
    // Arrange
    char buf[16] = "abc"; // [状態] - 連結先バッファーを "abc" で初期化する。

    // Pre-Assert

    // Act
    int ret =
        com_util_strncat(buf, sizeof(buf), NULL, 1u); // [手順] - 連結元に NULL を渡して com_util_strncat を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_strncat の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 文字列がトークンへ分割されることの確認
TEST_F(crt_stringTest, strtok_r_splits_tokens)
{
    // Arrange
    char text[] = "a,b,c"; // [状態] - カンマ区切りの文字列 "a,b,c" を用意する。
    char *saveptr = NULL;
    char *first;
    char *second;
    char *third;
    char *fourth;

    // Pre-Assert

    // Act
    first = com_util_strtok_r(text, ",", &saveptr);  // [手順] - 1 回目は対象文字列を指定して呼び出す。
    second = com_util_strtok_r(NULL, ",", &saveptr); // [手順] - 2 回目は NULL を指定して呼び出す。
    third = com_util_strtok_r(NULL, ",", &saveptr);  // [手順] - 3 回目は NULL を指定して呼び出す。
    fourth = com_util_strtok_r(NULL, ",", &saveptr); // [手順] - 4 回目は NULL を指定して呼び出す。

    // Assert
    EXPECT_STREQ("a", first);        // [確認_正常系] - 1 回目の com_util_strtok_r が "a" を返すこと。
    EXPECT_STREQ("b", second);       // [確認_正常系] - 2 回目の com_util_strtok_r が "b" を返すこと。
    EXPECT_STREQ("c", third);        // [確認_正常系] - 3 回目の com_util_strtok_r が "c" を返すこと。
    EXPECT_EQ((char *)NULL, fourth); // [確認_正常系] - 4 回目の com_util_strtok_r が NULL を返すこと。
}

// 区切り文字が NULL の場合に NULL を返すことの確認
TEST_F(crt_stringTest, strtok_r_null_delimiter_returns_null)
{
    // Arrange
    char text[] = "a,b"; // [状態] - カンマ区切りの文字列 "a,b" を用意する。
    char *saveptr = NULL;

    // Pre-Assert

    // Act
    char *token = com_util_strtok_r(text, NULL, &saveptr); // [手順] - 区切り文字に NULL を渡して呼び出す。

    // Assert
    EXPECT_EQ((char *)NULL, token); // [確認_異常系] - com_util_strtok_r の戻り値が NULL であること。
}

// 3 つのトークンが読み取られることの確認
TEST_F(crt_stringTest, sscanf_reads_three_tokens)
{
    // Arrange
    char token1[8];
    char token2[8];
    char token3[8]; // [状態] - トークンの受け取り先バッファーを 3 つ用意する。

    // Pre-Assert

    // Act
    int count = com_util_sscanf("one two three", "%7s %7s %7s", token1, token2,
                                token3); // [手順] - 入力 "one two three" を %7s x 3 で解析する。

    // Assert
    EXPECT_EQ(3, count);           // [確認_正常系] - 読み取り数が 3 であること。
    EXPECT_STREQ("one", token1);   // [確認_正常系] - 1 つ目のトークンが "one" であること。
    EXPECT_STREQ("two", token2);   // [確認_正常系] - 2 つ目のトークンが "two" であること。
    EXPECT_STREQ("three", token3); // [確認_正常系] - 3 つ目のトークンが "three" であること。
}

// 入力が不足している場合に読み取れた数だけが返ることの確認
TEST_F(crt_stringTest, sscanf_returns_partial_count_for_short_input)
{
    // Arrange
    char token1[8];
    char token2[8];
    char token3[8]; // [状態] - トークンの受け取り先バッファーを 3 つ用意する。

    // Pre-Assert

    // Act
    int count = com_util_sscanf("one two", "%7s %7s %7s", token1, token2,
                                token3); // [手順] - トークンが 2 つの入力 "one two" を %7s x 3 で解析する。

    // Assert
    EXPECT_EQ(2, count); // [確認_正常系] - 読み取り数が 2 であること。
}

// 連続する空白やタブが読み飛ばされることの確認
TEST_F(crt_stringTest, sscanf_skips_repeated_spaces)
{
    // Arrange
    char token1[8];
    char token2[8];
    char token3[8]; // [状態] - トークンの受け取り先バッファーを 3 つ用意する。

    // Pre-Assert

    // Act
    int count = com_util_sscanf("  one   two\tthree", "%7s %7s %7s", token1, token2,
                                token3); // [手順] - 連続空白とタブを含む入力を %7s x 3 で解析する。

    // Assert
    EXPECT_EQ(3, count);           // [確認_正常系] - 読み取り数が 3 であること。
    EXPECT_STREQ("one", token1);   // [確認_正常系] - 1 つ目のトークンが "one" であること。
    EXPECT_STREQ("two", token2);   // [確認_正常系] - 2 つ目のトークンが "two" であること。
    EXPECT_STREQ("three", token3); // [確認_正常系] - 3 つ目のトークンが "three" であること。
}

// 幅指定を超える入力が切り詰められることの確認
TEST_F(crt_stringTest, sscanf_respects_width_limit)
{
    // Arrange
    char token[4]; // [状態] - 4 バイトのトークン受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    int count = com_util_sscanf("abcdef", "%3s", token); // [手順] - 6 文字の入力 "abcdef" を %3s で解析する。

    // Assert
    EXPECT_EQ(1, count);        // [確認_正常系] - 読み取り数が 1 であること。
    EXPECT_STREQ("abc", token); // [確認_正常系] - 幅 3 で切り詰められた "abc" が得られること。
}

// vsscanf 版でも幅指定した文字列が読み取られることの確認
TEST_F(crt_stringTest, vsscanf_reads_width_limited_token)
{
    // Arrange
    char token[4]; // [状態] - 4 バイトのトークン受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    int count = call_com_util_vsscanf("abcdef", "%3s", token); // [手順] - "abcdef" を %3s で解析する。

    // Assert
    EXPECT_EQ(1, count);        // [確認_正常系] - com_util_vsscanf の戻り値が 1 であること。
    EXPECT_STREQ("abc", token); // [確認_正常系] - 先頭 3 文字が読み取られること。
}

// com_util_strdup が文字列を複製することの確認
TEST_F(crt_stringTest, strdup_duplicates_string)
{
    // Arrange
    const char *src = "hello"; // [状態] - 複製元の文字列を "hello" とする。

    // Pre-Assert

    // Act
    char *dup = com_util_strdup(src); // [手順] - "hello" を複製する。

    // Assert
    ASSERT_NE(nullptr, dup);    // [確認_正常系] - com_util_strdup の戻り値が NULL でないこと。
    EXPECT_STREQ("hello", dup); // [確認_正常系] - 複製された内容が "hello" であること。
    EXPECT_NE(src, dup);        // [確認_正常系] - 複製元とは異なる領域が返ること。

    // Cleanup
    free(dup);
}

// com_util_strdup が NULL に対して NULL を返すことの確認
TEST_F(crt_stringTest, strdup_null_returns_null)
{
    // Arrange

    // Pre-Assert

    // Act
    char *dup = com_util_strdup(NULL); // [手順] - 複製元に NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(nullptr, dup); // [確認_異常系] - com_util_strdup の戻り値が NULL であること。
}
