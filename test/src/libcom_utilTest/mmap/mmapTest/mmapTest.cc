#include <testfw.h>

#include <com_util/mmap/mmap.h>
#include <com_util/sync/sync.h>

#include <cstring>
#include <filesystem>
#include <string>

class mmapTest : public Test
{
  protected:
    std::string make_path(const char *name)
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/mmap/mmapTest/results";

        std::filesystem::create_directories(dir);
        return (dir / name).generic_string();
    }
};

// 新規ファイルを create_size で作成し、書き込んだ内容が別ハンドルの再アタッチ後も保持されることの確認 (マルチ フェーズ テスト)
TEST_F(mmapTest, attach_creates_new_file_and_persists_content_across_reattach)
{
    // Arrange
    std::string path = make_path("attach_persist.dat");
    com_util_mmap *map = NULL;
    com_util_mmap_result_t result;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。

    // Pre-Assert

    // Act
    result = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64,
                                  &map); // [手順] - create_size 64 で新規アタッチする。

    // Assert
    ASSERT_EQ(COM_UTIL_MMAP_OK, result); // [確認_正常系] - attach (新規作成) の戻り値が COM_UTIL_MMAP_OK であること。
    ASSERT_NE((com_util_mmap *)NULL, map);
    EXPECT_EQ((size_t)64,
              com_util_mmap_get_size(map)); // [確認_正常系] - マップ サイズが create_size (64) と一致すること。
    void *address = com_util_mmap_get_address(map);
    ASSERT_NE((void *)NULL, address); // [確認_正常系] - マップ済みアドレスが NULL でないこと。
    std::memcpy(address, "hello", 5);
    EXPECT_EQ(COM_UTIL_MMAP_OK, com_util_mmap_flush(map, NULL, 0)); // [手順] - マップ全体を flush する。
    com_util_mmap_detach(map);
    map = NULL;

    // Act_2
    result =
        com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 4096,
                             &map); // [手順] - 既存ファイルに対し異なる create_size (4096) を指定して再アタッチする。

    // Assert_2
    ASSERT_EQ(COM_UTIL_MMAP_OK,
              result); // [確認_正常系] - attach (既存ファイル) の戻り値が COM_UTIL_MMAP_OK であること。
    EXPECT_EQ((size_t)64,
              com_util_mmap_get_size(
                  map)); // [確認_正常系] - 既存ファイルでは create_size が無視され、実サイズ 64 が報告されること。
    void *address_2 = com_util_mmap_get_address(map);
    ASSERT_NE((void *)NULL, address_2);
    EXPECT_EQ(0, std::memcmp(address_2, "hello", 5)); // [確認_正常系] - 前回書き込んだ内容が読み取れること。

    // Cleanup
    com_util_mmap_detach(map);
    std::remove(path.c_str());
}

// create_size に 0 を渡した新規作成が INVALID_ARGUMENT で失敗することの確認
TEST_F(mmapTest, attach_fails_when_create_size_is_zero_for_new_file)
{
    // Arrange
    std::string path = make_path("zero_size.dat");
    com_util_mmap *map = NULL;

    std::remove(path.c_str()); // [状態] - 対象ファイルが存在しないことを保証する。

    // Pre-Assert

    // Act
    com_util_mmap_result_t result = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 0,
                                                         &map); // [手順] - create_size 0 で新規アタッチを試みる。

    // Assert
    EXPECT_EQ(
        COM_UTIL_MMAP_INVALID_ARGUMENT,
        result); // [確認_異常系] - attach (create_size 0、新規作成) が COM_UTIL_MMAP_INVALID_ARGUMENT を返すこと。
}

// 読み取り専用アクセスで存在しないファイルを指定すると失敗することの確認 (新規作成しない)
TEST_F(mmapTest, attach_read_only_fails_for_missing_file)
{
    // Arrange
    std::string path = make_path("missing_read_only.dat");
    com_util_mmap *map = NULL;

    std::remove(path.c_str()); // [状態] - 対象ファイルが存在しないことを保証する。

    // Pre-Assert

    // Act
    com_util_mmap_result_t result =
        com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_ONLY, 64,
                             &map); // [手順] - 存在しないファイルを READ_ONLY でアタッチを試みる。

    // Assert
    EXPECT_NE(COM_UTIL_MMAP_OK, result); // [確認_異常系] - attach (READ_ONLY、存在しないファイル) が失敗すること。
}

// 不正な引数で attach が INVALID_ARGUMENT を返すことの確認
TEST_F(mmapTest, attach_invalid_arguments_fail)
{
    // Arrange
    com_util_mmap *map = NULL;

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_MMAP_INVALID_ARGUMENT,
              com_util_mmap_attach(
                  NULL, COM_UTIL_MMAP_ACCESS_READ_WRITE, 64,
                  &map)); // [確認_異常系] - attach (path NULL) が COM_UTIL_MMAP_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_MMAP_INVALID_ARGUMENT,
        com_util_mmap_attach("x", COM_UTIL_MMAP_ACCESS_READ_WRITE, 64,
                             NULL)); // [確認_異常系] - attach (map NULL) が COM_UTIL_MMAP_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_MMAP_INVALID_ARGUMENT,
              com_util_mmap_attach(
                  "x", (com_util_mmap_access_t)99, 64,
                  &map)); // [確認_異常系] - attach (access 不正値) が COM_UTIL_MMAP_INVALID_ARGUMENT を返すこと。
}

// NULL ハンドルに対する get_address/get_size/get_rwlock/flush/detach が安全であることの確認
TEST_F(mmapTest, accessors_are_safe_for_null_handle)
{
    // Arrange

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ((void *)NULL, com_util_mmap_get_address(NULL)); // [確認_異常系] - get_address(NULL) が NULL を返すこと。
    EXPECT_EQ((size_t)0, com_util_mmap_get_size(NULL));       // [確認_異常系] - get_size(NULL) が 0 を返すこと。
    EXPECT_EQ((com_util_interprocess_rwlock *)NULL,
              com_util_mmap_get_rwlock(NULL)); // [確認_異常系] - get_rwlock(NULL) が NULL を返すこと。
    EXPECT_EQ(COM_UTIL_MMAP_INVALID_ARGUMENT,
              com_util_mmap_flush(NULL, NULL,
                                  0)); // [確認_異常系] - flush(NULL) が COM_UTIL_MMAP_INVALID_ARGUMENT を返すこと。
    com_util_mmap_detach(NULL);        // [確認_異常系] - detach(NULL) がクラッシュせずに完了すること。
}

// 内包するリーダーライター ロックで、複数リーダーの共有ロック同時取得と排他ロックの相互排他・タイムアウトを確認する (マルチ フェーズ テスト)
TEST_F(mmapTest, embedded_rwlock_allows_concurrent_readers_and_times_out_on_exclusive_conflict)
{
    // Arrange
    std::string path = make_path("rwlock.dat");
    com_util_mmap *map1 = NULL;
    com_util_mmap *map2 = NULL;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    ASSERT_EQ(COM_UTIL_MMAP_OK, com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64,
                                                     &map1)); // [状態] - 1 つ目のハンドルを新規作成でアタッチする。
    ASSERT_EQ(COM_UTIL_MMAP_OK, com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64,
                                                     &map2)); // [状態] - 2 つ目のハンドルを同一パスへ既存アタッチする。
    com_util_interprocess_rwlock *lock1 = com_util_mmap_get_rwlock(map1);
    com_util_interprocess_rwlock *lock2 = com_util_mmap_get_rwlock(map2);
    ASSERT_NE((com_util_interprocess_rwlock *)NULL, lock1);
    ASSERT_NE((com_util_interprocess_rwlock *)NULL, lock2);

    // Pre-Assert

    // Act
    com_util_sync_result_t shared_1 = com_util_interprocess_rwlock_lock_shared(
        lock1, COM_UTIL_SYNC_NO_WAIT); // [手順] - lock1 で共有ロックを取得する。
    com_util_sync_result_t shared_2 = com_util_interprocess_rwlock_lock_shared(
        lock2, COM_UTIL_SYNC_NO_WAIT); // [手順] - lock2 でも共有ロックを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_SYNC_OK, shared_1); // [確認_正常系] - lock1 の共有ロック取得が COM_UTIL_SYNC_OK であること。
    EXPECT_EQ(COM_UTIL_SYNC_OK, shared_2); // [確認_正常系] - 別ハンドルの lock2 も共有ロックを同時に取得できること。

    ASSERT_EQ(COM_UTIL_SYNC_OK, com_util_interprocess_rwlock_unlock(lock1));
    ASSERT_EQ(COM_UTIL_SYNC_OK, com_util_interprocess_rwlock_unlock(lock2));

    // Act_2
    com_util_sync_result_t exclusive_1 = com_util_interprocess_rwlock_lock_exclusive(
        lock1, COM_UTIL_SYNC_NO_WAIT); // [手順] - lock1 で排他ロックを取得する。
    com_util_sync_result_t exclusive_2_timeout = com_util_interprocess_rwlock_lock_exclusive(
        lock2, 50); // [手順] - lock1 が排他ロック保持中に lock2 で 50ms タイムアウトの排他ロックを試みる。

    // Assert_2
    ASSERT_EQ(COM_UTIL_SYNC_OK, exclusive_1); // [確認_正常系] - lock1 の排他ロック取得が COM_UTIL_SYNC_OK であること。
    EXPECT_EQ(COM_UTIL_SYNC_TIMEOUT,
              exclusive_2_timeout); // [確認_正常系] - 排他ロック競合時に lock2 が COM_UTIL_SYNC_TIMEOUT を返すこと。

    // Cleanup
    ASSERT_EQ(COM_UTIL_SYNC_OK, com_util_interprocess_rwlock_unlock(lock1));
    com_util_mmap_detach(map1);
    com_util_mmap_detach(map2);
    std::remove(path.c_str());
}
