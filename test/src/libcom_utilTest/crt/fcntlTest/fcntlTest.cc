#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <cstdio>
#include <string>

#if defined(PLATFORM_WINDOWS)
    #include <sys/stat.h>
#endif /* PLATFORM_WINDOWS */

class fcntlTest : public Test
{
  protected:
    std::string path_;

    void SetUp() override
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/fcntlTest/results";

        std::filesystem::create_directories(dir);
        path_ = (dir / "fcntlTest_work.bin").generic_string();
        std::remove(path_.c_str());
    }

    void TearDown() override
    {
        std::remove(path_.c_str());
    }

    int create_flags()
    {
#if defined(PLATFORM_LINUX)
        return O_RDWR | O_CREAT | O_TRUNC;
#elif defined(PLATFORM_WINDOWS)
        return _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY;
#endif /* PLATFORM_ */
    }

    int create_mode()
    {
#if defined(PLATFORM_LINUX)
        return 0644;
#elif defined(PLATFORM_WINDOWS)
        return _S_IREAD | _S_IWRITE;
#endif /* PLATFORM_ */
    }
};

// 新規ファイルが作成され記述子が返ることの確認
TEST_F(fcntlTest, creates_file_and_returns_descriptor)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int fd = com_util_open(path_.c_str(), create_flags(), create_mode(),
                           &detail); // [手順] - 存在しないパスに作成フラグを指定して com_util_open を呼び出す。

    // Assert
    EXPECT_LE(0, fd); // [確認_正常系] - com_util_open の戻り値が 0 以上のファイル記述子であること。
    EXPECT_EQ(0, com_util_error_is_set(&detail)); // [確認_正常系] - detail に詳細エラーが記録されないこと。

    // Cleanup
    com_util_close(fd, NULL);
}

// 存在しないファイルを読み取り専用で開くと失敗することの確認
TEST_F(fcntlTest, returns_minus1_for_missing_file)
{
    // Arrange
    std::string missing = path_ + ".missing"; // [状態] - 存在しないパスを用意する。
    com_util_error detail;                    // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int fd = com_util_open(missing.c_str(), O_RDONLY, 0,
                           &detail); // [手順] - 存在しないパスを読み取り専用で com_util_open に指定する。

    // Assert
    EXPECT_EQ(-1, fd); // [確認_異常系] - com_util_open の戻り値が -1 であること。
    EXPECT_EQ(1, com_util_error_is(&detail,
                                   COM_UTIL_CAUSE_NOT_FOUND)); // [確認_異常系] - 見つからないことが要因であること。
}

// パスに NULL を渡した場合に EINVAL とともに失敗することの確認
TEST_F(fcntlTest, returns_minus1_for_null_path)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int fd = com_util_open(NULL, O_RDONLY, 0, &detail); // [手順] - パスに NULL を指定して com_util_open を呼び出す。

    // Assert
    EXPECT_EQ(-1, fd); // [確認_異常系] - com_util_open の戻り値が -1 であること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
}

// detail_out に NULL を渡してもクラッシュしないことの確認
TEST_F(fcntlTest, accepts_null_detail_out)
{
    // Arrange

    // Pre-Assert

    // Act
    int fd = com_util_open(NULL, O_RDONLY, 0, NULL); // [手順] - detail_out に NULL を指定して com_util_open を呼び出す。

    // Assert
    EXPECT_EQ(-1, fd); // [確認_異常系] - com_util_open の戻り値が -1 であること。
}

#if defined(PLATFORM_WINDOWS)

// Windows でパスがワイド文字へ変換できない場合に ENAMETOOLONG が返ることの確認
// Windows の com_util_open は _wsopen_s を呼ぶ前に com_util_utf8_to_wpath で変換するため、
// この分岐は Windows のみに存在する
TEST_F(fcntlTest, returns_enametoolong_when_path_exceeds_wide_buffer)
{
    // Arrange
    std::string long_path(PLATFORM_PATH_MAX + 1u,
                          'a'); // [状態] - PLATFORM_PATH_MAX を 1 文字超えるパスを用意する。
    com_util_error detail;      // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int fd = com_util_open(long_path.c_str(), O_RDONLY, 0,
                           &detail); // [手順] - 変換先バッファーに収まらないパスで com_util_open を呼び出す。

    // Assert
    EXPECT_EQ(-1, fd); // [確認_異常系] - com_util_open の戻り値が -1 であること。
    EXPECT_EQ(1,
              com_util_error_is(&detail,
                                COM_UTIL_CAUSE_NAME_TOO_LONG)); // [確認_異常系] - ENAMETOOLONG の要因であること。
}

#endif /* PLATFORM_WINDOWS */
