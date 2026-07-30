#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/sys/stat.h>

#include <filesystem>
#include <string>

class rmdirTest : public Test
{
  protected:
    std::string make_path(const char *name)
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/rmdirTest/results";

        std::filesystem::create_directories(dir);
        return (dir / name).generic_string();
    }
};

// com_util_rmdir が空のディレクトリを削除することの確認
TEST_F(rmdirTest, removes_empty_directory)
{
    // Arrange
    std::string path = make_path("empty_dir");

    std::filesystem::remove_all(path);
    ASSERT_EQ(COM_UTIL_OK, com_util_mkdir(path.c_str())); // [状態] - 空のディレクトリを作成しておく。

    // Pre-Assert
    ASSERT_TRUE(std::filesystem::exists(path));

    // Act
    int rtc_rmdir = com_util_rmdir(path.c_str()); // [手順] - 空のディレクトリを削除する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_rmdir); // [確認_正常系] - com_util_rmdir の戻り値が COM_UTIL_OK であること。
    EXPECT_FALSE(std::filesystem::exists(path)); // [確認_正常系] - ディレクトリが存在しなくなること。
}

// com_util_rmdir が空でないディレクトリの削除に失敗することの確認
TEST_F(rmdirTest, fails_for_non_empty_directory)
{
    // Arrange
    std::string path = make_path("non_empty_dir");

    std::filesystem::remove_all(path);
    ASSERT_EQ(COM_UTIL_OK, com_util_mkdir(path.c_str()));
    std::filesystem::create_directories(std::filesystem::path(path) / "child"); // [状態] - 子ディレクトリを持つディレクトリを用意する。

    // Pre-Assert

    // Act
    int rtc_rmdir = com_util_rmdir(path.c_str()); // [手順] - 空でないディレクトリを削除する。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc_rmdir); // [確認_異常系] - 空でないディレクトリに対する com_util_rmdir の戻り値が COM_UTIL_OK でないこと。
    EXPECT_TRUE(std::filesystem::exists(path)); // [確認_異常系] - ディレクトリが残っていること。

    // Cleanup
    std::filesystem::remove_all(path);
}

// com_util_rmdir が NULL に対して COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(rmdirTest, null_path_is_rejected)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc_rmdir = com_util_rmdir(NULL); // [手順] - パスに NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc_rmdir); // [確認_異常系] - com_util_rmdir の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}
