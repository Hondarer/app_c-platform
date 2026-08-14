#include <testfw.h>
#include <mock_com_util.h>

#include <com_util/base/result.h>
#include <com_util/crt/fcntl.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

using namespace testing;

class fcntlFormatTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util;
};

static int call_vopen_fmt(int flags, int mode, com_util_error *detail_out, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    int actual_ret = com_util_vopen_fmt(flags, mode, detail_out, format, args);
    va_end(args);

    return actual_ret;
}

// 書式で組み立てたパスが com_util_open へ渡されることの確認
TEST_F(fcntlFormatTest, passes_formatted_path_to_open)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_open(StrEq("/tmp/sample_42.txt"), O_RDONLY, 0, &detail))
        .WillOnce(Return(7)); // [Pre-Assert確認_正常系] - com_util_open が展開後のパス "/tmp/sample_42.txt" を指定して 1 回呼び出されること。
                              // [Pre-Assert手順] - com_util_open からファイル記述子 7 を返却する。

    // Act
    int actual_ret = com_util_open_fmt(O_RDONLY, 0, &detail, "/tmp/sample_%d.txt",
                                42); // [手順] - 書式引数 42 を指定して com_util_open_fmt を呼び出す。

    // Assert
    EXPECT_EQ(7, actual_ret); // [確認_正常系] - com_util_open_fmt の戻り値が com_util_open の戻り値 7 であること。
}

// va_list 版が同じ経路を通ることの確認
TEST_F(fcntlFormatTest, vopen_fmt_passes_formatted_path_to_open)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_open(StrEq("/tmp/sample_7.txt"), O_RDONLY, 0, &detail))
        .WillOnce(Return(3)); // [Pre-Assert確認_正常系] - com_util_open が展開後のパス "/tmp/sample_7.txt" を指定して 1 回呼び出されること。
                              // [Pre-Assert手順] - com_util_open からファイル記述子 3 を返却する。

    // Act
    int actual_ret = call_vopen_fmt(O_RDONLY, 0, &detail, "/tmp/sample_%d.txt",
                             7); // [手順] - 書式引数 7 を指定して com_util_vopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ(3, actual_ret); // [確認_正常系] - com_util_vopen_fmt の戻り値が com_util_open の戻り値 3 であること。
}

// 書式展開に失敗した場合に com_util_open を呼ばずに -1 を返すことの確認
TEST_F(fcntlFormatTest, returns_minus1_without_open_when_format_fails)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_open(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_open が呼び出されないこと。

    // Act
    int actual_ret = com_util_open_fmt(O_RDONLY, 0, &detail,
                                NULL); // [手順] - 書式文字列に NULL を指定して com_util_open_fmt を呼び出す。

    // Assert
    EXPECT_EQ(-1, actual_ret); // [確認_異常系] - com_util_open_fmt の戻り値が -1 であること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
}
