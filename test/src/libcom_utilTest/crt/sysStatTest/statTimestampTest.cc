#include <testfw.h>
#include <mock_com_util.h>

#include <com_util/base/result.h>
#include <com_util/clock/timespec.h>
#include <com_util/crt/file.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/sys/stat.h>

namespace
{

const char kPath[] = "stat_timestamp.dat";

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

class statTimestampTest : public Test
{
  protected:
    void SetUp() override
    {
        (void)com_util_remove(kPath, NULL);
        create_file(kPath);
    }

    void TearDown() override
    {
        (void)com_util_remove(kPath, NULL);
    }
};

/*
 *  _wstat64 が成功したあと GetFileAttributesExW が失敗する経路は、対象パスが
 *  二つの呼び出しの間に消える競合など一時的な状態であり、安定して注入できない。
 *  失敗時は _wstat64 の時刻欄を残して COM_UTIL_OK を返す。
 */

// 設定した最終更新日時の秒部が com_util_stat の st_mtime と一致することの確認
TEST_F(statTimestampTest, seconds_agree_with_set_modified_timestamp)
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
