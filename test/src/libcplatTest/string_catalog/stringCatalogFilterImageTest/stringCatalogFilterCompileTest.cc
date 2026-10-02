#include <testfw.h>

#include "filterTestSupport.h"

#include <cplat/base/result.h>

#include <cstring>

using namespace filter_test;

class stringCatalogFilterCompileTest : public Test
{
};

// 空行、空白だけの行、# 行はフィルター オブジェクトへ格納されないことの確認
TEST_F(stringCatalogFilterCompileTest, blank_whitespace_and_comment_lines_are_not_stored)
{
    // Arrange
    static unsigned char image[kImageSize];
    const char *lines[] = {"", "   ", "# comment", "key == 1"};
    cplat_string_catalog_filter_info actual_info;
    int actual_compile_ret;
    int actual_info_ret;

    // Pre-Assert

    // Act
    actual_compile_ret =
        compile_lines(lines, 4U, image); // [手順] - 空行、空白行、コメント行、有効行を含む 4 行をコンパイルする。
    actual_info_ret =
        cplat_string_catalog_filter_get_info(image, kImageSize, &actual_info); // [手順] - ヘッダー情報を取得する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_compile_ret); // [確認_正常系] - コンパイルが成功すること。
    EXPECT_EQ(CPLAT_OK, actual_info_ret);    // [確認_正常系] - ヘッダー情報を取得できること。
    EXPECT_EQ(1U, actual_info.line_count);   // [確認_正常系] - 有効行の "key == 1" だけが格納されること。
}

// 閉じない引用符や不正な 16 進数表記などの字句の誤りが診断情報として通知されることの確認
TEST_F(stringCatalogFilterCompileTest, lexical_errors_are_diagnosed)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_diagnostic diagnostic;
    int actual_ret;
    std::size_t actual_invalid_count;

    // Pre-Assert

    // Act
    actual_ret = compile_single_line("id == \"abc", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                                     &actual_invalid_count); // [手順] - 閉じない引用符の行をコンパイルする。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - 無効な行があってもコンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_LEXICAL,
              diagnostic.error);          // [確認_正常系] - 原因が字句の誤りであること。
    EXPECT_EQ(0U, diagnostic.line_index); // [確認_正常系] - 入力の行番号が 0 であること。
    EXPECT_EQ(6U, diagnostic.column);     // [確認_正常系] - 開き引用符の位置 (6) が誤りの位置であること。

    // Act_2
    actual_ret = compile_single_line("key == 0x", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                                     &actual_invalid_count); // [手順] - 0x の後に桁がない行をコンパイルする。

    // Assert_2
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_LEXICAL,
              diagnostic.error);      // [確認_正常系] - 原因が字句の誤りであること。
    EXPECT_EQ(7U, diagnostic.column); // [確認_正常系] - "0x" の開始位置 (7) が誤りの位置であること。

    // Act_3
    actual_ret = compile_single_line("key == -0x1", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                                     &actual_invalid_count); // [手順] - 符号付き 16 進数の行をコンパイルする。

    // Assert_3
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_LEXICAL,
              diagnostic.error);      // [確認_正常系] - 原因が字句の誤りであること。
    EXPECT_EQ(7U, diagnostic.column); // [確認_正常系] - "-" の位置 (7) が誤りの位置であること。
}

// 被演算子の欠落や未定義の識別子などの構文の誤りが診断情報として通知されることの確認
TEST_F(stringCatalogFilterCompileTest, syntax_errors_are_diagnosed)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_diagnostic diagnostic;
    int actual_ret;
    std::size_t actual_invalid_count;

    // Pre-Assert

    // Act
    actual_ret = compile_single_line("key == 1 &&", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                                     &actual_invalid_count); // [手順] - 右辺が続かない && の行をコンパイルする。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_SYNTAX,
              diagnostic.error);       // [確認_正常系] - 原因が構文の誤りであること。
    EXPECT_EQ(11U, diagnostic.column); // [確認_正常系] - 行末 (11) が誤りの位置であること。

    // Act_2
    actual_ret = compile_single_line(
        "x.y", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
        &actual_invalid_count); // [手順] - key/id/category/arg のいずれでもない識別子の行をコンパイルする。

    // Assert_2
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_SYNTAX,
              diagnostic.error);      // [確認_正常系] - 原因が構文の誤りであること。
    EXPECT_EQ(0U, diagnostic.column); // [確認_正常系] - 識別子 "x" の位置 (0) が誤りの位置であること。
}

// 型の不一致や非互換な比較演算子などの誤りが診断情報として通知されることの確認
TEST_F(stringCatalogFilterCompileTest, type_mismatch_errors_are_diagnosed)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_diagnostic diagnostic;
    int actual_ret;
    std::size_t actual_invalid_count;

    // Pre-Assert

    // Act
    actual_ret =
        compile_single_line("category starts_with \"x\"", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                            &actual_invalid_count); // [手順] - 数値フィールドへ文字列演算子を使う行をコンパイルする。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_TYPE_MISMATCH,
              diagnostic.error); // [確認_正常系] - 原因が型の誤りであること。

    // Act_2
    actual_ret =
        compile_single_line("id < 3", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                            &actual_invalid_count); // [手順] - 文字列フィールドへ大小比較を使う行をコンパイルする。

    // Assert_2
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_TYPE_MISMATCH,
              diagnostic.error); // [確認_正常系] - 原因が型の誤りであること。

    // Act_3
    actual_ret = compile_single_line(
        "key == null", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
        &actual_invalid_count); // [手順] - null と比較できないフィールドへ null を使う行をコンパイルする。

    // Assert_3
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_TYPE_MISMATCH,
              diagnostic.error); // [確認_正常系] - 原因が型の誤りであること。
}

// 引数インデックスの上限超過が診断情報として通知されることの確認
TEST_F(stringCatalogFilterCompileTest, argument_index_limit_exceeded_is_diagnosed)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_diagnostic diagnostic;
    int actual_ret;
    std::size_t actual_invalid_count;

    // Pre-Assert

    // Act
    actual_ret =
        compile_single_line("arg[50] == 1", image, kImageSize, kLineWidth, kLineCapacity, &diagnostic, 1U,
                            &actual_invalid_count); // [手順] - 引数個数の上限 (50) と同じ添字の行をコンパイルする。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret);     // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count); // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_LIMIT_EXCEEDED,
              diagnostic.error); // [確認_正常系] - 原因が上限超過であること。
}

// フィルター オブジェクトの行数の上限を超えた入力行が、行数超過として診断されることの確認
TEST_F(stringCatalogFilterCompileTest, line_capacity_exceeded_is_diagnosed)
{
    // Arrange
    static unsigned char image[CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(2U, kLineWidth)];
    const char *lines[] = {"key == 1", "key == 2", "key == 3"};
    cplat_string_catalog_filter_diagnostic diagnostic;
    cplat_string_catalog_filter_info actual_info;
    int actual_compile_ret;
    std::size_t actual_invalid_count;

    // Pre-Assert

    // Act
    actual_compile_ret = compile_lines(
        lines, 3U, image, CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(2U, kLineWidth), kLineWidth, 2U, &diagnostic, 1U,
        &actual_invalid_count); // [手順] - 行数の上限 2 に対し、有効な行を 3 行与えてコンパイルする。
    (void)cplat_string_catalog_filter_get_info(image, CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(2U, kLineWidth),
                                               &actual_info); // [手順] - ヘッダー情報を取得する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_compile_ret); // [確認_正常系] - コンパイル自体は成功すること。
    EXPECT_EQ(1U, actual_invalid_count);     // [確認_正常系] - 上限を超えた 1 行が無効になること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_LINE_CAPACITY,
              diagnostic.error);           // [確認_正常系] - 原因が行数の上限超過であること。
    EXPECT_EQ(2U, diagnostic.line_index);  // [確認_正常系] - 入力の 3 行目 (index 2) が対象であること。
    EXPECT_EQ(2U, actual_info.line_count); // [確認_正常系] - 先着の 2 行だけが格納されること。
}

// image のバイト数が不足する場合に CPLAT_ERR_BUFFER_TOO_SMALL を返すことの確認
TEST_F(stringCatalogFilterCompileTest, image_buffer_too_small_returns_buffer_too_small)
{
    // Arrange
    unsigned char image[kImageSize];
    const char *line = "key == 1";
    int actual_ret;

    // Pre-Assert

    // Act
    actual_ret = compile_single_line(
        line, image, kImageSize - 1U); // [手順] - 必要量より 1 バイト小さい image_size でコンパイルする。

    // Assert
    EXPECT_EQ(CPLAT_ERR_BUFFER_TOO_SMALL, actual_ret); // [確認_異常系] - CPLAT_ERR_BUFFER_TOO_SMALL を返すこと。
}

// 行幅または行数の上限が範囲外の場合に CPLAT_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(stringCatalogFilterCompileTest, out_of_range_width_or_capacity_returns_invalid_argument)
{
    // Arrange
    unsigned char image[kImageSize];
    char row[kLineWidth];
    int actual_ret_narrow_width;
    int actual_ret_wide_width;
    int actual_ret_zero_capacity;
    int actual_ret_large_capacity;
    std::size_t invalid_count = 0U;

    std::memset(row, 0, sizeof(row));
    std::memcpy(row, "key == 1", sizeof("key == 1"));

    // Pre-Assert

    // Act
    actual_ret_narrow_width = cplat_string_catalog_filter_compile(
        row, 1U, CPLAT_STRING_CATALOG_FILTER_LINE_WIDTH_MIN - 1U, kLineCapacity, image, kImageSize, nullptr, 0U,
        &invalid_count); // [手順] - 行幅の下限より 1 小さい行幅でコンパイルする。
    actual_ret_wide_width = cplat_string_catalog_filter_compile(
        row, 1U, CPLAT_STRING_CATALOG_FILTER_LINE_WIDTH_MAX + 1U, kLineCapacity, image, kImageSize, nullptr, 0U,
        &invalid_count); // [手順] - 行幅の上限より 1 大きい行幅でコンパイルする。
    actual_ret_zero_capacity =
        cplat_string_catalog_filter_compile(row, 1U, kLineWidth, 0U, image, kImageSize, nullptr, 0U,
                                            &invalid_count); // [手順] - 行数の上限 0 でコンパイルする。
    actual_ret_large_capacity = cplat_string_catalog_filter_compile(
        row, 1U, kLineWidth, CPLAT_STRING_CATALOG_FILTER_LINE_MAX + 1U, image, kImageSize, nullptr, 0U,
        &invalid_count); // [手順] - 行数の上限より 1 大きい行数上限でコンパイルする。

    // Assert
    EXPECT_EQ(CPLAT_ERR_INVALID_ARGUMENT, actual_ret_narrow_width);  // [確認_異常系] - 行幅の下限未満は不正であること。
    EXPECT_EQ(CPLAT_ERR_INVALID_ARGUMENT, actual_ret_wide_width);    // [確認_異常系] - 行幅の上限超過は不正であること。
    EXPECT_EQ(CPLAT_ERR_INVALID_ARGUMENT, actual_ret_zero_capacity); // [確認_異常系] - 行数の上限 0 は不正であること。
    EXPECT_EQ(CPLAT_ERR_INVALID_ARGUMENT,
              actual_ret_large_capacity); // [確認_異常系] - 行数の上限超過は不正であること。
}

// CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE マクロの算出結果と、cplat_string_catalog_filter_get_info が返す image_size が一致することの確認
TEST_F(stringCatalogFilterCompileTest, image_size_macro_matches_get_info_result)
{
    // Arrange
    static unsigned char image[CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(4U, 64U)];
    const char *line = "key == 1";
    cplat_string_catalog_filter_info actual_info;
    int actual_compile_ret;
    int actual_info_ret;

    // Pre-Assert

    // Act
    actual_compile_ret = compile_single_line(line, image, CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(4U, 64U), 64U,
                                             4U); // [手順] - 行数上限 4・行幅 64 でコンパイルする。
    actual_info_ret = cplat_string_catalog_filter_get_info(image, CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(4U, 64U),
                                                           &actual_info); // [手順] - ヘッダー情報を取得する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_compile_ret); // [確認_正常系] - コンパイルが成功すること。
    EXPECT_EQ(CPLAT_OK, actual_info_ret);    // [確認_正常系] - ヘッダー情報を取得できること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(4U, 64U),
              actual_info.image_size); // [確認_正常系] - マクロの算出結果と image_size が一致すること。
}
