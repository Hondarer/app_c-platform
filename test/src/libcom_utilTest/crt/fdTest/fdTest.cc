#include <testfw.h>
#include <com_util/crt/unistd.h>
#include <com_util/crt/fcntl.h>

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

    void SetUp() override
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/fdTest/results";

        std::filesystem::create_directories(dir);
        path_ = (dir / "fdTest_work.bin").generic_string();

        fd_ = open_work_file();
        ASSERT_LE(0, fd_);
    }

    void TearDown() override
    {
        if (fd_ >= 0)
        {
            com_util_close(fd_);
            fd_ = -1;
        }
        std::remove(path_.c_str());
    }

    int open_work_file()
    {
#if defined(PLATFORM_LINUX)
        return com_util_open(path_.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
#elif defined(PLATFORM_WINDOWS)
        return com_util_open(path_.c_str(), _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
#endif /* PLATFORM_ */
    }
};

TEST_F(fdTest, write_read_lseek_roundtrip)
{
    // Arrange
    const char data[] = "abcdef"; // [状態] - 書き込みデータを "abcdef" (6 バイト) とする。
    char buf[16];

    // Pre-Assert

    // Act
    int64_t written = com_util_write(fd_, data, 6);          // [手順] - 6 バイトを書き込む。
    int64_t pos_head = com_util_lseek(fd_, 0, SEEK_SET);     // [手順] - 読み書き位置を先頭へ移動する。
    int64_t read_len = com_util_read(fd_, buf, sizeof(buf)); // [手順] - ファイル全体を読み取る。
    int64_t pos_end = com_util_lseek(fd_, 0, SEEK_END);      // [手順] - 読み書き位置を終端へ移動する。

    // Assert
    EXPECT_EQ(6, written);              // [確認] - 書き込んだバイト数が 6 であること。
    EXPECT_EQ(0, pos_head);             // [確認] - SEEK_SET 0 の戻り値が 0 であること。
    EXPECT_EQ(6, read_len);             // [確認] - 読み取ったバイト数が 6 であること。
    EXPECT_EQ(0, memcmp(data, buf, 6)); // [確認] - 読み取った内容が書き込んだ内容と一致すること。
    EXPECT_EQ(6, pos_end);              // [確認] - SEEK_END 0 の戻り値がファイル サイズであること。
}

TEST_F(fdTest, read_reaches_eof_returns_zero)
{
    // Arrange
    char buf[8]; // [状態] - 空ファイルの先頭で読み取る。

    // Pre-Assert

    // Act
    int64_t read_len = com_util_read(fd_, buf, sizeof(buf)); // [手順] - 空ファイルから読み取る。

    // Assert
    EXPECT_EQ(0, read_len); // [確認] - ファイル終端では 0 が返ること。
}

TEST_F(fdTest, dup_shares_file_offset)
{
    // Arrange

    // Pre-Assert

    // Act
    int dup_fd = com_util_dup(fd_); // [手順] - ファイル記述子を複製する。

    // Assert
    ASSERT_LE(0, dup_fd);                            // [確認] - 複製された記述子が有効であること。
    EXPECT_EQ(4, com_util_write(dup_fd, "wxyz", 4)); // [確認] - 複製側で 4 バイト書き込めること。
    EXPECT_EQ(4, com_util_lseek(fd_, 0, SEEK_CUR));  // [確認] - 複製元の読み書き位置が共有され 4 に進んでいること。
    EXPECT_EQ(0, com_util_close(dup_fd));            // [確認] - 複製側のクローズが成功すること。
}

TEST_F(fdTest, dup2_returns_zero_on_success)
{
    // Arrange
    int target_fd = com_util_dup(fd_); // [状態] - 複製先とする有効な記述子番号を確保する。

    ASSERT_LE(0, target_fd);

    // Pre-Assert

    // Act
    int rtc = com_util_dup2(fd_, target_fd); // [手順] - fd_ を target_fd へ複製する。

    // Assert
    EXPECT_EQ(0, rtc);                       // [確認] - POSIX/Windows とも成功時は 0 に正規化されること。
    EXPECT_EQ(0, com_util_close(target_fd)); // [確認] - 複製先のクローズが成功すること。
}

TEST_F(fdTest, close_success_returns_zero)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc = com_util_close(fd_); // [手順] - 開いている記述子を閉じる。

    // Assert
    EXPECT_EQ(0, rtc); // [確認] - クローズ成功時は 0 が返ること。
    fd_ = -1;
}

TEST_F(fdTest, negative_fd_returns_minus1)
{
    // Arrange
    char buf[4]; // [状態] - すべての関数に負のファイル記述子を与える。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(-1, com_util_lseek(-1, 0, SEEK_SET));      // [確認_異常系] - lseek が -1 を返すこと。
    EXPECT_EQ(-1, com_util_close(-1));                   // [確認_異常系] - close が -1 を返すこと。
    EXPECT_EQ(-1, com_util_dup(-1));                     // [確認_異常系] - dup が -1 を返すこと。
    EXPECT_EQ(-1, com_util_dup2(-1, fd_));               // [確認_異常系] - dup2 (oldfd 負) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_dup2(fd_, -1));               // [確認_異常系] - dup2 (newfd 負) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_read(-1, buf, sizeof(buf)));  // [確認_異常系] - read が -1 を返すこと。
    EXPECT_EQ(-1, com_util_write(-1, buf, sizeof(buf))); // [確認_異常系] - write が -1 を返すこと。
}

TEST_F(fdTest, null_buf_returns_minus1)
{
    // Arrange

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(-1, com_util_read(fd_, NULL, 4));  // [確認_異常系] - read (buf NULL) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_write(fd_, NULL, 4)); // [確認_異常系] - write (buf NULL) が -1 を返すこと。
}

TEST_F(fdTest, lseek_invalid_whence_returns_minus1)
{
    // Arrange

    // Pre-Assert

    // Act
    int64_t rtc = com_util_lseek(fd_, 0, 99); // [手順] - 定義外の whence を与える。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - OS の API を呼び出さずに -1 が返ること。
}

#if defined(PLATFORM_LINUX)
/* Windows の CRT はクローズ済み記述子で invalid parameter handler を起動するため、Linux でのみ実施する */
TEST_F(fdTest, closed_fd_operations_return_minus1)
{
    // Arrange
    int closed_fd = fd_; // [状態] - クローズ済みの記述子を用意する。
    char buf[4];

    ASSERT_EQ(0, com_util_close(fd_));
    fd_ = -1;

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(-1, com_util_lseek(closed_fd, 0, SEEK_SET));      // [確認_異常系] - lseek が -1 を返すこと。
    EXPECT_EQ(-1, com_util_read(closed_fd, buf, sizeof(buf)));  // [確認_異常系] - read が -1 を返すこと。
    EXPECT_EQ(-1, com_util_write(closed_fd, buf, sizeof(buf))); // [確認_異常系] - write が -1 を返すこと。
    EXPECT_EQ(-1, com_util_dup(closed_fd));                     // [確認_異常系] - dup が -1 を返すこと。
    EXPECT_EQ(-1, com_util_dup2(closed_fd, closed_fd));         // [確認_異常系] - dup2 が -1 を返すこと。
    EXPECT_EQ(-1, com_util_close(closed_fd));                   // [確認_異常系] - close が -1 を返すこと。
}
#endif /* PLATFORM_LINUX */
