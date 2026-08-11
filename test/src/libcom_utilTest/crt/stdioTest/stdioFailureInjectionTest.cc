#include <testfw.h>
#include <mock_stdio.h>

#include <com_util/base/platform.h>
#include <com_util/crt/stdio.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

// fopen の OS エラーを詳細エラーへ記録することの確認
TEST(stdioFailureInjectionTest, fopen_reports_mocked_os_failure)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_error detail = {};
    EXPECT_CALL(mock_stdio, fopen(_, _, _, _, _))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, const char *, const char *)
            {
                errno = EACCES;
                return static_cast<FILE *>(NULL);
            })); // [Pre-Assert確認_異常系] - fopen が EACCES で失敗すること。

    // Pre-Assert

    // Act
    FILE *stream =
        com_util_fopen("mocked-failure.txt", "rb", &detail); // [手順] - fopen の EACCES 失敗を注入してファイルを開く。

    // Assert
    EXPECT_EQ(static_cast<FILE *>(NULL), stream);         // [確認_異常系] - com_util_fopen の戻り値が NULL であること。
    EXPECT_EQ(EACCES, com_util_error_get_errno(&detail)); // [確認_異常系] - 詳細エラーへ EACCES が記録されること。
}

// fclose の失敗時に EIO を補完して記録することの確認
TEST(stdioFailureInjectionTest, fclose_reports_eio_when_errno_is_empty)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_error detail = {};
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(1));
    EXPECT_CALL(mock_stdio, fclose(_, _, _, stream)).WillOnce(Return(EOF));

    // Pre-Assert

    // Act
    int result = com_util_fclose(stream, &detail); // [手順] - fclose が EOF を返す失敗を注入する。

    // Assert
    EXPECT_EQ(EOF, result);                            // [確認_異常系] - com_util_fclose の戻り値が EOF であること。
    EXPECT_EQ(EIO, com_util_error_get_errno(&detail)); // [確認_異常系] - errno が空の場合に EIO が記録されること。
}

// fflush の失敗時に EIO を補完して記録することの確認
TEST(stdioFailureInjectionTest, fflush_reports_eio_when_errno_is_empty)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_error detail = {};
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(2));
    EXPECT_CALL(mock_stdio, fflush(_, _, _, stream)).WillOnce(Return(EOF));

    // Pre-Assert

    // Act
    int result = com_util_fflush(stream, &detail); // [手順] - fflush が EOF を返す失敗を注入する。

    // Assert
    EXPECT_EQ(EOF, result);                            // [確認_異常系] - com_util_fflush の戻り値が EOF であること。
    EXPECT_EQ(EIO, com_util_error_get_errno(&detail)); // [確認_異常系] - errno が空の場合に EIO が記録されること。
}

// fclose が errno を保持している失敗を詳細エラーへ記録することの確認
TEST(stdioFailureInjectionTest, fclose_preserves_nonzero_errno)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_error detail = {};
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(4));
    EXPECT_CALL(mock_stdio, fclose(_, _, _, stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, FILE *)
            {
                errno = EACCES;
                return EOF;
            }));

    // Pre-Assert

    // Act
    int result = com_util_fclose(stream, &detail); // [手順] - errno が EACCES の fclose 失敗を注入する。

    // Assert
    EXPECT_EQ(EOF, result); // [確認_異常系] - fclose の戻り値が EOF であること。
    EXPECT_EQ(EACCES, com_util_error_get_errno(&detail)); // [確認_異常系] - EACCES が詳細エラーへ記録されること。
}

// fflush が errno を保持している失敗を詳細エラーへ記録することの確認
TEST(stdioFailureInjectionTest, fflush_preserves_nonzero_errno)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_error detail = {};
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(5));
    EXPECT_CALL(mock_stdio, fflush(_, _, _, stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, FILE *)
            {
                errno = EACCES;
                return EOF;
            }));

    // Pre-Assert

    // Act
    int result = com_util_fflush(stream, &detail); // [手順] - errno が EACCES の fflush 失敗を注入する。

    // Assert
    EXPECT_EQ(EOF, result); // [確認_異常系] - fflush の戻り値が EOF であること。
    EXPECT_EQ(EACCES, com_util_error_get_errno(&detail)); // [確認_異常系] - EACCES が詳細エラーへ記録されること。
}

// fread と fwrite が引数、全量、短い入出力を分類することの確認
TEST(stdioFailureInjectionTest, fread_and_fwrite_classify_arguments_and_counts)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    FILE *stream = tmpfile();
    char data[2] = {};
    com_util_error read_detail = {};
    com_util_error write_detail = {};
    ASSERT_NE(static_cast<FILE *>(NULL), stream);
    EXPECT_CALL(mock_stdio, fread(_, _, _, _, 0u, 1u, stream)).WillOnce(Return(0u));
    EXPECT_CALL(mock_stdio, fread(_, _, _, _, 1u, 1u, stream))
        .WillOnce(Return(1u))
        .WillOnce(Return(0u));
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, 0u, 1u, stream)).WillOnce(Return(0u));
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, 1u, 1u, stream))
        .WillOnce(Return(1u))
        .WillOnce(Return(0u));

    // Pre-Assert

    // Act
    size_t read_null_buffer = com_util_fread(NULL, 1u, 1u, stream, &read_detail); // [手順] - NULL 読み込み先を指定する。
    size_t read_null_stream = com_util_fread(data, 1u, 1u, NULL, &read_detail); // [手順] - NULL ストリームを指定する。
    size_t read_zero_size = com_util_fread(NULL, 0u, 1u, stream, &read_detail); // [手順] - サイズ 0 の読み込みを指定する。
    size_t read_full = com_util_fread(data, 1u, 1u, stream, &read_detail); // [手順] - 全量読み込みを指定する。
    size_t read_short = com_util_fread(data, 1u, 1u, stream, &read_detail); // [手順] - 短い読み込みを指定する。
    size_t write_null_buffer = com_util_fwrite(NULL, 1u, 1u, stream, &write_detail); // [手順] - NULL 書き込み元を指定する。
    size_t write_null_stream = com_util_fwrite(data, 1u, 1u, NULL, &write_detail); // [手順] - NULL ストリームへ書き込む。
    size_t write_zero_size = com_util_fwrite(NULL, 0u, 1u, stream, &write_detail); // [手順] - サイズ 0 の書き込みを指定する。
    size_t write_full = com_util_fwrite(data, 1u, 1u, stream, &write_detail); // [手順] - 全量書き込みを指定する。
    size_t write_short = com_util_fwrite(data, 1u, 1u, stream, &write_detail); // [手順] - 短い書き込みを指定する。

    // Assert
    EXPECT_EQ(0u, read_null_buffer); // [確認_異常系] - NULL 読み込み先が 0 件になること。
    EXPECT_EQ(0u, read_null_stream); // [確認_異常系] - NULL ストリームが 0 件になること。
    EXPECT_EQ(0u, read_zero_size); // [確認_正常系] - サイズ 0 の読み込みが 0 件になること。
    EXPECT_EQ(1u, read_full); // [確認_正常系] - 全量読み込みが 1 件になること。
    EXPECT_EQ(0u, read_short); // [確認_正常系] - エラー フラグのない短い読み込みが 0 件になること。
    EXPECT_EQ(0u, write_null_buffer); // [確認_異常系] - NULL 書き込み元が 0 件になること。
    EXPECT_EQ(0u, write_null_stream); // [確認_異常系] - NULL ストリームが 0 件になること。
    EXPECT_EQ(0u, write_zero_size); // [確認_正常系] - サイズ 0 の書き込みが 0 件になること。
    EXPECT_EQ(1u, write_full); // [確認_正常系] - 全量書き込みが 1 件になること。
    EXPECT_EQ(0u, write_short); // [確認_異常系] - 短い書き込みが 0 件になること。

    // Cleanup
    fclose(stream);
}

// fwrite の短い書き込みを未知エラーとして報告することの確認
TEST(stdioFailureInjectionTest, fwrite_reports_short_write)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    com_util_error detail = {};
    const char data[] = "xy";
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(3));
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, 1u, 2u, stream)).WillOnce(Return(1u));

    // Pre-Assert

    // Act
    size_t result =
        com_util_fwrite(data, 1u, 2u, stream, &detail); // [手順] - 2 要素中 1 要素だけ書き込む失敗を注入する。

    // Assert
    EXPECT_EQ(1u, result); // [確認_異常系] - com_util_fwrite の戻り値が 1 要素であること。
    EXPECT_EQ(EIO,
              com_util_error_get_errno(&detail)); // [確認_異常系] - errno が空の短い書き込みへ EIO が記録されること。
}

// vsnprintf の失敗時に出力を空文字列へ変更することの確認
TEST(stdioFailureInjectionTest, snprintf_reports_formatting_failure)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    char buffer[8] = "stale";
    EXPECT_CALL(mock_stdio, vsnprintf(_, _, _, _, _, _)).WillOnce(Return(-1));

    // Pre-Assert

    // Act
    int result =
        com_util_snprintf(buffer, sizeof(buffer), "%s", "value"); // [手順] - vsnprintf が -1 を返す失敗を注入する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result);        // [確認_異常系] - com_util_snprintf の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_STREQ("", buffer); // [確認_異常系] - フォーマット失敗時に出力バッファーが空になること。
}
