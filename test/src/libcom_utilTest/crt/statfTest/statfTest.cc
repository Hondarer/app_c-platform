#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/crt/sys/stat.h>
#include <string.h>

class statfTest : public Test
{
};

TEST_F(statfTest, test_null_buf)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(_, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_stat が呼び出されないこと。

    // Act
    int ret = com_util_stat_fmt(NULL, "test_%d.txt", 1); // [手順] - buf に NULL を渡す。

    // Assert
    EXPECT_EQ(-1, ret); // [確認_異常系] - com_util_stat_fmt から -1 が返されること。
}

TEST_F(statfTest, test_null_format)
{
    // Arrange
    Mock_com_util mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(_, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_stat が呼び出されないこと。

    // Act
    int ret = com_util_stat_fmt(&st, NULL); // [手順] - format に NULL を渡す。

    // Assert
    EXPECT_EQ(-1, ret); // [確認_異常系] - com_util_stat_fmt から -1 が返されること。
}

TEST_F(statfTest, test_buffer_overflow)
{
    // Arrange
    Mock_com_util mock_com_util;
    com_util_file_stat_t st;
    // 非常に長いファイル名を生成 (バッファー サイズを超える)
    char long_string[5000];
    memset(long_string, 'a', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0';

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(_, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_stat が呼び出されないこと。

    // Act
    int ret = com_util_stat_fmt(&st, "%s.txt", long_string); // [手順] - バッファー サイズを超えるファイル名を指定する。

    // Assert
    EXPECT_EQ(-1, ret); // [確認_異常系] - com_util_stat_fmt から -1 が返されること。
}

TEST_F(statfTest, test_successful_call_with_format)
{
    // Arrange
    Mock_com_util mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(&st, StrEq("test_123.txt")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - com_util_stat が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    int ret = com_util_stat_fmt(&st, "test_%d.txt", 123); // [手順] - com_util_stat_fmt にフォーマット文字列でファイル名を指定する。

    // Assert
    EXPECT_EQ(0, ret); // [確認_正常系] - com_util_stat_fmt から 0 が返されること。
}

TEST_F(statfTest, test_successful_call_with_multiple_parameters)
{
    // Arrange
    Mock_com_util mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(&st, StrEq("output_1_2_3.txt")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - com_util_stat が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    int ret = com_util_stat_fmt(&st, "output_%d_%d_%d.txt", 1, 2, 3); // [手順] - com_util_stat_fmt に複数のフォーマット パラメーターを指定する。

    // Assert
    EXPECT_EQ(0, ret); // [確認_正常系] - com_util_stat_fmt から 0 が返されること。
}

TEST_F(statfTest, test_stat_returns_error)
{
    // Arrange
    Mock_com_util mock_com_util;
    com_util_file_stat_t st;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_stat(&st, StrEq("nonexistent.txt")))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - com_util_stat が正しくフォーマットされたファイル名で呼ばれること。
                               // [Pre-Assert手順_異常系] - com_util_stat から -1 を返す。

    // Act
    int ret = com_util_stat_fmt(&st, "nonexistent.txt"); // [手順] - com_util_stat_fmt を呼び出す。

    // Assert
    EXPECT_EQ(-1, ret); // [確認_異常系] - com_util_stat_fmt から -1 が返されること。
}
