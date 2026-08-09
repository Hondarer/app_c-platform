#include <testfw.h>

#include <com_util/regex/regex_utf8.h>

#include <string>
#include <vector>

using com_util::regex_detail::index_of_offset;
using com_util::regex_detail::offset_of_begin;
using com_util::regex_detail::offset_of_end;
using com_util::regex_detail::utf8_decode;
using com_util::regex_detail::utf8_encode;

class regexUtf8Test : public Test
{
};

// ASCII 文字列が 1 バイト 1 コード単位へ変換されることの確認
TEST_F(regexUtf8Test, decode_ascii_maps_one_byte_to_one_unit)
{
    // Arrange
    const std::string text = "abc";
    std::wstring units;
    std::vector<std::size_t> offsets;
    bool decoded = false;

    // Pre-Assert

    // Act
    decoded = utf8_decode(text.data(), text.size(), units,
                          offsets); // [手順] - ASCII 文字列 "abc" をコード単位列へ変換する。

    // Assert
    EXPECT_TRUE(decoded);                      // [確認_正常系] - utf8_decode の戻り値が true であること。
    EXPECT_EQ((std::size_t)3, units.size());   // [確認_正常系] - コード単位数が 3 であること。
    EXPECT_EQ(std::wstring(L"abc"), units);    // [確認_正常系] - コード単位列が L"abc" であること。
    ASSERT_EQ((std::size_t)4, offsets.size()); // [確認_正常系] - 写像表の要素数がコード単位数 + 1 であること。
    EXPECT_EQ((std::size_t)0, offsets[0]);     // [確認_正常系] - 1 文字目のバイト オフセットが 0 であること。
    EXPECT_EQ((std::size_t)1, offsets[1]);     // [確認_正常系] - 2 文字目のバイト オフセットが 1 であること。
    EXPECT_EQ((std::size_t)2, offsets[2]);     // [確認_正常系] - 3 文字目のバイト オフセットが 2 であること。
    EXPECT_EQ((std::size_t)3, offsets[3]);     // [確認_正常系] - 終端のバイト オフセットが 3 であること。
}

// 日本語 (BMP) の 1 文字が 1 コード単位 3 バイトへ対応することの確認
TEST_F(regexUtf8Test, decode_japanese_maps_three_bytes_to_one_unit)
{
    // Arrange
    const std::string text = u8"あいう";
    std::wstring units;
    std::vector<std::size_t> offsets;
    bool decoded = false;

    // Pre-Assert

    // Act
    decoded = utf8_decode(text.data(), text.size(), units,
                          offsets); // [手順] - 日本語 3 文字 "あいう" をコード単位列へ変換する。

    // Assert
    EXPECT_TRUE(decoded);                      // [確認_正常系] - utf8_decode の戻り値が true であること。
    EXPECT_EQ((std::size_t)3, units.size());   // [確認_正常系] - コード単位数が 3 であること。
    ASSERT_EQ((std::size_t)4, offsets.size()); // [確認_正常系] - 写像表の要素数が 4 であること。
    EXPECT_EQ((std::size_t)0, offsets[0]);     // [確認_正常系] - "あ" のバイト オフセットが 0 であること。
    EXPECT_EQ((std::size_t)3, offsets[1]);     // [確認_正常系] - "い" のバイト オフセットが 3 であること。
    EXPECT_EQ((std::size_t)6, offsets[2]);     // [確認_正常系] - "う" のバイト オフセットが 6 であること。
    EXPECT_EQ((std::size_t)9, offsets[3]);     // [確認_正常系] - 終端のバイト オフセットが 9 であること。
}

// BMP 外の 1 文字がサロゲート ペア 2 コード単位へ対応することの確認
TEST_F(regexUtf8Test, decode_astral_maps_one_code_point_to_surrogate_pair)
{
    // Arrange
    const std::string text = u8"\U0001F600";
    std::wstring units;
    std::vector<std::size_t> offsets;
    bool decoded = false;

    // Pre-Assert

    // Act
    decoded = utf8_decode(text.data(), text.size(), units,
                          offsets); // [手順] - BMP 外の 1 文字 U+1F600 をコード単位列へ変換する。

    // Assert
    EXPECT_TRUE(decoded);                      // [確認_正常系] - utf8_decode の戻り値が true であること。
    ASSERT_EQ((std::size_t)2, units.size());   // [確認_正常系] - コード単位数が 2 (サロゲート ペア) であること。
    EXPECT_EQ((wchar_t)0xD83D, units[0]);      // [確認_正常系] - 上位サロゲートが 0xD83D であること。
    EXPECT_EQ((wchar_t)0xDE00, units[1]);      // [確認_正常系] - 下位サロゲートが 0xDE00 であること。
    ASSERT_EQ((std::size_t)3, offsets.size()); // [確認_正常系] - 写像表の要素数が 3 であること。
    EXPECT_EQ((std::size_t)0, offsets[0]);     // [確認_正常系] - 上位サロゲートのバイト オフセットが 0 であること。
    EXPECT_EQ((std::size_t)0,
              offsets[1]); // [確認_正常系] - 下位サロゲートにも同じ開始バイト オフセット 0 が割り当たること。
    EXPECT_EQ((std::size_t)4, offsets[2]); // [確認_正常系] - 終端のバイト オフセットが 4 であること。
}

// 不正な UTF-8 が変換を拒否されることの確認
TEST_F(regexUtf8Test, decode_rejects_invalid_utf8)
{
    // Arrange
    std::wstring units;
    std::vector<std::size_t> offsets;
    const std::string overlong("\xC0\x80", 2);
    const std::string surrogate("\xED\xA0\x80", 3);
    const std::string too_large("\xF5\x80\x80\x80", 4);
    const std::string truncated("\xE3\x81", 2);
    const std::string lone_trail("\x80", 1);

    // Pre-Assert

    // Act

    // Assert
    EXPECT_FALSE(utf8_decode(overlong.data(), overlong.size(), units,
                             offsets)); // [確認_異常系] - オーバー ロング表現 C0 80 に対する utf8_decode の戻り値が
                                        // false であること。
    EXPECT_FALSE(utf8_decode(surrogate.data(), surrogate.size(), units,
                             offsets)); // [確認_異常系] - 単独サロゲート ED A0 80 に対する utf8_decode の戻り値が
                                        // false であること。
    EXPECT_FALSE(utf8_decode(too_large.data(), too_large.size(), units,
                             offsets)); // [確認_異常系] - U+10FFFF を超える F5 80 80 80 に対する utf8_decode の
                                        // 戻り値が false であること。
    EXPECT_FALSE(utf8_decode(truncated.data(), truncated.size(), units,
                             offsets)); // [確認_異常系] - 途中で切れた E3 81 に対する utf8_decode の戻り値が
                                        // false であること。
    EXPECT_FALSE(utf8_decode(lone_trail.data(), lone_trail.size(), units,
                             offsets)); // [確認_異常系] - 後続バイト単独の 80 に対する utf8_decode の戻り値が
                                        // false であること。
}

// コード単位列を UTF-8 へ戻せることの確認
TEST_F(regexUtf8Test, encode_restores_original_text)
{
    // Arrange
    const std::string text = u8"aあ\U0001F600z";
    std::wstring units;
    std::vector<std::size_t> offsets;
    std::string encoded;
    bool decoded = false;
    bool result = false;

    decoded = utf8_decode(text.data(), text.size(), units,
                          offsets); // [状態] - ASCII、日本語、BMP 外を含む文字列をコード単位列へ変換しておく。
    ASSERT_TRUE(decoded);

    // Pre-Assert

    // Act
    result = utf8_encode(units, encoded); // [手順] - コード単位列を UTF-8 へ戻す。

    // Assert
    EXPECT_TRUE(result);      // [確認_正常系] - utf8_encode の戻り値が true であること。
    EXPECT_EQ(text, encoded); // [確認_正常系] - 変換結果が元の UTF-8 文字列と一致すること。
}

// 単独の下位サロゲートが符号化を拒否されることの確認
TEST_F(regexUtf8Test, encode_rejects_lone_surrogate)
{
    // Arrange
    std::wstring units;
    std::string encoded;
    bool result = false;

    units.push_back((wchar_t)0xDE00); // [状態] - 対となる上位サロゲートを持たない下位サロゲートのみを用意する。

    // Pre-Assert

    // Act
    result = utf8_encode(units, encoded); // [手順] - 単独の下位サロゲートを UTF-8 へ変換する。

    // Assert
    EXPECT_FALSE(result); // [確認_異常系] - utf8_encode の戻り値が false であること。
}

// サロゲート ペアの内側を指す索引が丸められることの確認
TEST_F(regexUtf8Test, offset_conversion_rounds_inside_surrogate_pair)
{
    // Arrange
    const std::string text = u8"\U0001F600z";
    std::wstring units;
    std::vector<std::size_t> offsets;
    std::size_t begin_offset = 0;
    std::size_t end_offset = 0;

    ASSERT_TRUE(utf8_decode(text.data(), text.size(), units,
                            offsets)); // [状態] - BMP 外の 1 文字と ASCII 1 文字からなる文字列を変換しておく。

    // Pre-Assert
    ASSERT_EQ((std::size_t)3, units.size()); // [Pre-Assert確認_正常系] - コード単位数が 3 であること。

    // Act
    begin_offset = offset_of_begin(units, offsets,
                                   1); // [手順] - 下位サロゲートを指す索引 1 を開始位置として変換する。
    end_offset = offset_of_end(units, offsets,
                               1); // [手順] - 下位サロゲートを指す索引 1 を終了位置として変換する。

    // Assert
    EXPECT_EQ((std::size_t)0,
              begin_offset); // [確認_正常系] - offset_of_begin の戻り値がコード ポイント先頭の 0 であること。
    EXPECT_EQ((std::size_t)4,
              end_offset); // [確認_正常系] - offset_of_end の戻り値がコード ポイント末尾の 4 であること。
}

// バイト オフセットからコード単位索引を復元できることの確認
TEST_F(regexUtf8Test, index_of_offset_accepts_code_point_boundary_only)
{
    // Arrange
    const std::string text = u8"あい";
    std::wstring units;
    std::vector<std::size_t> offsets;
    std::size_t index = 0;

    ASSERT_TRUE(utf8_decode(text.data(), text.size(), units,
                            offsets)); // [状態] - 日本語 2 文字の文字列を変換しておく。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_TRUE(index_of_offset(offsets, 3,
                                index)); // [確認_正常系] - 境界であるオフセット 3 に対する index_of_offset の
                                         // 戻り値が true であること。
    EXPECT_EQ((std::size_t)1, index);    // [確認_正常系] - 復元された索引が 1 であること。
    EXPECT_TRUE(index_of_offset(offsets, 6,
                                index)); // [確認_正常系] - 終端であるオフセット 6 に対する index_of_offset の
                                         // 戻り値が true であること。
    EXPECT_EQ((std::size_t)2, index);    // [確認_正常系] - 復元された索引が 2 であること。
    EXPECT_FALSE(index_of_offset(offsets, 1,
                                 index)); // [確認_異常系] - 文字の途中であるオフセット 1 に対する index_of_offset
                                          // の戻り値が false であること。
    EXPECT_FALSE(index_of_offset(offsets, 7,
                                 index)); // [確認_異常系] - 範囲外であるオフセット 7 に対する index_of_offset の
                                          // 戻り値が false であること。
}
