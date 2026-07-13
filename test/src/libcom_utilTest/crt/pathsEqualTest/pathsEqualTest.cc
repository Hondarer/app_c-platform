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

static void build_three_part_path(char *path_out, size_t path_size, const char *lhs, const char *middle,
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

// 左辺パスが NULL の場合に EINVAL で失敗することの確認
TEST_F(pathsEqualTest, returns_einval_for_null_lhs)
{
    // Arrange
    int err = 0; // [状態] - errno_out の受け取り先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rc =
        com_util_paths_equal(nullptr, ".", &err); // [手順] - 左辺パスに NULL を渡して com_util_paths_equal を呼び出す。

    // Assert
    EXPECT_EQ(-1, rc);      // [確認_異常系] - com_util_paths_equal の戻り値が -1 であること。
    EXPECT_EQ(EINVAL, err); // [確認_異常系] - errno_out に EINVAL が返ること。
}

// 相対パスと絶対パスが同じ実体なら一致と判定されることの確認
TEST_F(pathsEqualTest, compares_relative_and_absolute_current_directory_as_equal)
{
    // Arrange
    char absolute_current_dir[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(absolute_current_dir, sizeof(absolute_current_dir),
                                 "."); // [状態] - カレント ディレクトリの絶対パスを取得する。

    // Pre-Assert

    // Act
    int rc = com_util_paths_equal(".", absolute_current_dir, &err); // [手順] - 相対パス "." と絶対パスを比較する。

    // Assert
    EXPECT_EQ(1, rc); // [確認_正常系] - com_util_paths_equal の戻り値が 1 (一致) であること。
}

// ".." やバックスラッシュを含むパスが正規化されて比較されることの確認
TEST_F(pathsEqualTest, normalizes_dotdot_and_backslash_segments_before_comparing)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_three_part_path(lhs, sizeof(lhs), base, "alpha\\..",
                          "beta.txt");              // [状態] - 左辺を "alpha\\.." を挟んだ表記ゆれパスとする。
    build_path(rhs, sizeof(rhs), base, "beta.txt"); // [状態] - 右辺を正規化済みの同一実体パスとする。

    // Pre-Assert

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 表記ゆれのある 2 つのパスを比較する。

    // Assert
    EXPECT_EQ(1, rc); // [確認_正常系] - com_util_paths_equal の戻り値が 1 (正規化後に一致) であること。
}

// 異なるパスが不一致と判定されることの確認
TEST_F(pathsEqualTest, returns_zero_for_different_paths)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_path(lhs, sizeof(lhs), base, "alpha.bin"); // [状態] - 左辺を "alpha.bin" とする。
    build_path(rhs, sizeof(rhs), base, "beta.bin");  // [状態] - 右辺を "beta.bin" とする。

    // Pre-Assert

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 異なる 2 つのパスを比較する。

    // Assert
    EXPECT_EQ(0, rc); // [確認_正常系] - com_util_paths_equal の戻り値が 0 (不一致) であること。
}

#if defined(PLATFORM_LINUX)
// Linux では大小文字を区別して比較されることの確認
TEST_F(pathsEqualTest, keeps_case_sensitive_comparison_on_linux)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_path(lhs, sizeof(lhs), base, "Case.bin"); // [状態] - 左辺を "Case.bin" とする。
    build_path(rhs, sizeof(rhs), base, "case.bin"); // [状態] - 右辺を大小文字だけが異なる "case.bin" とする。

    // Pre-Assert

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 大小文字だけが異なるパスを比較する。

    // Assert
    EXPECT_EQ(0, rc); // [確認_正常系] - com_util_paths_equal の戻り値が 0 (大小文字を区別して不一致) であること。
}
#elif defined(PLATFORM_WINDOWS)
// Windows では大小文字差を無視して比較されることの確認
TEST_F(pathsEqualTest, ignores_case_differences_on_windows)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char lhs[PLATFORM_PATH_MAX] = {};
    char rhs[PLATFORM_PATH_MAX] = {};
    int err = 0;

    assert_path_get_full_success(base, sizeof(base), ".");
    build_path(lhs, sizeof(lhs), base, "Case.bin"); // [状態] - 左辺を "Case.bin" とする。
    build_path(rhs, sizeof(rhs), base, "case.bin"); // [状態] - 右辺を大小文字だけが異なる "case.bin" とする。

    // Pre-Assert

    // Act
    int rc = com_util_paths_equal(lhs, rhs, &err); // [手順] - 大小文字だけが異なるパスを比較する。

    // Assert
    EXPECT_EQ(1, rc); // [確認_正常系] - com_util_paths_equal の戻り値が 1 (大小文字差を無視して一致) であること。
}
#endif
