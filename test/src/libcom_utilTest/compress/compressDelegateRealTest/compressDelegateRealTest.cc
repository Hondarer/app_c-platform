#include <testfw.h>
#include <com_util/compress/compress.h>
#include <mock_com_util.h>
#include <string.h>

namespace {

const char *const kMissingLibName = "libmissing_com_util_for_test" TESTFW_SHARED_LIBRARY_EXTENSION;

const uint8_t kPlainText[] = "delegate real path validation";

static void expect_round_trip_success()
{
    uint8_t compressed[256] = {};
    uint8_t decompressed[256] = {};
    size_t compressed_len = sizeof(compressed);
    size_t decompressed_len = sizeof(decompressed);
    int rtc;

    rtc = com_util_compress(compressed,
                            &compressed_len,
                            kPlainText,
                            sizeof(kPlainText) - 1U);
    ASSERT_EQ(0, rtc);
    ASSERT_GT(compressed_len, COM_UTIL_COMPRESS_HEADER_SIZE);

    rtc = com_util_decompress(decompressed,
                              &decompressed_len,
                              compressed,
                              compressed_len);
    ASSERT_EQ(0, rtc);
    ASSERT_EQ(sizeof(kPlainText) - 1U, decompressed_len);
    EXPECT_EQ(0, memcmp(kPlainText, decompressed, decompressed_len));
}

} // namespace

class compressDelegateRealTest : public Test
{
};

// mock 未注入時に本物の libcom_util へ移譲して圧縮・展開できることの確認
TEST_F(compressDelegateRealTest, compress_and_decompress_delegate_to_real_when_mock_is_not_attached)
{
    // Arrange

    // Pre-Assert

    // Act
    expect_round_trip_success(); // [手順] - mock 未注入のまま com_util_compress() と com_util_decompress() を順に呼び出す。

    // Assert
    // [確認_正常系] - mock 未注入時でも本物の libcom_util へ移譲して round-trip に成功すること。
}

// mock 注入済みでも既定動作では本物の libcom_util へ移譲して圧縮・展開できることの確認
TEST_F(compressDelegateRealTest, default_behavior_delegates_to_real_when_mock_is_attached)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util; // [状態] - Mock_com_util を注入するが、compress/decompress の個別指定は行わない。

    // Pre-Assert

    // Act
    expect_round_trip_success(); // [手順] - 既定動作のまま com_util_compress() と com_util_decompress() を順に呼び出す。

    // Assert
    // [確認_正常系] - Mock_com_util 注入済みでも既定動作は本物の libcom_util へ移譲され、round-trip に成功すること。
}

// テストコードで指定した期待値が既定の real delegate より優先されることの確認
TEST_F(compressDelegateRealTest, expect_call_overrides_default_real_delegate)
{
    // Arrange
    Mock_com_util mock_com_util;
    uint8_t compressed[256] = {};
    size_t compressed_len = sizeof(compressed);

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_compress(_, _, _, _))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
            {
                EXPECT_NE(nullptr, dst);
                EXPECT_NE(nullptr, dst_len);
                EXPECT_EQ(kPlainText, src);
                EXPECT_EQ(sizeof(kPlainText) - 1U, src_len);
                *dst_len = 7U;
                dst[0] = 0xAA;
                return -1;
            }); // [Pre-Assert確認_正常系] - com_util_compress() が 1 回呼び出されること。
              // [Pre-Assert手順] - com_util_compress() でテスト専用の戻り値 -1 と出力長 7 を返す。

    // Act
    int rtc = com_util_compress(compressed,
                                &compressed_len,
                                kPlainText,
                                sizeof(kPlainText) - 1U); // [手順] - 明示した EXPECT_CALL 付きで com_util_compress() を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);             // [確認_正常系] - 明示した戻り値 -1 が返ること。
    EXPECT_EQ(7U, compressed_len);  // [確認_正常系] - 明示した出力長 7 が反映されること。
    EXPECT_EQ(0xAA, compressed[0]); // [確認_正常系] - 明示した出力内容が反映されること。
}

// 実在しないライブラリ名では汎用ローダが失敗情報を返すことの確認
TEST_F(compressDelegateRealTest, try_resolve_returns_diagnostic_when_library_is_missing)
{
    // Arrange

    // Pre-Assert

    // Act
    SharedSymbolResult result =
        tryResolveSharedSymbol(kMissingLibName,
                               "com_util_compress"); // [手順] - 実在しないライブラリ名でシンボル解決を試行する。

    // Assert
    EXPECT_EQ(nullptr, result.symbol);          // [確認_正常系] - 解決結果が nullptr であること。
    EXPECT_FALSE(result.diagnostic.empty());    // [確認_正常系] - 失敗理由の文字列が返ること。
}

// 実在ライブラリに存在しないシンボル名を指定したときに失敗情報を返すことの確認
TEST_F(compressDelegateRealTest, try_resolve_returns_diagnostic_when_symbol_is_missing)
{
    // Arrange

    // Pre-Assert

    // Act
    SharedSymbolResult result =
        tryResolveSharedSymbol(kLibComUtilName,
                               "com_util_symbol_that_does_not_exist"); // [手順] - 実在ライブラリに存在しないシンボル名で解決を試行する。

    // Assert
    EXPECT_EQ(nullptr, result.symbol);       // [確認_正常系] - 解決結果が nullptr であること。
    EXPECT_FALSE(result.diagnostic.empty()); // [確認_正常系] - 失敗理由の文字列が返ること。
}
