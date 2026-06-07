#include <testfw.h>
#include <com_util/crt/path.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

#if defined(PLATFORM_LINUX)
    #include <stdlib.h>
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <direct.h>
#endif

namespace
{

static void assert_path_get_full_success(char *path_out, size_t path_size, const char *path)
{
    int err = 0;
    ASSERT_EQ(0, com_util_path_get_full(path_out, path_size, &err, path));
}

static void build_path(char *path_out, size_t path_size, const char *lhs, const char *rhs)
{
    int written = std::snprintf(path_out, path_size, "%s/%s", lhs, rhs);
    ASSERT_GE(written, 0);
    ASSERT_LT((size_t)written, path_size);
}

static void build_three_part_path(char *path_out,
                                  size_t path_size,
                                  const char *lhs,
                                  const char *middle,
                                  const char *rhs)
{
    int written = std::snprintf(path_out, path_size, "%s/%s/%s", lhs, middle, rhs);
    ASSERT_GE(written, 0);
    ASSERT_LT((size_t)written, path_size);
}

} // namespace

class pathGetFullTest : public Test
{
};

TEST_F(pathGetFullTest, returns_einval_for_null_path)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {};
    int err = 0;

    // Act
    int rc = com_util_path_get_full(path, sizeof(path), &err, nullptr); // [手順] - NULL パスを絶対化する。

    // Assert
    EXPECT_EQ(-1, rc); // [確認_異常系] - 引数不正で失敗すること。
    EXPECT_EQ(EINVAL, err); // [確認_異常系] - errno_out に EINVAL が返ること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - 出力は空文字列に初期化されること。
}

TEST_F(pathGetFullTest, expands_current_directory_to_absolute_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {};

    // Act
    assert_path_get_full_success(actual, sizeof(actual), "."); // [手順] - カレント ディレクトリを絶対化する。
    assert_path_get_full_success(expected, sizeof(expected), actual); // [手順] - 得られた絶対パスを再度正規化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - "." が絶対パスへ展開されること。
}

TEST_F(pathGetFullTest, normalizes_dotdot_and_backslash_segments)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char candidate[PLATFORM_PATH_MAX] = {};
    char actual[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {};

    assert_path_get_full_success(base, sizeof(base), ".");
    build_three_part_path(candidate, sizeof(candidate), base, "alpha\\..", "beta.txt");
    build_path(expected, sizeof(expected), base, "beta.txt");

    // Act
    assert_path_get_full_success(actual, sizeof(actual), candidate); // [手順] - '\\' と '..' を含む絶対パスを正規化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - セパレータと dot segment が正規化されること。
}

TEST_F(pathGetFullTest, returns_enametoolong_when_buffer_is_too_small)
{
    // Arrange
    char path[4] = {};
    int err = 0;

    // Act
    int rc = com_util_path_get_full(path, sizeof(path), &err, "."); // [手順] - 小さすぎる出力バッファーで絶対化する。

    // Assert
    EXPECT_EQ(-1, rc); // [確認_異常系] - バッファー不足で失敗すること。
    EXPECT_EQ(ENAMETOOLONG, err); // [確認_異常系] - errno_out に ENAMETOOLONG が返ること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - 出力は空文字列に初期化されること。
}

TEST_F(pathGetFullTest, returns_absolute_path_for_nonexistent_target)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char candidate[PLATFORM_PATH_MAX] = {};
    char actual[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {};

    assert_path_get_full_success(base, sizeof(base), ".");
    build_three_part_path(candidate, sizeof(candidate), base, "ghost-dir/..", "ghost.bin");
    build_path(expected, sizeof(expected), base, "ghost.bin");

    // Act
    assert_path_get_full_success(actual, sizeof(actual), candidate); // [手順] - 非存在パスを絶対化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - 実体解決できなくても絶対化済み文字列が返ること。
}

#if defined(PLATFORM_LINUX)
TEST_F(pathGetFullTest, resolves_symlink_to_target_path_when_target_exists)
{
    // Arrange
    char dir_template[] = "/tmp/pathGetFullTestXXXXXX";
    char *dir_path = mkdtemp(dir_template);
    char target_path[PLATFORM_PATH_MAX] = {};
    char link_path[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {};
    char actual[PLATFORM_PATH_MAX] = {};
    FILE *file;

    ASSERT_NE(nullptr, dir_path);
    build_path(target_path, sizeof(target_path), dir_path, "target.bin");
    build_path(link_path, sizeof(link_path), dir_path, "target-link.bin");

    file = std::fopen(target_path, "wb");
    ASSERT_NE(nullptr, file);
    ASSERT_EQ(1u, std::fwrite("x", 1u, 1u, file));
    ASSERT_EQ(0, std::fclose(file));
    ASSERT_EQ(0, symlink(target_path, link_path));

    assert_path_get_full_success(expected, sizeof(expected), target_path);

    // Act
    assert_path_get_full_success(actual, sizeof(actual), link_path); // [手順] - symlink パスを絶対化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - 実体ファイルのパスへ解決されること。

    ASSERT_EQ(0, unlink(link_path));
    ASSERT_EQ(0, unlink(target_path));
    ASSERT_EQ(0, rmdir(dir_path));
}
#endif /* PLATFORM_LINUX */
