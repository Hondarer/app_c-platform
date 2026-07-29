#include <testfw.h>
#include <com_util/crt/stdio.h>
#include <stdarg.h>

class snprintfTest : public Test
{
};

// com_util_vsnprintf へ可変長引数を橋渡しする補助関数
static int callVsnprintf(char *buf, size_t buf_size, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = com_util_vsnprintf(buf, buf_size, format, args);
    va_end(args);

    return ret;
}

// バッファーが十分な場合に書式化文字列が格納されることの確認
TEST_F(snprintfTest, snprintf_success)
{
    // Arrange
    char buf[16]; // [状態] - 16 バイトの出力先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret = com_util_snprintf(buf, sizeof(buf), "count=%d",
                                42); // [手順] - 書式 "count=%d" と 42 を指定して com_util_snprintf を呼び出す。

    // Assert
    EXPECT_EQ(8, ret);             // [確認_正常系] - com_util_snprintf の戻り値が書式化後の文字数 8 であること。
    EXPECT_STREQ("count=42", buf); // [確認_正常系] - バッファーに "count=42" が格納されること。
}

// バッファー不足の場合に切り詰めと NUL 終端が行われ、必要文字数が返ることの確認
TEST_F(snprintfTest, snprintf_truncates_and_returns_required_length)
{
    // Arrange
    char buf[4]; // [状態] - 書式化結果より小さい 4 バイトの出力先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret = com_util_snprintf(buf, sizeof(buf), "%s",
                                "abcdef"); // [手順] - 6 文字の "abcdef" を指定して com_util_snprintf を呼び出す。

    // Assert
    EXPECT_EQ(6, ret);        // [確認_異常系] - com_util_snprintf の戻り値が切り詰め前の必要文字数 6 であること。
    EXPECT_STREQ("abc", buf); // [確認_異常系] - バッファーに先頭 3 文字 "abc" と終端の NUL が格納されること。
}

// バッファーに NULL とサイズ 0 を指定した場合に必要文字数だけが返ることの確認
TEST_F(snprintfTest, snprintf_null_buffer_returns_required_length)
{
    // Arrange
    // (バッファーを使用しない)

    // Pre-Assert

    // Act
    int ret = com_util_snprintf(NULL, 0, "%s-%d", "abc",
                                12); // [手順] - バッファーに NULL、サイズに 0 を指定して com_util_snprintf を呼び出す。

    // Assert
    EXPECT_EQ(6, ret); // [確認_正常系] - com_util_snprintf の戻り値が必要文字数 6 であること。
}

// va_list 版でバッファーが十分な場合に書式化文字列が格納されることの確認
TEST_F(snprintfTest, vsnprintf_success)
{
    // Arrange
    char buf[16]; // [状態] - 16 バイトの出力先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret = callVsnprintf(buf, sizeof(buf), "id=%d",
                            7); // [手順] - 補助関数経由で書式 "id=%d" と 7 を指定して com_util_vsnprintf を呼び出す。

    // Assert
    EXPECT_EQ(4, ret);         // [確認_正常系] - com_util_vsnprintf の戻り値が書式化後の文字数 4 であること。
    EXPECT_STREQ("id=7", buf); // [確認_正常系] - バッファーに "id=7" が格納されること。
}

// va_list 版でバッファー不足の場合に切り詰めと NUL 終端が行われ、必要文字数が返ることの確認
TEST_F(snprintfTest, vsnprintf_truncates_and_returns_required_length)
{
    // Arrange
    char buf[4]; // [状態] - 書式化結果より小さい 4 バイトの出力先バッファーを用意する。

    // Pre-Assert

    // Act
    int ret =
        callVsnprintf(buf, sizeof(buf), "%s",
                      "abcdef"); // [手順] - 補助関数経由で 6 文字の "abcdef" を指定して com_util_vsnprintf を呼び出す。

    // Assert
    EXPECT_EQ(6, ret);        // [確認_異常系] - com_util_vsnprintf の戻り値が切り詰め前の必要文字数 6 であること。
    EXPECT_STREQ("abc", buf); // [確認_異常系] - バッファーに先頭 3 文字 "abc" と終端の NUL が格納されること。
}
