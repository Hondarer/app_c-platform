#include <testfw.h>
#include <com_util/base/error.h>
#include <com_util/base/error_message.h>
#include <com_util/base/result.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/file.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/stdlib.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/unistd.h>
#include <com_util/mmap/mmap.h>
#include <com_util/sync/sync.h>

#include <errno.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

typedef struct tls_thread_case
{
    com_util_error observed_error;
    com_util_error_cause requested_cause;
    int call_completed;
} tls_thread_case;

typedef struct detail_out_null_case
{
    const char *name;
    void (*invoke)(void);
    int expected_tls_set;
    unsigned int pad; /* 明示的アラインメント */
} detail_out_null_case;

static void invoke_vopen_fmt_with_null_detail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)com_util_vopen_fmt(0, 0, NULL, format, args);
    va_end(args);
}

static void invoke_vfopen_fmt_with_null_detail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)com_util_vfopen_fmt(NULL, NULL, format, args);
    va_end(args);
}

static void invoke_vremove_fmt_with_null_detail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)com_util_vremove_fmt(NULL, format, args);
    va_end(args);
}

static void invoke_vstat_fmt_with_null_detail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)com_util_vstat_fmt(NULL, NULL, format, args);
    va_end(args);
}

static void invoke_vmkdir_fmt_with_null_detail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)com_util_vmkdir_fmt(NULL, format, args);
    va_end(args);
}

static void invoke_vaccess_fmt_with_null_detail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    (void)com_util_vaccess_fmt(0, NULL, format, args);
    va_end(args);
}

static void record_thread_local_error(void *arg)
{
    tls_thread_case *test_case = static_cast<tls_thread_case *>(arg);

    if (test_case->requested_cause == COM_UTIL_CAUSE_NOT_FOUND)
    {
        (void)com_util_fopen("com_util_error_tls_missing_file", "rb", NULL);
    }
    else
    {
        (void)com_util_path_get_full(NULL, 0U, NULL, NULL);
    }

    com_util_error_get_last(&test_case->observed_error);
    test_case->call_completed = 1;
}

class errorContractTest : public Test
{
};

// 全公開 API が detail_out の NULL を受け付けることの確認
TEST_F(errorContractTest, all_detail_out_apis_accept_null)
{
    // Arrange
    const std::vector<detail_out_null_case> cases = {
        {"com_util_open", []() { (void)com_util_open(NULL, 0, 0, NULL); }, 1, 0U},
        {"com_util_open_fmt", []() { (void)com_util_open_fmt(0, 0, NULL, NULL); }, 1, 0U},
        {"com_util_vopen_fmt", []() { invoke_vopen_fmt_with_null_detail(NULL); }, 1, 0U},
        {"com_util_file_open", []() { (void)com_util_file_open(NULL, NULL, 0, NULL); }, 1, 0U},
        {"com_util_file_write", []() { (void)com_util_file_write(NULL, NULL, 1U, NULL); }, 1, 0U},
        {"com_util_file_read", []() { (void)com_util_file_read(NULL, NULL, 1U, NULL, NULL); }, 1, 0U},
        {"com_util_file_get_size", []() { (void)com_util_file_get_size(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_file_set_size", []() { (void)com_util_file_set_size(NULL, 0U, NULL); }, 1, 0U},
        {"com_util_file_get_id", []() { (void)com_util_file_get_id(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_file_get_path_id", []() { (void)com_util_file_get_path_id(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_file_flush", []() { (void)com_util_file_flush(NULL, NULL); }, 1, 0U},
        {"com_util_file_close", []() { (void)com_util_file_close(NULL, NULL); }, 1, 0U},
        {"com_util_path_get_full", []() { (void)com_util_path_get_full(NULL, 0U, NULL, NULL); }, 1, 0U},
        {"com_util_paths_equal", []() { (void)com_util_paths_equal(NULL, NULL, NULL, NULL); }, 1, 0U},
        {"com_util_get_temp_dir", []() { (void)com_util_get_temp_dir(NULL, 0U, NULL); }, 1, 0U},
        {"com_util_path_concat_n", []() { (void)com_util_path_concat_n(NULL, 0U, NULL, 1U, "x"); }, 1, 0U},
        {"com_util_path_concat", []() { (void)com_util_path_concat(NULL, 0U, NULL, "x"); }, 1, 0U},
        {"com_util_path_dirname", []() { (void)com_util_path_dirname(NULL, 0U, NULL, NULL); }, 1, 0U},
        {"com_util_path_strip_extension", []() { (void)com_util_path_strip_extension(NULL, 0U, NULL, NULL); }, 1, 0U},
        {"com_util_path_join_n", []() { (void)com_util_path_join_n(NULL, 0U, NULL, 1U, "x"); }, 1, 0U},
        {"com_util_path_join", []() { (void)com_util_path_join(NULL, 0U, NULL, "x"); }, 1, 0U},
        {"com_util_fopen", []() { (void)com_util_fopen(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_freopen", []() { (void)com_util_freopen(NULL, NULL, NULL, NULL); }, 1, 0U},
        {"com_util_fclose", []() { (void)com_util_fclose(NULL, NULL); }, 1, 0U},
        {"com_util_fflush", []() { (void)com_util_fflush(NULL, NULL); }, -1, 0U},
        {"com_util_fread", []() { (void)com_util_fread(NULL, 1U, 1U, NULL, NULL); }, 1, 0U},
        {"com_util_fwrite", []() { (void)com_util_fwrite(NULL, 1U, 1U, NULL, NULL); }, 1, 0U},
        {"com_util_remove", []() { (void)com_util_remove(NULL, NULL); }, 1, 0U},
        {"com_util_rename", []() { (void)com_util_rename(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_fopen_fmt", []() { (void)com_util_fopen_fmt(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_vfopen_fmt", []() { invoke_vfopen_fmt_with_null_detail(NULL); }, 1, 0U},
        {"com_util_remove_fmt", []() { (void)com_util_remove_fmt(NULL, NULL); }, 1, 0U},
        {"com_util_vremove_fmt", []() { invoke_vremove_fmt_with_null_detail(NULL); }, 1, 0U},
        {"com_util_fopen_temp", []() { (void)com_util_fopen_temp(NULL, NULL, NULL, 0U, NULL); }, 1, 0U},
        {"com_util_getenv", []() { (void)com_util_getenv(NULL, NULL, 0U, NULL, NULL); }, 1, 0U},
        {"com_util_setenv", []() { (void)com_util_setenv(NULL, NULL, 0, NULL); }, 1, 0U},
        {"com_util_unsetenv", []() { (void)com_util_unsetenv(NULL, NULL); }, 1, 0U},
        {"com_util_lseek", []() { (void)com_util_lseek(-1, 0, 0, NULL); }, 1, 0U},
        {"com_util_close", []() { (void)com_util_close(-1, NULL); }, 1, 0U},
        {"com_util_dup", []() { (void)com_util_dup(-1, NULL); }, 1, 0U},
        {"com_util_dup2", []() { (void)com_util_dup2(-1, -1, NULL); }, 1, 0U},
        {"com_util_read", []() { (void)com_util_read(-1, NULL, 1U, NULL); }, 1, 0U},
        {"com_util_write", []() { (void)com_util_write(-1, NULL, 1U, NULL); }, 1, 0U},
        {"com_util_access", []() { (void)com_util_access(NULL, 0, NULL); }, 1, 0U},
        {"com_util_access_fmt", []() { (void)com_util_access_fmt(0, NULL, NULL); }, 1, 0U},
        {"com_util_vaccess_fmt", []() { invoke_vaccess_fmt_with_null_detail(NULL); }, 1, 0U},
        {"com_util_stat", []() { (void)com_util_stat(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_mkdir", []() { (void)com_util_mkdir(NULL, NULL); }, 1, 0U},
        {"com_util_makedirs", []() { (void)com_util_makedirs(NULL, NULL); }, 1, 0U},
        {"com_util_rmdir", []() { (void)com_util_rmdir(NULL, NULL); }, 1, 0U},
        {"com_util_stat_fmt", []() { (void)com_util_stat_fmt(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_vstat_fmt", []() { invoke_vstat_fmt_with_null_detail(NULL); }, 1, 0U},
        {"com_util_mkdir_fmt", []() { (void)com_util_mkdir_fmt(NULL, NULL); }, 1, 0U},
        {"com_util_vmkdir_fmt", []() { invoke_vmkdir_fmt_with_null_detail(NULL); }, 1, 0U},
        {"com_util_mmap_attach",
         []() { (void)com_util_mmap_attach(NULL, COM_UTIL_MMAP_ACCESS_READ_ONLY, 0U, NULL, NULL); }, 1, 0U},
        {"com_util_mmap_get_rwlock", []() { (void)com_util_mmap_get_rwlock(NULL, NULL, NULL); }, 1, 0U},
        {"com_util_mmap_flush", []() { (void)com_util_mmap_flush(NULL, NULL, 0U, NULL); }, 1, 0U},
        {"com_util_mmap_detach", []() { (void)com_util_mmap_detach(NULL, NULL); }, 0, 0U},
    }; // [状態] - detail_out に NULL を指定する公開関数 56 件と公開マクロ 2 件を用意する。
    std::vector<int> tls_set_results;

    // Pre-Assert

    // Act
    for (const detail_out_null_case &item : cases)
    {
        com_util_error last_error;

        com_util_error_clear_last();
        item.invoke();
        com_util_error_get_last(&last_error);
        tls_set_results.push_back(com_util_error_is_set(&last_error));
    } // [手順] - 全公開 API の detail_out に NULL を指定し、呼び出し後の TLS を取得する。

    // Assert
    EXPECT_EQ(cases.size(), tls_set_results.size()); // [確認_正常系] - 全 58 件の公開 API 呼び出しが完了したこと。
    for (std::size_t index = 0U; index < cases.size(); ++index)
    {
        if (cases[index].expected_tls_set >= 0)
        {
            // [確認_正常系] - 各公開 API が NULL の detail_out を参照せず、TLS を規約どおり更新すること。
            EXPECT_EQ(cases[index].expected_tls_set, tls_set_results[index]) << cases[index].name;
        }
    }
}

// 実 API が失敗を記録し成功でクリアすることの確認
TEST_F(errorContractTest, real_api_records_failure_and_clears_it_on_success)
{
    // Arrange
    char temp_path[PLATFORM_PATH_MAX] = {};
    com_util_error failure_error;
    com_util_error success_error;

    (void)com_util_remove("com_util_error_tls_missing_file",
                          NULL); // [状態] - 失敗対象のファイルが存在しない状態にする。

    // Pre-Assert

    // Act
    FILE *missing =
        com_util_fopen("com_util_error_tls_missing_file", "rb", NULL); // [手順] - 詳細エラー出力なしで失敗させる。
    com_util_error_get_last(&failure_error);                           // [手順] - 失敗直後の TLS 詳細エラーを取得する。
    FILE *existing = com_util_fopen_temp("err", "w+b", temp_path, sizeof(temp_path),
                                         NULL); // [手順] - 次の詳細エラー記録対象 API を成功させる。
    com_util_error_get_last(&success_error);    // [手順] - 成功直後の TLS 詳細エラーを取得する。

    // Assert
    EXPECT_EQ((FILE *)NULL, missing); // [確認_異常系] - 存在しないファイルに対する com_util_fopen が NULL を返すこと。
    EXPECT_EQ(
        1, com_util_error_is(&failure_error,
                             COM_UTIL_CAUSE_NOT_FOUND)); // [確認_異常系] - 失敗直後の TLS 要因が NOT_FOUND であること。
    ASSERT_NE((FILE *)NULL, existing); // [確認_正常系] - 存在するファイルに対する com_util_fopen が成功すること。
    EXPECT_EQ(0, com_util_error_is_set(&success_error)); // [確認_正常系] - 成功直後の TLS 詳細エラーが空であること。

    // Cleanup
    (void)fclose(existing);
    (void)com_util_remove(temp_path, NULL);
}

// ネストした成功が直前の失敗をクリアすることの確認
TEST_F(errorContractTest, nested_paths_equal_success_clears_previous_failure)
{
    // Arrange
    int equal = 0;
    com_util_error last_error;

    (void)com_util_fopen("com_util_error_tls_missing_file", "rb", NULL); // [状態] - TLS へ失敗を記録する。

    // Pre-Assert

    // Act
    const int result =
        com_util_paths_equal(".", ".", &equal, NULL); // [手順] - 内部で 2 回絶対パス化する比較を成功させる。
    com_util_error_get_last(&last_error);             // [手順] - 比較成功直後の TLS 詳細エラーを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);   // [確認_正常系] - com_util_paths_equal の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1, equal); // [確認_正常系] - 同じパスの比較結果が一致であること。
    EXPECT_EQ(0,
              com_util_error_is_set(&last_error)); // [確認_正常系] - ネストした成功後の TLS 詳細エラーが空であること。
}

// スレッド間で last error が独立することの確認
TEST_F(errorContractTest, last_error_is_isolated_between_threads)
{
    // Arrange
    tls_thread_case not_found_case = {{COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL}, COM_UTIL_CAUSE_NOT_FOUND, 0};
    tls_thread_case invalid_case = {{COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL}, COM_UTIL_CAUSE_INVALID_ARGUMENT, 0};
    com_util_thread *not_found_thread = NULL;
    com_util_thread *invalid_thread = NULL;
    com_util_error main_error;
    int join_not_found_result = COM_UTIL_ERR_UNKNOWN;
    int join_invalid_result = COM_UTIL_ERR_UNKNOWN;

    (void)com_util_remove("com_util_error_tls_missing_file",
                          NULL); // [状態] - NOT_FOUND 用のファイルが存在しない状態にする。
    com_util_error_clear_last(); // [状態] - メイン スレッドの TLS 詳細エラーを空にする。

    // Pre-Assert

    // Act
    const int create_not_found_result =
        com_util_thread_create(&not_found_thread, record_thread_local_error,
                               &not_found_case); // [手順] - NOT_FOUND を記録するスレッドを起動する。
    const int create_invalid_result =
        com_util_thread_create(&invalid_thread, record_thread_local_error,
                               &invalid_case); // [手順] - INVALID_ARGUMENT を記録するスレッドを起動する。
    if (create_not_found_result == COM_UTIL_OK)
    {
        join_not_found_result = com_util_thread_join(
            not_found_thread, COM_UTIL_SYNC_WAIT_FOREVER); // [手順] - NOT_FOUND スレッドを待機する。
    }
    if (create_invalid_result == COM_UTIL_OK)
    {
        join_invalid_result = com_util_thread_join(
            invalid_thread, COM_UTIL_SYNC_WAIT_FOREVER); // [手順] - INVALID_ARGUMENT スレッドを待機する。
    }
    com_util_error_get_last(&main_error); // [手順] - 子スレッド終了後にメイン スレッドの TLS 詳細エラーを取得する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        create_not_found_result); // [確認_正常系] - 1 つ目の com_util_thread_create の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        create_invalid_result); // [確認_正常系] - 2 つ目の com_util_thread_create の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        join_not_found_result); // [確認_正常系] - 1 つ目の com_util_thread_join の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              join_invalid_result); // [確認_正常系] - 2 つ目の com_util_thread_join の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1,
              not_found_case.call_completed); // [確認_正常系] - 1 つ目のスレッドが詳細エラーの取得を完了したこと。
    EXPECT_EQ(1,
              invalid_case.call_completed); // [確認_正常系] - 2 つ目のスレッドが詳細エラーの取得を完了したこと。
    EXPECT_EQ(1, com_util_error_is(
                     &not_found_case.observed_error,
                     COM_UTIL_CAUSE_NOT_FOUND)); // [確認_正常系] - 1 つ目のスレッドが NOT_FOUND だけを取得すること。
    EXPECT_EQ(
        1,
        com_util_error_is(
            &invalid_case.observed_error,
            COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_正常系] - 2 つ目のスレッドが INVALID_ARGUMENT だけを取得すること。
    EXPECT_EQ(0, com_util_error_is_set(
                     &main_error)); // [確認_正常系] - メイン スレッドの TLS 詳細エラーが空のままであること。
}