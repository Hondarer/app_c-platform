#include <testfw.h>
#include <com_util/base/platform.h>
#include <com_util/crt/unistd.h>
#include <mock_unistd.h>

#include <cerrno>
#include <cstring>

using testing::_;
using testing::Assign;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;

namespace
{
const int kFakeFd = 7;
const int kDupFd = 8;
} // namespace

class fdTest : public testing::Test
{
  protected:
    NiceMock<Mock_unistd> mock_unistd_;
};

// write、lseek、read の一連の操作でデータが往復することの確認
TEST_F(fdTest, write_read_lseek_roundtrip)
{
    // Arrange
    const char data[] = "abcdef"; // [状態] - 書き込みデータを "abcdef" (6 バイト) とする。
    char buf[16] = {};

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, write(_, _, _, kFakeFd, data, 6u))
        .WillOnce(Return(6)); // [Pre-Assert確認_正常系] - write が番兵記述子 7 で 1 回呼び出されること。
                              // [Pre-Assert手順] - 6 を返却する。
    EXPECT_CALL(mock_unistd_, lseek(_, _, _, kFakeFd, static_cast<off_t>(0), SEEK_SET))
        .WillOnce(
            Return(static_cast<off_t>(0))); // [Pre-Assert確認_正常系] - lseek が SEEK_SET で 1 回呼び出されること。
                                            // [Pre-Assert手順] - 位置 0 を返却する。
    EXPECT_CALL(mock_unistd_, read(_, _, _, kFakeFd, buf, sizeof(buf)))
        .WillOnce(
            [](const char *, int, const char *, int, void *dest, size_t)
            {
                std::memcpy(dest, "abcdef", 6u);
                return static_cast<ssize_t>(6);
            }); // [Pre-Assert確認_正常系] - read が番兵記述子 7 で 1 回呼び出されること。
                // [Pre-Assert手順] - "abcdef" を格納し、6 を返却する。
    EXPECT_CALL(mock_unistd_, lseek(_, _, _, kFakeFd, static_cast<off_t>(0), SEEK_END))
        .WillOnce(
            Return(static_cast<off_t>(6))); // [Pre-Assert確認_正常系] - lseek が SEEK_END で 1 回呼び出されること。
                                            // [Pre-Assert手順] - 位置 6 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _write(_, _, _, kFakeFd, data, 6u)).WillOnce(Return(6));
    EXPECT_CALL(mock_unistd_, _lseeki64(_, _, _, kFakeFd, static_cast<__int64>(0), SEEK_SET))
        .WillOnce(Return(static_cast<__int64>(0)));
    EXPECT_CALL(mock_unistd_, _read(_, _, _, kFakeFd, buf, 16u))
        .WillOnce(
            [](const char *, int, const char *, int, void *dest, unsigned int)
            {
                std::memcpy(dest, "abcdef", 6u);
                return 6;
            });
    EXPECT_CALL(mock_unistd_, _lseeki64(_, _, _, kFakeFd, static_cast<__int64>(0), SEEK_END))
        .WillOnce(Return(static_cast<__int64>(6)));
#endif /* PLATFORM_ */

    // Act
    int64_t written = com_util_write(kFakeFd, data, 6, NULL);          // [手順] - 6 バイトを書き込む。
    int64_t pos_head = com_util_lseek(kFakeFd, 0, SEEK_SET, NULL);     // [手順] - 読み書き位置を先頭へ移動する。
    int64_t read_len = com_util_read(kFakeFd, buf, sizeof(buf), NULL); // [手順] - ファイル全体を読み取る。
    int64_t pos_end = com_util_lseek(kFakeFd, 0, SEEK_END, NULL);      // [手順] - 読み書き位置を終端へ移動する。

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
    char buf[8]; // [状態] - 読み取り先バッファーを用意する。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, read(_, _, _, kFakeFd, buf, sizeof(buf)))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - read が番兵記述子 7 で 1 回呼び出されること。
                              // [Pre-Assert手順] - 0 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _read(_, _, _, kFakeFd, buf, 8u)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    int64_t read_len = com_util_read(kFakeFd, buf, sizeof(buf), NULL); // [手順] - 空ファイルから読み取る。

    // Assert
    EXPECT_EQ(0, read_len); // [確認_正常系] - com_util_read の戻り値として、ファイル終端では 0 が返ること。
}

// dup で複製した記述子へ書き込めることの確認
TEST_F(fdTest, dup_shares_file_offset)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, dup(_, _, _, kFakeFd))
        .WillOnce(Return(kDupFd)); // [Pre-Assert確認_正常系] - dup が番兵記述子 7 で 1 回呼び出されること。
                                   // [Pre-Assert手順] - 複製記述子 8 を返却する。
    EXPECT_CALL(mock_unistd_, write(_, _, _, kDupFd, _, 4u))
        .WillOnce(Return(4)); // [Pre-Assert確認_正常系] - write が複製記述子 8 で 1 回呼び出されること。
                              // [Pre-Assert手順] - 4 を返却する。
    EXPECT_CALL(mock_unistd_, lseek(_, _, _, kFakeFd, static_cast<off_t>(0), SEEK_CUR))
        .WillOnce(
            Return(static_cast<off_t>(4))); // [Pre-Assert確認_正常系] - lseek が SEEK_CUR で 1 回呼び出されること。
                                            // [Pre-Assert手順] - 位置 4 を返却する。
    EXPECT_CALL(mock_unistd_, close(_, _, _, kDupFd))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - close が複製記述子 8 で 1 回呼び出されること。
                              // [Pre-Assert手順] - 0 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _dup(_, _, _, kFakeFd)).WillOnce(Return(kDupFd));
    EXPECT_CALL(mock_unistd_, _write(_, _, _, kDupFd, _, 4u)).WillOnce(Return(4));
    EXPECT_CALL(mock_unistd_, _lseeki64(_, _, _, kFakeFd, static_cast<__int64>(0), SEEK_CUR))
        .WillOnce(Return(static_cast<__int64>(4)));
    EXPECT_CALL(mock_unistd_, _close(_, _, _, kDupFd)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    int dup_fd = com_util_dup(kFakeFd, NULL);                  // [手順] - ファイル記述子を複製する。
    int64_t written = com_util_write(dup_fd, "wxyz", 4, NULL); // [手順] - 複製側へ 4 バイト書き込む。
    int64_t pos = com_util_lseek(kFakeFd, 0, SEEK_CUR, NULL);  // [手順] - 複製元の現在位置を取得する。
    int close_rtc = com_util_close(dup_fd, NULL);              // [手順] - 複製側を閉じる。

    // Assert
    EXPECT_EQ(kDupFd, dup_fd); // [確認_正常系] - com_util_dup の戻り値が複製記述子 8 であること。
    EXPECT_EQ(4, written);     // [確認_正常系] - 複製側へ書き込んだバイト数が 4 であること。
    EXPECT_EQ(4, pos);         // [確認_正常系] - SEEK_CUR を指定した com_util_lseek の戻り値が 4 であること。
    EXPECT_EQ(0, close_rtc);   // [確認_正常系] - 複製側の com_util_close の戻り値が 0 であること。
}

// dup2 の成功時に 0 が返ることの確認
TEST_F(fdTest, dup2_returns_zero_on_success)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, dup2(_, _, _, kFakeFd, kDupFd))
        .WillOnce(Return(kDupFd)); // [Pre-Assert確認_正常系] - dup2 が 7 から 8 へ 1 回呼び出されること。
                                   // [Pre-Assert手順] - 複製先記述子 8 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _dup2(_, _, _, kFakeFd, kDupFd)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_dup2(kFakeFd, kDupFd, NULL); // [手順] - 番兵記述子 7 を 8 へ複製する。

    // Assert
    EXPECT_EQ(0, rtc); // [確認_正常系] - POSIX/Windows とも成功時は 0 に正規化されること。
}

// close の成功時に 0 が返ることの確認
TEST_F(fdTest, close_success_returns_zero)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, close(_, _, _, kFakeFd))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - close が番兵記述子 7 で 1 回呼び出されること。
                              // [Pre-Assert手順] - 0 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _close(_, _, _, kFakeFd)).WillOnce(Return(0));
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_close(kFakeFd, NULL); // [手順] - 開いている記述子を閉じる。

    // Assert
    EXPECT_EQ(0, rtc); // [確認_正常系] - com_util_close の戻り値として、クローズ成功時は 0 が返ること。
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
    int rtc_dup2_oldfd = com_util_dup2(-1, kFakeFd, NULL);
    int rtc_dup2_newfd = com_util_dup2(kFakeFd, -1, NULL);
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
    int64_t rtc_read = com_util_read(kFakeFd, NULL, 4, NULL);
    int64_t rtc_write = com_util_write(kFakeFd, NULL, 4, NULL);

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
    int64_t rtc = com_util_lseek(kFakeFd, 0, 99, NULL); // [手順] - 定義外の whence を与える。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_lseek の戻り値として、OS の API を呼び出さずに -1 が返ること。
}

// 下位の lseek 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, lseek_returns_minus1_when_platform_lseek_fails)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, lseek(_, _, _, _, _, _))
        .WillOnce(
            Return(static_cast<off_t>(-1))); // [Pre-Assert確認_異常系] - 下位の lseek 系 API が 1 回呼び出されること。
                                             // [Pre-Assert手順] - 下位の lseek 系 API から -1 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _lseeki64(_, _, _, _, _, _)).WillOnce(Return(static_cast<__int64>(-1)));
#endif /* PLATFORM_ */

    // Act
    int64_t rtc = com_util_lseek(kFakeFd, 0, SEEK_SET, NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_lseek の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の close 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, close_returns_minus1_when_platform_close_fails)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, close(_, _, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 下位の close 系 API が 1 回呼び出されること。
                               // [Pre-Assert手順] - 下位の close 系 API から -1 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _close(_, _, _, _)).WillOnce(Return(-1));
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_close(kFakeFd, NULL); // [手順] - 有効な記述子で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_close の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の dup 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, dup_returns_minus1_when_platform_dup_fails)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, dup(_, _, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 下位の dup 系 API が 1 回呼び出されること。
                               // [Pre-Assert手順] - 下位の dup 系 API から -1 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _dup(_, _, _, _)).WillOnce(Return(-1));
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_dup(kFakeFd, NULL); // [手順] - 有効な記述子で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_dup の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の dup2 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, dup2_returns_minus1_when_platform_dup2_fails)
{
    // Arrange

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, dup2(_, _, _, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 下位の dup2 系 API が 1 回呼び出されること。
                               // [Pre-Assert手順] - 下位の dup2 系 API から -1 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _dup2(_, _, _, _, _)).WillOnce(Return(-1));
#endif /* PLATFORM_ */

    // Act
    int rtc = com_util_dup2(kFakeFd, kFakeFd, NULL); // [手順] - 有効な記述子で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_dup2 の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の read 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, read_returns_minus1_when_platform_read_fails)
{
    // Arrange
    char buf[4];

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, read(_, _, _, _, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 下位の read 系 API が 1 回呼び出されること。
                               // [Pre-Assert手順] - 下位の read 系 API から -1 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _read(_, _, _, _, _, _)).WillOnce(Return(-1));
#endif /* PLATFORM_ */

    // Act
    int64_t rtc = com_util_read(kFakeFd, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_read の戻り値として、OS の失敗がそのまま -1 として返ること。
}

// 下位の write 系 API が失敗した場合に -1 が返ることの確認
TEST_F(fdTest, write_returns_minus1_when_platform_write_fails)
{
    // Arrange
    const char buf[4] = "abc";

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_unistd_, write(_, _, _, _, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 下位の write 系 API が 1 回呼び出されること。
                               // [Pre-Assert手順] - 下位の write 系 API から -1 を返却する。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_unistd_, _write(_, _, _, _, _, _)).WillOnce(Return(-1));
#endif /* PLATFORM_ */

    // Act
    int64_t rtc = com_util_write(kFakeFd, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

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

    // Pre-Assert
    EXPECT_CALL(mock_unistd_, read(_, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(Return(4)); // [Pre-Assert確認_正常系] - 下位の read 系 API が 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の read 系 API から、errno に EINTR を設定した -1 ののち 4 を返却する。

    // Act
    int64_t rtc = com_util_read(kFakeFd, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ((int64_t)4,
              rtc); // [確認_正常系] - com_util_read の戻り値が、再試行後の転送量である 4 であること。
}

// 下位の write 系 API がシグナルで中断された場合に再試行されることの確認
TEST_F(fdTest, write_retries_after_interrupt)
{
    // Arrange
    const char buf[4] = "abc";

    // Pre-Assert
    EXPECT_CALL(mock_unistd_, write(_, _, _, _, _, _))
        .WillOnce(DoAll(Assign(&errno, EINTR), Return(-1)))
        .WillOnce(Return(4)); // [Pre-Assert確認_正常系] - 下位の write 系 API が 2 回呼び出されること。
    // [Pre-Assert手順] - 下位の write 系 API から、errno に EINTR を設定した -1 ののち 4 を返却する。

    // Act
    int64_t rtc = com_util_write(kFakeFd, buf, sizeof(buf), NULL); // [手順] - 有効な引数で呼び出す。

    // Assert
    EXPECT_EQ((int64_t)4,
              rtc); // [確認_正常系] - com_util_write の戻り値が、再試行後の転送量である 4 であること。
}

/* Windows の CRT はクローズ済み記述子で invalid parameter handler を起動するため、Linux でのみ実施する */
// クローズ済みの記述子で各関数が -1 を返すことの確認
TEST_F(fdTest, closed_fd_operations_return_minus1)
{
    // Arrange
    char buf[4];

    // Pre-Assert
    EXPECT_CALL(mock_unistd_, lseek(_, _, _, kFakeFd, _, _))
        .WillOnce(Return(
            static_cast<off_t>(-1))); // [Pre-Assert確認_異常系] - lseek がクローズ済み記述子で 1 回呼び出されること。
                                      // [Pre-Assert手順] - -1 を返却する。
    EXPECT_CALL(mock_unistd_, read(_, _, _, kFakeFd, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - read がクローズ済み記述子で 1 回呼び出されること。
                               // [Pre-Assert手順] - -1 を返却する。
    EXPECT_CALL(mock_unistd_, write(_, _, _, kFakeFd, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - write がクローズ済み記述子で 1 回呼び出されること。
                               // [Pre-Assert手順] - -1 を返却する。
    EXPECT_CALL(mock_unistd_, dup(_, _, _, kFakeFd))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - dup がクローズ済み記述子で 1 回呼び出されること。
                               // [Pre-Assert手順] - -1 を返却する。
    EXPECT_CALL(mock_unistd_, dup2(_, _, _, kFakeFd, kFakeFd))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - dup2 がクローズ済み記述子で 1 回呼び出されること。
                               // [Pre-Assert手順] - -1 を返却する。
    EXPECT_CALL(mock_unistd_, close(_, _, _, kFakeFd))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - close がクローズ済み記述子で 1 回呼び出されること。
                               // [Pre-Assert手順] - -1 を返却する。

    // Act
    // [手順] - クローズ済みのファイル記述子で lseek、read、write、dup、dup2、close を呼び出す。
    int64_t rtc_lseek = com_util_lseek(kFakeFd, 0, SEEK_SET, NULL);
    int64_t rtc_read = com_util_read(kFakeFd, buf, sizeof(buf), NULL);
    int64_t rtc_write = com_util_write(kFakeFd, buf, sizeof(buf), NULL);
    int rtc_dup = com_util_dup(kFakeFd, NULL);
    int rtc_dup2 = com_util_dup2(kFakeFd, kFakeFd, NULL);
    int rtc_close = com_util_close(kFakeFd, NULL);

    // Assert
    EXPECT_EQ(-1, rtc_lseek); // [確認_異常系] - lseek が -1 を返すこと。
    EXPECT_EQ(-1, rtc_read);  // [確認_異常系] - read が -1 を返すこと。
    EXPECT_EQ(-1, rtc_write); // [確認_異常系] - write が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup);   // [確認_異常系] - dup が -1 を返すこと。
    EXPECT_EQ(-1, rtc_dup2);  // [確認_異常系] - dup2 が -1 を返すこと。
    EXPECT_EQ(-1, rtc_close); // [確認_異常系] - close が -1 を返すこと。
}
#endif /* PLATFORM_LINUX */
