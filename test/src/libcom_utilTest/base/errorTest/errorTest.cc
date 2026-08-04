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
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_trivially_copyable<com_util_error>::value, "com_util_error must be trivially copyable");
static_assert(COM_UTIL_ERROR_DOMAIN_NONE == 0, "domain values are part of the ABI");
static_assert(COM_UTIL_ERROR_DOMAIN_ERRNO == 1, "domain values are part of the ABI");
static_assert(COM_UTIL_ERROR_DOMAIN_WINDOWS == 2, "domain values are part of the ABI");
static_assert(COM_UTIL_CAUSE_NONE == 0, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_OTHER == 1, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_NOT_FOUND == 2, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_ALREADY_EXISTS == 3, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_ACCESS_DENIED == 4, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_SHARING_VIOLATION == 5, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_NOT_A_DIRECTORY == 6, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_IS_A_DIRECTORY == 7, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_DIRECTORY_NOT_EMPTY == 8, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_NAME_TOO_LONG == 9, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_INVALID_ARGUMENT == 10, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_OUT_OF_MEMORY == 11, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_DISK_FULL == 12, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_BUSY == 13, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_TIMEOUT == 14, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_INTERRUPTED == 15, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_BROKEN_PIPE == 16, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_TOO_MANY_OPEN_FILES == 17, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_READ_ONLY == 18, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_BUFFER_TOO_SMALL == 19, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_UNSUPPORTED == 20, "cause values are part of the ABI");
static_assert(COM_UTIL_CAUSE_IO_ERROR == 21, "new cause values must be appended");

typedef struct tls_thread_case
{
    com_util_error observed_error;
    com_util_error_cause_t requested_cause;
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

class errorTest : public Test
{
};

// detail_out を持つ全公開 API が NULL を受け付け、TLS を規約どおり更新することの確認
TEST_F(errorTest, all_detail_out_apis_accept_null)
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
    EXPECT_EQ(cases.size(), tls_set_results.size()); // [確認_正常系] - 全58件の公開 API 呼び出しが完了したこと。
    for (std::size_t index = 0U; index < cases.size(); ++index)
    {
        if (cases[index].expected_tls_set >= 0)
        {
            // [確認_正常系] - 各公開 API が NULL の detail_out を参照せず、TLS を規約どおり更新すること。
            EXPECT_EQ(cases[index].expected_tls_set, tls_set_results[index]) << cases[index].name;
        }
    }
}

// errno の取り込みと参照 API の確認
TEST_F(errorTest, capture_errno_preserves_domain_result_and_code)
{
    // Arrange
    com_util_error error;

    com_util_error_clear(&error); // [状態] - 詳細エラーを空の値で初期化する。

    // Pre-Assert
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              error.domain); // [確認_事前条件] - 初期化後の domain が COM_UTIL_ERROR_DOMAIN_NONE であること。

    // Act
    com_util_error_capture_errno(&error, ENOENT); // [手順] - ENOENT を詳細エラーへ取り込む。

    // Assert
    EXPECT_EQ(1, com_util_error_is_set(&error)); // [確認_正常系] - com_util_error_is_set の戻り値が 1 であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_ERRNO,
              com_util_error_get_domain(&error)); // [確認_正常系] - domain が errno であること。
    EXPECT_EQ(
        ENOENT,
        com_util_error_get_errno(&error)); // [確認_正常系] - com_util_error_get_errno の戻り値が ENOENT であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              com_util_error_to_result(&error)); // [確認_正常系] - 共通結果コードが errno から変換した値であること。
    EXPECT_EQ(COM_UTIL_CAUSE_NOT_FOUND,
              com_util_error_get_cause(&error)); // [確認_正常系] - ENOENT の要因が NOT_FOUND であること。
    EXPECT_EQ(1,
              com_util_error_is(&error,
                                COM_UTIL_CAUSE_NOT_FOUND)); // [確認_正常系] - NOT_FOUND との一致判定が 1 であること。
}

// 現在の errno を呼び出し側で退避せずに取り込めることの確認
TEST_F(errorTest, capture_current_errno_preserves_current_value)
{
    // Arrange
    com_util_error error;

    errno = ENOENT; // [状態] - 現在の errno を ENOENT に設定する。

    // Pre-Assert

    // Act
    com_util_error_capture_current_errno(&error); // [手順] - 現在の errno を詳細エラーへ取り込む。

    // Assert
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_ERRNO,
              com_util_error_get_domain(&error)); // [確認_正常系] - error のドメインが errno であること。
    EXPECT_EQ(ENOENT, com_util_error_get_errno(
                          &error)); // [確認_正常系] - com_util_error_get_errno の戻り値が ENOENT であること。
}

// 保存済みの詳細エラーによる TLS の更新とクリアの確認
TEST_F(errorTest, set_last_copies_saved_error_and_null_clears_it)
{
    // Arrange
    const com_util_error saved_error = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_UNKNOWN, ENOENT};
    com_util_error copied_error;
    com_util_error cleared_error;

    com_util_error_clear_last(); // [状態] - 現在のスレッドの TLS 詳細エラーを空にする。

    // Pre-Assert

    // Act
    com_util_error_set_last(&saved_error);   // [手順] - 保存済みの詳細エラーを現在のスレッドの TLS へ設定する。
    com_util_error_get_last(&copied_error);  // [手順] - 設定後の TLS 詳細エラーを取得する。
    com_util_error_set_last(NULL);           // [手順] - NULL を指定して現在のスレッドの TLS をクリアする。
    com_util_error_get_last(&cleared_error); // [手順] - クリア後の TLS 詳細エラーを取得する。

    // Assert
    EXPECT_EQ(saved_error.domain,
              copied_error.domain); // [確認_正常系] - 設定後の TLS に保存済みの domain がコピーされること。
    EXPECT_EQ(saved_error.result,
              copied_error.result); // [確認_正常系] - 設定後の TLS に保存済みの result がコピーされること。
    EXPECT_EQ(saved_error.code,
              copied_error.code); // [確認_正常系] - 設定後の TLS に保存済みの code がコピーされること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              cleared_error.domain); // [確認_正常系] - NULL 指定後の TLS の domain が空であること。
    EXPECT_EQ(COM_UTIL_OK,
              cleared_error.result);    // [確認_正常系] - NULL 指定後の TLS の result が COM_UTIL_OK であること。
    EXPECT_EQ(0UL, cleared_error.code); // [確認_正常系] - NULL 指定後の TLS の code が 0 であること。
}

// NULL、空の値、ドメイン不一致に対する参照 API の確認
TEST_F(errorTest, accessors_reject_null_empty_and_mismatched_domain)
{
    // Arrange
    com_util_error empty;
    com_util_error windows_error = {COM_UTIL_ERROR_DOMAIN_WINDOWS, COM_UTIL_ERR_UNKNOWN, 5UL};

    com_util_error_capture_errno(&empty, 0); // [状態] - errno 0 を取り込んで空の値を作る。

    // Pre-Assert

    // Act
    com_util_error_clear(NULL);                          // [手順] - NULL の詳細エラーをクリアする。
    com_util_error_capture_errno(NULL, ENOENT);          // [手順] - NULL の格納先へ errno を取り込む。
    com_util_error_get_last(NULL);                       // [手順] - NULL の格納先へ TLS の値を取得する。
    const int null_is_set = com_util_error_is_set(NULL); // [手順] - NULL の設定状態を取得する。
    const com_util_error_domain_t null_domain = com_util_error_get_domain(NULL); // [手順] - NULL のドメインを取得する。
    const int null_errno = com_util_error_get_errno(NULL);  // [手順] - NULL から errno を取得する。
    const int null_result = com_util_error_to_result(NULL); // [手順] - NULL を共通結果コードへ変換する。
    const com_util_error_cause_t null_cause = com_util_error_get_cause(NULL); // [手順] - NULL の要因を取得する。
    const int null_matches = com_util_error_is(NULL, COM_UTIL_CAUSE_NONE);    // [手順] - NULL の要因一致を判定する。
    const int mismatched_errno =
        com_util_error_get_errno(&windows_error); // [手順] - Windows ドメインから errno を取得する。

    // Assert
    EXPECT_EQ(0, null_is_set); // [確認_異常系] - NULL に対する設定状態が 0 であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              null_domain);   // [確認_異常系] - NULL に対するドメインが COM_UTIL_ERROR_DOMAIN_NONE であること。
    EXPECT_EQ(0, null_errno); // [確認_異常系] - NULL から取得した errno が 0 であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_result); // [確認_異常系] - NULL に対する変換結果が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_CAUSE_NONE,
              null_cause);      // [確認_異常系] - NULL に対する要因が COM_UTIL_CAUSE_NONE であること。
    EXPECT_EQ(0, null_matches); // [確認_異常系] - NULL に対する要因一致が 0 であること。
    EXPECT_EQ(COM_UTIL_OK,
              com_util_error_to_result(&empty)); // [確認_正常系] - 空の値に対する変換結果が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_CAUSE_NONE,
              com_util_error_get_cause(&empty)); // [確認_正常系] - 空の値の要因が COM_UTIL_CAUSE_NONE であること。
    EXPECT_EQ(0, mismatched_errno);              // [確認_異常系] - Windows ドメインから取得した errno が 0 であること。
}

// errno と共通要因の排他的な対応の確認
TEST_F(errorTest, errno_values_map_to_one_cause)
{
    // Arrange
    const std::vector<std::pair<int, com_util_error_cause_t>> cases = {{ENOENT, COM_UTIL_CAUSE_NOT_FOUND},
                                                                       {EEXIST, COM_UTIL_CAUSE_ALREADY_EXISTS},
                                                                       {EACCES, COM_UTIL_CAUSE_ACCESS_DENIED},
                                                                       {ENOTDIR, COM_UTIL_CAUSE_NOT_A_DIRECTORY},
                                                                       {EISDIR, COM_UTIL_CAUSE_IS_A_DIRECTORY},
                                                                       {ENOTEMPTY, COM_UTIL_CAUSE_DIRECTORY_NOT_EMPTY},
                                                                       {ENAMETOOLONG, COM_UTIL_CAUSE_NAME_TOO_LONG},
                                                                       {EINVAL, COM_UTIL_CAUSE_INVALID_ARGUMENT},
                                                                       {ENOMEM, COM_UTIL_CAUSE_OUT_OF_MEMORY},
                                                                       {ENOSPC, COM_UTIL_CAUSE_DISK_FULL},
                                                                       {EBUSY, COM_UTIL_CAUSE_BUSY},
                                                                       {ETIMEDOUT, COM_UTIL_CAUSE_TIMEOUT},
                                                                       {EINTR, COM_UTIL_CAUSE_INTERRUPTED},
                                                                       {EPIPE, COM_UTIL_CAUSE_BROKEN_PIPE},
                                                                       {EMFILE, COM_UTIL_CAUSE_TOO_MANY_OPEN_FILES},
                                                                       {EROFS, COM_UTIL_CAUSE_READ_ONLY},
                                                                       {ERANGE, COM_UTIL_CAUSE_BUFFER_TOO_SMALL},
                                                                       {ENOTSUP, COM_UTIL_CAUSE_UNSUPPORTED},
                                                                       {EIO, COM_UTIL_CAUSE_IO_ERROR}};
    com_util_error error;
    std::vector<com_util_error_cause_t> actual_causes;
    std::vector<int> actual_matches;

    // Pre-Assert

    // Act
    for (const std::pair<int, com_util_error_cause_t> &item : cases)
    {
        com_util_error_capture_errno(&error, item.first); // [手順] - 各 errno を順番に詳細エラーへ取り込む。
        actual_causes.push_back(com_util_error_get_cause(&error));
        actual_matches.push_back(com_util_error_is(&error, item.second));
    }

    // Assert
    for (std::size_t index = 0U; index < cases.size(); ++index)
    {
        EXPECT_EQ(cases[index].second,
                  actual_causes[index]); // [確認_正常系] - 各 errno が対応する単一の要因へ変換されること。
        EXPECT_EQ(1,
                  actual_matches[index]); // [確認_正常系] - 対応する要因との一致判定が 1 であること。
    }
}

// 対応表にない errno の確認
TEST_F(errorTest, unknown_errno_maps_to_other)
{
    // Arrange
    com_util_error error;

    // Pre-Assert

    // Act
    com_util_error_capture_errno(&error, EDOM); // [手順] - 対応表にない EDOM を詳細エラーへ取り込む。

    // Assert
    EXPECT_EQ(COM_UTIL_CAUSE_OTHER,
              com_util_error_get_cause(&error)); // [確認_正常系] - 対応表にない errno の要因が OTHER であること。
}

// 詳細エラーの文字列化の確認
TEST_F(errorTest, error_message_dispatches_by_domain)
{
    // Arrange
    char buf[256];
    com_util_error error;

    memset(buf, 0, sizeof(buf)); // [状態] - 文字列の格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    com_util_error_clear(&error); // [手順] - 空の詳細エラーを文字列化する。
    const int none_result = com_util_error_message(buf, sizeof(buf), &error);
    const std::string none_message(buf);
    com_util_error_capture_errno(&error, ENOENT); // [手順] - errno ドメインの詳細エラーを文字列化する。
    const int errno_result = com_util_error_message(buf, sizeof(buf), &error);
    const std::string errno_message(buf);

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        none_result); // [確認_正常系] - 空の詳細エラーに対する com_util_error_message の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ("no error", none_message); // [確認_正常系] - 空の詳細エラーのメッセージが "no error" であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        errno_result); // [確認_正常系] - errno ドメインに対する com_util_error_message の戻り値が COM_UTIL_OK であること。
    EXPECT_FALSE(errno_message.empty()); // [確認_正常系] - errno ドメインのメッセージが空でないこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_error_message(NULL, sizeof(buf),
                               &error)); // [確認_異常系] - NULL の格納先が COM_UTIL_ERR_INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_error_message(buf, 0U,
                                     &error)); // [確認_異常系] - サイズ 0 が COM_UTIL_ERR_INVALID_ARGUMENT になること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_error_message(buf, sizeof(buf),
                               NULL)); // [確認_異常系] - NULL の詳細エラーが COM_UTIL_ERR_INVALID_ARGUMENT になること。
}

// 詳細エラー出力が NULL の場合も失敗を TLS へ記録し、次の成功でクリアすることの確認
TEST_F(errorTest, real_api_records_failure_and_clears_it_on_success)
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

// 内部で複数のパス API を呼ぶ成功ケースが TLS を空にすることの確認
TEST_F(errorTest, nested_paths_equal_success_clears_previous_failure)
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

// 2 スレッドの詳細エラーが互いに分離されることの確認
TEST_F(errorTest, last_error_is_isolated_between_threads)
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
