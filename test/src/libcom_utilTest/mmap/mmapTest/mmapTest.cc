#include <testfw.h>
#include "mmapTestCommon.h"

#include <errno.h>

class mmapTest : public mmapTestFixture
{
};

// 新規ファイルを create_size で作成し、再アタッチ時は既存サイズを使うことの確認 (マルチ フェーズ テスト)
TEST_F(mmapTest, attach_creates_new_file_and_ignores_create_size_on_reattach)
{
    // Arrange
    com_util_mmap *map = NULL;
    int result; // [状態] - 新規作成用のパス mmap.dat とハンドル格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_create_new(), _))
        .WillOnce(
            [](com_util_file *file, const char *, int flags, com_util_error *)
            {
                fill_open_file(file, flags);
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_正常系] - CREATE_NEW 付きの com_util_file_open が 1 回呼び出されること。
                // [Pre-Assert手順] - 番兵ハンドルを設定し、COM_UTIL_OK を返却する。
    EXPECT_CALL(mock_com_util_, com_util_file_set_size(_, kMapSize, _))
        .WillOnce(Return(
            COM_UTIL_OK)); // [Pre-Assert確認_正常系] - create_size 64 で com_util_file_set_size が呼び出されること。
                           // [Pre-Assert手順] - COM_UTIL_OK を返却する。

    // Act
    result = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                  NULL); // [手順] - create_size 64 で新規アタッチする。

    // Assert
    ASSERT_EQ(COM_UTIL_OK, result);        // [確認_正常系] - attach (新規作成) の戻り値が COM_UTIL_OK であること。
    ASSERT_NE((com_util_mmap *)NULL, map); // [確認_正常系] - attach (新規作成) のマップが NULL でないこと。
    EXPECT_EQ(kMapSize,
              com_util_mmap_get_size(map)); // [確認_正常系] - マップ サイズが create_size (64) と一致すること。
    EXPECT_EQ(static_cast<void *>(mapped_buf_),
              com_util_mmap_get_address(map)); // [確認_正常系] - マップ済みアドレスがモックの返却バッファーであること。

    // Pre-Assert_2
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_create_new(), _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_正常系] - 再アタッチで CREATE_NEW の com_util_file_open が 1 回呼び出されること。
                                    // [Pre-Assert手順] - COM_UTIL_ERR_UNKNOWN を返却する。
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_existing_rw(), _))
        .WillOnce(
            [](com_util_file *file, const char *, int flags, com_util_error *)
            {
                fill_open_file(file, flags);
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_正常系] - 再アタッチで CREATE_NEW が失敗し、既存オープンが成功すること。
                // [Pre-Assert手順] - CREATE_NEW はエラーを返し、既存オープンは番兵ハンドルを設定する。
    EXPECT_CALL(mock_com_util_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = kMapSize;
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_正常系] - 既存ファイルのサイズ取得が 1 回呼び出されること。
                // [Pre-Assert手順] - サイズ 64 を設定し、COM_UTIL_OK を返却する。

    // Act_2
    (void)com_util_mmap_detach(map, NULL);
    map = NULL;
    result =
        com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, 4096u, &map,
                             NULL); // [手順] - 既存ファイルに対し異なる create_size (4096) を指定して再アタッチする。

    // Assert_2
    ASSERT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - attach (既存ファイル) の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(kMapSize,
              com_util_mmap_get_size(
                  map)); // [確認_正常系] - 既存ファイルでは create_size が無視され、実サイズ 64 が報告されること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// create_size に 0 を渡した新規作成が INVALID_ARGUMENT で失敗することの確認
TEST_F(mmapTest, attach_fails_when_create_size_is_zero_for_new_file)
{
    // Arrange
    com_util_mmap *map = NULL; // [状態] - 新規作成用のパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_close(_, _))
        .Times(1); // [Pre-Assert確認_異常系] - create_size 0 のとき com_util_file_close が 1 回呼び出されること。
    EXPECT_CALL(mock_com_util_, com_util_remove(_, _))
        .Times(1); // [Pre-Assert確認_異常系] - create_size 0 のときファイルを閉じて削除すること。

    // Act
    int actual_ret = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, 0u, &map,
                                   NULL); // [手順] - create_size 0 で新規アタッチを試みる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - attach (create_size 0、新規作成) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - create_size 0 ではマップ ハンドルが設定されないこと。
}

// サイズ 0 の既存ファイルへのアタッチが失敗することの確認
TEST_F(mmapTest, attach_fails_for_empty_existing_file)
{
    // Arrange
    com_util_mmap *map = NULL; // [状態] - サイズ 0 の既存ファイルを開く前提を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_create_new(), _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - CREATE_NEW の com_util_file_open が 1 回呼び出されること。
                                    // [Pre-Assert手順] - COM_UTIL_ERR_UNKNOWN を返却する。
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_existing_rw(), _))
        .WillOnce(
            [](com_util_file *file, const char *, int flags, com_util_error *)
            {
                fill_open_file(file, flags);
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_異常系] - CREATE_NEW が失敗し、既存ファイルの再オープンが成功すること。
                // [Pre-Assert手順] - 再オープンで番兵ハンドルを設定する。
    EXPECT_CALL(mock_com_util_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 0u;
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_異常系] - 既存ファイルのサイズ取得が 0 を返すこと。
                // [Pre-Assert手順] - サイズ 0 を設定し、COM_UTIL_OK を返却する。

    // Act
    int actual_ret = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                   NULL); // [手順] - サイズ 0 の既存ファイルへアタッチする。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        actual_ret); // [確認_異常系] - 空の既存ファイルに対する com_util_mmap_attach の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - 空の既存ファイルではマップ ハンドルが設定されないこと。
}

// 既存ファイルを読み取り専用でマップできることの確認
TEST_F(mmapTest, attach_read_only_maps_existing_file)
{
    // Arrange
    com_util_mmap *map = NULL; // [状態] - 読み取り専用で開く既存ファイルのパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_read_only(), _))
        .WillOnce(
            [](com_util_file *file, const char *, int flags, com_util_error *)
            {
                fill_open_file(file, flags);
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_正常系] - 読み取り専用の com_util_file_open が 1 回呼び出されること。
                // [Pre-Assert手順] - 番兵ハンドルを設定し、COM_UTIL_OK を返却する。
    // [Pre-Assert確認_正常系] - 読み取り専用のマップ API が呼び出されること。
    // [Pre-Assert手順] - テスト用バッファーを返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_mman_, mmap(_, _, _, _, kMapSize, PROT_READ, MAP_SHARED, kFakeFileHandle, 0))
        .WillOnce(Return(mapped_buf_));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_windows_, CreateFileMappingA(_, _, _, kFakeFileHandle, _, PAGE_READONLY, _, _, _))
        .WillOnce(Return(kFakeMappingHandle));
    EXPECT_CALL(mock_windows_, MapViewOfFile(_, _, _, kFakeMappingHandle, FILE_MAP_READ, 0u, 0u, kMapSize))
        .WillOnce(Return(mapped_buf_));
#endif /* PLATFORM_ */

    // Act
    int actual_ret = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_ONLY, 0u, &map,
                                   NULL); // [手順] - 既存ファイルを読み取り専用でアタッチする。

    // Assert
    ASSERT_EQ(COM_UTIL_OK, actual_ret); // [確認_正常系] - 読み取り専用アタッチの戻り値が COM_UTIL_OK であること。
    ASSERT_NE((com_util_mmap *)NULL, map);
    EXPECT_EQ(kMapSize,
              com_util_mmap_get_size(map)); // [確認_正常系] - マップ サイズが既存ファイルの 64 バイトと一致すること。
    EXPECT_EQ(static_cast<void *>(mapped_buf_),
              com_util_mmap_get_address(
                  map)); // [確認_正常系] - 読み取り専用マップのアドレスがモックの返却バッファーであること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// 読み取り専用アクセスで存在しないファイルを指定すると失敗することの確認 (新規作成しない)
TEST_F(mmapTest, attach_read_only_fails_for_missing_file)
{
    // Arrange
    com_util_mmap *map = NULL; // [状態] - 存在しないファイルのパス mmap.dat を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_file_open(_, _, flags_read_only(), _))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 読み取り専用の com_util_file_open が 1 回呼び出されること。
                                    // [Pre-Assert手順] - COM_UTIL_ERR_UNKNOWN を返却する。

    // Act
    int actual_ret = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_ONLY, kMapSize, &map,
                                   NULL); // [手順] - 存在しないファイルを READ_ONLY でアタッチを試みる。

    // Assert
    EXPECT_NE(COM_UTIL_OK, actual_ret);           // [確認_異常系] - attach (READ_ONLY、存在しないファイル) が失敗すること。
    EXPECT_EQ((com_util_mmap *)NULL, map); // [確認_異常系] - 欠落ファイルではマップ ハンドルが設定されないこと。
}

// 不正な引数で attach が INVALID_ARGUMENT を返すことの確認
TEST_F(mmapTest, attach_invalid_arguments_fail)
{
    // Arrange
    com_util_mmap *map = NULL;
    int invalid_access_value = 99;
    com_util_mmap_access invalid_access = (com_util_mmap_access)invalid_access_value;
    int null_path_result;
    int null_map_result;
    int empty_path_result;
    int invalid_access_result; // [状態] - 不正な access 値 99 を用意する。

    // Pre-Assert

    // Act
    null_path_result = com_util_mmap_attach(NULL, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                            NULL); // [手順] - path に NULL を指定してアタッチする。
    null_map_result = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, NULL,
                                           NULL); // [手順] - map に NULL を指定してアタッチする。
    empty_path_result = com_util_mmap_attach("", COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                             NULL); // [手順] - 空文字列の path を指定してアタッチする。
    invalid_access_result = com_util_mmap_attach(kPath, invalid_access, kMapSize, &map,
                                                 NULL); // [手順] - 不正な access 値でアタッチする。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_path_result); // [確認_異常系] - path NULL の com_util_mmap_attach の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_map_result); // [確認_異常系] - map NULL の com_util_mmap_attach の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        empty_path_result); // [確認_異常系] - 空文字列 path の com_util_mmap_attach の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        invalid_access_result); // [確認_異常系] - access 不正値の com_util_mmap_attach の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// NULL ハンドルに対する get_address/get_size/get_rwlock/flush/detach が安全であることの確認
TEST_F(mmapTest, accessors_are_safe_for_null_handle)
{
    // Arrange
    com_util_interprocess_rwlock *lock = NULL;
    void *address;
    size_t size;
    int rwlock_result;
    int flush_result;
    int detach_result;

    // Pre-Assert

    // Act
    address = com_util_mmap_get_address(NULL);                   // [手順] - get_address に NULL を渡す。
    size = com_util_mmap_get_size(NULL);                         // [手順] - get_size に NULL を渡す。
    rwlock_result = com_util_mmap_get_rwlock(NULL, &lock, NULL); // [手順] - get_rwlock に NULL ハンドルを渡す。
    flush_result = com_util_mmap_flush(NULL, NULL, 0u, NULL);    // [手順] - flush に NULL ハンドルを渡す。
    detach_result = com_util_mmap_detach(NULL, NULL);            // [手順] - detach に NULL ハンドルを渡す。

    // Assert
    EXPECT_EQ((void *)NULL, address); // [確認_異常系] - get_address(NULL) が NULL を返すこと。
    EXPECT_EQ((size_t)0, size);       // [確認_異常系] - get_size(NULL) が 0 を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rwlock_result); // [確認_異常系] - get_rwlock(NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ((com_util_interprocess_rwlock *)NULL,
              lock); // [確認_異常系] - get_rwlock(NULL) が出力先を変更しないこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              flush_result);               // [確認_異常系] - flush(NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_OK, detach_result); // [確認_異常系] - detach(NULL) が COM_UTIL_OK を返すこと。
}

// get_rwlock を同一ハンドルへ繰り返し呼び出しても、遅延生成したロックが 1 つだけ返ることの確認
TEST_F(mmapTest, get_rwlock_returns_same_instance_on_repeated_calls)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_interprocess_rwlock *lock_first = NULL;
    com_util_interprocess_rwlock *lock_second = NULL;
    attachNewFile(&map); // [状態] - create_size 64 で新規アタッチしたハンドルを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_local_lock_lock(kFakeGuard, COM_UTIL_SYNC_WAIT_FOREVER))
        .Times(2)
        .WillRepeatedly(
            Return(COM_UTIL_OK)); // [Pre-Assert確認_正常系] - com_util_local_lock_lock が 2 回呼び出されること。
                                  // [Pre-Assert手順] - COM_UTIL_OK を返却する。
    EXPECT_CALL(mock_com_util_, com_util_interprocess_rwlock_open(_, _))
        .WillOnce(
            [](const char *, com_util_interprocess_rwlock **lock)
            {
                *lock = kFakeRwlock;
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_正常系] - プロセス間ロックの open が 1 回だけ呼び出されること。
                // [Pre-Assert手順] - 番兵ロックを設定し、COM_UTIL_OK を返却する。
    EXPECT_CALL(mock_com_util_, com_util_local_lock_unlock(kFakeGuard))
        .Times(2)
        .WillRepeatedly(
            Return(COM_UTIL_OK)); // [Pre-Assert確認_正常系] - ローカル ロックの取得と解放が 2 回ずつ行われること。

    // Act
    int result_first = com_util_mmap_get_rwlock(map, &lock_first, NULL); // [手順] - get_rwlock を 1 回目に呼び出す。
    int result_second = com_util_mmap_get_rwlock(map, &lock_second,
                                                 NULL); // [手順] - 同一ハンドルに対し get_rwlock を 2 回目に呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result_first); // [確認_正常系] - 1 回目の get_rwlock の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              result_second); // [確認_正常系] - 2 回目の get_rwlock の戻り値が COM_UTIL_OK であること。
    ASSERT_NE((com_util_interprocess_rwlock *)NULL,
              lock_first); // [確認_正常系] - 1 回目の get_rwlock の戻り値が NULL でないこと。
    EXPECT_EQ(lock_first,
              lock_second); // [確認_正常系] - 2 回目の get_rwlock の戻り値が 1 回目と同一のポインターであること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// ロック出力先が NULL の場合に get_rwlock が拒否することの確認
TEST_F(mmapTest, get_rwlock_rejects_null_output)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_error detail;
    attachNewFile(&map); // [状態] - create_size 64 でマップ ハンドルを用意する。

    // Pre-Assert

    // Act
    int actual_ret = com_util_mmap_get_rwlock(map, NULL,
                                       &detail); // [手順] - ロック出力先に NULL を指定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - get_rwlock の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(EINVAL,
              com_util_error_get_errno(&detail)); // [確認_異常系] - 詳細 errno が EINVAL であること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// get_rwlock を一度も呼び出さずに attach と detach を行っても、正常に完了することの確認
TEST_F(mmapTest, attach_and_detach_succeed_without_rwlock_access)
{
    // Arrange
    com_util_mmap *map = NULL; // [状態] - ロック未参照の新規アタッチ用パスを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_interprocess_rwlock_open(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - get_rwlock 未使用時にプロセス間ロックの生成が呼び出されないこと。
    EXPECT_CALL(mock_com_util_, com_util_interprocess_rwlock_dispose(_))
        .Times(0); // [Pre-Assert確認_正常系] - get_rwlock 未使用時にプロセス間ロックの生成と破棄が呼び出されないこと。

    // Act
    int result = com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, &map,
                                      NULL); // [手順] - create_size 64 で新規アタッチする。

    // Assert
    ASSERT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - get_rwlock を呼ばない場合も attach の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(static_cast<void *>(mapped_buf_),
              com_util_mmap_get_address(
                  map)); // [確認_正常系] - get_rwlock を呼ばない場合も get_address がマップ バッファーを返すこと。
    int flush_result = com_util_mmap_flush(map, NULL, 0u, NULL);
    EXPECT_EQ(COM_UTIL_OK,
              flush_result); // [確認_正常系] - get_rwlock を呼ばない場合も flush の戻り値が COM_UTIL_OK であること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// 指定したアドレス範囲の書き戻しが成功することの確認
TEST_F(mmapTest, flush_succeeds_for_explicit_address_range)
{
    // Arrange
    com_util_mmap *map = NULL;
    attachNewFile(&map); // [状態] - 書き戻し対象のマップを用意する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 先頭 1 byte を対象とする書き戻し API が呼び出されること。
    // [Pre-Assert手順] - 成功を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_sys_mman_, msync(_, _, _, mapped_buf_, 1u, MS_SYNC)).WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_windows_, FlushViewOfFile(_, _, _, mapped_buf_, 1u)).WillOnce(Return(TRUE));
    EXPECT_CALL(mock_windows_, FlushFileBuffers(_, _, _, kFakeFileHandle)).WillOnce(Return(TRUE));
#endif /* PLATFORM_ */

    // Act
    int actual_ret = com_util_mmap_flush(map, mapped_buf_, 1u,
                                  NULL); // [手順] - マップ先頭の 1 byte を指定して書き戻す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        actual_ret); // [確認_正常系] - 明示したアドレス範囲に対する com_util_mmap_flush の戻り値が COM_UTIL_OK であること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}

// get_rwlock の初回生成がローカル ロックで直列化され、open が 1 回だけ呼ばれることの確認
TEST_F(mmapTest, get_rwlock_serializes_open_with_local_lock)
{
    // Arrange
    com_util_mmap *map = NULL;
    com_util_interprocess_rwlock *lock = NULL;
    attachNewFile(&map); // [状態] - create_size 64 で新規アタッチしたハンドルを用意する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - lock、open、unlock の順で 1 回ずつ呼び出されること。
    // [Pre-Assert手順] - 番兵ロックを設定し、各呼び出しは成功を返却する。
    {
        testing::InSequence sequence;
        EXPECT_CALL(mock_com_util_, com_util_local_lock_lock(kFakeGuard, COM_UTIL_SYNC_WAIT_FOREVER))
            .WillOnce(Return(COM_UTIL_OK));
        EXPECT_CALL(mock_com_util_, com_util_interprocess_rwlock_open(_, _))
            .WillOnce(
                [](const char *, com_util_interprocess_rwlock **out)
                {
                    *out = kFakeRwlock;
                    return COM_UTIL_OK;
                });
        EXPECT_CALL(mock_com_util_, com_util_local_lock_unlock(kFakeGuard)).WillOnce(Return(COM_UTIL_OK));
    }

    // Act
    int actual_ret = com_util_mmap_get_rwlock(map, &lock, NULL); // [手順] - get_rwlock を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret);  // [確認_正常系] - get_rwlock の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(kFakeRwlock, lock); // [確認_正常系] - get_rwlock の出力が番兵ロックであること。

    // Cleanup
    (void)com_util_mmap_detach(map, NULL);
}
