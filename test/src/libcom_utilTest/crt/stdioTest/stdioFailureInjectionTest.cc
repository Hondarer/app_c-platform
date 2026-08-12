#include <testfw.h>
#include <mock_stdio.h>

#include <com_util/base/platform.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>

#include <errno.h>
#include <filesystem>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

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
    FILE *null_path = com_util_fopen(NULL, "rb", &detail);    // [手順] - path に NULL を指定して fopen する。
    FILE *null_modes = com_util_fopen("file", NULL, &detail); // [手順] - modes に NULL を指定して fopen する。

    // Assert
    EXPECT_EQ(static_cast<FILE *>(NULL), null_path);  // [確認_異常系] - path NULL の fopen が NULL を返すこと。
    EXPECT_EQ(static_cast<FILE *>(NULL), null_modes); // [確認_異常系] - modes NULL の fopen が NULL を返すこと。
    EXPECT_EQ(COM_UTIL_CAUSE_INVALID_ARGUMENT,
              com_util_error_get_cause(&detail)); // [確認_異常系] - NULL 引数の要因が INVALID_ARGUMENT であること。
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
    EXPECT_EQ(
        COM_UTIL_CAUSE_INVALID_ARGUMENT,
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
    EXPECT_EQ(EOF, result);                               // [確認_異常系] - fclose の戻り値が EOF であること。
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
    EXPECT_EQ(EOF, result);                               // [確認_異常系] - fflush の戻り値が EOF であること。
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
    EXPECT_CALL(mock_stdio, fread(_, _, _, _, 1u, 1u, stream)).WillOnce(Return(1u)).WillOnce(Return(0u));
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, 0u, 1u, stream)).WillOnce(Return(0u));
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, 1u, 1u, stream)).WillOnce(Return(1u)).WillOnce(Return(0u));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fread(_, _, _, _, 1u, 0u, stream))
        .WillOnce(Return(0u)); // [Pre-Assert確認_正常系] - fread が要素数 0 で 1 回呼び出されること。
                               // [Pre-Assert手順] - fread から読み込み件数 0 を返却する。
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, _, 1u, 0u, stream))
        .WillOnce(Return(0u)); // [Pre-Assert確認_正常系] - fwrite が要素数 0 で 1 回呼び出されること。
                               // [Pre-Assert手順] - fwrite から書き込み件数 0 を返却する。

    // Act
    size_t read_null_buffer =
        com_util_fread(NULL, 1u, 1u, stream, &read_detail);                     // [手順] - NULL 読み込み先を指定する。
    size_t read_null_stream = com_util_fread(data, 1u, 1u, NULL, &read_detail); // [手順] - NULL ストリームを指定する。
    size_t read_zero_size =
        com_util_fread(NULL, 0u, 1u, stream, &read_detail); // [手順] - サイズ 0 の読み込みを指定する。
    size_t read_zero_count =
        com_util_fread(NULL, 1u, 0u, stream, &read_detail);                 // [手順] - 要素数 0 の読み込みを指定する。
    size_t read_full = com_util_fread(data, 1u, 1u, stream, &read_detail);  // [手順] - 全量読み込みを指定する。
    size_t read_short = com_util_fread(data, 1u, 1u, stream, &read_detail); // [手順] - 短い読み込みを指定する。
    size_t write_null_buffer =
        com_util_fwrite(NULL, 1u, 1u, stream, &write_detail); // [手順] - NULL 書き込み元を指定する。
    size_t write_null_stream =
        com_util_fwrite(data, 1u, 1u, NULL, &write_detail); // [手順] - NULL ストリームへ書き込む。
    size_t write_zero_size =
        com_util_fwrite(NULL, 0u, 1u, stream, &write_detail); // [手順] - サイズ 0 の書き込みを指定する。
    size_t write_zero_count =
        com_util_fwrite(NULL, 1u, 0u, stream, &write_detail); // [手順] - 要素数 0 の書き込みを指定する。
    size_t write_full = com_util_fwrite(data, 1u, 1u, stream, &write_detail);  // [手順] - 全量書き込みを指定する。
    size_t write_short = com_util_fwrite(data, 1u, 1u, stream, &write_detail); // [手順] - 短い書き込みを指定する。

    // Assert
    EXPECT_EQ(0u, read_null_buffer);  // [確認_異常系] - NULL 読み込み先が 0 件になること。
    EXPECT_EQ(0u, read_null_stream);  // [確認_異常系] - NULL ストリームが 0 件になること。
    EXPECT_EQ(0u, read_zero_size);    // [確認_正常系] - サイズ 0 の読み込みが 0 件になること。
    EXPECT_EQ(0u, read_zero_count);   // [確認_正常系] - 要素数 0 の com_util_fread の戻り値が 0 件であること。
    EXPECT_EQ(1u, read_full);         // [確認_正常系] - 全量読み込みが 1 件になること。
    EXPECT_EQ(0u, read_short);        // [確認_正常系] - エラー フラグのない短い読み込みが 0 件になること。
    EXPECT_EQ(0u, write_null_buffer); // [確認_異常系] - NULL 書き込み元が 0 件になること。
    EXPECT_EQ(0u, write_null_stream); // [確認_異常系] - NULL ストリームが 0 件になること。
    EXPECT_EQ(0u, write_zero_size);   // [確認_正常系] - サイズ 0 の書き込みが 0 件になること。
    EXPECT_EQ(0u, write_zero_count);  // [確認_正常系] - 要素数 0 の com_util_fwrite の戻り値が 0 件であること。
    EXPECT_EQ(1u, write_full);        // [確認_正常系] - 全量書き込みが 1 件になること。
    EXPECT_EQ(0u, write_short);       // [確認_異常系] - 短い書き込みが 0 件になること。

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
    size_t read_count =
        com_util_fread(data, 1U, 1U, fread_stream, &fread_detail); // [手順] - 書き込み専用ストリームを fread へ渡す。
    int fgets_result = com_util_fgets(data, sizeof(data), fgets_stream,
                                      &fgets_detail); // [手順] - 書き込み専用ストリームを fgets へ渡す。

    // Assert
    EXPECT_EQ(0U, read_count); // [確認_異常系] - 読み込みエラー時の fread 件数が 0 であること。
    EXPECT_NE(COM_UTIL_OK,
              com_util_error_to_result(&fread_detail)); // [確認_異常系] - fread の詳細エラーが成功以外であること。
    EXPECT_NE(COM_UTIL_OK, fgets_result); // [確認_異常系] - 読み込みエラー時の fgets 結果が成功以外であること。
    EXPECT_NE(COM_UTIL_OK,
              com_util_error_to_result(&fgets_detail)); // [確認_異常系] - fgets の詳細エラーが成功以外であること。

    // Cleanup
    std::fclose(fread_stream);
    std::fclose(fgets_stream);
}

// remove と rename の成功・失敗が結果コードへ反映されることの確認
TEST(stdioFailureInjectionTest, remove_and_rename_classify_file_operations)
{
    // Arrange
    std::string root = findWorkspaceRoot();
    std::filesystem::path dir =
        std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/stdioTest/results";
    std::filesystem::create_directories(dir);
    std::string old_path_str = (dir / "com_util_stdio_old.txt").generic_string();
    std::string new_path_str = (dir / "com_util_stdio_new.txt").generic_string();
    const char *old_path = old_path_str.c_str();
    const char *new_path = new_path_str.c_str();
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
    int rename_result =
        com_util_rename(old_path, new_path, &detail);        // [手順] - 既存ファイルを新しいパスへ rename する。
    int remove_result = com_util_remove(new_path, &detail);  // [手順] - rename 後のファイルを remove する。
    int missing_result = com_util_remove(new_path, &detail); // [手順] - 存在しないファイルを remove する。
    int null_rename_result = com_util_rename(NULL, new_path, &detail);  // [手順] - oldpath NULL の rename を実行する。
    int null_newpath_result = com_util_rename(old_path, NULL, &detail); // [手順] - newpath NULL の rename を実行する。
    int null_remove_result = com_util_remove(NULL, &detail);            // [手順] - path NULL の remove を実行する。

    // Assert
    EXPECT_EQ(0, rename_result);        // [確認_正常系] - rename の戻り値が 0 であること。
    EXPECT_EQ(0, remove_result);        // [確認_正常系] - remove の戻り値が 0 であること。
    EXPECT_NE(0, missing_result);       // [確認_異常系] - 存在しないファイルの remove が失敗すること。
    EXPECT_EQ(-1, null_rename_result);  // [確認_異常系] - oldpath NULL の rename が -1 を返すこと。
    EXPECT_EQ(-1, null_newpath_result); // [確認_異常系] - newpath NULL の com_util_rename の戻り値が -1 であること。
    EXPECT_EQ(-1, null_remove_result);  // [確認_異常系] - path NULL の remove が -1 を返すこと。
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
    int seek_result = com_util_fseek(stream, 0, SEEK_SET);    // [手順] - ファイル位置を先頭へ移動する。
    int64_t position = com_util_ftell(stream);                // [手順] - 現在のファイル位置を取得する。

    // Assert
    EXPECT_EQ(3, print_result); // [確認_正常系] - fprintf の出力文字数が 3 であること。
    EXPECT_EQ(0, seek_result);  // [確認_正常系] - fseek の戻り値が 0 であること。
    EXPECT_EQ(0, position);     // [確認_正常系] - 先頭へ移動後の ftell が 0 であること。

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

// 標準入出力関数が設定した errno を短い入出力の詳細へ記録することの確認
TEST(stdioFailureInjectionTest, short_io_preserves_nonzero_errno)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    char data[2] = {};
    com_util_error read_detail = {};
    com_util_error write_detail = {};
    com_util_error line_detail = {};
    FILE *read_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(4));
    FILE *write_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(5));
    FILE *line_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(6));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fread(_, _, _, data, 1u, 1u, read_stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, void *, size_t, size_t, FILE *)
            {
                errno = EACCES;
                return 0u;
            })); // [Pre-Assert確認_異常系] - fread が対象バッファーとストリームで 1 回呼び出されること。
                 // [Pre-Assert手順] - errno に EACCES を設定して fread から 0 を返却する。
    EXPECT_CALL(mock_stdio, ferror(_, _, _, read_stream))
        .WillOnce(Return(1)); // [Pre-Assert確認_異常系] - fread 後に ferror が 1 回呼び出されること。
                              // [Pre-Assert手順] - ferror からエラーありを返却する。
    EXPECT_CALL(mock_stdio, fwrite(_, _, _, data, 1u, 1u, write_stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, const void *, size_t, size_t, FILE *)
            {
                errno = ENOSPC;
                return 0u;
            })); // [Pre-Assert確認_異常系] - fwrite が対象バッファーとストリームで 1 回呼び出されること。
                 // [Pre-Assert手順] - errno に ENOSPC を設定して fwrite から 0 を返却する。
    EXPECT_CALL(mock_stdio, fgets(_, _, _, data, static_cast<int>(sizeof(data)), line_stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, char *, int, FILE *) -> char *
            {
                errno = EIO;
                return NULL;
            })); // [Pre-Assert確認_異常系] - fgets が対象バッファーとストリームで 1 回呼び出されること。
                 // [Pre-Assert手順] - errno に EIO を設定して fgets から NULL を返却する。
    EXPECT_CALL(mock_stdio, ferror(_, _, _, line_stream))
        .WillOnce(Return(1)); // [Pre-Assert確認_異常系] - fgets 後に ferror が 1 回呼び出されること。
                              // [Pre-Assert手順] - ferror からエラーありを返却する。

    // Act
    size_t read_result =
        com_util_fread(data, 1u, 1u, read_stream, &read_detail); // [手順] - EACCES で失敗する fread を実行する。
    size_t write_result =
        com_util_fwrite(data, 1u, 1u, write_stream, &write_detail); // [手順] - ENOSPC で失敗する fwrite を実行する。
    int line_result =
        com_util_fgets(data, sizeof(data), line_stream, &line_detail); // [手順] - EIO で失敗する fgets を実行する。

    // Assert
    EXPECT_EQ(0u, read_result); // [確認_異常系] - EACCES 発生時の com_util_fread の戻り値が 0 であること。
    EXPECT_EQ(EACCES, com_util_error_get_errno(
                          &read_detail)); // [確認_異常系] - com_util_fread の詳細エラーに EACCES が記録されること。
    EXPECT_EQ(0u, write_result);          // [確認_異常系] - ENOSPC 発生時の com_util_fwrite の戻り値が 0 であること。
    EXPECT_EQ(ENOSPC, com_util_error_get_errno(
                          &write_detail)); // [確認_異常系] - com_util_fwrite の詳細エラーに ENOSPC が記録されること。
    EXPECT_NE(COM_UTIL_OK, line_result);   // [確認_異常系] - EIO 発生時の com_util_fgets の戻り値が成功以外であること。
    EXPECT_EQ(EIO, com_util_error_get_errno(
                       &line_detail)); // [確認_異常系] - com_util_fgets の詳細エラーに EIO が記録されること。
}

// errno が設定されない読み込みエラーを EIO として報告することの確認
TEST(stdioFailureInjectionTest, read_errors_without_errno_default_to_eio)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    char read_data[2] = {};
    char line_data[2] = {};
    com_util_error read_detail = {};
    com_util_error line_detail = {};
    FILE *read_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(10));
    FILE *line_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(11));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fread(_, _, _, read_data, 1u, 1u, read_stream))
        .WillOnce(Return(0u)); // [Pre-Assert確認_異常系] - fread が対象バッファーとストリームで 1 回呼び出されること。
                               // [Pre-Assert手順] - errno を設定せず fread から 0 を返却する。
    EXPECT_CALL(mock_stdio, ferror(_, _, _, read_stream))
        .WillOnce(Return(1)); // [Pre-Assert確認_異常系] - fread 後に ferror が 1 回呼び出されること。
                              // [Pre-Assert手順] - ferror からエラーありを返却する。
    EXPECT_CALL(mock_stdio, fgets(_, _, _, line_data, static_cast<int>(sizeof(line_data)), line_stream))
        .WillOnce(Return(static_cast<char *>(
            NULL))); // [Pre-Assert確認_異常系] - fgets が対象バッファーとストリームで 1 回呼び出されること。
                     // [Pre-Assert手順] - errno を設定せず fgets から NULL を返却する。
    EXPECT_CALL(mock_stdio, ferror(_, _, _, line_stream))
        .WillOnce(Return(1)); // [Pre-Assert確認_異常系] - fgets 後に ferror が 1 回呼び出されること。
                              // [Pre-Assert手順] - ferror からエラーありを返却する。

    // Act
    size_t read_result = com_util_fread(read_data, 1u, 1u, read_stream,
                                        &read_detail); // [手順] - errno を設定せず失敗する fread を実行する。
    int line_result = com_util_fgets(line_data, sizeof(line_data), line_stream,
                                     &line_detail); // [手順] - errno を設定せず失敗する fgets を実行する。

    // Assert
    EXPECT_EQ(0u, read_result); // [確認_異常系] - errno がないエラー時の com_util_fread の戻り値が 0 であること。
    EXPECT_EQ(EIO, com_util_error_get_errno(
                       &read_detail)); // [確認_異常系] - errno がない fread エラーの詳細へ EIO が記録されること。
    EXPECT_NE(COM_UTIL_OK,
              line_result); // [確認_異常系] - errno がないエラー時の com_util_fgets の戻り値が成功以外であること。
    EXPECT_EQ(EIO, com_util_error_get_errno(
                       &line_detail)); // [確認_異常系] - errno がない fgets エラーの詳細へ EIO が記録されること。
}

// fgets が空文字列とバッファー末尾の改行を分類することの確認
TEST(stdioFailureInjectionTest, fgets_classifies_empty_text_and_exact_newline)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    char empty_text[2] = {};
    char newline_text[2] = {};
    FILE *empty_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(7));
    FILE *newline_stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(8));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fgets(_, _, _, empty_text, 2, empty_stream))
        .WillOnce(Return(empty_text)); // [Pre-Assert確認_正常系] - 空文字列用の fgets が 1 回呼び出されること。
                                       // [Pre-Assert手順] - fgets から空文字列のバッファーを返却する。
    EXPECT_CALL(mock_stdio, fgets(_, _, _, newline_text, 2, newline_stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, char *dest, int, FILE *) -> char *
            {
                dest[0] = '\n';
                dest[1] = '\0';
                return dest;
            })); // [Pre-Assert確認_正常系] - 改行用の fgets が 1 回呼び出されること。
                 // [Pre-Assert手順] - fgets から改行だけを格納したバッファーを返却する。

    // Act
    int empty_result = com_util_fgets(empty_text, sizeof(empty_text), empty_stream,
                                      NULL); // [手順] - 空文字列を返す fgets を実行する。
    int newline_result = com_util_fgets(newline_text, sizeof(newline_text), newline_stream,
                                        NULL); // [手順] - バッファー末尾が改行の fgets を実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              empty_result);      // [確認_正常系] - 空文字列を返した com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("", empty_text); // [確認_正常系] - 空文字列が維持されること。
    EXPECT_EQ(COM_UTIL_OK,
              newline_result);      // [確認_正常系] - 改行を返した com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("", newline_text); // [確認_正常系] - 末尾の改行が除去されること。
}

// バッファーを使い切る最終行が切り詰めとして扱われないことの確認
TEST(stdioFailureInjectionTest, fgets_accepts_exact_final_line)
{
    // Arrange
    NiceMock<Mock_stdio> mock_stdio;
    char data[2] = {};
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(12));

    // Pre-Assert
    EXPECT_CALL(mock_stdio, fgets(_, _, _, data, 2, stream))
        .WillOnce(Invoke(
            [](const char *, const int, const char *, char *dest, int, FILE *) -> char *
            {
                dest[0] = 'x';
                dest[1] = '\0';
                return dest;
            })); // [Pre-Assert確認_正常系] - fgets が 1 回呼び出されること。
                 // [Pre-Assert手順] - fgets からバッファーを使い切る 1 文字の最終行を返却する。
    EXPECT_CALL(mock_stdio, feof(_, _, _, stream))
        .WillOnce(Return(1)); // [Pre-Assert確認_正常系] - 行の終端確認で feof が 1 回呼び出されること。
                              // [Pre-Assert手順] - feof から EOF 到達済みを返却する。

    // Act
    int result = com_util_fgets(data, sizeof(data), stream, NULL); // [手順] - バッファーを使い切る最終行を読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);       // [確認_正常系] - 最終行を読み取った com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("x", data); // [確認_正常系] - 最終行の 1 文字が保持されること。
}

// fgets が 0 と INT_MAX 超のバッファー サイズを拒否することの確認
TEST(stdioFailureInjectionTest, fgets_rejects_invalid_buffer_sizes)
{
    // Arrange
    char data[2] = {};
    FILE *stream = reinterpret_cast<FILE *>(static_cast<uintptr_t>(9));

    // Pre-Assert

    // Act
    int zero_result =
        com_util_fgets(data, 0u, stream, NULL); // [手順] - dest_size に 0 を指定して com_util_fgets を呼び出す。
    int oversized_result =
        com_util_fgets(data, static_cast<size_t>(INT_MAX) + 1u, stream,
                       NULL); // [手順] - dest_size に INT_MAX を 1 超える値を指定して com_util_fgets を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        zero_result); // [確認_異常系] - dest_size が 0 の com_util_fgets の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        oversized_result); // [確認_異常系] - dest_size が INT_MAX を超える com_util_fgets の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
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
