#include <testfw.h>
#include <mock_com_util.h>

#include <com_util/base/error.h>
#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/clock/timespec.h>
#include <com_util/crt/file.h>
#include <com_util/crt/stdio.h>

#if defined(PLATFORM_LINUX)
    #include <sys/mock_stat.h>
    #include <cerrno>
#elif defined(PLATFORM_WINDOWS)
    #include <mock_windows.h>
#endif /* PLATFORM_ */

#if defined(PLATFORM_LINUX)

using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::SetErrnoAndReturn;

namespace
{

const char kPath[] = "file_timestamp_failure.dat";

} // namespace

class fileTimestampFailureInjectionTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        FILE *stream = NULL;

        (void)com_util_remove(kPath, NULL);
        stream = com_util_fopen(kPath, "wb", NULL);
        ASSERT_NE(nullptr, stream);
        ASSERT_EQ(COM_UTIL_OK, com_util_fclose(stream, NULL));

        com_util_file_init(&file_);
        ASSERT_EQ(COM_UTIL_OK,
                  com_util_file_open(&file_, kPath, COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE, NULL));
    }

    void TearDown() override
    {
        (void)com_util_file_close(&file_, NULL);
        (void)com_util_remove(kPath, NULL);
    }

    NiceMock<Mock_sys_stat> mock_sys_stat_;
    com_util_file file_;
};

// fstat の失敗が com_util_file_get_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, get_reports_fstat_failure)
{
    // Arrange
    com_util_timespec actual = {0, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_sys_stat_, fstat(_, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EIO, -1)); // [Pre-Assert確認_異常系] - fstat が 1 回呼び出されること。
                                               // [Pre-Assert手順] - errno に EIO を設定して -1 を返却する。

    // Act
    int actual_ret_get =
        com_util_file_get_modified_timestamp(&file_, &actual, &detail); // [手順] - 最終更新日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_get); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// futimens の失敗が com_util_file_set_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, set_reports_futimens_failure)
{
    // Arrange
    const com_util_timespec timestamp = {1600000000, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_sys_stat_, futimens(_, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EIO, -1)); // [Pre-Assert確認_異常系] - futimens が 1 回呼び出されること。
                                               // [Pre-Assert手順] - errno に EIO を設定して -1 を返却する。

    // Act
    int actual_ret_set =
        com_util_file_set_modified_timestamp(&file_, &timestamp, &detail); // [手順] - 最終更新日時を設定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_set); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// utimensat の失敗が com_util_file_set_path_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, set_path_reports_utimensat_failure)
{
    // Arrange
    const com_util_timespec timestamp = {1600000000, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_sys_stat_, utimensat(_, _, _, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EIO, -1)); // [Pre-Assert確認_異常系] - utimensat が 1 回呼び出されること。
                                               // [Pre-Assert手順] - errno に EIO を設定して -1 を返却する。

    // Act
    int actual_ret_set =
        com_util_file_set_path_modified_timestamp(kPath, &timestamp, &detail); // [手順] - 最終更新日時を設定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_set); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// stat の失敗が com_util_file_get_path_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, get_path_reports_stat_failure)
{
    // Arrange
    com_util_timespec actual = {0, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_sys_stat_, stat(_, _, _, _, _))
        .WillOnce(SetErrnoAndReturn(EIO, -1)); // [Pre-Assert確認_異常系] - stat が 1 回呼び出されること。
                                               // [Pre-Assert手順] - errno に EIO を設定して -1 を返却する。

    // Act
    int actual_ret_get =
        com_util_file_get_path_modified_timestamp(kPath, &actual, &detail); // [手順] - 最終更新日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_get); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

#elif defined(PLATFORM_WINDOWS)

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace
{

const char kPath[] = "file_timestamp_failure.dat";

} // namespace

class fileTimestampFailureInjectionTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        FILE *stream = NULL;

        (void)com_util_remove(kPath, NULL);
        stream = com_util_fopen(kPath, "wb", NULL);
        ASSERT_NE(nullptr, stream);
        ASSERT_EQ(COM_UTIL_OK, com_util_fclose(stream, NULL));

        com_util_file_init(&file_);
        ASSERT_EQ(COM_UTIL_OK,
                  com_util_file_open(&file_, kPath, COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE, NULL));
    }

    void TearDown() override
    {
        (void)com_util_file_close(&file_, NULL);
        (void)com_util_remove(kPath, NULL);
    }

    NiceMock<Mock_windows> mock_windows_;
    com_util_file file_;
};

// GetFileTime の失敗が com_util_file_get_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, get_reports_GetFileTime_failure)
{
    // Arrange
    com_util_timespec actual = {0, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_windows_, GetFileTime(_, _, _, _, _, _, _))
        .WillOnce(Return(FALSE)); // [Pre-Assert確認_異常系] - GetFileTime が 1 回呼び出されること。
                                  // [Pre-Assert手順] - FALSE を返却する。
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _))
        .WillOnce(Return(ERROR_IO_DEVICE)); // [Pre-Assert確認_異常系] - GetLastError が 1 回呼び出されること。
                                            // [Pre-Assert手順] - ERROR_IO_DEVICE を返却する。

    // Act
    int actual_ret_get =
        com_util_file_get_modified_timestamp(&file_, &actual, &detail); // [手順] - 最終更新日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_get); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// SetFileTime の失敗が com_util_file_set_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, set_reports_SetFileTime_failure)
{
    // Arrange
    const com_util_timespec timestamp = {1600000000, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_windows_, SetFileTime(_, _, _, _, _, _, _))
        .WillOnce(Return(FALSE)); // [Pre-Assert確認_異常系] - SetFileTime が 1 回呼び出されること。
                                  // [Pre-Assert手順] - FALSE を返却する。
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _))
        .WillOnce(Return(ERROR_IO_DEVICE)); // [Pre-Assert確認_異常系] - GetLastError が 1 回呼び出されること。
                                            // [Pre-Assert手順] - ERROR_IO_DEVICE を返却する。

    // Act
    int actual_ret_set =
        com_util_file_set_modified_timestamp(&file_, &timestamp, &detail); // [手順] - 最終更新日時を設定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_set); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// パス版の GetFileTime の失敗が com_util_file_get_path_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, get_path_reports_GetFileTime_failure)
{
    // Arrange
    com_util_timespec actual = {0, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_windows_, GetFileTime(_, _, _, _, _, _, _))
        .WillOnce(Return(FALSE)); // [Pre-Assert確認_異常系] - GetFileTime が 1 回呼び出されること。
                                  // [Pre-Assert手順] - FALSE を返却する。
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _))
        .WillOnce(Return(ERROR_IO_DEVICE)); // [Pre-Assert確認_異常系] - GetLastError が 1 回呼び出されること。
                                            // [Pre-Assert手順] - ERROR_IO_DEVICE を返却する。

    // Act
    int actual_ret_get =
        com_util_file_get_path_modified_timestamp(kPath, &actual, &detail); // [手順] - 最終更新日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_get); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// パス版の SetFileTime の失敗が com_util_file_set_path_modified_timestamp から伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, set_path_reports_SetFileTime_failure)
{
    // Arrange
    const com_util_timespec timestamp = {1600000000, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_windows_, SetFileTime(_, _, _, _, _, _, _))
        .WillOnce(Return(FALSE)); // [Pre-Assert確認_異常系] - SetFileTime が 1 回呼び出されること。
                                  // [Pre-Assert手順] - FALSE を返却する。
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _))
        .WillOnce(Return(ERROR_IO_DEVICE)); // [Pre-Assert確認_異常系] - GetLastError が 1 回呼び出されること。
                                            // [Pre-Assert手順] - ERROR_IO_DEVICE を返却する。

    // Act
    int actual_ret_set =
        com_util_file_set_path_modified_timestamp(kPath, &timestamp, &detail); // [手順] - 最終更新日時を設定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_set); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// パス版の取得で CloseHandle の失敗が伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, get_path_reports_CloseHandle_failure)
{
    // Arrange
    com_util_timespec actual = {0, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_windows_, CloseHandle(_, _, _, _))
        .WillOnce(DoAll(Invoke(delegate_real_CloseHandle),
                        Return(FALSE))); // [Pre-Assert確認_異常系] - CloseHandle が 1 回呼び出されること。
                                         // [Pre-Assert手順] - 実ハンドルを閉じたうえで FALSE を返却する。
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _))
        .WillOnce(Return(ERROR_IO_DEVICE)); // [Pre-Assert確認_異常系] - GetLastError が 1 回呼び出されること。
                                            // [Pre-Assert手順] - ERROR_IO_DEVICE を返却する。

    // Act
    int actual_ret_get =
        com_util_file_get_path_modified_timestamp(kPath, &actual, &detail); // [手順] - 最終更新日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_get); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

// パス版の設定で CloseHandle の失敗が伝播することの確認
TEST_F(fileTimestampFailureInjectionTest, set_path_reports_CloseHandle_failure)
{
    // Arrange
    const com_util_timespec timestamp = {1600000000, 0};
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert
    EXPECT_CALL(mock_windows_, CloseHandle(_, _, _, _))
        .WillOnce(DoAll(Invoke(delegate_real_CloseHandle),
                        Return(FALSE))); // [Pre-Assert確認_異常系] - CloseHandle が 1 回呼び出されること。
                                         // [Pre-Assert手順] - 実ハンドルを閉じたうえで FALSE を返却する。
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _))
        .WillOnce(Return(ERROR_IO_DEVICE)); // [Pre-Assert確認_異常系] - GetLastError が 1 回呼び出されること。
                                            // [Pre-Assert手順] - ERROR_IO_DEVICE を返却する。

    // Act
    int actual_ret_set =
        com_util_file_set_path_modified_timestamp(kPath, &timestamp, &detail); // [手順] - 最終更新日時を設定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, actual_ret_set); // [確認_異常系] - 戻り値が COM_UTIL_ERR_UNKNOWN であること。
    // [確認_異常系] - 詳細エラーの要因が入出力エラーであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_IO_ERROR));
}

#endif /* PLATFORM_ */
