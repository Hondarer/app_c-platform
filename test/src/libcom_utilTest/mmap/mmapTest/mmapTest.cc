#include <testfw.h>

#include <com_util/mmap/mmap.h>
#include <com_util/sync/sync.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

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
    int result;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。

    // Pre-Assert

    // Act
    result = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map,
                                  NULL); // [手順] - create_size 64 で新規アタッチする。

    // Assert
    ASSERT_EQ(COM_UTIL_OK, result); // [確認_正常系] - attach (新規作成) の戻り値が COM_UTIL_OK であること。
    ASSERT_NE((com_util_mmap *)NULL, map);
    EXPECT_EQ((size_t)64,
              com_util_mmap_get_size(map)); // [確認_正常系] - マップ サイズが create_size (64) と一致すること。
    void *address = com_util_mmap_get_address(map);
    ASSERT_NE((void *)NULL, address); // [確認_正常系] - マップ済みアドレスが NULL でないこと。
    std::memcpy(address, "hello", 5);
    EXPECT_EQ(COM_UTIL_OK, com_util_mmap_flush(map, NULL, 0, NULL)); // [手順] - マップ全体を flush する。
    com_util_mmap_detach(map, NULL);
    map = NULL;

    // Act_2
    result =
        com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 4096, &map,
                             NULL); // [手順] - 既存ファイルに対し異なる create_size (4096) を指定して再アタッチする。

    // Assert_2
    ASSERT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - attach (既存ファイル) の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)64,
              com_util_mmap_get_size(
                  map)); // [確認_正常系] - 既存ファイルでは create_size が無視され、実サイズ 64 が報告されること。
    void *address_2 = com_util_mmap_get_address(map);
    ASSERT_NE((void *)NULL, address_2);
    EXPECT_EQ(0, std::memcmp(address_2, "hello", 5)); // [確認_正常系] - 前回書き込んだ内容が読み取れること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
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
    int result = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 0, &map,
                                      NULL); // [手順] - create_size 0 で新規アタッチを試みる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result); // [確認_異常系] - attach (create_size 0、新規作成) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
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
    int result = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_ONLY, 64, &map,
                                      NULL); // [手順] - 存在しないファイルを READ_ONLY でアタッチを試みる。

    // Assert
    EXPECT_NE(COM_UTIL_OK, result); // [確認_異常系] - attach (READ_ONLY、存在しないファイル) が失敗すること。
}

// 不正な引数で attach が INVALID_ARGUMENT を返すことの確認
TEST_F(mmapTest, attach_invalid_arguments_fail)
{
    // Arrange
    com_util_mmap *map = NULL;

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_mmap_attach(NULL, COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map,
                             NULL)); // [確認_異常系] - attach (path NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_mmap_attach("x", COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, NULL,
                             NULL)); // [確認_異常系] - attach (map NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_mmap_attach(
                  "x", (com_util_mmap_access)99, 64, &map,
                  NULL)); // [確認_異常系] - attach (access 不正値) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
}

// NULL ハンドルに対する get_address/get_size/get_rwlock/flush/detach が安全であることの確認
TEST_F(mmapTest, accessors_are_safe_for_null_handle)
{
    // Arrange
    com_util_interprocess_rwlock *lock = NULL;

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ((void *)NULL, com_util_mmap_get_address(NULL)); // [確認_異常系] - get_address(NULL) が NULL を返すこと。
    EXPECT_EQ((size_t)0, com_util_mmap_get_size(NULL));       // [確認_異常系] - get_size(NULL) が 0 を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_mmap_get_rwlock(
                  NULL, &lock,
                  NULL)); // [確認_異常系] - get_rwlock(NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ((com_util_interprocess_rwlock *)NULL,
              lock); // [確認_異常系] - get_rwlock(NULL) が出力先を変更しないこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_mmap_flush(NULL, NULL, 0,
                                  NULL)); // [確認_異常系] - flush(NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_OK, com_util_mmap_detach(NULL,
                                                NULL)); // [確認_異常系] - detach(NULL) が COM_UTIL_OK を返すこと。
}

// get_rwlock を同一ハンドルへ繰り返し呼び出しても、遅延生成したロックが 1 つだけ返ることの確認
TEST_F(mmapTest, get_rwlock_returns_same_instance_on_repeated_calls)
{
    // Arrange
    std::string path = make_path("lazy_rwlock_same.dat");
    com_util_mmap *map = NULL;
    com_util_interprocess_rwlock *lock_first = NULL;
    com_util_interprocess_rwlock *lock_second = NULL;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除した状態とする。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map,
                                   NULL)); // [状態] - create_size 64 で新規アタッチしたハンドルを用意する。

    // Pre-Assert

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
              lock_second); // [確認_正常系] - 2 回目の get_rwlock の戻り値が 1 回目と同一のポインタであること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
    std::remove(path.c_str());
}

// get_rwlock を一度も呼び出さずに attach と detach を行っても、正常に完了することの確認
TEST_F(mmapTest, attach_and_detach_succeed_without_rwlock_access)
{
    // Arrange
    std::string path = make_path("lazy_rwlock_unused.dat");
    com_util_mmap *map = NULL;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除した状態とする。

    // Pre-Assert

    // Act
    int result = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map,
                                      NULL); // [手順] - create_size 64 で新規アタッチする。

    // Assert
    ASSERT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - get_rwlock を呼ばない場合も attach の戻り値が COM_UTIL_OK であること。
    void *address = com_util_mmap_get_address(map);
    ASSERT_NE((void *)NULL,
              address); // [確認_正常系] - get_rwlock を呼ばない場合も get_address が NULL を返さないこと。
    std::memcpy(address, "lazy", 4);
    EXPECT_EQ(COM_UTIL_OK,
              com_util_mmap_flush(map, NULL, 0, NULL)); // [確認_正常系] - get_rwlock を呼ばない場合も flush の戻り値が
                                                        // COM_UTIL_OK であること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
    std::remove(path.c_str());
}

// 複数スレッドから同時に get_rwlock を呼び出しても、遅延生成が直列化され単一のロックへ収束することの確認
TEST_F(mmapTest, get_rwlock_returns_single_instance_under_concurrent_calls)
{
    // Arrange
    const size_t thread_count = 8;
    std::string path = make_path("lazy_rwlock_concurrent.dat");
    com_util_mmap *map = NULL;
    std::vector<com_util_interprocess_rwlock *> results(thread_count, NULL);
    std::vector<int> call_results(thread_count, COM_UTIL_ERR_UNKNOWN);
    std::vector<std::thread> threads;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除した状態とする。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map,
                                   NULL)); // [状態] - create_size 64 で新規アタッチしたハンドルを用意する。

    // Pre-Assert

    // Act
    for (size_t index = 0; index < thread_count; index++)
    {
        threads.emplace_back([&call_results, &results, map, index]()
                             { call_results[index] = com_util_mmap_get_rwlock(map, &results[index], NULL); });
    } // [手順] - 8 個のスレッドから同一ハンドルに対し get_rwlock を同時に呼び出す。
    for (std::thread &thread : threads)
    {
        thread.join();
    }

    // Assert
    ASSERT_NE((com_util_interprocess_rwlock *)NULL,
              results[0]); // [確認_正常系] - 並行呼び出し時の get_rwlock の戻り値が NULL でないこと。
    for (size_t index = 0; index < thread_count; index++)
    {
        EXPECT_EQ(COM_UTIL_OK, call_results[index]);
    } // [確認_正常系] - 全スレッドの get_rwlock の戻り値が COM_UTIL_OK であること。
    for (size_t index = 1; index < thread_count; index++)
    {
        EXPECT_EQ(results[0], results[index]);
    } // [確認_正常系] - 全スレッドの get_rwlock の戻り値が同一のポインタであること。

    // Cleanup
    com_util_mmap_detach(map, NULL);
    std::remove(path.c_str());
}

// 内包するリーダーライター ロックで、複数リーダーの共有ロック同時取得と排他ロックの相互排他・タイムアウトを確認する (マルチ フェーズ テスト)
TEST_F(mmapTest, embedded_rwlock_allows_concurrent_readers_and_times_out_on_exclusive_conflict)
{
    // Arrange
    std::string path = make_path("rwlock.dat");
    com_util_mmap *map1 = NULL;
    com_util_mmap *map2 = NULL;
    com_util_interprocess_rwlock *lock1 = NULL;
    com_util_interprocess_rwlock *lock2 = NULL;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map1,
                                                NULL)); // [状態] - 1 つ目のハンドルを新規作成でアタッチする。
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, 64, &map2,
                                                NULL)); // [状態] - 2 つ目のハンドルを同一パスへ既存アタッチする。
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_get_rwlock(map1, &lock1, NULL));
    ASSERT_EQ(COM_UTIL_OK, com_util_mmap_get_rwlock(map2, &lock2, NULL));
    ASSERT_NE((com_util_interprocess_rwlock *)NULL, lock1);
    ASSERT_NE((com_util_interprocess_rwlock *)NULL, lock2);

    // Pre-Assert

    // Act
    int shared_1 = com_util_interprocess_rwlock_lock_shared(
        lock1, COM_UTIL_SYNC_NO_WAIT); // [手順] - lock1 で共有ロックを取得する。
    int shared_2 = com_util_interprocess_rwlock_lock_shared(
        lock2, COM_UTIL_SYNC_NO_WAIT); // [手順] - lock2 でも共有ロックを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, shared_1); // [確認_正常系] - lock1 の共有ロック取得が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, shared_2); // [確認_正常系] - 別ハンドルの lock2 も共有ロックを同時に取得できること。

    ASSERT_EQ(COM_UTIL_OK, com_util_interprocess_rwlock_unlock(lock1));
    ASSERT_EQ(COM_UTIL_OK, com_util_interprocess_rwlock_unlock(lock2));

    // Act_2
    int exclusive_1 = com_util_interprocess_rwlock_lock_exclusive(
        lock1, COM_UTIL_SYNC_NO_WAIT); // [手順] - lock1 で排他ロックを取得する。
    int exclusive_2_timeout = com_util_interprocess_rwlock_lock_exclusive(
        lock2, 50); // [手順] - lock1 が排他ロック保持中に lock2 で 50ms タイムアウトの排他ロックを試みる。

    // Assert_2
    ASSERT_EQ(COM_UTIL_OK, exclusive_1); // [確認_正常系] - lock1 の排他ロック取得が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_TIMEOUT,
              exclusive_2_timeout); // [確認_正常系] - 排他ロック競合時に lock2 が COM_UTIL_ERR_TIMEOUT を返すこと。

    // Cleanup
    ASSERT_EQ(COM_UTIL_OK, com_util_interprocess_rwlock_unlock(lock1));
    com_util_mmap_detach(map1, NULL);
    com_util_mmap_detach(map2, NULL);
    std::remove(path.c_str());
}
