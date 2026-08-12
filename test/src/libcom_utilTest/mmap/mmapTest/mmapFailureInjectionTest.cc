#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/mmap/mmap.h>

#include <errno.h>

#include <filesystem>
#include <fstream>
#include <string>

#if defined(PLATFORM_LINUX)

    #include <mock_com_util.h>
    #include <mock_stdlib.h>
    #include <sys/mman.h>
    #include <sys/mock_mman.h>

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::DoDefault;
using testing::Invoke;
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

// 読み取り専用ファイルのサイズ取得に失敗した場合にファイルを閉じて失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_error_when_read_only_size_lookup_fails)
{
    // Arrange
    std::ofstream file(path_, std::ios::binary);
    file << std::string(64u, 'r');
    file.close();
    NiceMock<Mock_com_util> mock_com_util;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - サイズ 64 バイトの既存ファイルと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_file_get_size(_, _, _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 読み取り専用アタッチのサイズ取得が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_file_get_size にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_ONLY, 0u, &map,
                                   &detail); // [手順] - サイズ取得失敗を注入して読み取り専用アタッチを呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK,
              rtc); // [確認_異常系] - サイズ取得失敗時の com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL,
              map); // [確認_異常系] - サイズ取得失敗時に com_util_mmap_attach のマップが NULL であること。
}

// 新規ファイルのサイズ設定に失敗した場合にファイルを削除して失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_error_when_new_file_size_setting_fails)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 新規作成するマップと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_file_set_size(_, _, _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 新規ファイルのサイズ設定が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_file_set_size にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   &detail); // [手順] - サイズ設定失敗を注入して新規ファイルへのアタッチを呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK,
              rtc); // [確認_異常系] - サイズ設定失敗時の com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL,
              map); // [確認_異常系] - サイズ設定失敗時に com_util_mmap_attach のマップが NULL であること。
}

// 既存ファイルの再オープンに失敗した場合にアタッチが失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_error_when_existing_file_reopen_fails)
{
    // Arrange
    std::ofstream file(path_, std::ios::binary);
    file << std::string(64u, 'r');
    file.close();
    NiceMock<Mock_com_util> mock_com_util;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - サイズ 64 バイトの既存ファイルと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_file_open(_, _, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - CREATE_NEW と既存ファイル再オープンの com_util_file_open が順に呼び出されること。
                                    // [Pre-Assert手順] - 2 回の com_util_file_open にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   &detail); // [手順] - 既存ファイルの再オープン失敗を注入してアタッチを呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK,
              rtc); // [確認_異常系] - 再オープン失敗時の com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL,
              map); // [確認_異常系] - 再オープン失敗時に com_util_mmap_attach のマップが NULL であること。
}

// 既存ファイルのサイズ取得に失敗した場合にファイルを閉じて失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_error_when_existing_size_lookup_fails)
{
    // Arrange
    std::ofstream file(path_, std::ios::binary);
    file << std::string(64u, 'r');
    file.close();
    NiceMock<Mock_com_util> mock_com_util;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - サイズ 64 バイトの既存ファイルと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_file_open(_, _, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - CREATE_NEW の com_util_file_open が失敗し、既存ファイル再オープンは成功すること。
    // [Pre-Assert手順] - CREATE_NEW の com_util_file_open だけエラーを返し、再オープンは本物へ委譲する。
    EXPECT_CALL(mock_com_util, com_util_file_get_size(_, _, _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 既存ファイルのサイズ取得が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_file_get_size にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   &detail); // [手順] - 既存ファイルのサイズ取得失敗を注入してアタッチを呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - 既存ファイルのサイズ取得失敗時の com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        (com_util_mmap *)NULL,
        map); // [確認_異常系] - 既存ファイルのサイズ取得失敗時に com_util_mmap_attach のマップが NULL であること。
}

// ローカル ロックの生成に失敗した場合にマップを解放して失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_error_when_local_lock_creation_fails)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - ローカル ロック生成前のマップと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_local_lock_create(_))
        .WillOnce(
            Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - ローカル ロック生成が 1 回呼び出されること。
                                           // [Pre-Assert手順] - com_util_local_lock_create にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   &detail); // [手順] - ローカル ロック生成失敗を注入してアタッチを呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - ローカル ロック生成失敗時の com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL,
              map); // [確認_異常系] - ローカル ロック生成失敗時に com_util_mmap_attach のマップが NULL であること。
}

// ローカル ロックの取得に失敗した場合に get_rwlock が失敗することの確認
TEST_F(mmapFailureInjectionTest, get_rwlock_reports_error_when_local_lock_lock_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_interprocess_rwlock *lock = NULL;
    com_util_error detail;
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                                NULL)); // [状態] - get_rwlock 呼び出し前のマップを用意する。
    NiceMock<Mock_com_util> mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_local_lock_lock(_, COM_UTIL_SYNC_WAIT_FOREVER))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - get_rwlock のローカル ロック取得が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_local_lock_lock にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_get_rwlock(map, &lock,
                                       &detail); // [手順] - ローカル ロック取得失敗を注入して get_rwlock を呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - ローカル ロック取得失敗時の com_util_mmap_get_rwlock の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_interprocess_rwlock *)NULL,
              lock); // [確認_異常系] - ローカル ロック取得失敗時のロック出力が NULL であること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
}

// プロセス間 RW ロックの生成に失敗した場合に get_rwlock が失敗することの確認
TEST_F(mmapFailureInjectionTest, get_rwlock_reports_error_when_interprocess_lock_open_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_interprocess_rwlock *lock = NULL;
    com_util_error detail;
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                                NULL)); // [状態] - get_rwlock 呼び出し前のマップを用意する。
    NiceMock<Mock_com_util> mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_interprocess_rwlock_open(_, _))
        .WillOnce(
            Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 遅延 RW ロック生成が 1 回呼び出されること。
    // [Pre-Assert手順] - com_util_interprocess_rwlock_open にエラーを返却させる。

    // Act
    int rtc =
        com_util_mmap_get_rwlock(map, &lock,
                                 &detail); // [手順] - プロセス間 RW ロック生成失敗を注入して get_rwlock を呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - プロセス間 RW ロック生成失敗時の com_util_mmap_get_rwlock の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_interprocess_rwlock *)NULL,
              lock); // [確認_異常系] - プロセス間 RW ロック生成失敗時のロック出力が NULL であること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
}

// ファイルのクローズに失敗した場合に detach が失敗を返すことの確認
TEST_F(mmapFailureInjectionTest, detach_reports_error_when_file_close_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail;
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                                NULL)); // [状態] - detach 前のマップと詳細エラーの格納先を用意する。
    NiceMock<Mock_com_util> mock_com_util;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_file_close(_, _))
        .WillOnce(DoAll(Invoke(delegate_real_com_util_file_close), Return(COM_UTIL_ERR_UNKNOWN)));
    // [Pre-Assert確認_異常系] - detach のファイル クローズが 1 回呼び出されること。
    // [Pre-Assert手順] - 実ファイルを閉じたうえでエラーを返却させる。

    // Act
    int rtc = com_util_mmap_detach(map,
                                   &detail); // [手順] - ファイル クローズ失敗を注入して detach を呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - ファイル クローズ失敗時の com_util_mmap_detach の戻り値が COM_UTIL_OK 以外であること。
}

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

// マップ ハンドルの確保に失敗した場合にアタッチが失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_out_of_memory_when_handle_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_mmap *map = NULL; // [状態] - ハンドルの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - com_util_mmap_attach のハンドル確保で calloc が呼び出されること。
                          // [Pre-Assert手順] - ハンドル確保で NULL を返却する。

    // Act
    int rtc = com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                   NULL); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        rtc); // [確認_異常系] - ハンドル確保失敗時に com_util_mmap_attach の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
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
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - malloc が識別子の複製のために 1 回目に呼び出されること。
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
    EXPECT_EQ(
        EIO,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EIO であること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
}

// マップ解除に失敗した場合に errno が通知されることの確認
TEST_F(mmapFailureInjectionTest, detach_reports_errno_when_munmap_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail;

    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path_.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64u, &map,
                                                NULL)); // [状態] - アタッチ済みのメモリ マップを用意する。

    NiceMock<Mock_sys_mman> mock_sys_mman;

    // Pre-Assert
    EXPECT_CALL(mock_sys_mman, munmap(_, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EIO), Return(-1)))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - munmap が 1 回呼び出されること。
                                      // [Pre-Assert手順] - errno に EIO を設定し、munmap が -1 を返却する。

    // Act
    int rtc = com_util_mmap_detach(map, &detail); // [手順] - com_util_mmap_detach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK,
              rtc); // [確認_異常系] - munmap 失敗時に com_util_mmap_detach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        EIO,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_mmap_detach の詳細 errno が EIO であること。
}

#endif /* PLATFORM_LINUX */
