#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/base/result.h>
#include <com_util/crt/sys/stat.h>
#include <string.h>

class sysStatFormatTest : public Test
{
};

// buf が NULL の場合に com_util_stat を呼び出さず COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(sysStatFormatTest, test_null_buf)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_stat が呼び出されないこと。

    // Act
    int ret =
        com_util_stat_fmt(NULL, NULL, "test_%d.txt", 1); // [手順] - buf に NULL を渡して com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_stat_fmt から COM_UTIL_ERR_INVALID_ARGUMENT が返されること。
}

// format が NULL の場合に com_util_stat を呼び出さず COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(sysStatFormatTest, test_null_format)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_stat が呼び出されないこと。

    // Act
    int ret = com_util_stat_fmt(&st, NULL, NULL); // [手順] - format に NULL を渡して com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_stat_fmt から COM_UTIL_ERR_INVALID_ARGUMENT が返されること。
}

// フォーマット結果がバッファー サイズを超える場合に com_util_stat を呼び出さず COM_UTIL_ERR_BUFFER_TOO_SMALL を返すことの確認
TEST_F(sysStatFormatTest, test_buffer_overflow)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_file_stat_t st;
    char long_string[5000];
    memset(long_string, 'a', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0'; // [状態] - バッファー サイズを超える 4999 文字のファイル名を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_stat が呼び出されないこと。

    // Act
    int ret = com_util_stat_fmt(
        &st, NULL, "%s.txt",
        long_string); // [手順] - バッファー サイズを超えるファイル名を指定して com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              ret); // [確認_異常系] - com_util_stat_fmt から COM_UTIL_ERR_BUFFER_TOO_SMALL が返されること。
}

// フォーマット文字列を展開したファイル名で com_util_stat が呼び出されることの確認
TEST_F(sysStatFormatTest, test_successful_call_with_format)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(&st, _, StrEq("test_123.txt")))
        .WillOnce(Return(
            COM_UTIL_OK)); // [Pre-Assert確認_正常系] - com_util_stat が展開後のファイル名 "test_123.txt" で 1 回呼び出されること。
                           // [Pre-Assert手順] - com_util_stat から COM_UTIL_OK を返却する。

    // Act
    int ret = com_util_stat_fmt(
        &st, NULL, "test_%d.txt",
        123); // [手順] - フォーマット文字列 "test_%d.txt" と引数 123 を指定して com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_stat_fmt から COM_UTIL_OK が返されること。
}

// 複数のフォーマット パラメーターを展開したファイル名で com_util_stat が呼び出されることの確認
TEST_F(sysStatFormatTest, test_successful_call_with_multiple_parameters)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(&st, _, StrEq("output_1_2_3.txt")))
        .WillOnce(Return(
            COM_UTIL_OK)); // [Pre-Assert確認_正常系] - com_util_stat が展開後のファイル名 "output_1_2_3.txt" で 1 回呼び出されること。
                           // [Pre-Assert手順] - com_util_stat から COM_UTIL_OK を返却する。

    // Act
    int ret = com_util_stat_fmt(
        &st, NULL, "output_%d_%d_%d.txt", 1, 2,
        3); // [手順] - フォーマット文字列 "output_%d_%d_%d.txt" と引数 1, 2, 3 を指定して com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_stat_fmt から COM_UTIL_OK が返されること。
}

// com_util_stat が失敗した場合に、その結果コードをそのまま返すことの確認
TEST_F(sysStatFormatTest, test_stat_returns_error)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(&st, _, StrEq("nonexistent.txt")))
        .WillOnce(Return(
            COM_UTIL_ERR_NOT_FOUND)); // [Pre-Assert確認_異常系] - com_util_stat がファイル名 "nonexistent.txt" で 1 回呼び出されること。
                                      // [Pre-Assert手順] - com_util_stat から COM_UTIL_ERR_NOT_FOUND を返却する。

    // Act
    int ret = com_util_stat_fmt(
        &st, NULL,
        "nonexistent.txt"); // [手順] - 存在しないファイル名 "nonexistent.txt" を指定して com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              ret); // [確認_異常系] - com_util_stat_fmt から COM_UTIL_ERR_NOT_FOUND が返されること。
}
