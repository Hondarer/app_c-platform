#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/mmap/mmap.h>

#include <errno.h>

#include <filesystem>
#include <string>

#if defined(PLATFORM_LINUX)

    #include <mock_stdlib.h>
    #include <sys/mman.h>
    #include <sys/mock_mman.h>

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

class mmapFailureInjectionTest : public Test
{
  protected:
    std::string path_;

    void SetUp() override
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/mmap/mmapTest/results";

        std::filesystem::create_directories(dir);
        path_ = (dir / "mmapFailureInjectionTest.dat").generic_string();
        std::filesystem::remove(path_);
    }

    void TearDown() override
    {
        std::filesystem::remove(path_);
    }
};

// メモリ マップの作成に失敗した場合に errno が通知されることの確認
// Windows は CreateFileMapping / MapViewOfFile を使うため、この失敗経路は Linux のみに存在する
TEST_F(mmapFailureInjectionTest, attach_reports_errno_when_mmap_fails)
{
    // Arrange
    NiceMock<Mock_sys_mman> mock_sys_mman;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_mman, mmap(_, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, ENOMEM), Return(MAP_FAILED)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - mmap が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に ENOMEM を設定し、1 回目は MAP_FAILED を返却する。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   &detail); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
    EXPECT_EQ(
        ENOMEM,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が ENOMEM であること。
}

// 識別子の複製に失敗した場合にメモリ マップが解除されることの確認
// Windows は識別子の複製を伴わないため、この失敗経路は Linux のみに存在する
TEST_F(mmapFailureInjectionTest, attach_unmaps_when_identity_duplication_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    NiceMock<Mock_sys_mman> mock_sys_mman;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    /* ハンドルは calloc で確保されるため、attach 内の最初の malloc が識別子の複製になる */
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - malloc が識別子の複製のために 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。
    EXPECT_CALL(mock_sys_mman, munmap(_, _, _, _, _))
        .Times(1); // [Pre-Assert確認_異常系] - 確保済みのマップが munmap で 1 回解除されること。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   &detail); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
    EXPECT_EQ(
        ENOMEM,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が ENOMEM であること。
}

// 書き戻しに失敗した場合に errno が通知されることの確認
// Windows は FlushViewOfFile を使うため、この失敗経路は Linux のみに存在する
TEST_F(mmapFailureInjectionTest, flush_reports_errno_when_msync_fails)
{
    // Arrange
    com_util_mmap *map = NULL;

    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                                NULL)); // [状態] - アタッチ済みのメモリ マップを用意する。

    NiceMock<Mock_sys_mman> mock_sys_mman;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_mman, msync(_, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EIO), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - msync が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - errno に EIO を設定し、1 回目は -1 を返却する。

    // Act
    int rtc = com_util_mmap_flush(map, NULL, 0u, &detail); // [手順] - com_util_mmap_flush を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_flush の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(EIO,
              com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EIO であること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
}

#endif /* PLATFORM_LINUX */
