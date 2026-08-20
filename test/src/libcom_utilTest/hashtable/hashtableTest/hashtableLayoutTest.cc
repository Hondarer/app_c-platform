#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include <cstdint>
#include <vector>

namespace
{

void fill_config(com_util_hashtable_config *config, size_t capacity, size_t key_size, size_t record_size,
                 unsigned char lifetime, com_util_hashtable_key_type key_type)
{
    *config = {};
    config->capacity = capacity;
    config->key_type = key_type;
    config->timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD;
    config->key_size = key_size;
    config->record_size = record_size;
    config->lifetime = lifetime;
}

} // namespace

class hashtableLayoutTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

/*
 * hashtable_mgmt_layout 内の align_up_checked 呼び出しのうち、以下の 2 箇所は現行の実装では
 * 到達不能である(how-to-test.md「到達できない条件への対処」手順 3)。
 * - 先頭オフセットの整列: 入力が sizeof(struct com_util_hashtable) 固定の小さな値のため、
 *   uint64_t 境界への整列でオーバーフローする余地がない。
 * - バケット領域直後の整列: オフセットは header(8 バイト境界) + capacity * sizeof(uint64_t)
 *   の和であり、常に 8 の倍数になるため pad が 0 に固定され、加算は失敗しない。
 */

TEST_F(hashtableLayoutTest, rejects_entry_stride_overflow)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 1;
    size_t data_size = 1;

    fill_config(
        &config, 1, SIZE_MAX, 8, 5,
        COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - entry_stride_checked 内の加算が破綻する key_size を用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [手順] - 必要サイズを求める。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - キー ストライド計算のオーバーフローが INVALID_ARGUMENT であること。
}

TEST_F(hashtableLayoutTest, rejects_bucket_region_offset_overflow)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 1;
    size_t data_size = 1;

    fill_config(
        &config, SIZE_MAX / sizeof(uint64_t), 8, 8, 5,
        COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - バケット領域の乗算は成功するが加算が破綻する capacity を用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [手順] - 必要サイズを求める。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - バケット領域オフセット加算のオーバーフローが INVALID_ARGUMENT であること。
}

TEST_F(hashtableLayoutTest, rejects_entries_region_multiplication_overflow)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 1;
    size_t data_size = 1;

    fill_config(
        &config, 3, SIZE_MAX / 2, 8, 5,
        COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - entry_stride は算出できるが capacity 倍で破綻する組み合わせを用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [手順] - 必要サイズを求める。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - エントリ領域サイズ計算のオーバーフローが INVALID_ARGUMENT であること。
}

TEST_F(hashtableLayoutTest, rejects_entries_region_offset_overflow)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 1;
    size_t data_size = 1;

    fill_config(
        &config, 1, SIZE_MAX - 50, 8, 5,
        COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - entry_stride の乗算は成功するが加算が破綻する組み合わせを用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [手順] - 必要サイズを求める。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - エントリ領域オフセット加算のオーバーフローが INVALID_ARGUMENT であること。
}

TEST_F(hashtableLayoutTest, rejects_data_region_multiplication_overflow)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 1;
    size_t data_size = 1;

    fill_config(&config, 2, 8, SIZE_MAX, 5,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - データ領域サイズの乗算が破綻する record_size を用意する。

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [手順] - 必要サイズを求める。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - データ領域サイズ計算のオーバーフローが INVALID_ARGUMENT であること。
}

TEST_F(hashtableLayoutTest, required_size_accepts_zero_capacity)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 0;
    size_t data_size = 0;

    fill_config(
        &config, 0, 8, 8, 5,
        COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 0 の設定を用意する(required_size は capacity を検査しない)。

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [手順] - 必要サイズを求める。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret); // [確認_正常系] - capacity 0 では乗算があふれず、レイアウト計算自体は成功すること。
}

TEST_F(hashtableLayoutTest, create_rejects_misaligned_external_buffer)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = reinterpret_cast<com_util_hashtable *>(1);
    size_t mgmt_needed = 0;
    size_t data_needed = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    (void)com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed + 1, 0); // [状態] - 1 バイトずらして未整列にできる領域を用意する。
    std::vector<unsigned char> buf_data(data_needed, 0);

    // Pre-Assert

    // Act
    int actual_ret =
        com_util_hashtable_create(&config, buf_mgmt.data() + 1, mgmt_needed, buf_data.data(), buf_data.size(),
                                  &ht); // [手順] - 1 バイトずれた管理領域で構築する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              actual_ret);  // [確認_異常系] - 未整列の外部バッファーが BUFFER_TOO_SMALL であること。
    EXPECT_EQ(nullptr, ht); // [確認_異常系] - 失敗後の ht_out が NULL であること。
}

TEST_F(hashtableLayoutTest, create_internal_alloc_rejects_combined_size_overflow)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = reinterpret_cast<com_util_hashtable *>(1);
    size_t mgmt_size = 0;
    size_t data_size = 0;

    fill_config(&config, 1, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING);
    (void)com_util_hashtable_required_size(&config, &mgmt_size, &data_size); // [状態] - 管理領域サイズを求めておく。
    fill_config(&config, 1, 8, SIZE_MAX - mgmt_size + 1, 5,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 内部確保の mgmt_size + data_size 加算が破綻する record_size
                                                // を用意する(capacity 1 なので data_size 単体はあふれない)。

    // Pre-Assert

    // Act
    int actual_ret = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 内部確保で構築する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        actual_ret); // [確認_異常系] - 管理領域とデータ領域の合計サイズのオーバーフローが INVALID_ARGUMENT であること。
    EXPECT_EQ(nullptr, ht); // [確認_異常系] - 失敗後の ht_out が NULL であること。
}

TEST_F(hashtableLayoutTest, required_size_table_scope_is_smaller_than_record_scope)
{
    // Arrange
    com_util_hashtable_config table_config = {};
    com_util_hashtable_config record_config = {};
    size_t table_mgmt = 0;
    size_t table_data = 0;
    size_t record_mgmt = 0;
    size_t record_data = 0;

    fill_config(&table_config, 4, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING);
    table_config.timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE; // [状態] - テーブル横断のみの粒度にする。
    fill_config(&record_config, 4, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING);
    record_config.timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD; // [状態] - レコード単位の粒度にする。

    // Pre-Assert

    // Act
    int actual_ret_table = com_util_hashtable_required_size(&table_config, &table_mgmt, &table_data);
    int actual_ret_record = com_util_hashtable_required_size(&record_config, &record_mgmt, &record_data);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_table);  // [確認_正常系] - SCOPE_TABLE の必要サイズが求まること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_record); // [確認_正常系] - SCOPE_RECORD の必要サイズが求まること。
    EXPECT_LT(table_mgmt, record_mgmt);        // [確認_正常系] - SCOPE_TABLE の管理領域が厳密に小さいこと。
    EXPECT_EQ(table_data, record_data);        // [確認_正常系] - データ領域サイズは粒度で変わらないこと。
}
