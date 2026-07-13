#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/crt/stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

class fopenfTest : public Test
{
};

// modes が NULL の場合に com_util_fopen を呼び出さず NULL を返すことの確認
TEST_F(fopenfTest, test_null_modes)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt(NULL, NULL, "test_%d.txt",
                                  1); // [手順] - modes に NULL を渡して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}

// format が NULL の場合に com_util_fopen を呼び出さず NULL を返すことの確認
TEST_F(fopenfTest, test_null_format)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt("r", NULL, NULL); // [手順] - format に NULL を渡して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}

// フォーマット結果がバッファー サイズを超える場合に com_util_fopen を呼び出さず NULL を返すことの確認
TEST_F(fopenfTest, test_buffer_overflow)
{
    // Arrange
    Mock_com_util mock_com_util;
    char long_string[5000];
    memset(long_string, 'a', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0'; // [状態] - バッファー サイズを超える 4999 文字のファイル名を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - com_util_fopen が呼び出されないこと。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "w", NULL, "%s.txt",
        long_string); // [手順] - バッファー サイズを超えるファイル名を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}

// フォーマット文字列を展開したファイル名で com_util_fopen が呼び出されることの確認
TEST_F(fopenfTest, test_successful_call_with_format)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp =
        (FILE *)(uintptr_t)0x12345678; // [状態] - com_util_fopen が返すファイル ポインターの期待値を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("test_123.txt"), StrEq("r"), _))
        .WillOnce(Return(
            expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が展開後のファイル名 "test_123.txt" とモード "r" で 1 回呼び出されること。
    // [Pre-Assert手順] - com_util_fopen から expected_fp を返却する。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "r", NULL, "test_%d.txt",
        123); // [手順] - フォーマット文字列 "test_%d.txt" と引数 123 を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から expected_fp が返されること。
}

// 複数のフォーマット パラメーターを展開したファイル名で com_util_fopen が呼び出されることの確認
TEST_F(fopenfTest, test_successful_call_with_multiple_parameters)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp =
        (FILE *)(uintptr_t)0x87654321; // [状態] - com_util_fopen が返すファイル ポインターの期待値を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("output_1_2_3.txt"), StrEq("w"), _))
        .WillOnce(Return(
            expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen が展開後のファイル名 "output_1_2_3.txt" とモード "w" で 1 回呼び出されること。
                           // [Pre-Assert手順] - com_util_fopen から expected_fp を返却する。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "w", NULL, "output_%d_%d_%d.txt", 1, 2,
        3); // [手順] - フォーマット文字列 "output_%d_%d_%d.txt" と引数 1, 2, 3 を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から expected_fp が返されること。
}

// com_util_fopen が NULL を返した場合に NULL を返すことの確認
TEST_F(fopenfTest, test_fopen_returns_null)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("nonexistent.txt"), StrEq("r"), _))
        .WillOnce(Return(
            (FILE *)
                NULL)); // [Pre-Assert確認_異常系] - com_util_fopen がファイル名 "nonexistent.txt" とモード "r" で 1 回呼び出されること。
                        // [Pre-Assert手順] - com_util_fopen から NULL を返却する。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "r", NULL,
        "nonexistent.txt"); // [手順] - 存在しないファイル名 "nonexistent.txt" を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}

#if defined(PLATFORM_WINDOWS)
// 書き込み保護されたファイルで com_util_fopen が NULL を返した場合に NULL を返すことの確認
TEST_F(fopenfTest, test_fopen_s_access_denied)
{
    // Arrange
    Mock_com_util mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("protected.txt"), StrEq("w"), _))
        .WillOnce(Return(
            (FILE *)
                NULL)); // [Pre-Assert確認_異常系] - com_util_fopen がファイル名 "protected.txt" とモード "w" で 1 回呼び出されること。
                        // [Pre-Assert手順] - com_util_fopen から NULL を返却する。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "w", NULL,
        "protected.txt"); // [手順] - 書き込み保護されたファイル名 "protected.txt" を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp); // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
}
#endif /* PLATFORM_WINDOWS */

// com_util_fopen が失敗した場合にエラー コードが取得できることの確認
TEST_F(fopenfTest, test_fopen_returns_null_with_errno)
{
    // Arrange
    Mock_com_util mock_com_util;
    int error_code = 0; // [状態] - エラー コードの受け取り先を 0 で初期化する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("nonexistent.txt"), StrEq("r"), _))
        .WillOnce(DoAll(
            SetArgPointee<2>(ENOENT),
            Return(
                (FILE *)
                    NULL))); // [Pre-Assert確認_異常系] - com_util_fopen がファイル名 "nonexistent.txt" とモード "r" で 1 回呼び出されること。
                             // [Pre-Assert手順] - com_util_fopen から NULL を返却し、errno_out に ENOENT を設定する。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "r", &error_code,
        "nonexistent.txt"); // [手順] - error_code の受け取り先を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)NULL, fp);   // [確認_異常系] - com_util_fopen_fmt から NULL が返されること。
    EXPECT_EQ(ENOENT, error_code); // [確認_異常系] - error_code に ENOENT が設定されること。
}

// com_util_fopen が成功した場合にエラー コードが変更されないことの確認
TEST_F(fopenfTest, test_fopen_success_errno_not_set)
{
    // Arrange
    Mock_com_util mock_com_util;
    FILE *expected_fp =
        (FILE *)(uintptr_t)0x12345678; // [状態] - com_util_fopen が返すファイル ポインターの期待値を用意する。
    int error_code = 999;              // [状態] - エラー コードの受け取り先を初期値 999 とする。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("success.txt"), StrEq("r"), _))
        .WillOnce(Return(
            expected_fp)); // [Pre-Assert確認_正常系] - com_util_fopen がファイル名 "success.txt" とモード "r" で 1 回呼び出されること。
                           // [Pre-Assert手順] - com_util_fopen から expected_fp を返却する。

    // Act
    FILE *fp = com_util_fopen_fmt(
        "r", &error_code, "success.txt"); // [手順] - error_code の受け取り先を指定して com_util_fopen_fmt を呼び出す。

    // Assert
    EXPECT_EQ(expected_fp, fp); // [確認_正常系] - com_util_fopen_fmt から expected_fp が返されること。
    EXPECT_EQ(999, error_code); // [確認_正常系] - 成功時は error_code が初期値 999 から変更されないこと。
}
