#include <testfw.h>
#include <com_util/base/error.h>
#include <com_util/base/error_internal.h>
#include <com_util/base/result.h>

#include <errno.h>

#if defined(PLATFORM_LINUX)
    #include <netdb.h>
#endif

#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

/* 値はライブラリの ABI の一部であり、既存の値を変更してはならない */
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

class errorTest : public Test
{
};

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
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              com_util_error_to_result(&error)); // [確認_正常系] - 共通結果コードが errno から変換した値であること。
    EXPECT_EQ(COM_UTIL_CAUSE_NOT_FOUND,
              com_util_error_get_cause(&error)); // [確認_正常系] - ENOENT の要因が NOT_FOUND であること。
    EXPECT_EQ(1,
              com_util_error_is(&error,
                                COM_UTIL_CAUSE_NOT_FOUND)); // [確認_正常系] - NOT_FOUND との一致判定が 1 であること。
}

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

TEST_F(errorTest, set_last_copies_saved_error_and_null_clears_it)
{
    // Arrange
    const com_util_error saved_error = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_NOT_FOUND, ENOENT};
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

TEST_F(errorTest, accessors_reject_null_empty_and_mismatched_domain)
{
    // Arrange
    com_util_error empty;
    com_util_error windows_error = {COM_UTIL_ERROR_DOMAIN_WINDOWS, COM_UTIL_ERR_UNKNOWN, 5UL};
    com_util_error invalid_error = {COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL};
    const int invalid_domain_value = 99;

    com_util_error_capture_errno(&empty, 0); // [状態] - errno 0 を取り込んで空の値を作る。
    std::memcpy(&invalid_error.domain, &invalid_domain_value,
                sizeof(invalid_error.domain)); // [状態] - 未知のドメイン値を持つ不正な詳細エラーを用意する。

    // Pre-Assert

    // Act
    com_util_error_clear(NULL);                          // [手順] - NULL の詳細エラーをクリアする。
    com_util_error_capture_errno(NULL, ENOENT);          // [手順] - NULL の格納先へ errno を取り込む。
    com_util_error_get_last(NULL);                       // [手順] - NULL の格納先へ TLS の値を取得する。
    const int null_is_set = com_util_error_is_set(NULL); // [手順] - NULL の設定状態を取得する。
    const com_util_error_domain null_domain = com_util_error_get_domain(NULL); // [手順] - NULL のドメインを取得する。
    const int null_errno = com_util_error_get_errno(NULL);                     // [手順] - NULL から errno を取得する。
    const int null_result = com_util_error_to_result(NULL); // [手順] - NULL を共通結果コードへ変換する。
    const com_util_error_cause null_cause = com_util_error_get_cause(NULL); // [手順] - NULL の要因を取得する。
    const int null_matches = com_util_error_is(NULL, COM_UTIL_CAUSE_NONE);  // [手順] - NULL の要因一致を判定する。
    const com_util_error_domain mismatched_domain =
        com_util_error_get_domain(&windows_error); // [手順] - Windows ドメインを取得する。
    const com_util_error_cause windows_cause =
        com_util_error_get_cause(&windows_error); // [手順] - Windows ドメインの要因を取得する。
    const com_util_error_domain invalid_domain =
        com_util_error_get_domain(&invalid_error); // [手順] - 未知のドメインを取得する。
    const com_util_error_cause invalid_cause =
        com_util_error_get_cause(&invalid_error); // [手順] - 未知のドメインの要因を取得する。
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
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_WINDOWS,
              mismatched_domain); // [確認_正常系] - Windows ドメインがそのまま取得できること。
#if defined(PLATFORM_LINUX)
    EXPECT_EQ(COM_UTIL_CAUSE_OTHER,
              windows_cause); // [確認_正常系] - Linux で Windows ドメイン エラー コード 5 の要因が OTHER になること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_EQ(COM_UTIL_CAUSE_ACCESS_DENIED,
              windows_cause); // [確認_正常系] - Windows で Windows ドメイン エラー コード 5 の要因が ACCESS_DENIED になること。
#endif /* PLATFORM_ */
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              invalid_domain); // [確認_異常系] - 未知のドメインが NONE へ正規化されること。
    EXPECT_EQ(COM_UTIL_CAUSE_OTHER,
              invalid_cause);       // [確認_異常系] - 未知のドメインの要因が OTHER になること。
    EXPECT_EQ(0, mismatched_errno); // [確認_異常系] - Windows ドメインから取得した errno が 0 であること。
}

TEST_F(errorTest, errno_values_map_to_one_cause)
{
    // Arrange
    const std::vector<std::pair<int, com_util_error_cause>> cases = {{ENOENT, COM_UTIL_CAUSE_NOT_FOUND},
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
    std::vector<com_util_error_cause> actual_causes;
    std::vector<int> actual_matches;

    // Pre-Assert

    // Act
    for (const std::pair<int, com_util_error_cause> &item : cases)
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

#if defined(ENOSYS)
// ENOSYS が UNSUPPORTED の要因へ変換されることの確認
TEST_F(errorTest, enosys_maps_to_unsupported)
{
    // Arrange
    com_util_error error;

    // Pre-Assert

    // Act
    com_util_error_capture_errno(&error, ENOSYS); // [手順] - ENOSYS を詳細エラーへ取り込む。

    // Assert
    EXPECT_EQ(COM_UTIL_CAUSE_UNSUPPORTED,
              com_util_error_get_cause(&error)); // [確認_正常系] - ENOSYS の要因が UNSUPPORTED であること。
}
#endif

// errno の成功値と明示結果コードが詳細エラーへ記録されることの確認
TEST_F(errorTest, report_errno_records_success_and_explicit_result)
{
    // Arrange
    com_util_error detail;
    com_util_error last_error;

    // Pre-Assert

    // Act
    int success_result = com_util_error_report_errno(&detail, 0); // [手順] - errno 0 を成功として記録する。
    int mapped_result =
        com_util_error_report_errno(&detail, ENOENT); // [手順] - ENOENT を対応する結果コードへ変換して記録する。
    int explicit_result = com_util_error_report_errno_as(&detail, EIO, COM_UTIL_ERR_BUSY);
    // [手順] - EIO に対して明示した COM_UTIL_ERR_BUSY を記録する。
    com_util_error_get_last(&last_error); // [手順] - 最後に記録された詳細エラーを取得する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        success_result); // [確認_正常系] - errno 0 を指定した com_util_error_report_errno の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_ERR_NOT_FOUND,
        mapped_result); // [確認_正常系] - ENOENT を指定した com_util_error_report_errno の戻り値が COM_UTIL_ERR_NOT_FOUND であること。
    EXPECT_EQ(
        COM_UTIL_ERR_BUSY,
        explicit_result); // [確認_正常系] - 明示結果を指定した com_util_error_report_errno_as の戻り値が COM_UTIL_ERR_BUSY であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_ERRNO,
              detail.domain); // [確認_正常系] - 明示結果の詳細エラーが errno ドメインであること。
    EXPECT_EQ(EIO, com_util_error_get_errno(&detail)); // [確認_正常系] - 詳細エラーへ EIO が記録されること。
    EXPECT_EQ(
        COM_UTIL_ERR_BUSY,
        com_util_error_to_result(&last_error)); // [確認_正常系] - TLS の結果コードが COM_UTIL_ERR_BUSY であること。
}

// 成功報告が詳細エラーと TLS の双方をクリアすることの確認
TEST_F(errorTest, report_success_clears_detail_and_last_error)
{
    // Arrange
    com_util_error detail = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_UNKNOWN, EIO};
    com_util_error last_error;

    // Pre-Assert

    // Act
    int result = com_util_error_report_success(&detail); // [手順] - 詳細エラーを成功状態へ更新する。
    com_util_error_get_last(&last_error);                // [手順] - 更新後の TLS 詳細エラーを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_error_report_success の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              detail.domain); // [確認_正常系] - 出力詳細エラーのドメインが空であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              last_error.domain); // [確認_正常系] - TLS 詳細エラーのドメインが空であること。
}

// ソケット errno が待機要因と通常要因を区別して記録されることの確認
TEST_F(errorTest, report_socket_errno_uses_socket_domain_and_would_block_cause)
{
    // Arrange
    com_util_error error;
    com_util_error last_error;

    // Pre-Assert

    // Act
    int blocked_result = com_util_error_report_socket_errno(&error, EAGAIN); // [手順] - EAGAIN をソケット errno として記録する。
    const com_util_error_domain blocked_domain = com_util_error_get_domain(&error); // [手順] - EAGAIN の記録ドメインを取得する。
    const com_util_error_cause blocked_cause = com_util_error_get_cause(&error); // [手順] - ソケット EAGAIN の要因を取得する。
    int success_result = com_util_error_report_socket_errno(&error, 0); // [手順] - errno 0 をソケット成功として記録する。
    com_util_error_get_last(&last_error); // [手順] - ソケット成功後の TLS 詳細エラーを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUSY, blocked_result); // [確認_正常系] - ソケット EAGAIN の戻り値が COM_UTIL_ERR_BUSY であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO, blocked_domain); // [確認_正常系] - EAGAIN の記録ドメインが SOCKET_ERRNO であること。
    EXPECT_EQ(COM_UTIL_CAUSE_WOULD_BLOCK, blocked_cause); // [確認_正常系] - ソケット EAGAIN の要因が WOULD_BLOCK であること。
    EXPECT_EQ(COM_UTIL_OK, success_result); // [確認_正常系] - ソケット errno 0 の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE, last_error.domain); // [確認_正常系] - ソケット成功後の TLS ドメインが NONE であること。
}

// getaddrinfo のエラー コードが要因と結果へ分類されることの確認
TEST_F(errorTest, report_gai_error_maps_standard_codes_and_unknown_code)
{
    // Arrange
    com_util_error error;

    // Pre-Assert

    // Act
#if defined(PLATFORM_LINUX)
    int not_found_result = com_util_error_report_gai_error(&error, EAI_NONAME); // [手順] - EAI_NONAME を記録する。
    const com_util_error_cause not_found_cause = com_util_error_get_cause(&error); // [手順] - EAI_NONAME の要因を取得する。
    int again_result = com_util_error_report_gai_error(&error, EAI_AGAIN); // [手順] - EAI_AGAIN を記録する。
    const com_util_error_cause again_cause = com_util_error_get_cause(&error); // [手順] - EAI_AGAIN の要因を取得する。
    int memory_result = com_util_error_report_gai_error(&error, EAI_MEMORY); // [手順] - EAI_MEMORY を記録する。
    const com_util_error_cause memory_cause = com_util_error_get_cause(&error); // [手順] - EAI_MEMORY の要因を取得する。
    int family_result = com_util_error_report_gai_error(&error, EAI_FAMILY); // [手順] - EAI_FAMILY を記録する。
    const com_util_error_cause family_cause = com_util_error_get_cause(&error); // [手順] - EAI_FAMILY の要因を取得する。
    int flags_result = com_util_error_report_gai_error(&error, EAI_BADFLAGS); // [手順] - EAI_BADFLAGS を記録する。
    const com_util_error_cause flags_cause = com_util_error_get_cause(&error); // [手順] - EAI_BADFLAGS の要因を取得する。
    int unknown_result = com_util_error_report_gai_error(&error, -9999); // [手順] - 未知の EAI 値を記録する。
    const com_util_error_cause unknown_cause = com_util_error_get_cause(&error); // [手順] - 未知の EAI 値の要因を取得する。
    int success_result = com_util_error_report_gai_error(&error, 0); // [手順] - EAI 0 を成功として記録する。
#else
    int success_result = COM_UTIL_OK;
#endif

    // Assert
#if defined(PLATFORM_LINUX)
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, not_found_result); // [確認_正常系] - EAI_NONAME の戻り値が COM_UTIL_ERR_NOT_FOUND であること。
    EXPECT_EQ(COM_UTIL_CAUSE_NOT_FOUND, not_found_cause); // [確認_正常系] - EAI_NONAME の要因が NOT_FOUND であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, again_result); // [確認_正常系] - EAI_AGAIN の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_CAUSE_BUSY, again_cause); // [確認_正常系] - EAI_AGAIN の要因が BUSY であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, memory_result); // [確認_正常系] - EAI_MEMORY の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_CAUSE_OUT_OF_MEMORY, memory_cause); // [確認_正常系] - EAI_MEMORY の要因が OUT_OF_MEMORY であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, family_result); // [確認_正常系] - EAI_FAMILY の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_CAUSE_UNSUPPORTED, family_cause); // [確認_正常系] - EAI_FAMILY の要因が UNSUPPORTED であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, flags_result); // [確認_正常系] - EAI_BADFLAGS の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_CAUSE_INVALID_ARGUMENT, flags_cause); // [確認_正常系] - EAI_BADFLAGS の要因が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, unknown_result); // [確認_異常系] - 未知の EAI 値の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_CAUSE_OTHER, unknown_cause); // [確認_異常系] - 未知の EAI 値の要因が OTHER であること。
#endif
    EXPECT_EQ(COM_UTIL_OK, success_result); // [確認_正常系] - EAI 0 の戻り値が COM_UTIL_OK であること。
}

// 詳細エラーの全ドメインと errno の追加分類が取得できることの確認
TEST_F(errorTest, accessors_cover_socket_gai_and_extended_errno_causes)
{
    // Arrange
    const std::vector<std::pair<int, com_util_error_cause>> cases = {
#if defined(EINPROGRESS)
        {EINPROGRESS, COM_UTIL_CAUSE_IN_PROGRESS},
#endif
#if defined(ECONNREFUSED)
        {ECONNREFUSED, COM_UTIL_CAUSE_CONNECTION_REFUSED},
#endif
#if defined(ECONNRESET)
        {ECONNRESET, COM_UTIL_CAUSE_CONNECTION_RESET},
#endif
#if defined(ECONNABORTED)
        {ECONNABORTED, COM_UTIL_CAUSE_CONNECTION_ABORTED},
#endif
#if defined(ENOTCONN)
        {ENOTCONN, COM_UTIL_CAUSE_NOT_CONNECTED},
#endif
#if defined(EISCONN)
        {EISCONN, COM_UTIL_CAUSE_ALREADY_CONNECTED},
#endif
#if defined(EADDRINUSE)
        {EADDRINUSE, COM_UTIL_CAUSE_ADDRESS_IN_USE},
#endif
#if defined(EADDRNOTAVAIL)
        {EADDRNOTAVAIL, COM_UTIL_CAUSE_ADDRESS_NOT_AVAILABLE},
#endif
#if defined(ENETDOWN)
        {ENETDOWN, COM_UTIL_CAUSE_NETWORK_DOWN},
#endif
#if defined(ENETUNREACH)
        {ENETUNREACH, COM_UTIL_CAUSE_NETWORK_UNREACHABLE},
#endif
#if defined(EHOSTUNREACH)
        {EHOSTUNREACH, COM_UTIL_CAUSE_HOST_UNREACHABLE},
#endif
#if defined(EMSGSIZE)
        {EMSGSIZE, COM_UTIL_CAUSE_MESSAGE_SIZE},
#endif
#if defined(ESHUTDOWN)
        {ESHUTDOWN, COM_UTIL_CAUSE_SHUTDOWN},
#endif
        {EAGAIN, COM_UTIL_CAUSE_BUSY},
        {EPERM, COM_UTIL_CAUSE_ACCESS_DENIED}};
    com_util_error error;

    // Pre-Assert

    // Act
    com_util_error_report_socket_errno(&error, EIO); // [手順] - EIO をソケット errno として記録する。
    const com_util_error_cause socket_io_cause = com_util_error_get_cause(&error); // [手順] - ソケット EIO の要因を取得する。
    com_util_error_report_gai_error(&error, 0); // [手順] - GAI 0 を記録する。
    const com_util_error_domain gai_domain = com_util_error_get_domain(&error); // [手順] - GAI 成功値のドメインを取得する。
    com_util_error winsock_error = {COM_UTIL_ERROR_DOMAIN_WINSOCK, COM_UTIL_ERR_UNKNOWN, 1UL};
    const com_util_error_cause winsock_cause = com_util_error_get_cause(&winsock_error); // [手順] - 非 Windows の WINSOCK ドメイン要因を取得する。
    std::vector<com_util_error_cause> causes;
    for (const std::pair<int, com_util_error_cause> &item : cases)
    {
        com_util_error_capture_errno(&error, item.first); // [手順] - 追加 errno を順番に取り込む。
        causes.push_back(com_util_error_get_cause(&error));
    }

    // Assert
    EXPECT_EQ(COM_UTIL_CAUSE_IO_ERROR, socket_io_cause); // [確認_正常系] - ソケット EIO の要因が IO_ERROR であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE, gai_domain); // [確認_正常系] - GAI 成功値のドメインが NONE であること。
    EXPECT_EQ(COM_UTIL_CAUSE_OTHER, winsock_cause); // [確認_正常系] - Linux の WINSOCK ドメイン要因が OTHER であること。
    ASSERT_EQ(cases.size(), causes.size());
    for (std::size_t index = 0U; index < cases.size(); ++index)
    {
        EXPECT_EQ(cases[index].second, causes[index]); // [確認_正常系] - 追加 errno が期待する要因へ分類されること。
    }
}

// errno 0 と非成功結果の組合せがエラー ドメインとして保持されることの確認
TEST_F(errorTest, report_errno_as_keeps_domain_when_result_is_not_success)
{
    // Arrange
    com_util_error error;

    // Pre-Assert

    // Act
    int result = com_util_error_report_errno_as(&error, 0, COM_UTIL_ERR_UNKNOWN); // [手順] - errno 0 と非成功結果を明示して記録する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, result); // [確認_正常系] - 明示した非成功結果がそのまま返ること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_ERRNO, error.domain); // [確認_正常系] - 非成功結果のドメインが ERRNO であること。
}
