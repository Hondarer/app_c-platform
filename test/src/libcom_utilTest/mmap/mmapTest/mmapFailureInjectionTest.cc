#include <testfw.h>
#include "mmapTestCommon.h"

#include <mock_stdlib.h>

#include <errno.h>

using testing::Assign;
using testing::DoAll;

class mmapFailureInjectionTest : public mmapTestFixture
{
};

// 読み取り専用ファイルのサイズ取得に失敗した場合にファイルを閉じて失敗することの確認
TEST_F(mmapFailureInjectionTest, attach_returns_error_when_read_only_size_lookup_fails)
{
    // Arrange
    com_util_mmap *map = NULL; // [状態] - 読み取り専用アタッチ用のパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_read_only(), _))
        .WillOnce(
            [](com_util_file *file, const char *, int flags, com_util_error *)
            {
                fill_open_file(file, flags);
                return COM_UTIL_OK;
            });
    EXPECT_CALL(mock_com_util_, com_util_file_get_size(_, _, _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 読み取り専用アタッチのサイズ取得が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_file_get_size にエラーを返却させる。
    EXPECT_CALL(mock_com_util_, com_util_file_close(_, _)).Times(1);

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_ONLY, 0u, &map,
                                   NULL); // [手順] - サイズ取得失敗を注入して読み取り専用アタッチを呼び出す。

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
    com_util_mmap *map = NULL; // [状態] - 新規作成するマップのパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_set_size(_, _, _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 新規ファイルのサイズ設定が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_file_set_size にエラーを返却させる。
    EXPECT_CALL(mock_com_util_, com_util_file_close(_, _)).Times(1);
    EXPECT_CALL(mock_com_util_, com_util_remove(_, _)).Times(1);

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   NULL); // [手順] - サイズ設定失敗を注入して新規ファイルへのアタッチを呼び出す。

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
    com_util_mmap *map = NULL; // [状態] - 既存ファイル再オープン失敗用のパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_create_new(), _)).WillOnce(Return(COM_UTIL_ERR_UNKNOWN));
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_existing_rw(), _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - CREATE_NEW と既存ファイル再オープンの com_util_file_open が順に呼び出されること。
                                    // [Pre-Assert手順] - 2 回の com_util_file_open にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   NULL); // [手順] - 既存ファイルの再オープン失敗を注入してアタッチを呼び出す。

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
    com_util_mmap *map = NULL; // [状態] - 既存ファイルのサイズ取得失敗用のパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_create_new(), _)).WillOnce(Return(COM_UTIL_ERR_UNKNOWN));
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_existing_rw(), _))
        .WillOnce(
            [](com_util_file *file, const char *, int flags, com_util_error *)
            {
                fill_open_file(file, flags);
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_異常系] - CREATE_NEW の com_util_file_open が失敗し、既存ファイル再オープンは成功すること。
                // [Pre-Assert手順] - CREATE_NEW だけエラーを返し、再オープンは番兵ハンドルを設定する。
    EXPECT_CALL(mock_com_util_, com_util_file_get_size(_, _, _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 既存ファイルのサイズ取得が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_file_get_size にエラーを返却させる。
    EXPECT_CALL(mock_com_util_, com_util_file_close(_, _)).Times(1);

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   NULL); // [手順] - 既存ファイルのサイズ取得失敗を注入してアタッチを呼び出す。

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
    com_util_mmap *map = NULL; // [状態] - ローカル ロック生成前のマップ用パスを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_create(_))
        .WillOnce(
            Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - ローカル ロック生成が 1 回呼び出されること。
                                           // [Pre-Assert手順] - com_util_local_lock_create にエラーを返却させる。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_mman_, munmap(_, _, _, _, _)).Times(1);
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_windows_, UnmapViewOfFile(_, _, _, _)).Times(1);
    EXPECT_CALL(mock_windows_, CloseHandle(_, _, _, kFakeMappingHandle)).Times(1);
#endif /* PLATFORM_ */
    // [Pre-Assert確認_異常系] - ロック生成失敗時に確保済みのマップが解除されること。
    EXPECT_CALL(mock_com_util_, com_util_file_close(_, _)).Times(1);

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   NULL); // [手順] - ローカル ロック生成失敗を注入してアタッチを呼び出す。

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
    attachNewFile(&map); // [状態] - get_rwlock 呼び出し前のマップを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_lock(_, COM_UTIL_SYNC_WAIT_FOREVER))
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
    (void)com_util_mmap_detach(map, NULL);
}

// プロセス間 RW ロックの生成に失敗した場合に get_rwlock が失敗することの確認
TEST_F(mmapFailureInjectionTest, get_rwlock_reports_error_when_interprocess_lock_open_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_interprocess_rwlock *lock = NULL;
    com_util_error detail;
    attachNewFile(&map); // [状態] - get_rwlock 呼び出し前のマップを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_interprocess_rwlock_open(_, _))
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
    (void)com_util_mmap_detach(map, NULL);
}

// ファイルのクローズに失敗した場合に detach が失敗を返すことの確認
TEST_F(mmapFailureInjectionTest, detach_reports_error_when_file_close_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail;
    attachNewFile(&map); // [状態] - detach 前のマップと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_close(_, _)).WillOnce(Return(COM_UTIL_ERR_UNKNOWN));
    // [Pre-Assert確認_異常系] - detach のファイル クローズが 1 回呼び出されること。
    // [Pre-Assert手順] - com_util_file_close にエラーを返却させる。

    // Act
    int rtc = com_util_mmap_detach(map,
                                   &detail); // [手順] - ファイル クローズ失敗を注入して detach を呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - ファイル クローズ失敗時の com_util_mmap_detach の戻り値が COM_UTIL_OK 以外であること。
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
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   NULL); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        rtc); // [確認_異常系] - ハンドル確保失敗時に com_util_mmap_attach の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
}

// 識別子の複製に失敗した場合にメモリ マップが解除されることの確認
TEST_F(mmapFailureInjectionTest, attach_unmaps_when_identity_duplication_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    /* ハンドルは calloc で確保されるため、attach 内の最初の malloc が識別子の複製になる */
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - malloc が識別子の複製のために 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_mman_, munmap(_, _, _, _, _)).Times(1);
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_windows_, UnmapViewOfFile(_, _, _, _)).Times(1);
    EXPECT_CALL(mock_windows_, CloseHandle(_, _, _, kFakeMappingHandle)).Times(1);
#endif /* PLATFORM_ */
    // [Pre-Assert確認_異常系] - 確保済みのマップが 1 回解除されること。

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   &detail); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
    EXPECT_EQ(
        ENOMEM,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が ENOMEM であること。
}

#if defined(PLATFORM_LINUX)
// メモリ マップの作成に失敗した場合に errno が通知されることの確認
TEST_F(mmapFailureInjectionTest, attach_reports_errno_when_mmap_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_mman_, mmap(_, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, ENOMEM), Return(MAP_FAILED)));
    // [Pre-Assert確認_異常系] - mmap が 1 回呼び出されること。
    // [Pre-Assert手順] - errno に ENOMEM を設定し、MAP_FAILED を返却する。

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   &detail); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
    EXPECT_EQ(
        ENOMEM,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が ENOMEM であること。
}

// 書き戻しに失敗した場合に errno が通知されることの確認
TEST_F(mmapFailureInjectionTest, flush_reports_errno_when_msync_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    attachNewFile(&map);
    com_util_error detail; // [状態] - アタッチ済みのメモリ マップと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_mman_, msync(_, _, _, _, _, _)).WillOnce(DoAll(Assign(&errno, EIO), Return(-1)));
    // [Pre-Assert確認_異常系] - msync が 1 回呼び出されること。
    // [Pre-Assert手順] - errno に EIO を設定し、-1 を返却する。

    // Act
    int rtc = com_util_mmap_flush(map, NULL, 0u, &detail); // [手順] - com_util_mmap_flush を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_flush の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        EIO,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EIO であること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// マップ解除に失敗した場合に errno が通知されることの確認
TEST_F(mmapFailureInjectionTest, detach_reports_errno_when_munmap_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail;
    attachNewFile(&map); // [状態] - アタッチ済みのメモリ マップを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_sys_mman_, munmap(_, _, _, _, _)).WillOnce(DoAll(Assign(&errno, EIO), Return(-1)));
    // [Pre-Assert確認_異常系] - munmap が 1 回呼び出されること。
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

#elif defined(PLATFORM_WINDOWS)
// CreateFileMapping に失敗した場合に Windows エラーが通知されることの確認
TEST_F(mmapFailureInjectionTest, attach_reports_error_when_create_file_mapping_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_windows_, CreateFileMappingA(_, _, _, _, _, _, _, _, _)).WillOnce(Return((HANDLE)NULL));
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _)).WillOnce(Return(ERROR_NOT_ENOUGH_MEMORY));
    // [Pre-Assert確認_異常系] - CreateFileMappingA が 1 回呼び出されること。
    // [Pre-Assert手順] - NULL を返し、GetLastError に ERROR_NOT_ENOUGH_MEMORY を返却する。

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   &detail); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
    EXPECT_EQ(
        static_cast<unsigned long>(ERROR_NOT_ENOUGH_MEMORY),
        com_util_error_get_windows_error(
            &detail)); // [確認_異常系] - com_util_error_get_windows_error の戻り値が ERROR_NOT_ENOUGH_MEMORY であること。
}

// MapViewOfFile に失敗した場合に Windows エラーが通知されることの確認
TEST_F(mmapFailureInjectionTest, attach_reports_error_when_map_view_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_windows_, MapViewOfFile(_, _, _, _, _, _, _, _)).WillOnce(Return((LPVOID)NULL));
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _)).WillOnce(Return(ERROR_NOT_ENOUGH_MEMORY));
    EXPECT_CALL(mock_windows_, CloseHandle(_, _, _, kFakeMappingHandle)).Times(1);
    // [Pre-Assert確認_異常系] - MapViewOfFile が 1 回呼び出されること。
    // [Pre-Assert手順] - NULL を返し、GetLastError に ERROR_NOT_ENOUGH_MEMORY を返却する。

    // Act
    int rtc = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   &detail); // [手順] - com_util_mmap_attach を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_attach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - ハンドルが設定されないこと。
    EXPECT_EQ(
        static_cast<unsigned long>(ERROR_NOT_ENOUGH_MEMORY),
        com_util_error_get_windows_error(
            &detail)); // [確認_異常系] - com_util_error_get_windows_error の戻り値が ERROR_NOT_ENOUGH_MEMORY であること。
}

// FlushViewOfFile に失敗した場合に Windows エラーが通知されることの確認
TEST_F(mmapFailureInjectionTest, flush_reports_error_when_flush_view_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    attachNewFile(&map);
    com_util_error detail; // [状態] - アタッチ済みのメモリ マップと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_windows_, FlushViewOfFile(_, _, _, _, _)).WillOnce(Return(FALSE));
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _)).WillOnce(Return(ERROR_LOCK_VIOLATION));
    // [Pre-Assert確認_異常系] - FlushViewOfFile が 1 回呼び出されること。
    // [Pre-Assert手順] - FALSE を返し、GetLastError に ERROR_LOCK_VIOLATION を返却する。

    // Act
    int rtc = com_util_mmap_flush(map, NULL, 0u, &detail); // [手順] - com_util_mmap_flush を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_flush の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        static_cast<unsigned long>(ERROR_LOCK_VIOLATION),
        com_util_error_get_windows_error(
            &detail)); // [確認_異常系] - com_util_error_get_windows_error の戻り値が ERROR_LOCK_VIOLATION であること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// FlushFileBuffers に失敗した場合に Windows エラーが通知されることの確認
TEST_F(mmapFailureInjectionTest, flush_reports_error_when_flush_file_buffers_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    attachNewFile(&map);
    com_util_error detail; // [状態] - アタッチ済みのメモリ マップと詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_windows_, FlushViewOfFile(_, _, _, _, _)).WillOnce(Return(TRUE));
    EXPECT_CALL(mock_windows_, FlushFileBuffers(_, _, _, _)).WillOnce(Return(FALSE));
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _)).WillOnce(Return(ERROR_LOCK_VIOLATION));
    // [Pre-Assert確認_異常系] - FlushFileBuffers が 1 回呼び出されること。
    // [Pre-Assert手順] - FALSE を返し、GetLastError に ERROR_LOCK_VIOLATION を返却する。

    // Act
    int rtc = com_util_mmap_flush(map, NULL, 0u, &detail); // [手順] - com_util_mmap_flush を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK, rtc); // [確認_異常系] - com_util_mmap_flush の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(
        static_cast<unsigned long>(ERROR_LOCK_VIOLATION),
        com_util_error_get_windows_error(
            &detail)); // [確認_異常系] - com_util_error_get_windows_error の戻り値が ERROR_LOCK_VIOLATION であること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// UnmapViewOfFile に失敗した場合に Windows エラーが通知されることの確認
TEST_F(mmapFailureInjectionTest, detach_reports_error_when_unmap_view_fails)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail;
    attachNewFile(&map); // [状態] - アタッチ済みのメモリ マップを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_windows_, UnmapViewOfFile(_, _, _, _)).WillOnce(Return(FALSE));
    EXPECT_CALL(mock_windows_, GetLastError(_, _, _)).WillOnce(Return(ERROR_INVALID_PARAMETER));
    // [Pre-Assert確認_異常系] - UnmapViewOfFile が 1 回呼び出されること。
    // [Pre-Assert手順] - FALSE を返し、GetLastError に ERROR_INVALID_PARAMETER を返却する。

    // Act
    int rtc = com_util_mmap_detach(map, &detail); // [手順] - com_util_mmap_detach を呼び出す。

    // Assert
    EXPECT_NE(
        COM_UTIL_OK,
        rtc); // [確認_異常系] - UnmapViewOfFile 失敗時に com_util_mmap_detach の戻り値が COM_UTIL_OK 以外であること。
    EXPECT_EQ(static_cast<unsigned long>(ERROR_INVALID_PARAMETER),
              com_util_error_get_windows_error(
                  &detail)); // [確認_異常系] - com_util_mmap_detach の詳細エラーが ERROR_INVALID_PARAMETER であること。
}

#endif /* PLATFORM_ */
