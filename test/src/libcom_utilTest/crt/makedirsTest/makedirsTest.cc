#include <testfw.h>
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

class makedirsTest : public Test
{
};

// NULL パスは -1 を返すことの確認
TEST_F(makedirsTest, test_null_path_returns_minus_one)
{
    // Act
    int ret = com_util_makedirs(NULL); // [手順] - NULL パスで makedirs を呼ぶ。

    // Assert
    EXPECT_EQ(-1, ret); // [確認_異常系] - -1 が返ること。
}

// 空文字列パスは -1 を返すことの確認
TEST_F(makedirsTest, test_empty_path_returns_minus_one)
{
    // Act
    int ret = com_util_makedirs(""); // [手順] - 空文字列パスで makedirs を呼ぶ。

    // Assert
    EXPECT_EQ(-1, ret); // [確認_異常系] - -1 が返ること。
}

// 単一階層ディレクトリの新規作成と冪等性の確認
TEST_F(makedirsTest, test_single_level_creates_directory)
{
    // Arrange
    char tmp[PLATFORM_PATH_MAX];
    char dir[PLATFORM_PATH_MAX];
    com_util_file_stat_t st;

    ASSERT_EQ(0, com_util_get_temp_dir(tmp, sizeof(tmp), NULL));
    ASSERT_EQ(0, com_util_path_concat(dir, sizeof(dir), NULL, tmp, PLATFORM_PATH_SEP,
                                      "makedirsTest_single"));
    remove_dir(dir); // 残留物があれば削除

    // Act - 新規作成
    int ret = com_util_makedirs(dir); // [手順] - 存在しない単一階層ディレクトリを makedirs で作成する。

    // Assert - 作成成功
    EXPECT_EQ(0, ret);                         // [確認_正常系] - 0 が返ること。
    EXPECT_EQ(0, com_util_stat(&st, dir));     // [確認_正常系] - ディレクトリが存在すること。

    // Act - 冪等: 既存ディレクトリへの再呼び出し
    int ret2 = com_util_makedirs(dir); // [手順] - 既存ディレクトリに makedirs を再呼び出しする。

    // Assert - 冪等
    EXPECT_EQ(0, ret2); // [確認_正常系] - 既存ディレクトリでも 0 が返ること (冪等)。

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

    ASSERT_EQ(0, com_util_get_temp_dir(tmp, sizeof(tmp), NULL));
    ASSERT_EQ(0, com_util_path_concat(root, sizeof(root), NULL, tmp, PLATFORM_PATH_SEP,
                                      "makedirsTest_root"));
    ASSERT_EQ(0, com_util_path_concat(nested, sizeof(nested), NULL, root, PLATFORM_PATH_SEP,
                                      "sub", PLATFORM_PATH_SEP, "leaf"));
    remove_dir(root); // 残留物があれば削除

    // Act - 2 階層まとめて作成
    int ret = com_util_makedirs(nested); // [手順] - 中間ディレクトリが欠けた 2 階層パスを makedirs で作成する。

    // Assert - リーフを含む全階層が存在すること
    EXPECT_EQ(0, ret);                              // [確認_正常系] - 0 が返ること。
    EXPECT_EQ(0, com_util_stat(&st, nested));       // [確認_正常系] - リーフ ディレクトリが存在すること。

    // Cleanup
    remove_dir(root);
}
