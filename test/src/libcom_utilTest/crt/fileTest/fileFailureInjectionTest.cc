#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/file.h>
#include <mock_unistd.h>
#include <sys/mock_stat.h>

#include <errno.h>

#include <filesystem>
#include <cstdio>
#include <string>

#if defined(PLATFORM_LINUX)

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

class fileFailureInjectionTest : public Test
{
  protected:
    std::string path_;
    com_util_file file_ = {};

    void SetUp() override
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/fileTest/results";

        std::filesystem::create_directories(dir);
        path_ = (dir / "fileFailureInjectionTest_work.bin").generic_string();

        com_util_file_init(&file_);
        ASSERT_EQ(COM_UTIL_OK, com_util_file_open(&file_, path_.c_str(),
                                                  COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE, NULL));
    }

    void TearDown() override
    {
        com_util_file_close(&file_, NULL);
        std::remove(path_.c_str());
    }
};

// サイズ変更に失敗した場合に errno が通知されることの確認
// Windows の com_util_file_set_size は SetEndOfFile を使うため、この失敗経路は Linux のみに存在する
TEST_F(fileFailureInjectionTest, set_size_reports_errno_when_ftruncate_fails)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_unistd, ftruncate(_, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EIO), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - ftruncate が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に EIO を設定し、1 回目は -1 を返却する。

    // Act
    int rtc = com_util_file_set_size(&file_, 16u, &detail); // [手順] - com_util_file_set_size を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_file_set_size の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(EIO,
              com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EIO であること。
}

// サイズ取得に失敗した場合に errno が通知されることの確認
// Windows の com_util_file_get_size は GetFileSizeEx を使うため、この失敗経路は Linux のみに存在する
TEST_F(fileFailureInjectionTest, get_size_reports_errno_when_fstat_fails)
{
    // Arrange
    NiceMock<Mock_sys_stat> mock_sys_stat;
    size_t size = 0u;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_stat, fstat(_, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - fstat が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に EBADF を設定し、1 回目は -1 を返却する。

    // Act
    int rtc = com_util_file_get_size(&file_, &size, &detail); // [手順] - com_util_file_get_size を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_file_get_size の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        EBADF,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EBADF であること。
}

// ファイル識別の取得に失敗した場合に errno が通知されることの確認
// Windows の com_util_file_get_id は GetFileInformationByHandle を使うため、この失敗経路は Linux のみに存在する
TEST_F(fileFailureInjectionTest, get_id_reports_errno_when_fstat_fails)
{
    // Arrange
    NiceMock<Mock_sys_stat> mock_sys_stat;
    com_util_file_id id = {};
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_stat, fstat(_, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EBADF), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - fstat が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に EBADF を設定し、1 回目は -1 を返却する。

    // Act
    int rtc = com_util_file_get_id(&file_, &id, &detail); // [手順] - com_util_file_get_id を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_file_get_id の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        EBADF,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EBADF であること。
}

#endif /* PLATFORM_LINUX */
