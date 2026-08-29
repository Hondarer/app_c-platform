#include <testfw.h>
#include <cplat/runtime/memory_lock.h>

#include <cstring>

class secureZeroTest : public Test
{
};

// 指定範囲がすべて 0 になることの確認
TEST_F(secureZeroTest, clears_whole_range)
{
    // Arrange
    unsigned char buf[64];
    size_t non_zero_count = 0U;

    memset(buf, 0xA5, sizeof(buf)); // [状態] - バッファー 64 byte を 0xA5 で埋める。

    // Pre-Assert

    // Act
    cplat_secure_zero(buf, sizeof(buf)); // [手順] - バッファー全体を指定して cplat_secure_zero を呼び出す。

    // Assert
    for (size_t i = 0U; i < sizeof(buf); i++)
    {
        if (buf[i] != 0U)
        {
            non_zero_count++;
        }
    }
    EXPECT_EQ(0U, non_zero_count); // [確認_正常系] - 指定範囲に 0 以外のバイトが残らないこと。
}

// 指定範囲外を書き換えないことの確認
TEST_F(secureZeroTest, does_not_touch_outside_range)
{
    // Arrange
    unsigned char buf[16];

    memset(buf, 0xA5, sizeof(buf)); // [状態] - バッファー 16 byte を 0xA5 で埋める。

    // Pre-Assert

    // Act
    cplat_secure_zero(buf, 8U); // [手順] - 先頭 8 byte だけを指定して cplat_secure_zero を呼び出す。

    // Assert
    EXPECT_EQ(0, buf[0]);     // [確認_正常系] - 指定範囲の先頭が 0 になること。
    EXPECT_EQ(0, buf[7]);     // [確認_正常系] - 指定範囲の末尾が 0 になること。
    EXPECT_EQ(0xA5, buf[8]);  // [確認_正常系] - 指定範囲の直後が 0xA5 のまま保たれること。
    EXPECT_EQ(0xA5, buf[15]); // [確認_正常系] - バッファー末尾が 0xA5 のまま保たれること。
}

// NULL とサイズ 0 で何も起こらないことの確認
TEST_F(secureZeroTest, null_or_zero_size_is_noop)
{
    // Arrange
    unsigned char buf[8];

    memset(buf, 0xA5, sizeof(buf)); // [状態] - バッファー 8 byte を 0xA5 で埋める。

    // Pre-Assert

    // Act
    cplat_secure_zero(NULL, 8U); // [手順] - バッファーに NULL を指定して呼び出す。
    cplat_secure_zero(buf, 0U);  // [手順] - サイズに 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(0xA5, buf[0]); // [確認_正常系] - サイズ 0 の指定でバッファーが書き換わらないこと。
    EXPECT_EQ(0xA5, buf[7]); // [確認_正常系] - バッファー末尾も書き換わらないこと。
}
