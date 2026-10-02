#include <testfw.h>

#include "filterTestSupport.h"

#include "gen/filter_test_trace.h"
#include "filter_test_trace_key_names.h"

/* 命令形式を書き換えるため、モジュール私有ヘッダーを取り込む */
#include "filter.h"

#include <cplat/base/result.h>

#include <cstdint>
#include <cstring>

using namespace filter_test;

class stringCatalogFilterApplyTest : public Test
{
  protected:
    void SetUp() override
    {
        ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_create(
                                filter_test_trace_catalog(), filter_test_trace_key_names(),
                                filter_test_trace_key_name_count(), nullptr, kLineCapacity, kLineWidth, &slot_));
    }

    void TearDown() override
    {
        cplat_string_catalog_filter_slot_dispose(&slot_);
    }

    cplat_string_catalog_filter_slot *slot_ = nullptr;
};

// 作成直後のスロットは、すべての文字列キーが常に不一致であることの確認
TEST_F(stringCatalogFilterApplyTest, freshly_created_slot_marks_all_keys_never_match)
{
    // Arrange
    cplat_string_catalog_filter_state actual_state_worker_started;
    cplat_string_catalog_filter_state actual_state_job_failed;
    int actual_ret_worker_started;
    int actual_ret_job_failed;

    // Pre-Assert

    // Act
    actual_ret_worker_started = cplat_string_catalog_filter_slot_test(
        slot_, FILTER_TEST_TRACE_KEY_WORKER_STARTED,
        &actual_state_worker_started); // [手順] - WORKER_STARTED の状態を取得する。
    actual_ret_job_failed = cplat_string_catalog_filter_slot_test(
        slot_, FILTER_TEST_TRACE_KEY_JOB_FAILED, &actual_state_job_failed); // [手順] - JOB_FAILED の状態を取得する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret_worker_started); // [確認_正常系] - WORKER_STARTED の状態を取得できること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_NEVER_MATCH,
              actual_state_worker_started);     // [確認_正常系] - WORKER_STARTED が常に不一致であること。
    EXPECT_EQ(CPLAT_OK, actual_ret_job_failed); // [確認_正常系] - JOB_FAILED の状態を取得できること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_NEVER_MATCH,
              actual_state_job_failed); // [確認_正常系] - JOB_FAILED が常に不一致であること。
}

// category <= 2 が、WARNING 以上 (分類値が 2 以下) の項目だけを常に一致にすることの確認
TEST_F(stringCatalogFilterApplyTest, category_le_2_marks_warning_and_above_as_always_match)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_state actual_state_job_failed;
    cplat_string_catalog_filter_state actual_state_worker_started;
    int actual_apply_ret;

    ASSERT_EQ(CPLAT_OK, compile_single_line("category <= 2", image)); // [状態] - 分類値による絞り込みをコンパイルする。

    // Pre-Assert

    // Act
    actual_apply_ret = cplat_string_catalog_filter_slot_apply(slot_, image, kImageSize, nullptr, 0U,
                                                              nullptr); // [手順] - スロットへ適用する。
    (void)cplat_string_catalog_filter_slot_test(
        slot_, FILTER_TEST_TRACE_KEY_JOB_FAILED,
        &actual_state_job_failed); // [手順] - JOB_FAILED (WARNING=2) の状態を取得する。
    (void)cplat_string_catalog_filter_slot_test(
        slot_, FILTER_TEST_TRACE_KEY_WORKER_STARTED,
        &actual_state_worker_started); // [手順] - WORKER_STARTED (INFO=3) の状態を取得する。

    // Assert
    ASSERT_EQ(CPLAT_OK, actual_apply_ret); // [確認_正常系] - 適用が成功すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_job_failed); // [確認_正常系] - 分類値 2 (WARNING) 以下の項目は常に一致であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_NEVER_MATCH,
              actual_state_worker_started); // [確認_正常系] - 分類値 3 (INFO) は常に不一致のままであること。
}

// 文字列キーの名前解決と整数指定が、同じ判定結果になることの確認
TEST_F(stringCatalogFilterApplyTest, key_name_and_integer_resolve_to_same_result)
{
    // Arrange
    static unsigned char image_by_name[kImageSize];
    static unsigned char image_by_integer[kImageSize];
    cplat_string_catalog_filter_state actual_state_by_name;
    cplat_string_catalog_filter_state actual_state_by_integer;

    ASSERT_EQ(CPLAT_OK, compile_single_line("key == FILTER_TEST_TRACE_KEY_JOB_RECEIVED",
                                            image_by_name)); // [状態] - 列挙定数名で指定した条件式をコンパイルする。
    ASSERT_EQ(CPLAT_OK, compile_single_line(
                            "key == 2", image_by_integer)); // [状態] - 整数値 (2) で指定した条件式をコンパイルする。

    // Pre-Assert

    // Act
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_apply(slot_, image_by_name, kImageSize, nullptr, 0U,
                                                               nullptr)); // [手順] - 名前指定の条件式を適用する。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
                                                &actual_state_by_name); // [手順] - JOB_RECEIVED の状態を取得する。

    // Assert
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_by_name); // [確認_正常系] - 名前指定でも常に一致になること。

    // Act_2
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_apply(slot_, image_by_integer, kImageSize, nullptr, 0U,
                                                               nullptr)); // [手順] - 整数指定の条件式を適用する。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
                                                &actual_state_by_integer); // [手順] - JOB_RECEIVED の状態を取得する。

    // Assert_2
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_by_integer); // [確認_正常系] - 整数指定でも常に一致になること。
    EXPECT_EQ(actual_state_by_name,
              actual_state_by_integer); // [確認_正常系] - 名前指定と整数指定の結果が一致すること。
}

// 引数を含む行が、その引数を持つ項目だけを引数値に依存させることの確認
TEST_F(stringCatalogFilterApplyTest, argument_predicate_marks_only_entries_with_that_argument)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_state actual_state_job_received;
    cplat_string_catalog_filter_state actual_state_worker_started;
    int actual_apply_ret;

    ASSERT_EQ(CPLAT_OK, compile_single_line("arg.priority == 5",
                                            image)); // [状態] - JOB_RECEIVED だけが持つ引数の条件式をコンパイルする。

    // Pre-Assert

    // Act
    actual_apply_ret = cplat_string_catalog_filter_slot_apply(slot_, image, kImageSize, nullptr, 0U,
                                                              nullptr); // [手順] - スロットへ適用する。
    (void)cplat_string_catalog_filter_slot_test(
        slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
        &actual_state_job_received); // [手順] - JOB_RECEIVED (priority を持つ) の状態を取得する。
    (void)cplat_string_catalog_filter_slot_test(
        slot_, FILTER_TEST_TRACE_KEY_WORKER_STARTED,
        &actual_state_worker_started); // [手順] - WORKER_STARTED (priority を持たない) の状態を取得する。

    // Assert
    ASSERT_EQ(CPLAT_OK, actual_apply_ret); // [確認_正常系] - 適用が成功すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ARGUMENT_DEPENDENT,
              actual_state_job_received); // [確認_正常系] - priority を持つ項目は引数値に依存すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_NEVER_MATCH,
              actual_state_worker_started); // [確認_正常系] - priority を持たない項目は常に不一致であること。
}

// 名前解決できない文字列キー名の行が、適用時に無効となり診断されることの確認
TEST_F(stringCatalogFilterApplyTest, unresolved_key_name_disables_line_and_is_diagnosed)
{
    // Arrange
    static unsigned char image[kImageSize];
    const char *lines[] = {"key == UNKNOWN_KEY_NAME", "key == 2"};
    cplat_string_catalog_filter_diagnostic diagnostics[4];
    static unsigned char snapshot_image[kImageSize];
    std::uint64_t actual_enabled_lines = 0U;
    std::size_t actual_invalid_count = 0U;
    int actual_apply_ret;

    ASSERT_EQ(CPLAT_OK,
              compile_lines(lines, 2U, image)); // [状態] - 名前解決できない行と、解決できる行をコンパイルする。

    // Pre-Assert

    // Act
    actual_apply_ret = cplat_string_catalog_filter_slot_apply(slot_, image, kImageSize, diagnostics, 4U,
                                                              &actual_invalid_count); // [手順] - スロットへ適用する。
    (void)cplat_string_catalog_filter_slot_snapshot(
        slot_, snapshot_image, kImageSize,
        &actual_enabled_lines); // [手順] - 適用中のイメージと有効行の集合を取得する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_apply_ret); // [確認_正常系] - 名前解決できない行があっても適用は成功すること。
    EXPECT_EQ(1U, actual_invalid_count);   // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_UNRESOLVED_KEY_NAME,
              diagnostics[0].error);                   // [確認_正常系] - 原因が名前解決できない文字列キーであること。
    EXPECT_EQ(0U, diagnostics[0].line_index);          // [確認_正常系] - イメージ内の行 0 が対象であること。
    EXPECT_EQ(0U, actual_enabled_lines & (1ULL << 0)); // [確認_正常系] - 行 0 のビットが立っていないこと。
    EXPECT_NE(0U, actual_enabled_lines & (1ULL << 1)); // [確認_正常系] - 行 1 のビットは立っていること。
}

// 名前解決できない引数名の行が、適用時に無効となり診断されることの確認
TEST_F(stringCatalogFilterApplyTest, unresolved_argument_name_disables_line_and_is_diagnosed)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_diagnostic diagnostics[4];
    static unsigned char snapshot_image[kImageSize];
    std::uint64_t actual_enabled_lines = 0xFFFFFFFFFFFFFFFFULL;
    std::size_t actual_invalid_count = 0U;
    int actual_apply_ret;

    ASSERT_EQ(CPLAT_OK, compile_single_line("arg.nonexistent_argument == 1",
                                            image)); // [状態] - カタログのどの項目にもない引数名の行をコンパイルする。

    // Pre-Assert

    // Act
    actual_apply_ret = cplat_string_catalog_filter_slot_apply(slot_, image, kImageSize, diagnostics, 4U,
                                                              &actual_invalid_count); // [手順] - スロットへ適用する。
    (void)cplat_string_catalog_filter_slot_snapshot(slot_, snapshot_image, kImageSize,
                                                    &actual_enabled_lines); // [手順] - 適用中の有効行の集合を取得する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_apply_ret); // [確認_正常系] - 名前解決できない行があっても適用は成功すること。
    EXPECT_EQ(1U, actual_invalid_count);   // [確認_正常系] - 無効にした行が 1 件であること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_LINE_ERROR_UNRESOLVED_ARGUMENT_NAME,
              diagnostics[0].error);                   // [確認_正常系] - 原因が名前解決できない引数名であること。
    EXPECT_EQ(0U, diagnostics[0].line_index);          // [確認_正常系] - イメージ内の行 0 が対象であること。
    EXPECT_EQ(0U, actual_enabled_lines & (1ULL << 0)); // [確認_正常系] - 行 0 のビットが立っていないこと。
}

// 検証に失敗するイメージの適用が、以前の判定状態を維持することの確認
TEST_F(stringCatalogFilterApplyTest, apply_with_corrupt_image_keeps_previous_state)
{
    // Arrange
    static unsigned char valid_image[kImageSize];
    static unsigned char corrupt_image[kImageSize];
    cplat_string_catalog_filter_state actual_state_before;
    cplat_string_catalog_filter_state actual_state_after;
    int actual_corrupt_apply_ret;

    ASSERT_EQ(CPLAT_OK,
              compile_single_line("key == 2",
                                  valid_image)); // [状態] - JOB_RECEIVED (key=2) に一致する条件式をコンパイルする。
    ASSERT_EQ(CPLAT_OK,
              compile_single_line("key == 1", corrupt_image));    // [状態] - 別の内容をコンパイルしたうえで破損させる。
    corrupt_image[0] = (unsigned char)(corrupt_image[0] ^ 0xFFU); // [状態] - 署名を破損させる。
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_apply(slot_, valid_image, kImageSize, nullptr, 0U,
                                                               nullptr)); // [状態] - 正常なイメージを適用する。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED, &actual_state_before);
    ASSERT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_before); // [状態確認] - 適用直後は常に一致であること。

    // Pre-Assert

    // Act
    actual_corrupt_apply_ret = cplat_string_catalog_filter_slot_apply(
        slot_, corrupt_image, kImageSize, nullptr, 0U, nullptr); // [手順] - 破損したイメージを適用しようとする。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
                                                &actual_state_after); // [手順] - 適用の試行後の状態を取得する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_CORRUPT_DESCRIPTOR,
              actual_corrupt_apply_ret); // [確認_異常系] - 破損したイメージの適用は失敗すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_after); // [確認_異常系] - 以前の判定状態 (常に一致) が維持されること。
}

// ハッシュ値は正しく、構造の検査で拒否されるイメージの適用が、以前の判定状態を維持することの確認
TEST_F(stringCatalogFilterApplyTest, apply_with_structurally_broken_image_keeps_previous_state)
{
    // Arrange
    static unsigned char valid_image[kImageSize];
    static unsigned char broken_image[kImageSize];
    unsigned char *record;
    string_catalog_filter_instruction instruction;
    string_catalog_filter_record_header record_header;
    string_catalog_filter_image_header image_header;
    cplat_string_catalog_filter_state actual_state_after;
    int actual_broken_apply_ret;

    ASSERT_EQ(CPLAT_OK,
              compile_single_line("key == 2",
                                  valid_image)); // [状態] - JOB_RECEIVED (key=2) に一致する条件式をコンパイルする。
    ASSERT_EQ(CPLAT_OK, compile_single_line("key == 1", broken_image)); // [状態] - 別の内容をコンパイルする。
    record = filter_test_record_address(broken_image, kLineWidth, 0U);
    string_catalog_filter_read_instruction(record, 0U, &instruction);
    instruction.opcode = 0x7FU;
    string_catalog_filter_write_instruction(record, 0U, &instruction); // [状態] - 命令の種類を存在しない値にする。
    string_catalog_filter_read_record_header(record, &record_header);
    record_header.line_hash = string_catalog_filter_compute_line_hash(record, (uint32_t)kLineWidth);
    string_catalog_filter_write_record_header(record, &record_header);
    string_catalog_filter_read_image_header(broken_image, &image_header);
    string_catalog_filter_update_content_hash(broken_image, &image_header); // [状態] - ハッシュ値を計算し直す。
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_apply(slot_, valid_image, kImageSize, nullptr, 0U,
                                                               nullptr)); // [状態] - 正常なイメージを適用する。

    // Pre-Assert

    // Act
    actual_broken_apply_ret = cplat_string_catalog_filter_slot_apply(
        slot_, broken_image, kImageSize, nullptr, 0U, nullptr); // [手順] - 構造が壊れたイメージを適用しようとする。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
                                                &actual_state_after); // [手順] - 適用の試行後の状態を取得する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_CORRUPT_DESCRIPTOR,
              actual_broken_apply_ret); // [確認_異常系] - 構造が壊れたイメージの適用は失敗すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_after); // [確認_異常系] - 以前の判定状態 (常に一致) が維持されること。
}

// 行数の上限や行幅がスロットと異なるイメージの適用が、CPLAT_ERR_CORRUPT_DESCRIPTOR を返し状態を維持することの確認
TEST_F(stringCatalogFilterApplyTest, apply_with_mismatched_line_width_returns_corrupt_descriptor_and_keeps_state)
{
    // Arrange
    static unsigned char valid_image[kImageSize];
    static unsigned char mismatched_image[kImageSize];
    cplat_string_catalog_filter_state actual_state_before;
    cplat_string_catalog_filter_state actual_state_after;
    int actual_mismatched_apply_ret;

    ASSERT_EQ(CPLAT_OK,
              compile_single_line("key == 2",
                                  valid_image)); // [状態] - JOB_RECEIVED (key=2) に一致する条件式をコンパイルする。
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_apply(slot_, valid_image, kImageSize, nullptr, 0U,
                                                               nullptr)); // [状態] - 正常なイメージを適用する。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED, &actual_state_before);
    ASSERT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_before); // [状態確認] - 適用直後は常に一致であること。

    std::memset(mismatched_image, 0, kImageSize); // [状態] - スロットと同じバイト数の領域を確保する。
    ASSERT_EQ(CPLAT_OK,
              compile_single_line(
                  "key == 1", mismatched_image, CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(kLineCapacity, 80U), 80U,
                  kLineCapacity)); // [状態] - スロットとは行幅が異なる (80) イメージを、その領域内にコンパイルする。

    // Pre-Assert

    // Act
    actual_mismatched_apply_ret = cplat_string_catalog_filter_slot_apply(
        slot_, mismatched_image, kImageSize, nullptr, 0U, nullptr); // [手順] - 行幅が異なるイメージを適用しようとする。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
                                                &actual_state_after); // [手順] - 適用の試行後の状態を取得する。

    // Assert
    EXPECT_EQ(
        CPLAT_ERR_CORRUPT_DESCRIPTOR,
        actual_mismatched_apply_ret); // [確認_異常系] - 行幅の不一致により CPLAT_ERR_CORRUPT_DESCRIPTOR を返すこと。
    EXPECT_EQ(CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
              actual_state_after); // [確認_異常系] - 以前の判定状態が維持されること。
}

// 呼び出し側のイメージ領域がスロット内部へ複製され、適用後に元の領域を 0 で上書きしても判定結果が変わらないことの確認
TEST_F(stringCatalogFilterApplyTest, apply_copies_image_so_caller_buffer_can_be_cleared_afterwards)
{
    // Arrange
    static unsigned char image[kImageSize];
    cplat_string_catalog_filter_state actual_state;
    int actual_apply_ret;

    ASSERT_EQ(CPLAT_OK, compile_single_line("key == 2",
                                            image)); // [状態] - JOB_RECEIVED (key=2) に一致する条件式をコンパイルする。

    // Pre-Assert

    // Act
    actual_apply_ret = cplat_string_catalog_filter_slot_apply(slot_, image, kImageSize, nullptr, 0U,
                                                              nullptr); // [手順] - スロットへ適用する。
    std::memset(image, 0, kImageSize); // [手順] - 適用に使った呼び出し側の領域を 0 で上書きする。
    (void)cplat_string_catalog_filter_slot_test(slot_, FILTER_TEST_TRACE_KEY_JOB_RECEIVED,
                                                &actual_state); // [手順] - 領域を上書きした後に判定状態を取得する。

    // Assert
    ASSERT_EQ(CPLAT_OK, actual_apply_ret); // [確認_正常系] - 適用が成功すること。
    EXPECT_EQ(
        CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH,
        actual_state); // [確認_正常系] - 呼び出し側の領域を破壊しても、スロット内部の複製により判定結果が変わらないこと。
}
