#include <testfw.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <io.h>
#endif /* PLATFORM_ */

class fopenTempTest : public Test
{
  protected:
    static bool path_exists(const char *path)
    {
#if defined(PLATFORM_LINUX)
        return access(path, F_OK) == 0;
#elif defined(PLATFORM_WINDOWS)
        return _access(path, 0) == 0;
#endif /* PLATFORM_ */
    }

    static std::string basename_of(const std::string &path)
    {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos)
        {
            return path;
        }
        return path.substr(pos + 1);
    }
};

// 書き込み可能な一時ファイルが開かれ、実在するパスが報告されることの確認
TEST_F(fopenTempTest, opens_writable_file_and_reports_path)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - パスの受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path),
                                   nullptr); // [手順] - prefix "ptr"、モード "wb" で com_util_fopen_temp を呼び出す。

    // Assert
    ASSERT_NE((FILE *)nullptr, fp); // [確認_正常系] - com_util_fopen_temp の戻り値として、FILE* が返ること。
    EXPECT_NE('\0', path[0]);       // [確認_正常系] - path_out が空文字列でないこと。

    const char data[] = "hello-temp";
    EXPECT_EQ(sizeof(data), fwrite(data, 1, sizeof(data), fp)); // [確認_正常系] - 開いたファイルへ書き込めること。
    EXPECT_EQ(0, fclose(fp));

    EXPECT_TRUE(path_exists(path)); // [確認_正常系] - 戻されたパスにファイルが実在すること。
    std::remove(path);
}

// 繰り返し呼び出しで毎回異なるパスが返ることの確認
TEST_F(fopenTempTest, returns_unique_paths_for_repeated_calls)
{
    // Arrange
    std::set<std::string> seen; // [状態] - 返されたパスの記録用セットを用意する。

    // Pre-Assert

    // Act
    // Assert
    // [手順] - com_util_fopen_temp を 4 回呼び出す。
    for (int i = 0; i < 4; ++i)
    {
        char path[PLATFORM_PATH_MAX] = {};
        FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path), nullptr);
        ASSERT_NE((FILE *)nullptr, fp);
        fclose(fp);
        EXPECT_TRUE(seen.insert(path).second); // [確認_正常系] - 過去に返された path と重複しないこと。
        std::remove(path);
    }
}

// prefix がファイル名 (basename) に含まれることの確認
TEST_F(fopenTempTest, prefix_is_part_of_basename)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - パスの受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("abc", "wb", path, sizeof(path),
                                   nullptr); // [手順] - prefix "abc" で com_util_fopen_temp を呼び出す。

    // Assert
    ASSERT_NE((FILE *)nullptr, fp);
    fclose(fp);

    std::string base = basename_of(path);
    EXPECT_NE(std::string::npos, base.find("abc")); // [確認_正常系] - basename に prefix "abc" が含まれること。
    std::remove(path);
}

// prefix が NULL でも受理されることの確認
TEST_F(fopenTempTest, null_prefix_is_accepted)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - パスの受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp(nullptr, "wb", path, sizeof(path),
                                   nullptr); // [手順] - prefix に NULL を渡して com_util_fopen_temp を呼び出す。

    // Assert
    ASSERT_NE((FILE *)nullptr,
              fp); // [確認_正常系] - com_util_fopen_temp の戻り値として、prefix=NULL でも FILE* が返ること。
    fclose(fp);
    std::remove(path);
}

// modes が NULL の場合に EINVAL で失敗することの確認
TEST_F(fopenTempTest, null_modes_returns_einval)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {};
    int err = 0; // [状態] - errno_out の受け取り先を 0 で初期化する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", nullptr, path, sizeof(path),
                                   &err); // [手順] - modes に NULL を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(EINVAL, err);         // [確認_異常系] - errno_out に EINVAL が格納されること。
}

// path_out が NULL の場合に EINVAL で失敗することの確認
TEST_F(fopenTempTest, null_path_out_returns_einval)
{
    // Arrange
    int err = 0; // [状態] - errno_out の受け取り先を 0 で初期化する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", nullptr, 0u,
                                   &err); // [手順] - path_out に NULL を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(EINVAL, err);         // [確認_異常系] - errno_out に EINVAL が格納されること。
}

// path_size が 0 の場合に EINVAL で失敗することの確認
TEST_F(fopenTempTest, zero_path_size_returns_einval)
{
    // Arrange
    char path[1] = {'x'};
    int err = 0; // [状態] - errno_out の受け取り先を 0 で初期化する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, 0u,
                                   &err); // [手順] - path_size に 0 を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(EINVAL, err);         // [確認_異常系] - errno_out に EINVAL が格納されること。
}

#if defined(PLATFORM_LINUX)
// path_size が必要長未満の場合に ENAMETOOLONG で失敗することの確認
TEST_F(fopenTempTest, path_size_too_small_returns_enametoolong)
{
    // Arrange
    /* "<dir>/<prefix>XXXXXX" + NUL に満たない長さ */
    char path[4] = {}; // [状態] - 必要長に満たない 4 バイトのバッファーを用意する。
    int err = 0;       // [状態] - errno_out の受け取り先を 0 で初期化する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path),
                                   &err); // [手順] - path_size に 4 を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(ENAMETOOLONG, err);   // [確認_異常系] - errno_out に ENAMETOOLONG が格納されること。
}
#endif /* PLATFORM_LINUX */

// 3 文字を超える prefix が先頭 3 文字に切り詰められることの確認
TEST_F(fopenTempTest, prefix_longer_than_three_chars_is_truncated)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - パスの受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("abcd", "wb", path, sizeof(path),
                                   nullptr); // [手順] - 4 文字の prefix "abcd" で com_util_fopen_temp を呼び出す。

    // Assert
    ASSERT_NE((FILE *)nullptr,
              fp); // [確認_正常系] - com_util_fopen_temp の戻り値として、4 文字以上の prefix でも FILE* が返ること。
    fclose(fp);
    std::string base = basename_of(path);
    EXPECT_NE(std::string::npos, base.find("abc")); // [確認_正常系] - 先頭 3 文字 "abc" が basename に含まれること。
    std::remove(path);
}
