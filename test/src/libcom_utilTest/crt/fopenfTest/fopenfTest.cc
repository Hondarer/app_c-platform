#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/crt/stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

class fopenfTest : public Test
{
};

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_null_modes)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt(NULL, NULL, "test_%d.txt", 1); // [手順] - modes に NULL を渡す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_null_modes)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt(NULL, NULL, "test_%d.txt", 1); // [手順] - modes に NULL を渡す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_null_format)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, NULL); // [手順] - format に NULL を渡す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_null_format)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, NULL); // [手順] - format に NULL を渡す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_buffer_overflow)
{
    // Arrange
    Mock_com_util mock_com_util;
    // 非常に長いファイル名を生成 (バッファー サイズを超える)
    char long_string[5000];
    memset(long_string, 'a', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0';

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt("w", NULL, "%s.txt", long_string); // [手順] - バッファー サイズを超えるファイル名を指定する。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_buffer_overflow)
{
    // Arrange
    Mock_com_util mock_com_util;
    // 非常に長いファイル名を生成 (バッファー サイズを超える)
    char long_string[5000];
    memset(long_string, 'a', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0';

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt("w", NULL, "%s.txt", long_string); // [手順] - バッファー サイズを超えるファイル名を指定する。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_successful_call_with_format)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp = (FILE *)(uintptr_t)0x12345678;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("test_123.txt"), StrEq("r"), _))
        .WillOnce(Return(expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, "test_%d.txt", 123); // [手順] - com_util_fopen_fmt にフォーマット文字列でファイル名を指定する。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から fp が返されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_successful_call_with_format)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp = (FILE *)(uintptr_t)0x12345678;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("test_123.txt"), StrEq("r"), _))
        .WillOnce(Return(expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, "test_%d.txt", 123); // [手順] - com_util_fopen_fmt にフォーマット文字列でファイル名を指定する。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から fp が返されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_successful_call_with_multiple_parameters)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp = (FILE *)(uintptr_t)0x87654321;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("output_1_2_3.txt"), StrEq("w"), _))
        .WillOnce(Return(expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    FILE *fp = com_util_fopen_fmt("w", NULL, "output_%d_%d_%d.txt", 1, 2, 3); // [手順] - com_util_fopen_fmt に複数のフォーマット パラメーターを指定する。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から fp が返されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_successful_call_with_multiple_parameters)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp = (FILE *)(uintptr_t)0x87654321;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("output_1_2_3.txt"), StrEq("w"), _))
        .WillOnce(Return(expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    FILE *fp = com_util_fopen_fmt("w", NULL, "output_%d_%d_%d.txt", 1, 2, 3); // [手順] - com_util_fopen_fmt に複数のフォーマット パラメーターを指定する。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から fp が返されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_fopen_returns_null)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("nonexistent.txt"), StrEq("r"), _))
        .WillOnce(Return((FILE *)NULL)); // [Pre-Assert確認_異常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。
                                         // [Pre-Assert手順_異常系] - com_util_fopen から NULL を返す。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, "nonexistent.txt"); // [手順] - com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_fopen_returns_null)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("nonexistent.txt"), StrEq("r"), _))
        .WillOnce(Return((FILE *)NULL)); // [Pre-Assert確認_異常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。
                                         // [Pre-Assert手順_異常系] - com_util_fopen から NULL を返す。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, "nonexistent.txt"); // [手順] - com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}

TEST_F(fopenfTest, test_fopen_s_access_denied)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("protected.txt"), StrEq("w"), _))
        .WillOnce(Return((FILE *)NULL)); // [Pre-Assert確認_異常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。
                                         // [Pre-Assert手順_異常系] - com_util_fopen から NULL を返す。

    // Act
    FILE *fp = com_util_fopen_fmt("w", NULL, "protected.txt"); // [手順] - com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_fopen_returns_null_with_errno)
{
    // Arrange
    Mock_com_util mock_com_util;
    int error_code = 0;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("nonexistent.txt"), StrEq("r"), _))
        .WillOnce(DoAll(SetArgPointee<2>(ENOENT), Return((FILE *)NULL))); // [Pre-Assert確認_異常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。
                                                                          // [Pre-Assert手順_異常系] - com_util_fopen から NULL を返し、errno_out に ENOENT を設定する。

    // Act
    FILE *fp = com_util_fopen_fmt("r", &error_code, "nonexistent.txt"); // [手順] - com_util_fopen_fmt を呼び出し、エラー コードを取得する。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp);   // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
    EXPECT_EQ(ENOENT, error_code); // [確認_異常系] - errno に ENOENT が設定されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_fopen_s_returns_error_with_errno)
{
    // Arrange
    Mock_com_util mock_com_util;
    int error_code = 0;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("nonexistent.txt"), StrEq("r"), _))
        .WillOnce(DoAll(SetArgPointee<2>(ENOENT), Return((FILE *)NULL))); // [Pre-Assert確認_異常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。
                                                                          // [Pre-Assert手順_異常系] - com_util_fopen から NULL を返し、errno_out に ENOENT を設定する。

    // Act
    FILE *fp = com_util_fopen_fmt("r", &error_code, "nonexistent.txt"); // [手順] - com_util_fopen_fmt を呼び出し、エラー コードを取得する。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp);   // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
    EXPECT_EQ(ENOENT, error_code); // [確認_異常系] - error_code に ENOENT が設定されること。
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)
TEST_F(fopenfTest, test_fopen_success_errno_not_set)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp = (FILE *)(uintptr_t)0x12345678;
    int error_code = 999; // 初期値を設定

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("success.txt"), StrEq("r"), _))
        .WillOnce(Return(expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    FILE *fp = com_util_fopen_fmt("r", &error_code, "success.txt"); // [手順] - com_util_fopen_fmt を呼び出し、エラー コード ポインターを渡す。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から fp が返されること。
    EXPECT_EQ(999, error_code); // [確認_正常系] - 成功時は error_code が変更されないこと。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(fopenfTest, test_fopen_s_success_errno_not_set)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp = (FILE *)(uintptr_t)0x12345678;
    int error_code = 999; // 初期値を設定

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("success.txt"), StrEq("r"), _))
        .WillOnce(Return(expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が正しくフォーマットされたファイル名で呼ばれること。

    // Act
    FILE *fp = com_util_fopen_fmt("r", &error_code, "success.txt"); // [手順] - com_util_fopen_fmt を呼び出し、エラー コード ポインターを渡す。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から fp が返されること。
    EXPECT_EQ(999, error_code); // [確認_正常系] - 成功時は error_code が変更されないこと。
}
#endif /* PLATFORM_ */
