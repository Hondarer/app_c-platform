#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/path.h>
#include <mock_stdlib.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

#if defined(PLATFORM_LINUX)
    #include <stdlib.h>
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <direct.h>
#endif

using namespace testing;

namespace
{

static void assert_path_get_full_success(char *path_out, size_t path_size, const char *path)
{
    com_util_error err;
    ASSERT_EQ(COM_UTIL_OK, com_util_path_get_full(path_out, path_size, &err, path));
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

class pathGetFullTest : public Test
{
};

// パスが NULL の場合に EINVAL で失敗することの確認
TEST_F(pathGetFullTest, returns_einval_for_null_path)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - 出力バッファーを用意する。
    com_util_error err;                // [状態] - 詳細エラーの受け取り先を用意する。
    com_util_error last_error;

    // Pre-Assert

    // Act
    int rc = com_util_path_get_full(path, sizeof(path), &err,
                                    nullptr); // [手順] - パスに NULL を渡して com_util_path_get_full を呼び出す。
    com_util_error_get_last(&last_error);     // [手順] - TLS に記録された詳細エラーを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rc); // [確認_異常系] - com_util_path_get_full の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(1, com_util_error_is(&err, COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_異常系] - EINVAL の要因が返ること。
    EXPECT_EQ(1, com_util_error_is_set(&last_error)); // [確認_異常系] - TLS に詳細エラーが記録されること。
    EXPECT_EQ('\0', path[0]);                         // [確認_異常系] - 出力は空文字列に初期化されること。
}

// カレント ディレクトリが絶対パスへ展開されることの確認
TEST_F(pathGetFullTest, expands_current_directory_to_absolute_path)
{
    // Arrange
    char actual[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {}; // [状態] - 出力バッファーを 2 つ用意する。

    // Pre-Assert

    // Act
    assert_path_get_full_success(actual, sizeof(actual), "."); // [手順] - カレント ディレクトリ "." を絶対化する。
    assert_path_get_full_success(expected, sizeof(expected), actual); // [手順] - 得られた絶対パスを再度正規化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - "." が絶対パスへ展開され、再正規化しても同一であること。
}

// ".." やバックスラッシュを含むセグメントが正規化されることの確認
TEST_F(pathGetFullTest, normalizes_dotdot_and_backslash_segments)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char candidate[PLATFORM_PATH_MAX] = {};
    char actual[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {};

    assert_path_get_full_success(base, sizeof(base), ".");
    build_three_part_path(candidate, sizeof(candidate), base, "alpha\\..",
                          "beta.txt"); // [状態] - 入力を "alpha\\.." を挟んだ表記ゆれパスとする。
    build_path(expected, sizeof(expected), base, "beta.txt"); // [状態] - 期待値を正規化済みパスとする。

    // Pre-Assert

    // Act
    assert_path_get_full_success(actual, sizeof(actual), candidate); // [手順] - '\\' と '..' を含むパスを正規化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - セパレータと dot segment が正規化されること。
}

// 出力バッファーが小さすぎる場合に ENAMETOOLONG で失敗することの確認
TEST_F(pathGetFullTest, returns_enametoolong_when_buffer_is_too_small)
{
    // Arrange
    char path[4] = {};  // [状態] - 4 バイトの小さすぎる出力バッファーを用意する。
    com_util_error err; // [状態] - 詳細エラーの受け取り先を用意する。

    // Pre-Assert

    // Act
    int rc = com_util_path_get_full(path, sizeof(path), &err,
                                    "."); // [手順] - 小さすぎる出力バッファーで com_util_path_get_full を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              rc); // [確認_異常系] - com_util_path_get_full の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ(1,
              com_util_error_is(&err, COM_UTIL_CAUSE_NAME_TOO_LONG)); // [確認_異常系] - ENAMETOOLONG の要因が返ること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - 出力は空文字列に初期化されること。
}

// 存在しないパスでも絶対化済み文字列が返ることの確認
TEST_F(pathGetFullTest, returns_absolute_path_for_nonexistent_target)
{
    // Arrange
    char base[PLATFORM_PATH_MAX] = {};
    char candidate[PLATFORM_PATH_MAX] = {};
    char actual[PLATFORM_PATH_MAX] = {};
    char expected[PLATFORM_PATH_MAX] = {};

    assert_path_get_full_success(base, sizeof(base), ".");
    build_three_part_path(candidate, sizeof(candidate), base, "ghost-dir/..",
                          "ghost.bin"); // [状態] - 入力を存在しない "ghost-dir/.." を挟んだパスとする。
    build_path(expected, sizeof(expected), base, "ghost.bin"); // [状態] - 期待値を正規化済みパスとする。

    // Pre-Assert

    // Act
    assert_path_get_full_success(actual, sizeof(actual), candidate); // [手順] - 存在しないパスを絶対化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - 実体解決できなくても絶対化済み文字列が返ること。
}

#if defined(PLATFORM_LINUX)
// 正規化用メモリを確保できない場合に ENOMEM で失敗することの確認
TEST_F(pathGetFullTest, returns_enomem_when_normalization_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    char path[PLATFORM_PATH_MAX] = {'x'}; // [状態] - 出力バッファーを空文字列以外で初期化する。
    com_util_error err;                   // [状態] - 詳細エラーの受け取り先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, PLATFORM_PATH_MAX, sizeof(size_t)))
        .WillOnce(Return(nullptr)); // [Pre-Assert確認_異常系] - 正規化用メモリの calloc が 1 回呼び出されること。
                                    // [Pre-Assert手順] - calloc から NULL を返却する。

    // Act
    int rc = com_util_path_get_full(path, sizeof(path), &err,
                                    "/missing"); // [手順] - 絶対パスを指定して com_util_path_get_full を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              rc); // [確認_異常系] - com_util_path_get_full の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(1, com_util_error_is(&err, COM_UTIL_CAUSE_OUT_OF_MEMORY)); // [確認_異常系] - ENOMEM の要因が返ること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - 出力は空文字列に初期化されること。
}

// symlink が実体ファイルのパスへ解決されることの確認
TEST_F(pathGetFullTest, resolves_symlink_to_target_path_when_target_exists)
{
    // Arrange
    char dir_template[] = "/tmp/pathGetFullTestXXXXXX";
    char *dir_path = mkdtemp(dir_template); // [状態] - 一時ディレクトリを作成する。
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
    ASSERT_EQ(0, std::fclose(file));               // [状態] - 実体ファイル target.bin を作成する。
    ASSERT_EQ(0, symlink(target_path, link_path)); // [状態] - target.bin への symlink target-link.bin を作成する。

    assert_path_get_full_success(expected, sizeof(expected), target_path);

    // Pre-Assert

    // Act
    assert_path_get_full_success(actual, sizeof(actual), link_path); // [手順] - symlink のパスを絶対化する。

    // Assert
    EXPECT_STREQ(expected, actual); // [確認_正常系] - 実体ファイルのパスへ解決されること。

    // Cleanup
    ASSERT_EQ(0, unlink(link_path));
    ASSERT_EQ(0, unlink(target_path));
    ASSERT_EQ(0, rmdir(dir_path));
}
#endif /* PLATFORM_LINUX */
