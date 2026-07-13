#include <testfw.h>
#include <com_util/crt/path.h>
#include <errno.h>
#include <string.h>

class pathExtensionTest : public Test
{
};

// 複数のドットを含むファイル名で最後の拡張子が返ることの確認
TEST_F(pathExtensionTest, returns_last_extension_of_multi_dot_name)
{
    // Arrange
    const char *path = "a/b.tar.gz"; // [状態] - 複数のドットを含むパスを用意する。

    // Pre-Assert

    // Act
    const char *actual = com_util_path_extension(path); // [手順] - com_util_path_extension(path) を呼び出す。

    // Assert
    EXPECT_STREQ(
        ".gz", actual); // [確認_正常系] - com_util_path_extension の戻り値として、ドット込みの最後の拡張子が返ること。
}

// ドットファイルの先頭ドットが拡張子とみなされないことの確認
TEST_F(pathExtensionTest, does_not_treat_leading_dot_as_extension)
{
    // Arrange
    const char *path = ".bashrc"; // [状態] - ドットファイルを用意する。

    // Pre-Assert

    // Act
    const char *actual = com_util_path_extension(path); // [手順] - com_util_path_extension(path) を呼び出す。

    // Assert
    EXPECT_STREQ(
        "", actual); // [確認_正常系] - com_util_path_extension の戻り値として、拡張子なしとして空文字列が返ること。
}

// ディレクトリ名にドットが含まれても拡張子とみなされないことの確認
TEST_F(pathExtensionTest, ignores_dot_in_directory_component)
{
    // Arrange
    const char *path = "dir.d/file"; // [状態] - ディレクトリ名にドットを含むパスを用意する。

    // Pre-Assert

    // Act
    const char *actual = com_util_path_extension(path); // [手順] - com_util_path_extension(path) を呼び出す。

    // Assert
    EXPECT_STREQ(
        "",
        actual); // [確認_正常系] - com_util_path_extension の戻り値として、ベース名にドットがないため空文字列が返ること。
}

// 末尾がドットのみの場合にそのドットが拡張子として返ることの確認
TEST_F(pathExtensionTest, returns_dot_only_extension)
{
    // Arrange
    const char *path = "a."; // [状態] - 末尾がドットのみのパスを用意する。

    // Pre-Assert

    // Act
    const char *actual = com_util_path_extension(path); // [手順] - com_util_path_extension(path) を呼び出す。

    // Assert
    EXPECT_STREQ(".", actual); // [確認_正常系] - com_util_path_extension の戻り値として、"." が返ること。
}

// NULL を渡した場合に NULL が返ることの確認
TEST_F(pathExtensionTest, returns_null_for_null_input)
{
    // Arrange

    // Pre-Assert

    // Act
    const char *actual = com_util_path_extension(NULL); // [手順] - com_util_path_extension(NULL) を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, actual); // [確認_異常系] - com_util_path_extension の戻り値が NULL であること。
}

// 拡張子なし時に入力の終端ポインターが返ることの確認
TEST_F(pathExtensionTest, returns_pointer_to_terminator_when_no_extension)
{
    // Arrange
    const char path[] = "noext"; // [状態] - 拡張子を含まないパスを用意する。

    // Pre-Assert

    // Act
    const char *actual = com_util_path_extension(path); // [手順] - com_util_path_extension(path) を呼び出す。

    // Assert
    EXPECT_EQ(path + strlen(path), actual); // [確認_正常系] - path の終端 '\0' を指すこと。
}

// パスから拡張子を除いたパスがコピーされることの確認
TEST_F(pathExtensionTest, strip_extension_removes_extension)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_strip_extension(
        actual, sizeof(actual), NULL,
        "a/b.txt"); // [手順] - com_util_path_strip_extension(actual, size, NULL, "a/b.txt") を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);           // [確認_正常系] - com_util_path_strip_extension の戻り値が 0 であること。
    EXPECT_STREQ("a/b", actual); // [確認_正常系] - 拡張子を除いたパスが返ること。
}

// 拡張子がない場合そのままコピーされることの確認
TEST_F(pathExtensionTest, strip_extension_copies_as_is_when_no_extension)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_strip_extension(
        actual, sizeof(actual), NULL,
        "noext"); // [手順] - com_util_path_strip_extension(actual, size, NULL, "noext") を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);             // [確認_正常系] - com_util_path_strip_extension の戻り値が 0 であること。
    EXPECT_STREQ("noext", actual); // [確認_正常系] - 入力がそのままコピーされること。
}

// NULL path_out で EINVAL が返ることの確認
TEST_F(pathExtensionTest, strip_extension_returns_einval_for_null_path_out)
{
    // Arrange
    int err = 0; // [状態] - エラー コード格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_strip_extension(
        NULL, 16, &err,
        "a.txt"); // [手順] - com_util_path_strip_extension(NULL, 16, &err, "a.txt") を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);     // [確認_異常系] - com_util_path_strip_extension の戻り値が -1 であること。
    EXPECT_EQ(EINVAL, err); // [確認_異常系] - errno_out が EINVAL であること。
}

// バッファー不足で ENAMETOOLONG が返ることの確認
TEST_F(pathExtensionTest, strip_extension_returns_enametoolong_when_buffer_too_small)
{
    // Arrange
    char actual[2]; // [状態] - "ab" (2 バイト必要) に対し 2 バイトの出力バッファーを用意する。
    int err = 0;    // [状態] - エラー コード格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc = com_util_path_strip_extension(
        actual, sizeof(actual), &err,
        "abc.txt"); // [手順] - com_util_path_strip_extension(actual, 2, &err, "abc.txt") を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);           // [確認_異常系] - com_util_path_strip_extension の戻り値が -1 であること。
    EXPECT_EQ(ENAMETOOLONG, err); // [確認_異常系] - errno_out が ENAMETOOLONG であること。
}
