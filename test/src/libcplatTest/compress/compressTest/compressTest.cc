#include <testfw.h>
#include <cplat/base/result.h>
#include <cplat/compress/compress.h>
#include <mock_cplat.h>

#if defined(PLATFORM_LINUX)
    #include <mock_zlib.h>
#endif /* PLATFORM_LINUX */

#include <cstdint>
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
            text += "cplat compress round trip ";
        }

        return text;
    }
};

// 圧縮した結果を展開すると元のデータへ戻ることの確認
TEST_F(compressTest, round_trip_restores_original_bytes)
{
    // Arrange
    std::string plain = make_plain_text(); // [状態] - 繰り返しを含む平文を用意する。
    std::vector<uint8_t> compressed(plain.size() + CPLAT_COMPRESS_HEADER_SIZE + 64u);
    std::vector<uint8_t> restored(plain.size() + 1u);
    size_t compressed_len = compressed.size();
    size_t restored_len = restored.size();

    // Pre-Assert

    // Act
    int actual_ret_compress =
        cplat_compress(compressed.data(), &compressed_len, reinterpret_cast<const uint8_t *>(plain.data()),
                       plain.size()); // [手順] - 平文を cplat_compress で圧縮する。
    int actual_ret_decompress = cplat_decompress(restored.data(), &restored_len, compressed.data(),
                                                 compressed_len); // [手順] - 圧縮結果を cplat_decompress で展開する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret_compress);   // [確認_正常系] - cplat_compress の戻り値が CPLAT_OK であること。
    EXPECT_EQ(CPLAT_OK, actual_ret_decompress); // [確認_正常系] - cplat_decompress の戻り値が CPLAT_OK であること。
    EXPECT_GT(plain.size(), compressed_len);    // [確認_正常系] - 圧縮後のサイズが元のサイズより小さくなること。
    EXPECT_EQ(plain.size(), restored_len);      // [確認_正常系] - 展開後のサイズが元のサイズと一致すること。
    EXPECT_EQ(0, memcmp(plain.data(), restored.data(),
                        restored_len)); // [確認_正常系] - 展開後の内容が元のデータと一致すること。
}

// cplat_compress が不正な引数を拒否することの確認
TEST_F(compressTest, compress_rejects_invalid_arguments)
{
    // Arrange
    uint8_t dst[64];
    const uint8_t src[] = "data";
    size_t dst_len = sizeof(dst); // [状態] - 64 byte の出力バッファーを用意する。

    // Pre-Assert

    // Act
    int actual_ret_null_dst =
        cplat_compress(NULL, &dst_len, src, sizeof(src)); // [手順] - dst に NULL を指定して呼び出す。
    int actual_ret_null_dst_len =
        cplat_compress(dst, NULL, src, sizeof(src)); // [手順] - dst_len に NULL を指定して呼び出す。
    int actual_ret_null_src =
        cplat_compress(dst, &dst_len, NULL, sizeof(src));                 // [手順] - src に NULL を指定して呼び出す。
    int actual_ret_zero_src_len = cplat_compress(dst, &dst_len, src, 0u); // [手順] - src_len に 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_null_dst); // [確認_異常系] - dst が NULL のとき cplat_compress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_null_dst_len); // [確認_異常系] - dst_len が NULL のとき cplat_compress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_null_src); // [確認_異常系] - src が NULL のとき cplat_compress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_zero_src_len); // [確認_異常系] - src_len が 0 のとき cplat_compress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
}

#if SIZE_MAX > UINT32_MAX

// 圧縮前サイズが 4 GiB 以上の場合に上限超過を通知することの確認
// cplat-req: id=CPLAT-COMPRESS-FUNC-003; uuid=a6d4c8e1-3b27-4f9a-8e05-1c7b9d2f4a60
TEST_F(compressTest, compress_returns_limit_exceeded_when_src_len_exceeds_max)
{
    // Arrange
    uint8_t dst[64];
    const uint8_t src[] = "data";
    size_t dst_len = sizeof(dst); // [状態] - 64 byte の出力バッファーを用意する。
    size_t too_large_src_len = (size_t)CPLAT_COMPRESS_MAX_UNCOMPRESSED_SIZE +
                               1u; // [状態] - 処理上限を 1 byte 超える入力サイズを用意する。実データは確保しない。

    // Pre-Assert

    // Act
    int actual_ret =
        cplat_compress(dst, &dst_len, src,
                       too_large_src_len); // [手順] - src_len が処理上限を超える値で cplat_compress を呼び出す。

    // Assert
    EXPECT_EQ(
        CPLAT_ERR_LIMIT_EXCEEDED,
        actual_ret); // [確認_異常系] - src_len が処理上限を超えるとき cplat_compress の戻り値が CPLAT_ERR_LIMIT_EXCEEDED であること。
}

#endif /* SIZE_MAX > UINT32_MAX */

// Linux の cplat_compress は zlib の avail_out が uInt を超えるとき deflate を繰り返す。
// その継続条件は 4 GiB 超の出力バッファーが必要なため、単体テストでは到達できない。

// ヘッダーと最低 1 byte を収められない出力バッファーが拒否されることの確認
TEST_F(compressTest, compress_returns_buffer_too_small_for_header_only_buffer)
{
    // Arrange
    uint8_t dst[CPLAT_COMPRESS_HEADER_SIZE];
    const uint8_t src[] = "data";
    size_t dst_len = sizeof(dst); // [状態] - ヘッダー分ちょうどの出力バッファーを用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        cplat_compress(dst, &dst_len, src,
                       sizeof(src)); // [手順] - ヘッダー分しかない出力バッファーで cplat_compress を呼び出す。

    // Assert
    EXPECT_EQ(CPLAT_ERR_BUFFER_TOO_SMALL,
              actual_ret); // [確認_異常系] - cplat_compress の戻り値が CPLAT_ERR_BUFFER_TOO_SMALL であること。
}

// 圧縮結果が収まらない出力バッファーで圧縮が失敗することの確認
TEST_F(compressTest, compress_returns_unknown_when_output_does_not_fit)
{
    // Arrange
    std::string plain(4096u, 'x');
    /* Windows 実装は CK プレフィックス 2 byte 分を含めて圧縮するため、
     * ヘッダー + 1 byte では CPLAT_ERR_BUFFER_TOO_SMALL 側の分岐に入ってしまう。
     * ヘッダー + 3 byte にして、両プラットフォームで「バッファーは足りているが
     * 圧縮結果が収まらない」経路を通す。 */
    uint8_t dst[CPLAT_COMPRESS_HEADER_SIZE + 3u];
    size_t dst_len = sizeof(dst); // [状態] - 圧縮結果が収まらない出力バッファーを用意する。

    // Pre-Assert

    // Act
    int actual_ret = cplat_compress(dst, &dst_len, reinterpret_cast<const uint8_t *>(plain.data()),
                                    plain.size()); // [手順] - 出力が収まらないサイズで cplat_compress を呼び出す。

    // Assert
    EXPECT_EQ(
        CPLAT_ERR_UNKNOWN,
        actual_ret); // [確認_異常系] - deflate が完了しないため cplat_compress の戻り値が CPLAT_ERR_UNKNOWN であること。
}

// cplat_decompress が不正な引数を拒否することの確認
TEST_F(compressTest, decompress_rejects_invalid_arguments)
{
    // Arrange
    uint8_t dst[64];
    uint8_t src[CPLAT_COMPRESS_HEADER_SIZE + 8u] = {0};
    size_t dst_len = sizeof(dst); // [状態] - 64 byte の出力バッファーを用意する。

    // Pre-Assert

    // Act
    int actual_ret_null_dst =
        cplat_decompress(NULL, &dst_len, src, sizeof(src)); // [手順] - dst に NULL を指定して呼び出す。
    int actual_ret_null_dst_len =
        cplat_decompress(dst, NULL, src, sizeof(src)); // [手順] - dst_len に NULL を指定して呼び出す。
    int actual_ret_null_src =
        cplat_decompress(dst, &dst_len, NULL, sizeof(src)); // [手順] - src に NULL を指定して呼び出す。
    int actual_ret_header_only = cplat_decompress(
        dst, &dst_len, src, CPLAT_COMPRESS_HEADER_SIZE); // [手順] - src_len をヘッダー長ちょうどに指定して呼び出す。

    // Assert
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_null_dst); // [確認_異常系] - dst が NULL のとき cplat_decompress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_null_dst_len); // [確認_異常系] - dst_len が NULL のとき cplat_decompress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_null_src); // [確認_異常系] - src が NULL のとき cplat_decompress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        CPLAT_ERR_INVALID_ARGUMENT,
        actual_ret_header_only); // [確認_異常系] - src_len がヘッダー長以下のとき cplat_decompress の戻り値が CPLAT_ERR_INVALID_ARGUMENT であること。
}

// ヘッダーが示す元サイズに満たない出力バッファーが拒否されることの確認
TEST_F(compressTest, decompress_returns_buffer_too_small_when_dst_is_shorter_than_original)
{
    // Arrange
    std::string plain = make_plain_text();
    std::vector<uint8_t> compressed(plain.size() + CPLAT_COMPRESS_HEADER_SIZE + 64u);
    size_t compressed_len = compressed.size();
    uint8_t dst[16];
    size_t dst_len = sizeof(dst); // [状態] - 元サイズに満たない 16 byte の出力バッファーを用意する。

    ASSERT_EQ(CPLAT_OK,
              cplat_compress(compressed.data(), &compressed_len, reinterpret_cast<const uint8_t *>(plain.data()),
                             plain.size())); // [状態] - 平文を圧縮しておく。
                                             // [状態確認] - cplat_compress の戻り値が CPLAT_OK であること。

    // Pre-Assert

    // Act
    int actual_ret = cplat_decompress(dst, &dst_len, compressed.data(),
                                      compressed_len); // [手順] - 元サイズに満たない出力バッファーで展開を試みる。

    // Assert
    EXPECT_EQ(CPLAT_ERR_BUFFER_TOO_SMALL,
              actual_ret); // [確認_異常系] - cplat_decompress の戻り値が CPLAT_ERR_BUFFER_TOO_SMALL であること。
}

// DEFLATE ストリームとして不正なデータの展開が失敗することの確認
TEST_F(compressTest, decompress_returns_unknown_for_corrupt_stream)
{
    // Arrange
    uint8_t src[CPLAT_COMPRESS_HEADER_SIZE + 8u];
    uint8_t dst[64];
    size_t dst_len = sizeof(dst);

    memset(src, 0xFF, sizeof(src));
    src[0] = 0x00;
    src[1] = 0x00;
    src[2] = 0x00;
    src[3] = 0x00;
    src[4] = 0x00;
    src[5] = 0x00;
    src[6] = 0x00;
    src[7] = 0x08; // [状態] - 元サイズ 8 byte を示すヘッダーと、DEFLATE として不正な本体を用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        cplat_decompress(dst, &dst_len, src,
                         sizeof(src)); // [手順] - 不正な DEFLATE ストリームで cplat_decompress を呼び出す。

    // Assert
    EXPECT_EQ(
        CPLAT_ERR_UNKNOWN,
        actual_ret); // [確認_異常系] - inflate が完了しないため cplat_decompress の戻り値が CPLAT_ERR_UNKNOWN であること。
}

// ヘッダーの展開後長さが 4 GiB 以上の場合に上限超過を通知することの確認
// cplat-req: id=CPLAT-COMPRESS-FUNC-003; uuid=a6d4c8e1-3b27-4f9a-8e05-1c7b9d2f4a60
TEST_F(compressTest, decompress_returns_limit_exceeded_when_original_size_exceeds_max)
{
    // Arrange
    uint8_t src[CPLAT_COMPRESS_HEADER_SIZE + 1u] = {0};
    uint8_t dst[16];
    size_t dst_len = sizeof(dst);

    src[3] = 0x01; // [状態] - 展開後長さ 4 GiB を示す 8 バイトヘッダーと 1 byte の本体を用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        cplat_decompress(dst, &dst_len, src,
                         sizeof(src)); // [手順] - 展開後長さが処理上限以上のヘッダーで cplat_decompress を呼び出す。

    // Assert
    EXPECT_EQ(CPLAT_ERR_LIMIT_EXCEEDED,
              actual_ret); // [確認_異常系] - cplat_decompress の戻り値が CPLAT_ERR_LIMIT_EXCEEDED であること。
}

#if defined(PLATFORM_LINUX)

// 圧縮ストリームの初期化に失敗した場合に通知されることの確認
// Windows の cplat_compress は Compression API を使うため、この失敗経路は Linux のみに存在する
TEST_F(compressTest, compress_returns_unknown_when_deflate_init_fails)
{
    // Arrange
    NiceMock<Mock_zlib> mock_zlib;
    const uint8_t src[] = "payload";
    uint8_t dst[64];
    size_t dst_len = sizeof(dst); // [状態] - 十分な大きさの出力バッファーを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_zlib, deflateInit2_(_, _, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(Z_MEM_ERROR))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - deflateInit2_ が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は Z_MEM_ERROR を返却し、以降は本物へ委譲する。

    // Act
    int actual_ret = cplat_compress(dst, &dst_len, src, sizeof(src) - 1u); // [手順] - cplat_compress を呼び出す。

    // Assert
    EXPECT_EQ(CPLAT_ERR_UNKNOWN,
              actual_ret); // [確認_異常系] - cplat_compress の戻り値が CPLAT_ERR_UNKNOWN であること。
}

// 展開ストリームの初期化に失敗した場合に通知されることの確認
// Windows の cplat_decompress は Compression API を使うため、この失敗経路は Linux のみに存在する
TEST_F(compressTest, decompress_returns_unknown_when_inflate_init_fails)
{
    // Arrange
    NiceMock<Mock_zlib> mock_zlib;
    uint8_t src[CPLAT_COMPRESS_HEADER_SIZE + 8u] = {0};
    uint8_t dst[64];
    size_t dst_len = sizeof(dst); // [状態] - ヘッダー長を超える入力と出力バッファーを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_zlib, inflateInit2_(_, _, _, _, _, _, _))
        .WillOnce(Return(Z_MEM_ERROR))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - inflateInit2_ が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は Z_MEM_ERROR を返却し、以降は本物へ委譲する。

    // Act
    int actual_ret = cplat_decompress(dst, &dst_len, src, sizeof(src)); // [手順] - cplat_decompress を呼び出す。

    // Assert
    EXPECT_EQ(CPLAT_ERR_UNKNOWN,
              actual_ret); // [確認_異常系] - cplat_decompress の戻り値が CPLAT_ERR_UNKNOWN であること。
}

#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)

// 一時バッファーの確保に失敗した場合に通知されることの確認
// Linux の cplat_decompress は zlib の inflate を使うため、この失敗経路は Windows のみに存在する
TEST_F(compressTest, decompress_returns_out_of_memory_when_malloc_fails)
{
    // Arrange
    NiceMock<Mock_cplat> mock_cplat;
    uint8_t src[CPLAT_COMPRESS_HEADER_SIZE + 8u] = {0};
    uint8_t dst[64];
    size_t dst_len = sizeof(dst); // [状態] - ヘッダー長を超える入力と出力バッファーを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_cplat, cplat_malloc(_))
        .WillOnce(Return(
            nullptr)); // [Pre-Assert確認_異常系] - cplat_malloc が一時バッファー確保のために 1 回呼び出されること。
                       // [Pre-Assert手順] - cplat_malloc から NULL を返却する。

    // Act
    int actual_ret = cplat_decompress(dst, &dst_len, src, sizeof(src)); // [手順] - cplat_decompress を呼び出す。

    // Assert
    EXPECT_EQ(CPLAT_ERR_OUT_OF_MEMORY,
              actual_ret); // [確認_異常系] - cplat_decompress の戻り値が CPLAT_ERR_OUT_OF_MEMORY であること。
}

#endif /* PLATFORM_WINDOWS */
