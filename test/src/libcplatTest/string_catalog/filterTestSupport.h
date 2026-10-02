/**
 *******************************************************************************
 *  @file           filterTestSupport.h
 *  @brief          条件式フィルターのテストが共有するヘルパーを宣言します。
 *
 *  条件式リストは `char lines[N][M]` の形式で受け渡すため、行の作成を毎回書き下すと
 *  テストの本質が読み取りにくくなります。本ヘッダーは、行の作成とコンパイルの呼び出しだけを
 *  まとめ、期待値の検証はテスト本体に残します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#ifndef FILTER_TEST_SUPPORT_PRIVATE_H
#define FILTER_TEST_SUPPORT_PRIVATE_H

/* テスト対象が使用する標準ライブラリ関数の mock。Windows で実装オブジェクトを取り込むため、ヘッダーを取り込む */
#include <mock_stdio.h>
#include <mock_stdlib.h>
#include <mock_string.h>
#include <cplat/string_catalog/filter.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 *  @brief          生成器が付与する文脈引数 (`{40}` から `{45}`) の 6 個と、
 *                  テスト用カタログが定義するコンテキスト引数 (`{46}` の sequence_number) の 1 個、
 *                  合わせて 7 個を、型どおりの可変長引数として展開します。
 *  @param[in]      seq `{46}` (sequence_number) に渡す値。
 *
 *  利用者引数に続けて渡す前提です (source_file_path, source_file_name, source_line, function_name,
 *  process_id, thread_id, sequence_number の順)。\n
 *  値そのものに関心がないテストでは、固定値 (例: 7) を渡してください。
 */
#define FILTER_TEST_CONTEXT_ARGS(seq) "f.c", "f.c", (int32_t)10, "fn", (uint32_t)1, (uint32_t)2, (int32_t)(seq)

namespace filter_test
{
/** テストで共通して使用する行幅です。実在するどの条件式の例より十分に広く取ります。 */
constexpr std::size_t kLineWidth = 160U;

/** テストで共通して使用する行数の上限です。 */
constexpr std::size_t kLineCapacity = 8U;

/** kLineWidth と kLineCapacity から求まるフィルター オブジェクトのバイト数です。 */
constexpr std::size_t kImageSize = CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(kLineCapacity, kLineWidth);

/** compile_lines に渡せる行数の上限です (テストの入力配列のバッファー サイズ)。 */
constexpr std::size_t kMaxTestLines = 16U;

/**
     *  @brief          条件式の配列を、既定の行幅で `char[count][line_width]` へ並べてコンパイルします。
     *
     *  @p texts の各要素は NUL 終端の条件式 (または空行、コメント行) です。
     */
inline int compile_lines(const char *const *texts, std::size_t count, void *image, std::size_t image_size = kImageSize,
                         std::size_t line_width = kLineWidth, std::size_t line_capacity = kLineCapacity,
                         cplat_string_catalog_filter_diagnostic *diagnostics = nullptr,
                         std::size_t diagnostic_capacity = 0U, std::size_t *invalid_count_out = nullptr)
{
    char rows[kMaxTestLines][kLineWidth];
    std::size_t local_invalid_count = 0U;

    std::memset(rows, 0, sizeof(rows));
    for (std::size_t index = 0; index < count; index++)
    {
        const std::size_t text_length = std::strlen(texts[index]);
        const std::size_t copy_length = (text_length < (line_width - 1U)) ? text_length : (line_width - 1U);

        std::memcpy(rows[index], texts[index], copy_length);
    }

    return cplat_string_catalog_filter_compile(
        &rows[0][0], count, line_width, line_capacity, image, image_size, diagnostics, diagnostic_capacity,
        (invalid_count_out != nullptr) ? invalid_count_out : &local_invalid_count);
}

/** 条件式 1 個だけを既定の行幅・行数上限でコンパイルします。 */
inline int compile_single_line(const char *text, void *image, std::size_t image_size = kImageSize,
                               std::size_t line_width = kLineWidth, std::size_t line_capacity = kLineCapacity,
                               cplat_string_catalog_filter_diagnostic *diagnostics = nullptr,
                               std::size_t diagnostic_capacity = 0U, std::size_t *invalid_count_out = nullptr)
{
    return compile_lines(&text, 1U, image, image_size, line_width, line_capacity, diagnostics, diagnostic_capacity,
                         invalid_count_out);
}

/**
     *  @brief          フィルター オブジェクト内の行レコードの先頭を求めます。
     *  @param[in]      image      フィルター オブジェクト。
     *  @param[in]      line_width 行幅。
     *  @param[in]      line_index 行の位置 (0 起点)。
     *  @return         行レコードの先頭。
     *
     *  公開マクロのヘッダーと行レコードのバイト数から求め、ライブラリの内部関数には依存しません。
     */
inline unsigned char *filter_test_record_address(void *image, std::size_t line_width, std::size_t line_index)
{
    return static_cast<unsigned char *>(image) + CPLAT_STRING_CATALOG_FILTER_HEADER_SIZE +
           (line_index * CPLAT_STRING_CATALOG_FILTER_RECORD_SIZE(line_width));
}

/** @copydoc filter_test_record_address */
inline const unsigned char *filter_test_record_address(const void *image, std::size_t line_width,
                                                       std::size_t line_index)
{
    return static_cast<const unsigned char *>(image) + CPLAT_STRING_CATALOG_FILTER_HEADER_SIZE +
           (line_index * CPLAT_STRING_CATALOG_FILTER_RECORD_SIZE(line_width));
}
} // namespace filter_test

#endif /* FILTER_TEST_SUPPORT_PRIVATE_H */
