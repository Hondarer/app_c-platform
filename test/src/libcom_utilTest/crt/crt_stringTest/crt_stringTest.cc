#include <testfw.h>
#include <com_util/crt/string.h>
#include <errno.h>
#include <string.h>

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
    EXPECT_EQ(EINVAL, ret); // [確認_異常系] - com_util_strcpy の戻り値が EINVAL であること。
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
    EXPECT_EQ(ERANGE, ret); // [確認_異常系] - com_util_strcpy の戻り値が ERANGE であること。
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
