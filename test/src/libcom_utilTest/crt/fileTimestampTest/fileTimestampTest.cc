#include <testfw.h>
#include <mock_com_util.h>

#include <com_util/base/error.h>
#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/clock/timespec.h>
#include <com_util/crt/file.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/sys/stat.h>

namespace
{

const char kPath[] = "file_timestamp.dat";
const char kMissingPath[] = "file_timestamp_missing.dat";

/*
 *  検証に使う固定の時刻です。
 *  ナノ秒部は 100 の倍数にしています。Windows の FILETIME の分解能が 100 ナノ秒であり、
 *  それ未満は切り捨てられるためです。
 */
com_util_timespec make_timestamp(time_t tv_sec, int64_t tv_nsec)
{
    com_util_timespec timestamp;

    timestamp.tv_sec = tv_sec;
    timestamp.tv_nsec = tv_nsec;
    return timestamp;
}

void create_file(const char *path)
{
    FILE *stream = com_util_fopen(path, "wb", NULL);

    ASSERT_NE(nullptr, stream);
    ASSERT_EQ(1U, com_util_fwrite("x", 1U, 1U, stream, NULL));
    ASSERT_EQ(COM_UTIL_OK, com_util_fclose(stream, NULL));
}

} // namespace

class fileTimestampTest : public Test
{
  protected:
    void SetUp() override
    {
        (void)com_util_remove(kPath, NULL);
        (void)com_util_remove(kMissingPath, NULL);
        create_file(kPath);
    }

    void TearDown() override
    {
        (void)com_util_remove(kPath, NULL);
    }

    /* 書き込みアクセスで対象ファイルを開きます。 */
    void open_writable(com_util_file *file)
    {
        com_util_file_init(file);
        ASSERT_EQ(COM_UTIL_OK,
                  com_util_file_open(file, kPath, COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE, NULL));
    }
};

// パス版で設定した最終更新日時が、パス版で取得し直すと一致することの確認
TEST_F(fileTimestampTest, path_set_then_path_get_round_trips)
{
    // Arrange
    const com_util_timespec expected = make_timestamp(1600000000, 123456700); // [状態] - サブ秒を含む時刻を用意する。
    com_util_timespec actual = make_timestamp(0, 0);

    // Pre-Assert

    // Act
    int actual_ret_set = com_util_file_set_path_modified_timestamp(kPath, &expected,
                                                                   NULL); // [手順] - パス版で日時を設定する。
    int actual_ret_get = com_util_file_get_path_modified_timestamp(kPath, &actual,
                                                                   NULL); // [手順] - パス版で日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_set);    // [確認_正常系] - 設定が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_get);    // [確認_正常系] - 取得が COM_UTIL_OK であること。
    EXPECT_EQ(expected.tv_sec, actual.tv_sec); // [確認_正常系] - 秒部が一致すること。
    // [確認_正常系] - ナノ秒部が一致すること。サブ秒を保持するファイル システムが前提。
    EXPECT_EQ(expected.tv_nsec, actual.tv_nsec);
}

// ハンドル版で設定した最終更新日時が、ハンドル版で取得し直すと一致することの確認
TEST_F(fileTimestampTest, handle_set_then_handle_get_round_trips)
{
    // Arrange
    const com_util_timespec expected = make_timestamp(1500000000, 987654300); // [状態] - サブ秒を含む時刻を用意する。
    com_util_timespec actual = make_timestamp(0, 0);
    com_util_file file;

    open_writable(&file); // [状態] - 書き込みアクセスでファイルを開く。

    // Pre-Assert

    // Act
    int actual_ret_set = com_util_file_set_modified_timestamp(&file, &expected,
                                                              NULL); // [手順] - ハンドル版で日時を設定する。
    int actual_ret_get = com_util_file_get_modified_timestamp(&file, &actual,
                                                              NULL); // [手順] - ハンドル版で日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_set);      // [確認_正常系] - 設定が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_get);      // [確認_正常系] - 取得が COM_UTIL_OK であること。
    EXPECT_EQ(expected.tv_sec, actual.tv_sec);   // [確認_正常系] - 秒部が一致すること。
    EXPECT_EQ(expected.tv_nsec, actual.tv_nsec); // [確認_正常系] - ナノ秒部が一致すること。

    EXPECT_EQ(COM_UTIL_OK, com_util_file_close(&file, NULL));
}

// パス版で設定した最終更新日時を、ハンドル版で取得しても一致することの確認
TEST_F(fileTimestampTest, path_set_is_visible_from_handle_get)
{
    // Arrange
    const com_util_timespec expected = make_timestamp(1400000000, 500000000); // [状態] - サブ秒を含む時刻を用意する。
    com_util_timespec actual = make_timestamp(0, 0);
    com_util_file file;

    ASSERT_EQ(COM_UTIL_OK, com_util_file_set_path_modified_timestamp(kPath, &expected, NULL));
    open_writable(&file); // [状態] - 設定後にファイルを開く。

    // Pre-Assert

    // Act
    int actual_ret_get = com_util_file_get_modified_timestamp(&file, &actual,
                                                              NULL); // [手順] - ハンドル版で日時を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_get);      // [確認_正常系] - 取得が COM_UTIL_OK であること。
    EXPECT_EQ(expected.tv_sec, actual.tv_sec);   // [確認_正常系] - 秒部がパス版の設定値と一致すること。
    EXPECT_EQ(expected.tv_nsec, actual.tv_nsec); // [確認_正常系] - ナノ秒部がパス版の設定値と一致すること。

    EXPECT_EQ(COM_UTIL_OK, com_util_file_close(&file, NULL));
}

// 取得した最終更新日時の秒部が com_util_stat の st_mtime と一致することの確認
TEST_F(fileTimestampTest, seconds_agree_with_com_util_stat)
{
    // Arrange
    const com_util_timespec expected = make_timestamp(1300000000, 250000000); // [状態] - サブ秒を含む時刻を用意する。
    com_util_file_stat_t file_stat;

    ASSERT_EQ(COM_UTIL_OK, com_util_file_set_path_modified_timestamp(kPath, &expected, NULL));

    // Pre-Assert

    // Act
    int actual_ret_stat = com_util_stat(&file_stat, NULL, kPath); // [手順] - com_util_stat でファイル情報を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_stat); // [確認_正常系] - com_util_stat が COM_UTIL_OK であること。
    // [確認_正常系] - st_mtime が設定した秒部と一致すること。
    EXPECT_EQ(expected.tv_sec, static_cast<time_t>(file_stat.st_mtime));
}

// 過去と未来のいずれの日時も往復することの確認
TEST_F(fileTimestampTest, past_and_future_timestamps_round_trip)
{
    // Arrange
    const com_util_timespec past = make_timestamp(1, 0);            // [状態] - Unix epoch の直後を用意する。
    const com_util_timespec future = make_timestamp(4000000000, 0); // [状態] - 2038 年より後を用意する。
    com_util_timespec actual_past = make_timestamp(0, 0);
    com_util_timespec actual_future = make_timestamp(0, 0);

    // Pre-Assert

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_file_set_path_modified_timestamp(kPath, &past, NULL));
    int actual_ret_get_past =
        com_util_file_get_path_modified_timestamp(kPath, &actual_past, NULL); // [手順] - 過去の日時を往復させる。
    ASSERT_EQ(COM_UTIL_OK, com_util_file_set_path_modified_timestamp(kPath, &future, NULL));
    int actual_ret_get_future =
        com_util_file_get_path_modified_timestamp(kPath, &actual_future, NULL); // [手順] - 未来の日時を往復させる。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_get_past);    // [確認_正常系] - 過去の取得が COM_UTIL_OK であること。
    EXPECT_EQ(past.tv_sec, actual_past.tv_sec);     // [確認_正常系] - 過去の秒部が一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_get_future);  // [確認_正常系] - 未来の取得が COM_UTIL_OK であること。
    EXPECT_EQ(future.tv_sec, actual_future.tv_sec); // [確認_正常系] - 未来の秒部が一致すること。
}

// 最終更新日時の設定が最終アクセス日時を変更しないことの確認
TEST_F(fileTimestampTest, set_does_not_change_access_time)
{
    // Arrange
    const com_util_timespec expected = make_timestamp(1200000000, 0); // [状態] - 設定する時刻を用意する。
    com_util_file_stat_t before;
    com_util_file_stat_t after;

    ASSERT_EQ(COM_UTIL_OK, com_util_stat(&before, NULL, kPath)); // [状態] - 設定前の最終アクセス日時を控える。

    // Pre-Assert

    // Act
    int actual_ret_set = com_util_file_set_path_modified_timestamp(kPath, &expected,
                                                                   NULL); // [手順] - 最終更新日時を設定する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_set); // [確認_正常系] - 設定が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_stat(&after, NULL, kPath));
    EXPECT_EQ(before.st_atime, after.st_atime); // [確認_正常系] - 最終アクセス日時が変化しないこと。
}

// 読み取り専用で開いたハンドルへの設定が権限エラーになることの確認
TEST_F(fileTimestampTest, set_on_read_only_handle_is_permission_denied)
{
    // Arrange
    const com_util_timespec timestamp = make_timestamp(1100000000, 0);
    com_util_error detail;
    com_util_file file;

    com_util_error_clear(&detail);
    com_util_file_init(&file);
    ASSERT_EQ(COM_UTIL_OK,
              com_util_file_open(&file, kPath, COM_UTIL_FILE_OPEN_READ, NULL)); // [状態] - 読み取り専用で開く。

    // Pre-Assert

    // Act
    int actual_ret_set = com_util_file_set_modified_timestamp(&file, &timestamp,
                                                              &detail); // [手順] - 最終更新日時の設定を試みる。

    // Assert
    // [確認_異常系] - 戻り値が COM_UTIL_ERR_PERMISSION_DENIED であること。
    EXPECT_EQ(COM_UTIL_ERR_PERMISSION_DENIED, actual_ret_set);
    // [確認_異常系] - 詳細エラーの要因がアクセス拒否であること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_ACCESS_DENIED));

    EXPECT_EQ(COM_UTIL_OK, com_util_file_close(&file, NULL));
}

// 存在しないパスに対する取得が対象なしの要因になることの確認
TEST_F(fileTimestampTest, get_on_missing_path_reports_not_found)
{
    // Arrange
    com_util_timespec actual = make_timestamp(0, 0);
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert

    // Act
    int actual_ret_get = com_util_file_get_path_modified_timestamp(kMissingPath, &actual,
                                                                   &detail); // [手順] - 存在しないパスを指定する。

    // Assert
    EXPECT_NE(COM_UTIL_OK, actual_ret_get); // [確認_異常系] - 戻り値が COM_UTIL_OK 以外であること。
    // [確認_異常系] - 詳細エラーの要因が対象なしであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_NOT_FOUND));
}

// 存在しないパスに対する設定が対象なしの要因になることの確認
TEST_F(fileTimestampTest, set_on_missing_path_reports_not_found)
{
    // Arrange
    const com_util_timespec timestamp = make_timestamp(1000000000, 0);
    com_util_error detail;

    com_util_error_clear(&detail);

    // Pre-Assert

    // Act
    int actual_ret_set = com_util_file_set_path_modified_timestamp(kMissingPath, &timestamp,
                                                                   &detail); // [手順] - 存在しないパスを指定する。

    // Assert
    EXPECT_NE(COM_UTIL_OK, actual_ret_set); // [確認_異常系] - 戻り値が COM_UTIL_OK 以外であること。
    // [確認_異常系] - 詳細エラーの要因が対象なしであること。
    EXPECT_EQ(1, com_util_error_is(&detail, COM_UTIL_CAUSE_NOT_FOUND));
}

// 引数に NULL を指定した場合に引数不正を返すことの確認
TEST_F(fileTimestampTest, null_arguments_are_rejected)
{
    // Arrange
    const com_util_timespec timestamp = make_timestamp(900000000, 0);
    com_util_timespec actual = make_timestamp(0, 0);
    com_util_file file;
    com_util_file open_file;

    com_util_file_init(&file); // [状態] - 無効なハンドルを用意する。
    open_writable(&open_file); // [状態] - 有効なハンドルを用意する。

    // Pre-Assert

    // Act
    int actual_ret_get_path = com_util_file_get_path_modified_timestamp(NULL, &actual, NULL);
    int actual_ret_get_path_out = com_util_file_get_path_modified_timestamp(kPath, NULL, NULL);
    int actual_ret_set_path = com_util_file_set_path_modified_timestamp(NULL, &timestamp, NULL);
    int actual_ret_set_path_value = com_util_file_set_path_modified_timestamp(kPath, NULL, NULL);
    int actual_ret_get_handle = com_util_file_get_modified_timestamp(NULL, &actual, NULL);
    int actual_ret_get_handle_closed = com_util_file_get_modified_timestamp(&file, &actual, NULL);
    int actual_ret_set_handle = com_util_file_set_modified_timestamp(NULL, &timestamp, NULL);
    int actual_ret_get_handle_out = com_util_file_get_modified_timestamp(&open_file, NULL, NULL);
    int actual_ret_set_handle_value = com_util_file_set_modified_timestamp(&open_file, NULL, NULL);
    // [手順] - 各関数へ NULL または無効なハンドルを指定する。
    // 有効なハンドルと NULL の出力引数を組み合わせ、短絡評価の両側を通す。

    // Assert
    // [確認_異常系] - いずれも COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_get_path);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_get_path_out);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_set_path);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_set_path_value);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_get_handle);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_get_handle_closed);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_set_handle);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_get_handle_out);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_set_handle_value);

    EXPECT_EQ(COM_UTIL_OK, com_util_file_close(&open_file, NULL));
}
