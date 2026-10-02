#include <testfw.h>

#include "filterTestSupport.h"

#include <cplat/base/result.h>

#include <cstring>

using namespace filter_test;

class stringCatalogFilterDecompileTest : public Test
{
};

namespace
{
/**
     *  @brief          1 行をデコンパイルし、その結果を再コンパイルした行レコードが元のバイト列と一致するかを返します。
     */
bool decompiled_text_recompiles_to_same_bytes(const void *original_image, std::size_t line_index, char *decoded)
{
    static unsigned char recompiled_image[kImageSize];
    const unsigned char *original_record;
    const unsigned char *recompiled_record;
    int decompile_ret;
    int recompile_ret;

    decompile_ret =
        cplat_string_catalog_filter_decompile_line(original_image, kImageSize, line_index, decoded, kLineWidth);
    if (decompile_ret != CPLAT_OK)
    {
        return false;
    }
    recompile_ret = compile_single_line(decoded, recompiled_image);
    if (recompile_ret != CPLAT_OK)
    {
        return false;
    }

    original_record = filter_test_record_address(original_image, kLineWidth, line_index);
    recompiled_record = filter_test_record_address(recompiled_image, kLineWidth, 0U);
    return std::memcmp(original_record, recompiled_record, CPLAT_STRING_CATALOG_FILTER_RECORD_SIZE(kLineWidth)) == 0;
}
} // namespace

// 各種判定要素を含む条件式をデコンパイルして再コンパイルすると、元のフィルター オブジェクトとバイト単位で一致することの確認
TEST_F(stringCatalogFilterDecompileTest, decompile_then_recompile_matches_original_bytes)
{
    // Arrange
    static unsigned char image[kImageSize];
    const char *lines[] = {
        "key in [1, 2, 3]",   "arg.priority between -5 and 10", "category == 0x03", "arg.ratio == 1.5",
        "arg.command == 's'", "arg.job_name == \"a\\\"b\\nc\"", "has(arg[45])",
    };
    char decoded[kLineWidth];
    int actual_compile_ret;
    bool actual_matches_in;
    bool actual_matches_between;
    bool actual_matches_hex;
    bool actual_matches_float;
    bool actual_matches_char;
    bool actual_matches_escaped_string;
    bool actual_matches_has;

    // Pre-Assert

    // Act
    actual_compile_ret =
        compile_lines(lines, 7U, image); // [手順] - 7 種類の判定要素を含む条件式リストをコンパイルする。
    actual_matches_in =
        decompiled_text_recompiles_to_same_bytes(image, 0U, decoded); // [手順] - in の行を復元、再コンパイルする。
    actual_matches_between =
        decompiled_text_recompiles_to_same_bytes(image, 1U, decoded); // [手順] - between の行を復元、再コンパイルする。
    actual_matches_hex =
        decompiled_text_recompiles_to_same_bytes(image, 2U, decoded); // [手順] - 16 進数の行を復元、再コンパイルする。
    actual_matches_float = decompiled_text_recompiles_to_same_bytes(
        image, 3U, decoded); // [手順] - 浮動小数点数の行を復元、再コンパイルする。
    actual_matches_char =
        decompiled_text_recompiles_to_same_bytes(image, 4U, decoded); // [手順] - 文字定数の行を復元、再コンパイルする。
    actual_matches_escaped_string = decompiled_text_recompiles_to_same_bytes(
        image, 5U, decoded); // [手順] - エスケープを含む文字列の行を復元、再コンパイルする。
    actual_matches_has =
        decompiled_text_recompiles_to_same_bytes(image, 6U, decoded); // [手順] - has の行を復元、再コンパイルする。

    // Assert
    ASSERT_EQ(CPLAT_OK, actual_compile_ret); // [確認_正常系] - コンパイルが成功すること。
    EXPECT_TRUE(actual_matches_in);          // [確認_正常系] - in の行がバイト単位で一致すること。
    EXPECT_TRUE(actual_matches_between);     // [確認_正常系] - between の行がバイト単位で一致すること。
    EXPECT_TRUE(actual_matches_hex);         // [確認_正常系] - 16 進数の行がバイト単位で一致すること。
    EXPECT_TRUE(actual_matches_float);       // [確認_正常系] - 浮動小数点数の行がバイト単位で一致すること。
    EXPECT_TRUE(actual_matches_char);        // [確認_正常系] - 文字定数の行がバイト単位で一致すること。
    EXPECT_TRUE(
        actual_matches_escaped_string); // [確認_正常系] - エスケープを含む文字列の行がバイト単位で一致すること。
    EXPECT_TRUE(actual_matches_has);    // [確認_正常系] - has の行がバイト単位で一致すること。
}

// 否定の対象が判定要素の場合に、括弧付きで復元されることの確認
TEST_F(stringCatalogFilterDecompileTest, not_of_predicate_is_decompiled_with_parenthesis)
{
    // Arrange
    static unsigned char image[kImageSize];
    char actual_text[kLineWidth];
    int actual_compile_ret;
    int actual_decompile_ret;

    // Pre-Assert

    // Act
    actual_compile_ret = compile_single_line("!(key == 1)", image); // [手順] - "!(key == 1)" をコンパイルする。
    actual_decompile_ret =
        cplat_string_catalog_filter_decompile_line(image, kImageSize, 0U, actual_text,
                                                   sizeof(actual_text)); // [手順] - 行 0 を復元する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_compile_ret);   // [確認_正常系] - コンパイルが成功すること。
    EXPECT_EQ(CPLAT_OK, actual_decompile_ret); // [確認_正常系] - デコンパイルが成功すること。
    EXPECT_STREQ("!(key == 1)", actual_text);  // [確認_正常系] - "!" の対象が括弧付きで復元されること。
}

// a && (b && c) の形が、右結合を保つために括弧付きで復元されることの確認
TEST_F(stringCatalogFilterDecompileTest, right_grouped_and_chain_keeps_parenthesis)
{
    // Arrange
    static unsigned char image[kImageSize];
    char actual_text[kLineWidth];
    int actual_compile_ret;
    int actual_decompile_ret;

    // Pre-Assert

    // Act
    actual_compile_ret = compile_single_line("key == 1 && (key == 2 && key == 3)",
                                             image); // [手順] - 右側を括弧でまとめた && の連結をコンパイルする。
    actual_decompile_ret =
        cplat_string_catalog_filter_decompile_line(image, kImageSize, 0U, actual_text,
                                                   sizeof(actual_text)); // [手順] - 行 0 を復元する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_compile_ret);   // [確認_正常系] - コンパイルが成功すること。
    EXPECT_EQ(CPLAT_OK, actual_decompile_ret); // [確認_正常系] - デコンパイルが成功すること。
    EXPECT_STREQ("key == 1 && (key == 2 && key == 3)",
                 actual_text); // [確認_正常系] - 右側の括弧が保たれ、左結合の再解析と一致する表記で復元されること。
}

// 出力先が小さい場合に CPLAT_ERR_BUFFER_TOO_SMALL を返し、切り詰めたうえで NUL 終端することの確認
TEST_F(stringCatalogFilterDecompileTest, small_destination_is_truncated_and_null_terminated)
{
    // Arrange
    static unsigned char image[kImageSize];
    char actual_text[3];
    int actual_compile_ret;
    int actual_decompile_ret;

    // Pre-Assert

    // Act
    actual_compile_ret = compile_single_line("key == 1", image); // [手順] - "key == 1" をコンパイルする。
    actual_decompile_ret = cplat_string_catalog_filter_decompile_line(
        image, kImageSize, 0U, actual_text, sizeof(actual_text)); // [手順] - 3 バイトの出力先へ復元する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_compile_ret); // [確認_正常系] - コンパイルが成功すること。
    EXPECT_EQ(CPLAT_ERR_BUFFER_TOO_SMALL,
              actual_decompile_ret); // [確認_異常系] - 切り詰めにより CPLAT_ERR_BUFFER_TOO_SMALL を返すこと。
    EXPECT_EQ('\0', actual_text[std::strlen(actual_text)]);   // [確認_異常系] - 切り詰め後も NUL 終端していること。
    EXPECT_LT(std::strlen(actual_text), sizeof(actual_text)); // [確認_異常系] - 出力先のバイト数を超えていないこと。
}
