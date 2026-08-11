#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/path.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <io.h>
#endif /* PLATFORM_ */

class stdioTempTest : public Test
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
TEST_F(stdioTempTest, opens_writable_file_and_reports_path)
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
    EXPECT_TRUE(path_exists(path)); // [確認_正常系] - 戻されたパスにファイルが実在すること。

    // Cleanup
    EXPECT_EQ(0, fclose(fp));
    std::remove(path);
}

// 繰り返し呼び出しで毎回異なるパスが返ることの確認
TEST_F(stdioTempTest, returns_unique_paths_for_repeated_calls)
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
        EXPECT_TRUE(seen.insert(path).second); // [確認_正常系] - 過去に返された path と重複しないこと。

        // Cleanup
        fclose(fp);
        std::remove(path);
    }
}

// prefix がファイル名 (basename) に含まれることの確認
TEST_F(stdioTempTest, prefix_is_part_of_basename)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - パスの受け取り先バッファーを用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("abc", "wb", path, sizeof(path),
                                   nullptr); // [手順] - prefix "abc" で com_util_fopen_temp を呼び出す。

    // Assert
    ASSERT_NE((FILE *)nullptr, fp);
    std::string base = basename_of(path);
    EXPECT_NE(std::string::npos, base.find("abc")); // [確認_正常系] - basename に prefix "abc" が含まれること。

    // Cleanup
    fclose(fp);
    std::remove(path);
}

// prefix が NULL でも受理されることの確認
TEST_F(stdioTempTest, null_prefix_is_accepted)
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

    // Cleanup
    fclose(fp);
    std::remove(path);
}

// modes が NULL の場合に EINVAL で失敗することの確認
TEST_F(stdioTempTest, null_modes_returns_einval)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {};
    com_util_error err; // [状態] - 詳細エラーの受け取り先を用意する。
    com_util_error last_error;

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", nullptr, path, sizeof(path),
                                   &err); // [手順] - modes に NULL を渡して com_util_fopen_temp を呼び出す。
    com_util_error_get_last(&last_error); // [手順] - TLS に記録された詳細エラーを取得する。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(
        1, com_util_error_is(&err, COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_異常系] - EINVAL の要因が格納されること。
    EXPECT_EQ(1, com_util_error_is_set(&last_error)); // [確認_異常系] - TLS に詳細エラーが記録されること。
}

// path_out が NULL の場合に EINVAL で失敗することの確認
TEST_F(stdioTempTest, null_path_out_returns_einval)
{
    // Arrange
    com_util_error err; // [状態] - 詳細エラーの受け取り先を用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", nullptr, 0u,
                                   &err); // [手順] - path_out に NULL を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(
        1, com_util_error_is(&err, COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_異常系] - EINVAL の要因が格納されること。
}

// path_size が 0 の場合に EINVAL で失敗することの確認
TEST_F(stdioTempTest, zero_path_size_returns_einval)
{
    // Arrange
    char path[1] = {'x'};
    com_util_error err; // [状態] - 詳細エラーの受け取り先を用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, 0u,
                                   &err); // [手順] - path_size に 0 を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(
        1, com_util_error_is(&err, COM_UTIL_CAUSE_INVALID_ARGUMENT)); // [確認_異常系] - EINVAL の要因が格納されること。
}

#if defined(PLATFORM_LINUX)
// path_size が必要長未満の場合に ENAMETOOLONG で失敗することの確認
TEST_F(stdioTempTest, path_size_too_small_returns_enametoolong)
{
    // Arrange
    /* "<dir>/<prefix>XXXXXX" + NUL に満たない長さ */
    char path[4] = {};  // [状態] - 必要長に満たない 4 バイトのバッファーを用意する。
    com_util_error err; // [状態] - 詳細エラーの受け取り先を用意する。

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path),
                                   &err); // [手順] - path_size に 4 を渡して com_util_fopen_temp を呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(1, com_util_error_is(
                     &err, COM_UTIL_CAUSE_NAME_TOO_LONG)); // [確認_異常系] - ENAMETOOLONG の要因が格納されること。
}
#endif /* PLATFORM_LINUX */

// 3 文字を超える prefix が先頭 3 文字に切り詰められることの確認
TEST_F(stdioTempTest, prefix_longer_than_three_chars_is_truncated)
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
    std::string base = basename_of(path);
    EXPECT_NE(std::string::npos, base.find("abc")); // [確認_正常系] - 先頭 3 文字 "abc" が basename に含まれること。

    // Cleanup
    fclose(fp);
    std::remove(path);
}

#if defined(PLATFORM_LINUX)
// TMPDIR の取得結果がバッファーに収まらない場合に失敗することの確認
TEST_F(stdioTempTest, tmpdir_buffer_too_small_returns_enametoolong)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    char path[PLATFORM_PATH_MAX] = {};
    com_util_error err;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_getenv(StrEq("TMPDIR"), _, _, _, _))
        .WillOnce(Return(COM_UTIL_ERR_BUFFER_TOO_SMALL));

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path),
                                   &err); // [手順] - バッファー不足を返す TMPDIR を指定して呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(1, com_util_error_is(
                     &err, COM_UTIL_CAUSE_NAME_TOO_LONG)); // [確認_異常系] - ENAMETOOLONG の要因が格納されること。
}

// 一時ディレクトリが存在しない場合に mkostemp のエラーを返すことの確認
TEST_F(stdioTempTest, mkostemp_failure_reports_errno)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    char path[PLATFORM_PATH_MAX] = {};
    com_util_error err;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_getenv(StrEq("TMPDIR"), _, _, _, _))
        .WillOnce(
            [](const char *, char *buffer, size_t buffer_size, int *, com_util_error *)
            {
                std::strncpy(buffer, "/com_util/no-such-temp-directory", buffer_size);
                buffer[buffer_size - 1u] = '\0';
                return COM_UTIL_OK;
            });

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path),
                                   &err); // [手順] - 存在しない TMPDIR を指定して呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(ENOENT,
              com_util_error_get_errno(&err)); // [確認_異常系] - mkostemp の ENOENT が格納されること。
}

// 一時ファイルのモードが不正な場合に fdopen のエラーを返すことの確認
TEST_F(stdioTempTest, invalid_modes_reports_fdopen_error)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {};
    com_util_error err;

    // Pre-Assert

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "q", path, sizeof(path),
                                   &err); // [手順] - 不正なモードを指定して呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(EINVAL,
              com_util_error_get_errno(&err)); // [確認_異常系] - fdopen の EINVAL が格納されること。
    EXPECT_FALSE(path_exists(path)); // [確認_異常系] - fdopen 失敗後に一時ファイルが削除されること。
}

// 一時ファイルのパス整形に失敗した場合に ENAMETOOLONG で失敗することの確認
TEST_F(stdioTempTest, path_formatting_failure_returns_enametoolong)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    char path[PLATFORM_PATH_MAX] = {};
    com_util_error err;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_snprintf(_, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN));

    // Act
    FILE *fp = com_util_fopen_temp("ptr", "wb", path, sizeof(path),
                                   &err); // [手順] - パス整形失敗を注入して呼び出す。

    // Assert
    EXPECT_EQ((FILE *)nullptr, fp); // [確認_異常系] - com_util_fopen_temp の戻り値が NULL であること。
    EXPECT_EQ(1, com_util_error_is(
                     &err, COM_UTIL_CAUSE_NAME_TOO_LONG)); // [確認_異常系] - ENAMETOOLONG の要因が格納されること。
}
#endif /* PLATFORM_LINUX */
