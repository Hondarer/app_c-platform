#include <testfw.h>
#include <mock_stdio.h>

#include <com_util/base/platform.h>
#include <com_util/crt/path.h>
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
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_stdio, fopen(_, _, _, _, _))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, const char *, const char *)
            {
                errno = EACCES;
                return static_cast<FILE *>(NULL);
            })); // [Pre-Assert確認_異常系] - fopen が EACCES で失敗すること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_stdio, _wfsopen(_, _, _, _, _, _))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, const wchar_t *, const wchar_t *, int)
            {
                errno = EACCES;
                return static_cast<FILE *>(NULL);
            })); // [Pre-Assert確認_異常系] - _wfsopen が EACCES で失敗すること。
#endif /* PLATFORM_ */

    // Pre-Assert

    // Act
    FILE *stream =
        com_util_fopen("mocked-failure.txt", "rb", &detail); // [手順] - fopen の EACCES 失敗を注入してファイルを開く。

    // Assert
    EXPECT_EQ(static_cast<FILE *>(NULL), stream);         // [確認_異常系] - com_util_fopen の戻り値が NULL であること。
    EXPECT_EQ(EACCES, com_util_error_get_errno(&detail)); // [確認_異常系] - 詳細エラーへ EACCES が記録されること。
}

// fopen の NULL 引数が不正引数として分類されることの確認
TEST(stdioFailureInjectionTest, fopen_rejects_null_arguments)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert

    // Act
    FILE *null_path = com_util_fopen(NULL, "rb", &detail); // [手順] - path に NULL を指定して fopen する。
    FILE *null_modes = com_util_fopen("file", NULL, &detail); // [手順] - modes に NULL を指定して fopen する。

    // Assert
    EXPECT_EQ(static_cast<FILE *>(NULL), null_path); // [確認_異常系] - path NULL の fopen が NULL を返すこと。
    EXPECT_EQ(static_cast<FILE *>(NULL), null_modes); // [確認_異常系] - modes NULL の fopen が NULL を返すこと。
    EXPECT_EQ(COM_UTIL_CAUSE_INVALID_ARGUMENT, com_util_error_get_cause(&detail)); // [確認_異常系] - NULL 引数の要因が INVALID_ARGUMENT であること。
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

// fclose の NULL 引数が不正引数として分類されることの確認
TEST(stdioFailureInjectionTest, fclose_rejects_null_stream)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert

    // Act
    int result = com_util_fclose(NULL, &detail); // [手順] - stream に NULL を指定して fclose する。

    // Assert
    EXPECT_EQ(EOF, result); // [確認_異常系] - NULL stream の fclose が EOF を返すこと。
    EXPECT_EQ(COM_UTIL_CAUSE_INVALID_ARGUMENT,
              com_util_error_get_cause(&detail)); // [確認_異常系] - NULL stream が INVALID_ARGUMENT として記録されること。
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

// fflush の成功時に詳細エラーがクリアされることの確認
TEST(stdioFailureInjectionTest, fflush_reports_success_for_valid_stream)
{
    // Arrange
    FILE *stream = NULL;
#if defined(PLATFORM_LINUX)
    stream = std::tmpfile();
#elif defined(PLATFORM_WINDOWS)
    (void)tmpfile_s(&stream);
#endif /* PLATFORM_ */
    com_util_error detail = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_NOT_FOUND, ENOENT};
    ASSERT_NE(static_cast<FILE *>(NULL), stream);

    // Pre-Assert

    // Act
    int result = com_util_fflush(stream, &detail); // [手順] - 有効な一時ファイルへ fflush を実行する。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - com_util_fflush の戻り値が 0 であること。
    EXPECT_EQ(COM_UTIL_OK,
              com_util_error_to_result(&detail)); // [確認_正常系] - 成功時に詳細エラーがクリアされること。

    // Cleanup
    std::fclose(stream);
}

#if defined(PLATFORM_LINUX)
// freopen の OS 失敗が詳細エラーへ記録されることの確認
TEST(stdioFailureInjectionTest, freopen_reports_os_failure)
{
    // Arrange
    FILE *stream = std::tmpfile();
    com_util_error detail = {};
    ASSERT_NE(static_cast<FILE *>(NULL), stream);

    // Pre-Assert

    // Act
    FILE *reopened = com_util_freopen("/com_util/path/does/not/exist", "rb", stream,
                                      &detail); // [手順] - 存在しないパスへ freopen して OS 失敗を発生させる。

    // Assert
    EXPECT_EQ(static_cast<FILE *>(NULL), reopened); // [確認_異常系] - com_util_freopen の戻り値が NULL であること。
    EXPECT_NE(COM_UTIL_OK,
              com_util_error_to_result(&detail)); // [確認_異常系] - OS 失敗が詳細エラーへ記録されること。
}
#endif /* PLATFORM_LINUX */

// fread と fwrite が引数、全量、短い入出力を分類することの確認
TEST(stdioFailureInjectionTest, fread_and_fwrite_classify_arguments_and_counts)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    FILE *stream = nullptr;
#if defined(PLATFORM_LINUX)
    stream = tmpfile();
#elif defined(PLATFORM_WINDOWS)
    (void)tmpfile_s(&stream);
#endif /* PLATFORM_ */
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

// 実ストリームの読み込みエラーが詳細エラーへ記録されることの確認
TEST(stdioFailureInjectionTest, fread_and_fgets_report_stream_errors)
{
    // Arrange
    char data[2] = {};
    com_util_error fread_detail = {};
    com_util_error fgets_detail = {};
    FILE *fread_stream = NULL;
    FILE *fgets_stream = NULL;
#if defined(PLATFORM_LINUX)
    fread_stream = std::fopen(PLATFORM_NULL_DEVICE_PATH, "wb");
    fgets_stream = std::fopen(PLATFORM_NULL_DEVICE_PATH, "wb");
#elif defined(PLATFORM_WINDOWS)
    (void)fopen_s(&fread_stream, PLATFORM_NULL_DEVICE_PATH, "wb");
    (void)fopen_s(&fgets_stream, PLATFORM_NULL_DEVICE_PATH, "wb");
#endif /* PLATFORM_ */
    ASSERT_NE(static_cast<FILE *>(NULL), fread_stream);
    ASSERT_NE(static_cast<FILE *>(NULL), fgets_stream);

    // Pre-Assert

    // Act
    size_t read_count = com_util_fread(data, 1U, 1U, fread_stream, &fread_detail); // [手順] - 書き込み専用ストリームを fread へ渡す。
    int fgets_result = com_util_fgets(data, sizeof(data), fgets_stream, &fgets_detail); // [手順] - 書き込み専用ストリームを fgets へ渡す。

    // Assert
    EXPECT_EQ(0U, read_count); // [確認_異常系] - 読み込みエラー時の fread 件数が 0 であること。
    EXPECT_NE(COM_UTIL_OK, com_util_error_to_result(&fread_detail)); // [確認_異常系] - fread の詳細エラーが成功以外であること。
    EXPECT_NE(COM_UTIL_OK, fgets_result); // [確認_異常系] - 読み込みエラー時の fgets 結果が成功以外であること。
    EXPECT_NE(COM_UTIL_OK, com_util_error_to_result(&fgets_detail)); // [確認_異常系] - fgets の詳細エラーが成功以外であること。

    // Cleanup
    std::fclose(fread_stream);
    std::fclose(fgets_stream);
}

// remove と rename の成功・失敗が結果コードへ反映されることの確認
TEST(stdioFailureInjectionTest, remove_and_rename_classify_file_operations)
{
    // Arrange
    const char *old_path = "/tmp/com_util_stdio_old.txt";
    const char *new_path = "/tmp/com_util_stdio_new.txt";
    com_util_error detail = {};
    FILE *stream = NULL;
#if defined(PLATFORM_LINUX)
    stream = std::fopen(old_path, "wb");
#elif defined(PLATFORM_WINDOWS)
    (void)fopen_s(&stream, old_path, "wb");
#endif /* PLATFORM_ */
    ASSERT_NE(static_cast<FILE *>(NULL), stream);
    std::fclose(stream);
    (void)std::remove(new_path);

    // Pre-Assert

    // Act
    int rename_result = com_util_rename(old_path, new_path, &detail); // [手順] - 既存ファイルを新しいパスへ rename する。
    int remove_result = com_util_remove(new_path, &detail); // [手順] - rename 後のファイルを remove する。
    int missing_result = com_util_remove(new_path, &detail); // [手順] - 存在しないファイルを remove する。
    int null_rename_result = com_util_rename(NULL, new_path, &detail); // [手順] - oldpath NULL の rename を実行する。
    int null_remove_result = com_util_remove(NULL, &detail); // [手順] - path NULL の remove を実行する。

    // Assert
    EXPECT_EQ(0, rename_result); // [確認_正常系] - rename の戻り値が 0 であること。
    EXPECT_EQ(0, remove_result); // [確認_正常系] - remove の戻り値が 0 であること。
    EXPECT_NE(0, missing_result); // [確認_異常系] - 存在しないファイルの remove が失敗すること。
    EXPECT_EQ(-1, null_rename_result); // [確認_異常系] - oldpath NULL の rename が -1 を返すこと。
    EXPECT_EQ(-1, null_remove_result); // [確認_異常系] - path NULL の remove が -1 を返すこと。
}

// rename の OS 失敗が詳細エラーへ記録されることの確認
TEST(stdioFailureInjectionTest, rename_reports_os_failure)
{
    // Arrange
    com_util_error detail = {};

    // Pre-Assert

    // Act
    int result = com_util_rename("/com_util/path/does/not/exist", "/tmp/com_util_stdio_target.txt",
                                &detail); // [手順] - 存在しないパスを rename して OS 失敗を発生させる。

    // Assert
    EXPECT_NE(0, result); // [確認_異常系] - com_util_rename の戻り値が 0 以外であること。
    EXPECT_NE(COM_UTIL_OK,
              com_util_error_to_result(&detail)); // [確認_異常系] - OS 失敗が詳細エラーへ記録されること。
}

// fprintf、fseek、ftell の Linux ラッパーが標準 I/O を通過させることの確認
TEST(stdioFailureInjectionTest, formatted_output_and_file_position_wrappers_work)
{
    // Arrange
    FILE *stream = NULL;
#if defined(PLATFORM_LINUX)
    stream = std::tmpfile();
#elif defined(PLATFORM_WINDOWS)
    (void)tmpfile_s(&stream);
#endif /* PLATFORM_ */
    ASSERT_NE(static_cast<FILE *>(NULL), stream);

    // Pre-Assert

    // Act
    int print_result = com_util_fprintf(stream, "%s", "abc"); // [手順] - 一時ファイルへ文字列を fprintf する。
    int seek_result = com_util_fseek(stream, 0, SEEK_SET); // [手順] - ファイル位置を先頭へ移動する。
    int64_t position = com_util_ftell(stream); // [手順] - 現在のファイル位置を取得する。

    // Assert
    EXPECT_EQ(3, print_result); // [確認_正常系] - fprintf の出力文字数が 3 であること。
    EXPECT_EQ(0, seek_result); // [確認_正常系] - fseek の戻り値が 0 であること。
    EXPECT_EQ(0, position); // [確認_正常系] - 先頭へ移動後の ftell が 0 であること。

    // Cleanup
    std::fclose(stream);
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
