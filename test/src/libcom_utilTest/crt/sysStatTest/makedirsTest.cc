#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/base/result.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>
#include <filesystem>
#include <string>
#include <cstring>

// 後始末: テスト用ディレクトリをリーフから再帰削除する
static void remove_dir(const std::string &path)
{
    std::error_code ec;
    (void)std::filesystem::remove_all(path, ec);
}

// 一時ディレクトリ配下のテスト用パスを組み立てる
static std::string temp_path(const std::string &relative)
{
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::temp_directory_path(ec);

    return (dir / relative).generic_string();
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
TEST_F(makedirsTest, null_path_returns_invalid_argument)
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
TEST_F(makedirsTest, empty_path_returns_invalid_argument)
{
    // Arrange

    // Pre-Assert

    // Act
    int ret = com_util_makedirs("", NULL); // [手順] - 空文字列パスで com_util_makedirs を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_makedirs の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// com_util_stat が NULL 引数を拒否することの確認
TEST_F(makedirsTest, stat_rejects_null_arguments)
{
    // Arrange
    com_util_file_stat_t stat_buffer;

    // Pre-Assert

    // Act
    const int null_buffer_result =
        com_util_stat(NULL, NULL, "missing"); // [手順] - stat の出力先に NULL を指定する。
    const int null_path_result =
        com_util_stat(&stat_buffer, NULL, NULL); // [手順] - stat のパスに NULL を指定する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_buffer_result); // [確認_異常系] - 出力先 NULL の com_util_stat が INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_path_result); // [確認_異常系] - パス NULL の com_util_stat が INVALID_ARGUMENT を返すこと。
}

// 存在しないパスの stat が失敗することの確認
TEST_F(makedirsTest, stat_reports_missing_path)
{
    // Arrange
    com_util_file_stat_t stat_buffer;
    const std::string path = temp_path("makedirsTest_missing_stat");
    remove_dir(path); // [状態] - 対象パスが存在しないことを保証する。

    // Pre-Assert

    // Act
    const int result =
        com_util_stat(&stat_buffer, NULL, path.c_str()); // [手順] - 存在しないパスを指定して stat を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK,
              result); // [確認_異常系] - 存在しないパスの com_util_stat が COM_UTIL_OK 以外を返すこと。
}

// com_util_mkdir が NULL パスを拒否することの確認
TEST_F(makedirsTest, mkdir_rejects_null_path)
{
    // Arrange

    // Pre-Assert

    // Act
    const int result = com_util_mkdir(NULL, NULL); // [手順] - パスに NULL を指定して mkdir を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result); // [確認_異常系] - NULL パスの com_util_mkdir が INVALID_ARGUMENT を返すこと。
}

// 既存ディレクトリの mkdir が失敗することの確認
TEST_F(makedirsTest, mkdir_reports_existing_directory)
{
    // Arrange
    const std::string dir = temp_path("makedirsTest_existing_mkdir");
    remove_dir(dir);
    std::filesystem::create_directories(dir); // [状態] - 既存ディレクトリを用意する。

    // Pre-Assert

    // Act
    const int result =
        com_util_mkdir(dir.c_str(), NULL); // [手順] - 既存ディレクトリを指定して mkdir を呼び出す。

    // Assert
    EXPECT_NE(COM_UTIL_OK,
              result); // [確認_異常系] - 既存ディレクトリの com_util_mkdir が COM_UTIL_OK 以外を返すこと。

    // Cleanup
    remove_dir(dir);
}

// PLATFORM_PATH_MAX 以上のパスが拒否されることの確認
TEST_F(makedirsTest, overlong_path_returns_name_too_long)
{
    // Arrange
    const std::string path(PLATFORM_PATH_MAX, 'x'); // [状態] - PLATFORM_PATH_MAX 以上のパスを用意する。

    // Pre-Assert

    // Act
    const int result =
        com_util_makedirs(path.c_str(), NULL); // [手順] - 長過ぎるパスを指定して makedirs を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              result); // [確認_異常系] - 長過ぎるパスの com_util_makedirs が BUFFER_TOO_SMALL を返すこと。
}

// 単一階層ディレクトリの新規作成とべき等性の確認
TEST_F(makedirsTest, single_level_creates_directory)
{
    // Arrange
    com_util_file_stat_t st;
    std::string dir = temp_path("makedirsTest_single"); // [状態] - 一時ディレクトリ配下の作成対象パスを組み立てる。

    remove_dir(dir); // [状態] - 残留物があれば削除しておく。

    // Pre-Assert

    // Act
    // Assert
    int ret = com_util_makedirs(dir.c_str(),
                                NULL); // [手順] - 存在しない単一階層ディレクトリを com_util_makedirs で作成する。
    EXPECT_EQ(COM_UTIL_OK, ret);       // [確認_正常系] - com_util_makedirs の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, com_util_stat(&st, NULL, dir.c_str())); // [確認_正常系] - ディレクトリが存在すること。

    int ret2 = com_util_makedirs(dir.c_str(), NULL); // [手順] - 既存ディレクトリに com_util_makedirs を再呼び出しする。
    EXPECT_EQ(
        COM_UTIL_OK,
        ret2); // [確認_正常系] - 既存ディレクトリに対する com_util_makedirs の戻り値が COM_UTIL_OK であり、べき等であること。

    // Cleanup
    remove_dir(dir);
}

// 複数階層ディレクトリの再帰作成の確認
TEST_F(makedirsTest, nested_levels_creates_all_directories)
{
    // Arrange
    com_util_file_stat_t st;
    std::string root = temp_path("makedirsTest_root");
    std::string nested =
        root + "/sub/leaf"; // [状態] - 中間ディレクトリが欠けた root/sub/leaf の 2 階層パスを組み立てる。

    remove_dir(root); // [状態] - 残留物があれば削除しておく。

    // Pre-Assert

    // Act
    int ret = com_util_makedirs(nested.c_str(),
                                NULL); // [手順] - 中間ディレクトリが欠けた 2 階層パスを com_util_makedirs で作成する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_makedirs の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              com_util_stat(&st, NULL, nested.c_str())); // [確認_正常系] - リーフ ディレクトリが存在すること。

    // Cleanup
    remove_dir(root);
}

// 相対パスの複数階層ディレクトリを作成できることの確認
TEST_F(makedirsTest, relative_path_creates_nested_directories)
{
    // Arrange
    const std::string path = "makedirsTest_relative/sub";
    remove_dir(path); // [状態] - 相対パスの残留物があれば削除しておく。

    // Pre-Assert

    // Act
    const int result =
        com_util_makedirs(path.c_str(), NULL); // [手順] - 相対パスの複数階層を指定して makedirs を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - 相対パスの com_util_makedirs が COM_UTIL_OK を返すこと。
    EXPECT_TRUE(std::filesystem::is_directory(path)); // [確認_正常系] - 相対パスの末尾ディレクトリが作成されること。

    // Cleanup
    remove_dir("makedirsTest_relative");
}

#if defined(PLATFORM_WINDOWS)
// Windows スタイル区切りの絶対パスで再帰作成できることの確認
TEST_F(makedirsTest, windows_separator_path_creates_directory)
{
    // Arrange
    com_util_file_stat_t st;
    std::string root = temp_path("makedirsTest_windows_sep");
    std::string nested = root + "/sub/leaf";

    remove_dir(root); // [状態] - 残留物があれば削除しておく。

    std::string windows_path =
        to_windows_sep(nested.c_str()); // [状態] - 区切りを '\\' に置き換えた Windows スタイルのパスとする。

    // Pre-Assert

    // Act
    // Assert
    int ret = com_util_makedirs(windows_path.c_str(),
                                NULL); // [手順] - Windows スタイル区切りの 2 階層パスを com_util_makedirs で作成する。
    EXPECT_EQ(COM_UTIL_OK, ret);       // [確認_正常系] - com_util_makedirs の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              com_util_stat(&st, NULL, nested.c_str())); // [確認_正常系] - 正規化後のパスでリーフが存在すること。

    int ret2 = com_util_makedirs(windows_path.c_str(),
                                 NULL); // [手順] - 既存ディレクトリに com_util_makedirs を再呼び出しする。
    EXPECT_EQ(
        COM_UTIL_OK,
        ret2); // [確認_正常系] - 既存ディレクトリに対する com_util_makedirs の戻り値が COM_UTIL_OK であり、べき等であること。

    // Cleanup
    remove_dir(root);
}
#endif /* PLATFORM_WINDOWS */
