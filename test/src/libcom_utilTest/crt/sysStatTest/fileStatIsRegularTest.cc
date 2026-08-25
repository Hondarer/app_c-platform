#include <testfw.h>
#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/sys/stat.h>

namespace
{

const char kFilePath[] = "is_regular.dat";
const char kDirPath[] = "is_regular_dir";

} // namespace

class fileStatIsRegularTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        FILE *stream = NULL;

        (void)com_util_remove(kFilePath, NULL);
        (void)com_util_rmdir(kDirPath, NULL);

        stream = com_util_fopen(kFilePath, "wb", NULL);
        ASSERT_NE(nullptr, stream);
        ASSERT_EQ(COM_UTIL_OK, com_util_fclose(stream, NULL));
        ASSERT_EQ(COM_UTIL_OK, com_util_mkdir(kDirPath, NULL));
    }

    void TearDown() override
    {
        (void)com_util_remove(kFilePath, NULL);
        (void)com_util_rmdir(kDirPath, NULL);
    }
};

// com_util_file_stat_is_regular が通常ファイルに対して 1 を返すことの確認
TEST_F(fileStatIsRegularTest, returns_one_for_regular_file)
{
    // Arrange
    com_util_file_stat_t file_stat;

    ASSERT_EQ(COM_UTIL_OK, com_util_stat(&file_stat, NULL, kFilePath)); // [状態] - 通常ファイルの情報を取得する。

    // Pre-Assert

    // Act
    int actual_ret_is_regular = com_util_file_stat_is_regular(&file_stat); // [手順] - 種別を判定する。

    // Assert
    EXPECT_EQ(1, actual_ret_is_regular); // [確認_正常系] - 戻り値が 1 であること。
}

// com_util_file_stat_is_regular がディレクトリに対して 0 を返すことの確認
TEST_F(fileStatIsRegularTest, returns_zero_for_directory)
{
    // Arrange
    com_util_file_stat_t file_stat;

    ASSERT_EQ(COM_UTIL_OK, com_util_stat(&file_stat, NULL, kDirPath)); // [状態] - ディレクトリの情報を取得する。

    // Pre-Assert

    // Act
    int actual_ret_is_regular = com_util_file_stat_is_regular(&file_stat); // [手順] - 種別を判定する。

    // Assert
    EXPECT_EQ(0, actual_ret_is_regular); // [確認_異常系] - 戻り値が 0 であること。
}

// com_util_file_stat_is_regular が NULL に対して 0 を返すことの確認
TEST_F(fileStatIsRegularTest, returns_zero_for_null)
{
    // Arrange

    // Pre-Assert

    // Act
    int actual_ret_is_regular = com_util_file_stat_is_regular(NULL); // [手順] - NULL を指定する。

    // Assert
    EXPECT_EQ(0, actual_ret_is_regular); // [確認_異常系] - 戻り値が 0 であること。
}
