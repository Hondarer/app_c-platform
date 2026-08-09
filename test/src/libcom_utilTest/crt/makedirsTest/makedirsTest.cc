#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>
#include <string>
#include <cstring>

// 後始末: テスト用ディレクトリをリーフから再帰削除する
static void remove_dir(const char *path)
{
    std::string cmd = std::string("rm -rf \"") + path + "\"";
    (void)system(cmd.c_str());
}

#if defined(PLATFORM_WINDOWS)
static std::string to_windows_sep(const char *path)
{
    std::string result(path);
    for (char &ch : result)
    {
        if (ch == '/')
        {
            ch = '\\';
        }
    }
    return result;
}
#endif /* PLATFORM_WINDOWS */

class makedirsTest : public Test
{
};

// NULL パスは COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(makedirsTest, test_null_path_returns_invalid_argument)
{
    // Arrange

    // Pre-Assert

    // Act
    int ret = com_util_makedirs(NULL, NULL); // [手順] - パスに NULL を渡して com_util_makedirs を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_makedirs の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 空文字列パスは COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(makedirsTest, test_empty_path_returns_invalid_argument)
{
    // Arrange

    // Pre-Assert

    // Act
    int ret = com_util_makedirs("", NULL); // [手順] - 空文字列パスで com_util_makedirs を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_makedirs の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 単一階層ディレクトリの新規作成と冪等性の確認
TEST_F(makedirsTest, test_single_level_creates_directory)
{
    // Arrange
    char tmp[PLATFORM_PATH_MAX];
    char dir[PLATFORM_PATH_MAX];
    com_util_file_stat_t st;

    ASSERT_EQ(COM_UTIL_OK, com_util_get_temp_dir(tmp, sizeof(tmp), NULL));
    ASSERT_EQ(COM_UTIL_OK,
              com_util_path_concat(dir, sizeof(dir), NULL, tmp, PLATFORM_PATH_SEP,
                                   "makedirsTest_single")); // [状態] - 一時ディレクトリ配下の作成対象パスを組み立てる。
    remove_dir(dir);                                        // [状態] - 残留物があれば削除しておく。

    // Pre-Assert

    // Act
    // Assert
    int ret = com_util_makedirs(dir, NULL); // [手順] - 存在しない単一階層ディレクトリを com_util_makedirs で作成する。
    EXPECT_EQ(COM_UTIL_OK, ret);            // [確認_正常系] - com_util_makedirs の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, com_util_stat(&st, NULL, dir)); // [確認_正常系] - ディレクトリが存在すること。

    int ret2 = com_util_makedirs(dir, NULL); // [手順] - 既存ディレクトリに com_util_makedirs を再呼び出しする。
    EXPECT_EQ(
        COM_UTIL_OK,
        ret2); // [確認_正常系] - 既存ディレクトリに対する com_util_makedirs の戻り値が COM_UTIL_OK であり、冪等であること。

    // Cleanup
    remove_dir(dir);
}

// 複数階層ディレクトリの再帰作成の確認
TEST_F(makedirsTest, test_nested_levels_creates_all_directories)
{
    // Arrange
    char tmp[PLATFORM_PATH_MAX];
    char root[PLATFORM_PATH_MAX];
    char nested[PLATFORM_PATH_MAX];
    com_util_file_stat_t st;

    ASSERT_EQ(COM_UTIL_OK, com_util_get_temp_dir(tmp, sizeof(tmp), NULL));
    ASSERT_EQ(COM_UTIL_OK, com_util_path_concat(root, sizeof(root), NULL, tmp, PLATFORM_PATH_SEP, "makedirsTest_root"));
    ASSERT_EQ(COM_UTIL_OK, com_util_path_concat(
                               nested, sizeof(nested), NULL, root, PLATFORM_PATH_SEP, "sub", PLATFORM_PATH_SEP,
                               "leaf")); // [状態] - 中間ディレクトリが欠けた root/sub/leaf の 2 階層パスを組み立てる。
    remove_dir(root);                    // [状態] - 残留物があれば削除しておく。

    // Pre-Assert

    // Act
    int ret = com_util_makedirs(nested,
                                NULL); // [手順] - 中間ディレクトリが欠けた 2 階層パスを com_util_makedirs で作成する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_makedirs の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, com_util_stat(&st, NULL, nested)); // [確認_正常系] - リーフ ディレクトリが存在すること。

    // Cleanup
    remove_dir(root);
}

#if defined(PLATFORM_WINDOWS)
// Windows スタイル区切りの絶対パスで再帰作成できることの確認
TEST_F(makedirsTest, test_windows_separator_path_creates_directory)
{
    // Arrange
    char tmp[PLATFORM_PATH_MAX];
    char root[PLATFORM_PATH_MAX];
    char nested[PLATFORM_PATH_MAX];
    com_util_file_stat_t st;

    ASSERT_EQ(COM_UTIL_OK, com_util_get_temp_dir(tmp, sizeof(tmp), NULL));
    ASSERT_EQ(COM_UTIL_OK,
              com_util_path_concat(root, sizeof(root), NULL, tmp, PLATFORM_PATH_SEP, "makedirsTest_windows_sep"));
    ASSERT_EQ(COM_UTIL_OK, com_util_path_concat(nested, sizeof(nested), NULL, root, PLATFORM_PATH_SEP, "sub",
                                                PLATFORM_PATH_SEP, "leaf"));
    remove_dir(root); // [状態] - 残留物があれば削除しておく。

    std::string windows_path =
        to_windows_sep(nested); // [状態] - 区切りを '\\' に置き換えた Windows スタイルのパスとする。

    // Pre-Assert

    // Act
    // Assert
    int ret = com_util_makedirs(windows_path.c_str(),
                                NULL); // [手順] - Windows スタイル区切りの 2 階層パスを com_util_makedirs で作成する。
    EXPECT_EQ(COM_UTIL_OK, ret);       // [確認_正常系] - com_util_makedirs の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, com_util_stat(&st, NULL, nested)); // [確認_正常系] - 正規化後のパスでリーフが存在すること。

    int ret2 = com_util_makedirs(windows_path.c_str(),
                                 NULL); // [手順] - 既存ディレクトリに com_util_makedirs を再呼び出しする。
    EXPECT_EQ(
        COM_UTIL_OK,
        ret2); // [確認_正常系] - 既存ディレクトリに対する com_util_makedirs の戻り値が COM_UTIL_OK であり、冪等であること。

    // Cleanup
    remove_dir(root);
}
#endif /* PLATFORM_WINDOWS */
