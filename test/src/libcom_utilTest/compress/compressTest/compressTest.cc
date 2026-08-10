#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/compress/compress.h>

#include <cstring>
#include <vector>
#include <string>

class compressTest : public Test
{
  protected:
    /* 圧縮しやすいように同じ並びを繰り返す平文 */
    static std::string make_plain_text()
    {
        std::string text;

        for (int i = 0; i < 64; ++i)
        {
            text += "com_util compress round trip ";
        }

        return text;
    }
};

// 圧縮した結果を展開すると元のデータへ戻ることの確認
TEST_F(compressTest, round_trip_restores_original_bytes)
{
    // Arrange
    std::string plain = make_plain_text(); // [状態] - 繰り返しを含む平文を用意する。
    std::vector<uint8_t> compressed(plain.size() + COM_UTIL_COMPRESS_HEADER_SIZE + 64u);
    std::vector<uint8_t> restored(plain.size() + 1u);
    size_t compressed_len = compressed.size();
    size_t restored_len = restored.size();

    // Pre-Assert

    // Act
    int rtc_compress =
        com_util_compress(compressed.data(), &compressed_len, reinterpret_cast<const uint8_t *>(plain.data()),
                          plain.size()); // [手順] - 平文を com_util_compress で圧縮する。
    int rtc_decompress = com_util_decompress(restored.data(), &restored_len, compressed.data(),
                                             compressed_len); // [手順] - 圧縮結果を com_util_decompress で展開する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_compress);    // [確認_正常系] - com_util_compress の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_decompress);  // [確認_正常系] - com_util_decompress の戻り値が COM_UTIL_OK であること。
    EXPECT_GT(plain.size(), compressed_len); // [確認_正常系] - 圧縮後のサイズが元のサイズより小さくなること。
    EXPECT_EQ(plain.size(), restored_len);   // [確認_正常系] - 展開後のサイズが元のサイズと一致すること。
    EXPECT_EQ(0, memcmp(plain.data(), restored.data(),
                        restored_len)); // [確認_正常系] - 展開後の内容が元のデータと一致すること。
}

// com_util_compress が不正な引数を拒否することの確認
TEST_F(compressTest, compress_rejects_invalid_arguments)
{
    // Arrange
    uint8_t dst[64];
    const uint8_t src[] = "data";
    size_t dst_len = sizeof(dst); // [状態] - 64 byte の出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc_null_dst = com_util_compress(NULL, &dst_len, src, sizeof(src)); // [手順] - dst に NULL を指定して呼び出す。
    int rtc_null_dst_len =
        com_util_compress(dst, NULL, src, sizeof(src)); // [手順] - dst_len に NULL を指定して呼び出す。
    int rtc_null_src = com_util_compress(dst, &dst_len, NULL, sizeof(src)); // [手順] - src に NULL を指定して呼び出す。
    int rtc_zero_src_len = com_util_compress(dst, &dst_len, src, 0u); // [手順] - src_len に 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_dst); // [確認_異常系] - dst が NULL のとき com_util_compress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_dst_len); // [確認_異常系] - dst_len が NULL のとき com_util_compress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_src); // [確認_異常系] - src が NULL のとき com_util_compress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_zero_src_len); // [確認_異常系] - src_len が 0 のとき com_util_compress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// ヘッダーと最低 1 byte を収められない出力バッファーが拒否されることの確認
TEST_F(compressTest, compress_returns_buffer_too_small_for_header_only_buffer)
{
    // Arrange
    uint8_t dst[COM_UTIL_COMPRESS_HEADER_SIZE];
    const uint8_t src[] = "data";
    size_t dst_len = sizeof(dst); // [状態] - ヘッダー分ちょうどの出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc =
        com_util_compress(dst, &dst_len, src,
                          sizeof(src)); // [手順] - ヘッダー分しかない出力バッファーで com_util_compress を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              rtc); // [確認_異常系] - com_util_compress の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
}

// 圧縮結果が収まらない出力バッファーで圧縮が失敗することの確認
TEST_F(compressTest, compress_returns_unknown_when_output_does_not_fit)
{
    // Arrange
    std::string plain(4096u, 'x');
    /* Windows 実装は CK プレフィックス 2 byte 分を含めて圧縮するため、
     * ヘッダー + 1 byte では COM_UTIL_ERR_BUFFER_TOO_SMALL 側の分岐に入ってしまう。
     * ヘッダー + 3 byte にして、両プラットフォームで「バッファーは足りているが
     * 圧縮結果が収まらない」経路を通す。 */
    uint8_t dst[COM_UTIL_COMPRESS_HEADER_SIZE + 3u];
    size_t dst_len = sizeof(dst); // [状態] - 圧縮結果が収まらない出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_compress(dst, &dst_len, reinterpret_cast<const uint8_t *>(plain.data()),
                                plain.size()); // [手順] - 出力が収まらないサイズで com_util_compress を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc); // [確認_異常系] - deflate が完了しないため com_util_compress の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// com_util_decompress が不正な引数を拒否することの確認
TEST_F(compressTest, decompress_rejects_invalid_arguments)
{
    // Arrange
    uint8_t dst[64];
    uint8_t src[COM_UTIL_COMPRESS_HEADER_SIZE + 8u] = {0};
    size_t dst_len = sizeof(dst); // [状態] - 64 byte の出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc_null_dst =
        com_util_decompress(NULL, &dst_len, src, sizeof(src)); // [手順] - dst に NULL を指定して呼び出す。
    int rtc_null_dst_len =
        com_util_decompress(dst, NULL, src, sizeof(src)); // [手順] - dst_len に NULL を指定して呼び出す。
    int rtc_null_src =
        com_util_decompress(dst, &dst_len, NULL, sizeof(src)); // [手順] - src に NULL を指定して呼び出す。
    int rtc_header_only = com_util_decompress(
        dst, &dst_len, src, COM_UTIL_COMPRESS_HEADER_SIZE); // [手順] - src_len をヘッダー長ちょうどに指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_dst); // [確認_異常系] - dst が NULL のとき com_util_decompress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_dst_len); // [確認_異常系] - dst_len が NULL のとき com_util_decompress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_src); // [確認_異常系] - src が NULL のとき com_util_decompress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_header_only); // [確認_異常系] - src_len がヘッダー長以下のとき com_util_decompress の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// ヘッダーが示す元サイズに満たない出力バッファーが拒否されることの確認
TEST_F(compressTest, decompress_returns_buffer_too_small_when_dst_is_shorter_than_original)
{
    // Arrange
    std::string plain = make_plain_text();
    std::vector<uint8_t> compressed(plain.size() + COM_UTIL_COMPRESS_HEADER_SIZE + 64u);
    size_t compressed_len = compressed.size();
    uint8_t dst[16];
    size_t dst_len = sizeof(dst); // [状態] - 元サイズに満たない 16 byte の出力バッファーを用意する。

    ASSERT_EQ(COM_UTIL_OK,
              com_util_compress(compressed.data(), &compressed_len, reinterpret_cast<const uint8_t *>(plain.data()),
                                plain.size())); // [状態] - 平文を圧縮しておく。

    // Pre-Assert

    // Act
    int rtc = com_util_decompress(dst, &dst_len, compressed.data(),
                                  compressed_len); // [手順] - 元サイズに満たない出力バッファーで展開を試みる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              rtc); // [確認_異常系] - com_util_decompress の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
}

// DEFLATE ストリームとして不正なデータの展開が失敗することの確認
TEST_F(compressTest, decompress_returns_unknown_for_corrupt_stream)
{
    // Arrange
    uint8_t src[COM_UTIL_COMPRESS_HEADER_SIZE + 8u];
    uint8_t dst[64];
    size_t dst_len = sizeof(dst);

    memset(src, 0xFF, sizeof(src));
    src[0] = 0x00;
    src[1] = 0x00;
    src[2] = 0x00;
    src[3] = 0x08; // [状態] - 元サイズ 8 byte を示すヘッダーと、DEFLATE として不正な本体を用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_decompress(dst, &dst_len, src,
                                  sizeof(src)); // [手順] - 不正な DEFLATE ストリームで com_util_decompress を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc); // [確認_異常系] - inflate が完了しないため com_util_decompress の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}
