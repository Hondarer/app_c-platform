#include <testfw.h>
#include <com_util/crt/path.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

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

class pathsEqualTest : public Test
{
};

TEST_F(pathsEqualTest, returns_einval_for_null_lhs)
{
    // Arrange
    int err = 0;

    // Act
    int rc = com_util_paths_equal(nullptr, ".", &err); // [手順] - NULL の左辺パスで比較する。

    // Assert
    EXPECT_EQ(-1, rc); // [確認_異常系] - 引数不正で失敗すること。
    EXPECT_EQ(EINVAL, err); // [確認_異常系] - errno_out に EINVAL が返ること。
}

TEST_F(pathsEqualTest, compares_relative_and_absolute_current_directory_as_equal)
{
    // Arrange
    char absolute_current_dir[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(absolute_current_dir, sizeof(absolute_current_dir), ".");

    // Act
    int rc = com_util_paths_equal(".", absolute_current_dir, &err); // [手順] - 相対パスと絶対パスを比較する。

    // Assert
    EXPECT_EQ(1, rc); // [確認_正常系] - 同じ実体なら一致と判定されること。
}

TEST_F(pathsEqualTest, normalizes_dotdot_and_backslash_segments_before_comparing)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_three_part_path(lhs, sizeof(lhs), base, "alpha\\..", "beta.txt");
    build_path(rhs, sizeof(rhs), base, "beta.txt");

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 表記ゆれのある 2 つのパスを比較する。

    // Assert
    EXPECT_EQ(1, rc); // [確認_正常系] - 正規化後に同じパスなら一致と判定されること。
}

TEST_F(pathsEqualTest, returns_zero_for_different_paths)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_path(lhs, sizeof(lhs), base, "alpha.bin");
    build_path(rhs, sizeof(rhs), base, "beta.bin");

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 異なる 2 つのパスを比較する。

    // Assert
    EXPECT_EQ(0, rc); // [確認_正常系] - 異なるパスは不一致と判定されること。
}

#if defined(PLATFORM_LINUX)
TEST_F(pathsEqualTest, keeps_case_sensitive_comparison_on_linux)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_path(lhs, sizeof(lhs), base, "Case.bin");
    build_path(rhs, sizeof(rhs), base, "case.bin");

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 大小文字だけが異なるパスを比較する。

    // Assert
    EXPECT_EQ(0, rc); // [確認_正常系] - Linux では大小文字を区別して不一致と判定されること。
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(pathsEqualTest, ignores_case_differences_on_windows)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_path(lhs, sizeof(lhs), base, "Case.bin");
    build_path(rhs, sizeof(rhs), base, "case.bin");

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 大小文字だけが異なるパスを比較する。

    // Assert
    EXPECT_EQ(1, rc); // [確認_正常系] - Windows では大小文字差を無視して一致と判定されること。
}
#endif
