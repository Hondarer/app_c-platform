#include <testfw.h>
#include <com_util/base/error.h>
#include <com_util/base/result.h>

#include <errno.h>

#include <type_traits>

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
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
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
    const com_util_error_domain null_domain = com_util_error_get_domain(NULL); // [手順] - NULL のドメインを取得する。
    const int null_errno = com_util_error_get_errno(NULL);                     // [手順] - NULL から errno を取得する。
    const int null_result = com_util_error_to_result(NULL); // [手順] - NULL を共通結果コードへ変換する。
    const com_util_error_cause null_cause = com_util_error_get_cause(NULL); // [手順] - NULL の要因を取得する。
    const int null_matches = com_util_error_is(NULL, COM_UTIL_CAUSE_NONE);  // [手順] - NULL の要因一致を判定する。
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
