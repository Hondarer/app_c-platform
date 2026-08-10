#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crypto/random.h>

#include <climits>
#include <cstring>

class randomTest : public Test
{
};

// 要求したバイト数がすべて満たされることの確認
TEST_F(randomTest, fills_requested_size)
{
    // Arrange
    unsigned char buf[32];
    unsigned char sentinel[32];

    memset(buf, 0xCD, sizeof(buf));           // [状態] - バッファー 32 byte を 0xCD で埋める。
    memset(sentinel, 0xCD, sizeof(sentinel)); // [状態] - 比較用に同じ内容の領域を用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_random_bytes(buf, sizeof(buf)); // [手順] - 32 byte を要求して com_util_random_bytes を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_random_bytes の戻り値が COM_UTIL_OK であること。
    EXPECT_NE(0, memcmp(buf, sentinel,
                        sizeof(buf))); // [確認_正常系] - バッファーが初期値 0xCD のままではなく書き換わること。
}

// 続けて取得した乱数が一致しないことの確認
TEST_F(randomTest, successive_calls_differ)
{
    // Arrange
    unsigned char first[32];
    unsigned char second[32];

    memset(first, 0, sizeof(first));   // [状態] - 1 回目の格納先を 0 で初期化する。
    memset(second, 0, sizeof(second)); // [状態] - 2 回目の格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc_first = com_util_random_bytes(first, sizeof(first));    // [手順] - 1 回目の取得を行う。
    int rtc_second = com_util_random_bytes(second, sizeof(second)); // [手順] - 2 回目の取得を行う。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_first); // [確認_正常系] - 1 回目の com_util_random_bytes の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              rtc_second); // [確認_正常系] - 2 回目の com_util_random_bytes の戻り値が COM_UTIL_OK であること。
    EXPECT_NE(0, memcmp(first, second, sizeof(first))); // [確認_正常系] - 2 回の取得結果が一致しないこと。
}

// サイズ 0 を成功として扱うことの確認
TEST_F(randomTest, zero_size_succeeds)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc = com_util_random_bytes(NULL, 0U); // [手順] - バッファーに NULL、サイズに 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc); // [確認_正常系] - サイズ 0 の場合に com_util_random_bytes の戻り値が COM_UTIL_OK であること。
}

// バッファーが NULL の場合に引数不正を返すことの確認
TEST_F(randomTest, null_buffer_returns_invalid_argument)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc = com_util_random_bytes(NULL, 16U); // [手順] - バッファーに NULL、サイズに 16 を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_random_bytes の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 要求バイト数が INT_MAX を超える場合に拒否されることの確認
// RAND_bytes が要求バイト数を int で受けるため、範囲外は引数不正として扱われる
TEST_F(randomTest, size_over_int_max_returns_invalid_argument)
{
    // Arrange
    unsigned char buf[1]; // [状態] - 実際には書き込まれない 1 byte のバッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_random_bytes(
        buf, (size_t)INT_MAX + 1U); // [手順] - INT_MAX を 1 超えるサイズで com_util_random_bytes を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_random_bytes の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}
