#include <testfw.h>
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
    EXPECT_EQ(0, rtc);           // [確認_正常系] - 戻り値が 0 であること。
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
    EXPECT_EQ(0, rtc);         // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ(".", actual); // [確認_正常系] - "." が返ること。
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
    EXPECT_EQ(0, rtc);         // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("/", actual); // [確認_正常系] - "/" が返ること。
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
    EXPECT_EQ(0, rtc);         // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("/", actual); // [確認_正常系] - "/" が返ること。
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
    EXPECT_EQ(0, rtc);         // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("a", actual); // [確認_正常系] - "a" が返ること。
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
    EXPECT_EQ(0, rtc);           // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("a/b", actual); // [確認_正常系] - '/' 正規化された "a/b" が返ること。
}

// NULL path_out で EINVAL が返ることの確認
TEST_F(pathDirnameTest, returns_einval_for_null_path_out)
{
    // Arrange
    int err = 0; // [状態] - エラー コード格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(NULL, 16, &err,
                                    "a/b"); // [手順] - com_util_path_dirname(NULL, 16, &err, "a/b") を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);     // [確認_異常系] - 戻り値が -1 であること。
    EXPECT_EQ(EINVAL, err); // [確認_異常系] - errno_out が EINVAL であること。
}

// 空文字列パスで EINVAL が返ることの確認
TEST_F(pathDirnameTest, returns_einval_for_empty_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。
    int err = 0;                    // [状態] - エラー コード格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), &err,
                                    ""); // [手順] - com_util_path_dirname(actual, size, &err, "") を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);     // [確認_異常系] - 戻り値が -1 であること。
    EXPECT_EQ(EINVAL, err); // [確認_異常系] - errno_out が EINVAL であること。
}

// バッファー不足で ENAMETOOLONG が返ることの確認
TEST_F(pathDirnameTest, returns_enametoolong_when_buffer_too_small)
{
    // Arrange
    char actual[2]; // [状態] - "a/b" の親 "a" (2 バイト必要) に対し 2 バイトの出力バッファーを用意する。
    int err = 0;    // [状態] - エラー コード格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), &err,
                                    "ab/c"); // [手順] - com_util_path_dirname(actual, 2, &err, "ab/c") を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);           // [確認_異常系] - 戻り値が -1 であること。
    EXPECT_EQ(ENAMETOOLONG, err); // [確認_異常系] - errno_out が ENAMETOOLONG であること。
}

// errno_out に NULL を渡しても動作することの確認
TEST_F(pathDirnameTest, allows_null_errno_out)
{
    // Arrange
    char actual[1]; // [状態] - 意図的に不足するバッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_dirname(actual, sizeof(actual), NULL,
                                    "a/b"); // [手順] - com_util_path_dirname(actual, 1, NULL, "a/b") を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - errno_out が NULL でもクラッシュせず -1 が返ること。
}
