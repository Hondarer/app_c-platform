#include <testfw.h>
#include <com_util/crt/unistd.h>
#include <com_util/crt/fcntl.h>
#include <mock_unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(PLATFORM_WINDOWS)
    #include <sys/stat.h>
#endif /* PLATFORM_WINDOWS */

class fdTest : public Test
{
  protected:
    std::string path_;
    int fd_ = -1;
    int pad_ = 0;

    void SetUp() override
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/unistdTest/results";

        std::filesystem::create_directories(dir);
        path_ = (dir / "fdTest_work.bin").generic_string();

        fd_ = open_work_file();
        ASSERT_LE(0, fd_);
    }

    void TearDown() override
    {
        if (fd_ >= 0)
        {
            com_util_close(fd_, NULL);
            fd_ = -1;
        }
        std::remove(path_.c_str());
    }

    int open_work_file()
    {
#if defined(PLATFORM_LINUX)
        return com_util_open(path_.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644, NULL);
#elif defined(PLATFORM_WINDOWS)
        return com_util_open(path_.c_str(), _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE, NULL);
#endif /* PLATFORM_ */
    }
};

// write、lseek、read の一連の操作でデータが往復することの確認
TEST_F(fdTest, write_read_lseek_roundtrip)
{
    // Arrange
    const char data[] = "abcdef"; // [状態] - 書き込みデータを "abcdef" (6 バイト) とする。
    char buf[16];

    // Pre-Assert

    // Act
    int64_t written = com_util_write(fd_, data, 6, NULL);          // [手順] - 6 バイトを書き込む。
    int64_t pos_head = com_util_lseek(fd_, 0, SEEK_SET, NULL);     // [手順] - 読み書き位置を先頭へ移動する。
    int64_t read_len = com_util_read(fd_, buf, sizeof(buf), NULL); // [手順] - ファイル全体を読み取る。
    int64_t pos_end = com_util_lseek(fd_, 0, SEEK_END, NULL);      // [手順] - 読み書き位置を終端へ移動する。

    // Assert
    EXPECT_EQ(6, written);  // [確認_正常系] - 書き込んだバイト数が 6 であること。
    EXPECT_EQ(0, pos_head); // [確認_正常系] - SEEK_SET 0 を指定した com_util_lseek の戻り値が 0 であること。
    EXPECT_EQ(6, read_len); // [確認_正常系] - 読み取ったバイト数が 6 であること。
    EXPECT_EQ(0, memcmp(data, buf, 6)); // [確認_正常系] - 読み取った内容が書き込んだ内容と一致すること。
    EXPECT_EQ(6, pos_end); // [確認_正常系] - SEEK_END 0 を指定した com_util_lseek の戻り値がファイル サイズであること。
}

// ファイル終端の読み取りで 0 が返ることの確認
TEST_F(fdTest, read_reaches_eof_returns_zero)
{
    // Arrange
    char buf[8]; // [状態] - 読み取り先バッファーを用意する。ファイルは空のままとする。

    // Pre-Assert

    // Act
    int64_t read_len = com_util_read(fd_, buf, sizeof(buf), NULL); // [手順] - 空ファイルから読み取る。

    // Assert
    EXPECT_EQ(0, read_len); // [確認_正常系] - com_util_read の戻り値として、ファイル終端では 0 が返ること。
}

// dup で複製した記述子が読み書き位置を共有することの確認
TEST_F(fdTest, dup_shares_file_offset)
{
    // Arrange

    // Pre-Assert

    // Act
    int dup_fd = com_util_dup(fd_, NULL); // [手順] - ファイル記述子を複製する。

    // Assert
    ASSERT_LE(0, dup_fd);                                  // [確認_正常系] - 複製された記述子が有効であること。
    EXPECT_EQ(4, com_util_write(dup_fd, "wxyz", 4, NULL)); // [確認_正常系] - 複製側で 4 バイト書き込めること。
    EXPECT_EQ(4, com_util_lseek(fd_, 0, SEEK_CUR,
                                NULL)); // [確認_正常系] - 複製元の読み書き位置が共有され 4 に進んでいること。
    EXPECT_EQ(
        0,
        com_util_close(
            dup_fd, NULL)); // [確認_正常系] - com_util_close の戻り値から、複製側のクローズが成功したと判断できること。
}

// dup2 の成功時に 0 が返ることの確認
TEST_F(fdTest, dup2_returns_zero_on_success)
{
    // Arrange
    int target_fd = com_util_dup(fd_, NULL); // [状態] - 複製先とする有効な記述子番号を確保する。

    ASSERT_LE(0, target_fd);

    // Pre-Assert

    // Act
    int rtc = com_util_dup2(fd_, target_fd, NULL); // [手順] - fd_ を target_fd へ複製する。

    // Assert
    EXPECT_EQ(0, rtc); // [確認_正常系] - POSIX/Windows とも成功時は 0 に正規化されること。
    EXPECT_EQ(0,
              com_util_close(
                  target_fd,
                  NULL)); // [確認_正常系] - com_util_close の戻り値から、複製先のクローズが成功したと判断できること。
}

// close の成功時に 0 が返ることの確認
TEST_F(fdTest, close_success_returns_zero)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc = com_util_close(fd_, NULL); // [手順] - 開いている記述子を閉じる。

    // Assert
    EXPECT_EQ(0, rtc); // [確認_正常系] - com_util_close の戻り値として、クローズ成功時は 0 が返ること。
    fd_ = -1;
}

// 負のファイル記述子で各関数が -1 を返すことの確認
TEST_F(fdTest, negative_fd_returns_minus1)
{
    // Arrange
    char buf[4]; // [状態] - 読み書き用バッファーを用意する。

    // Pre-Assert

    // Act
    // [手順] - 負のファイル記述子で lseek、close、dup、dup2、read、write を呼び出す。
    int64_t rtc_lseek = com_util_lseek(-1, 0, SEEK_SET, NULL);
    int rtc_close = com_util_close(-1, NULL);
    int rtc_dup = com_util_dup(-1, NULL);
    int rtc_dup2_oldfd = com_util_dup2(-1, fd_, NULL);
    int rtc_dup2_newfd = com_util_dup2(fd_, -1, NULL);
    int64_t rtc_read = com_util_read(-1, buf, sizeof(buf), NULL);
    int64_t rtc_write = com_util_write(-1, buf, sizeof(buf), NULL);

    // Assert
    EXPECT_EQ(-1, rtc_lseek);      // [確認_異常系] - lseek が -1 を返すこと。
    EXPECT_EQ(-1, rtc_close);      // [確認_異常系] - close が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup);        // [確認_異常系] - dup が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup2_oldfd); // [確認_異常系] - dup2 (oldfd 負) が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup2_newfd); // [確認_異常系] - dup2 (newfd 負) が -1 を返すこと。
    EXPECT_EQ(-1, rtc_read);       // [確認_異常系] - read が -1 を返すこと。
    EXPECT_EQ(-1, rtc_write);      // [確認_異常系] - write が -1 を返すこと。
}

// バッファーが NULL の場合に read / write が -1 を返すことの確認
TEST_F(fdTest, null_buf_returns_minus1)
{
    // Arrange

    // Pre-Assert

    // Act
    // [手順] - NULL バッファーで read と write を呼び出す。
    int64_t rtc_read = com_util_read(fd_, NULL, 4, NULL);
    int64_t rtc_write = com_util_write(fd_, NULL, 4, NULL);

    // Assert
    EXPECT_EQ(-1, rtc_read);  // [確認_異常系] - read (buf NULL) が -1 を返すこと。
    EXPECT_EQ(-1, rtc_write); // [確認_異常系] - write (buf NULL) が -1 を返すこと。
}

// 定義外の whence を与えた lseek が -1 を返すことの確認
TEST_F(fdTest, lseek_invalid_whence_returns_minus1)
{
    // Arrange

    // Pre-Assert

    // Act
    int64_t rtc = com_util_lseek(fd_, 0, 99, NULL); // [手順] - 定義外の whence を与える。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_lseek の戻り値として、OS の API を呼び出さずに -1 が返ること。
}

// 下位の lseek 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, lseek_returns_minus1_when_platform_lseek_fails)
{
    // Arrange
    Mock_unistd mock_unistd; // [状態] - 下位の lseek 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の lseek 系 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の lseek 系 API から -1 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd, lseek(_, _, _, _, _, _)).WillOnce(Return(-1));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd, _lseeki64(_, _, _, _, _, _)).WillOnce(Return(-1));
#endif

    // Act
    int64_t rtc = com_util_lseek(fd_, 0, SEEK_SET, NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_lseek の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の close 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, close_returns_minus1_when_platform_close_fails)
{
    // Arrange
    Mock_unistd mock_unistd; // [状態] - 下位の close 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の close 系 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の close 系 API から -1 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd, close(_, _, _, _)).WillOnce(Return(-1));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd, _close(_, _, _, _)).WillOnce(Return(-1));
#endif

    // Act
    int rtc = com_util_close(fd_, NULL); // [手順] - 有効な記述子で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_close の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の dup 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, dup_returns_minus1_when_platform_dup_fails)
{
    // Arrange
    Mock_unistd mock_unistd; // [状態] - 下位の dup 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の dup 系 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の dup 系 API から -1 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd, dup(_, _, _, _)).WillOnce(Return(-1));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd, _dup(_, _, _, _)).WillOnce(Return(-1));
#endif

    // Act
    int rtc = com_util_dup(fd_, NULL); // [手順] - 有効な記述子で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_dup の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の dup2 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, dup2_returns_minus1_when_platform_dup2_fails)
{
    // Arrange
    Mock_unistd mock_unistd; // [状態] - 下位の dup2 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の dup2 系 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の dup2 系 API から -1 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd, dup2(_, _, _, _, _)).WillOnce(Return(-1));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd, _dup2(_, _, _, _, _)).WillOnce(Return(-1));
#endif

    // Act
    int rtc = com_util_dup2(fd_, fd_, NULL); // [手順] - 有効な記述子で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_dup2 の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の read 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, read_returns_minus1_when_platform_read_fails)
{
    // Arrange
    char buf[4];
    Mock_unistd mock_unistd; // [状態] - 下位の read 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の read 系 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の read 系 API から -1 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd, read(_, _, _, _, _, _)).WillOnce(Return(-1));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd, _read(_, _, _, _, _, _)).WillOnce(Return(-1));
#endif

    // Act
    int64_t rtc = com_util_read(fd_, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_read の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の write 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, write_returns_minus1_when_platform_write_fails)
{
    // Arrange
    const char buf[4] = "abc";
    Mock_unistd mock_unistd; // [状態] - 下位の write 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_異常系] - 下位の write 系 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 下位の write 系 API から -1 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd, write(_, _, _, _, _, _)).WillOnce(Return(-1));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd, _write(_, _, _, _, _, _)).WillOnce(Return(-1));
#endif

    // Act
    int64_t rtc = com_util_write(fd_, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_write の戻り値として、OS の失敗がそのまま -1 として返ること。
}

#if defined(PLATFORM_LINUX)
/* シグナルによる中断は Linux 固有のため、再試行の確認は Linux でのみ実施する */
// 下位の read 系 API がシグナルで中断された場合に再試行されることの確認
TEST_F(fdTest, read_retries_after_interrupt)
{
    // Arrange
    char buf[4];
    Mock_unistd mock_unistd; // [状態] - 下位の read 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の read 系 API が 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の read 系 API から、errno に EINTR を設定した -1 ののち 4 を返却する。
    EXPECT_CALL(mock_unistd, read(_, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(Return(4));

    // Act
    int64_t rtc = com_util_read(fd_, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ((int64_t)4,
              rtc); // [確認_正常系] - com_util_read の戻り値が、再試行後の転送量である 4 であること。
}

// 下位の write 系 API がシグナルで中断された場合に再試行されることの確認
TEST_F(fdTest, write_retries_after_interrupt)
{
    // Arrange
    const char buf[4] = "abc";
    Mock_unistd mock_unistd; // [状態] - 下位の write 系 API をモック化する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 下位の write 系 API が 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の write 系 API から、errno に EINTR を設定した -1 ののち 4 を返却する。
    EXPECT_CALL(mock_unistd, write(_, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(Return(4));

    // Act
    int64_t rtc = com_util_write(fd_, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ((int64_t)4,
              rtc); // [確認_正常系] - com_util_write の戻り値が、再試行後の転送量である 4 であること。
}
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_LINUX)
/* Windows の CRT はクローズ済み記述子で invalid parameter handler を起動するため、Linux でのみ実施する */
// クローズ済みの記述子で各関数が -1 を返すことの確認
TEST_F(fdTest, closed_fd_operations_return_minus1)
{
    // Arrange
    int closed_fd = fd_; // [状態] - クローズ済みの記述子を用意する。
    char buf[4];

    ASSERT_EQ(0, com_util_close(fd_, NULL));
    fd_ = -1;

    // Pre-Assert

    // Act
    // [手順] - クローズ済みのファイル記述子で lseek、read、write、dup、dup2、close を呼び出す。
    int64_t rtc_lseek = com_util_lseek(closed_fd, 0, SEEK_SET, NULL);
    int64_t rtc_read = com_util_read(closed_fd, buf, sizeof(buf), NULL);
    int64_t rtc_write = com_util_write(closed_fd, buf, sizeof(buf), NULL);
    int rtc_dup = com_util_dup(closed_fd, NULL);
    int rtc_dup2 = com_util_dup2(closed_fd, closed_fd, NULL);
    int rtc_close = com_util_close(closed_fd, NULL);

    // Assert
    EXPECT_EQ(-1, rtc_lseek); // [確認_異常系] - lseek が -1 を返すこと。
    EXPECT_EQ(-1, rtc_read);  // [確認_異常系] - read が -1 を返すこと。
    EXPECT_EQ(-1, rtc_write); // [確認_異常系] - write が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup);   // [確認_異常系] - dup が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup2);  // [確認_異常系] - dup2 が -1 を返すこと。
    EXPECT_EQ(-1, rtc_close); // [確認_異常系] - close が -1 を返すこと。
}
#endif /* PLATFORM_LINUX */
