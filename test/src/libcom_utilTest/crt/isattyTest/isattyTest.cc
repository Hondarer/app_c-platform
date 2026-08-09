#include <testfw.h>
#include <com_util/crt/unistd.h>

class isattyTest : public Test
{
};

// 定義外のストリーム指定で 0 が返ることの確認
TEST_F(isattyTest, invalid_stream_returns_zero)
{
    // Arrange
    /* 列挙範囲外の不正値を意図的に渡す (定数キャストは -Wconversion になるため変数経由) */
    int invalid_stream_value = 99;
    com_util_stream invalid_stream = (com_util_stream)invalid_stream_value; // [状態] - 定義外の enum 値 99 とする。

    // Pre-Assert

    // Act
    int ret = com_util_isatty(invalid_stream); // [手順] - 定義外の enum 値を渡して com_util_isatty を呼び出す。

    // Assert
    EXPECT_EQ(0, ret); // [確認_異常系] - com_util_isatty の戻り値が 0 であること。
}

// stdin の判定が 0 か 1 の範囲で返ることの確認
TEST_F(isattyTest, stdin_returns_int)
{
    // Arrange

    // Pre-Assert

    // Act
    int ret =
        com_util_isatty(COM_UTIL_STREAM_STDIN); // [手順] - COM_UTIL_STREAM_STDIN を渡して com_util_isatty を呼び出す。

    // Assert
    /* テスト ハーネス下では stdin はリダイレクトされているため戻り値は環境依存 */
    EXPECT_TRUE(ret == 0 ||
                ret == 1); // [確認_正常系] - com_util_isatty の戻り値が 0 または 1 であり、クラッシュしないこと。
}

// stdout の判定が 0 か 1 の範囲で返ることの確認
TEST_F(isattyTest, stdout_returns_int)
{
    // Arrange

    // Pre-Assert

    // Act
    int ret = com_util_isatty(
        COM_UTIL_STREAM_STDOUT); // [手順] - COM_UTIL_STREAM_STDOUT を渡して com_util_isatty を呼び出す。

    // Assert
    EXPECT_TRUE(ret == 0 ||
                ret == 1); // [確認_正常系] - com_util_isatty の戻り値が 0 または 1 であり、クラッシュしないこと。
}

// stderr の判定が 0 か 1 の範囲で返ることの確認
TEST_F(isattyTest, stderr_returns_int)
{
    // Arrange

    // Pre-Assert

    // Act
    int ret = com_util_isatty(
        COM_UTIL_STREAM_STDERR); // [手順] - COM_UTIL_STREAM_STDERR を渡して com_util_isatty を呼び出す。

    // Assert
    EXPECT_TRUE(ret == 0 ||
                ret == 1); // [確認_正常系] - com_util_isatty の戻り値が 0 または 1 であり、クラッシュしないこと。
}
