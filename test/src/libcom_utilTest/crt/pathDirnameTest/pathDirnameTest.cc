#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/path.h>
#include <errno.h>
#include <string.h>

class pathDirnameTest : public Test
{
};

// 複数階層パスから親ディレクトリが取り出せることの確認
TEST_F(pathDirnameTest, returns_parent_of_nested_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc =
        com_util_path_dirname(actual, sizeof(actual), NULL,
                              "a/b/c"); // [手順] - com_util_path_dirname(actual, size, NULL, "a/b/c") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_path_dirname の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("a/b", actual); // [確認_正常系] - 親ディレクトリが "a/b" であること。
}

// セパレータを含まないパスで "." が返ることの確認
TEST_F(pathDirnameTest, returns_dot_when_no_separator)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), NULL,
                                    "c"); // [手順] - com_util_path_dirname(actual, size, NULL, "c") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_path_dirname の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ(".", actual);   // [確認_正常系] - "." が返ること。
}

// ルートのみのパスでルートが返ることの確認
TEST_F(pathDirnameTest, returns_root_for_root_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), NULL,
                                    "/"); // [手順] - com_util_path_dirname(actual, size, NULL, "/") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_path_dirname の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("/", actual);   // [確認_正常系] - "/" が返ること。
}

// ルート直下のパスでルートが返ることの確認
TEST_F(pathDirnameTest, returns_root_for_top_level_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc =
        com_util_path_dirname(actual, sizeof(actual), NULL,
                              "/name"); // [手順] - com_util_path_dirname(actual, size, NULL, "/name") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_path_dirname の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("/", actual);   // [確認_正常系] - "/" が返ること。
}

// 末尾セパレータが除去されて親ディレクトリが取れることの確認
TEST_F(pathDirnameTest, strips_trailing_separator_before_computing_parent)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), NULL,
                                    "a/b/"); // [手順] - com_util_path_dirname(actual, size, NULL, "a/b/") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_path_dirname の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("a", actual);   // [確認_正常系] - "a" が返ること。
}

// '\\' 区切りの入力が '/' 正規化されて返ることの確認
TEST_F(pathDirnameTest, normalizes_backslash_separator_in_output)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc =
        com_util_path_dirname(actual, sizeof(actual), NULL,
                              "a\\b\\c"); // [手順] - com_util_path_dirname(actual, size, NULL, "a\\b\\c") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_path_dirname の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("a/b", actual); // [確認_正常系] - '/' 正規化された "a/b" が返ること。
}

// NULL path_out で EINVAL が返ることの確認
TEST_F(pathDirnameTest, returns_einval_for_null_path_out)
{
    // Arrange
    com_util_error err; // [状態] - 詳細エラーの格納先を用意する。
    com_util_error last_error;

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(NULL, 16, &err,
                                    "a/b"); // [手順] - com_util_path_dirname(NULL, 16, &err, "a/b") を呼び出す。
    com_util_error_get_last(&last_error);   // [手順] - TLS に記録された詳細エラーを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_path_dirname の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(1, com_util_error_is(&err, COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_異常系] - EINVAL の要因であること。
    EXPECT_EQ(1, com_util_error_is_set(&last_error)); // [確認_異常系] - TLS に詳細エラーが記録されること。
}

// 空文字列パスで EINVAL が返ることの確認
TEST_F(pathDirnameTest, returns_einval_for_empty_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。
    com_util_error err;             // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), &err,
                                    ""); // [手順] - com_util_path_dirname(actual, size, &err, "") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_path_dirname の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(1, com_util_error_is(&err, COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_異常系] - EINVAL の要因であること。
}

// バッファー不足で ENAMETOOLONG が返ることの確認
TEST_F(pathDirnameTest, returns_enametoolong_when_buffer_too_small)
{
    // Arrange
    char actual[2]; // [状態] - "a/b" の親 "a" (2 バイト必要) に対し 2 バイトの出力バッファーを用意する。
    com_util_error err; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), &err,
                                    "ab/c"); // [手順] - com_util_path_dirname(actual, 2, &err, "ab/c") を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              rtc); // [確認_異常系] - com_util_path_dirname の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ(1,
              com_util_error_is(&err, COM_UTIL_CAUSE_NAME_TOO_LONG)); // [確認_異常系] - ENAMETOOLONG の要因であること。
}

// detail_out に NULL を渡しても動作することの確認
TEST_F(pathDirnameTest, allows_null_detail_out)
{
    // Arrange
    char actual[1]; // [状態] - 意図的に不足するバッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), NULL,
                                    "a/b"); // [手順] - com_util_path_dirname(actual, 1, NULL, "a/b") を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        rtc); // [確認_異常系] - com_util_path_dirname の戻り値として、detail_out が NULL でもクラッシュせず COM_UTIL_ERR_BUFFER_TOO_SMALL が返ること。
}
