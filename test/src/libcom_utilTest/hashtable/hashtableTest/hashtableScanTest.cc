#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include <cstring>
#include <vector>

namespace
{

void fill_config(com_util_hashtable_config *config, size_t capacity, unsigned char lifetime)
{
    *config = {};
    config->capacity = capacity;
    config->key_type = COM_UTIL_HASHTABLE_FIELD_FIXED_STRING;
    config->value_type = COM_UTIL_HASHTABLE_FIELD_FIXED_BINARY;
    config->timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD;
    config->key_size = 8;
    config->value_size = 8;
    config->lifetime = lifetime;
}

/* status_mask に該当するレコード番号を、走査して順に集める。 */
std::vector<uint64_t> collect(const com_util_hashtable *ht, unsigned int status_mask, int *last_ret)
{
    std::vector<uint64_t> records;
    uint64_t cursor = 0;
    int has_record = 1;

    while (has_record != 0)
    {
        uint64_t record = 0;

        *last_ret = com_util_hashtable_next_record(ht, cursor, status_mask, &record, &has_record);
        if (*last_ret != COM_UTIL_OK)
        {
            break;
        }
        if (has_record == 0)
        {
            break;
        }
        records.push_back(record);
        cursor = record;
    }
    return records;
}

} // namespace

class hashtableScanTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableScanTest, walks_each_status_group)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    int ret_in_use = COM_UTIL_ERR_UNKNOWN;
    int ret_deleted = COM_UTIL_ERR_UNKNOWN;
    int ret_empty = COM_UTIL_ERR_UNKNOWN;
    int ret_all = COM_UTIL_ERR_UNKNOWN;

    fill_config(&config, 4, COM_UTIL_HASHTABLE_LIFETIME_INFINITE);
    config.timestamp_scope =
        COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE; // [状態] - 時刻と世代を省いて直接配置できる粒度にする。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_insert_direct(ht, 1, "a", 1, value.data(), NULL, 0);
    (void)com_util_hashtable_insert_direct(ht, 2, "b", 2, value.data(), NULL, 0);
    (void)com_util_hashtable_insert_direct(ht, 3, "c", 1, value.data(), NULL, 0);
    /* レコード 4 は空のまま残す。 */
    std::vector<uint64_t> in_use = collect(ht, COM_UTIL_HASHTABLE_SCAN_IN_USE, &ret_in_use);
    std::vector<uint64_t> deleted = collect(ht, COM_UTIL_HASHTABLE_SCAN_DELETED, &ret_deleted);
    std::vector<uint64_t> empty = collect(ht, COM_UTIL_HASHTABLE_SCAN_EMPTY, &ret_empty);
    std::vector<uint64_t> all = collect(
        ht, COM_UTIL_HASHTABLE_SCAN_IN_USE | COM_UTIL_HASHTABLE_SCAN_DELETED | COM_UTIL_HASHTABLE_SCAN_EMPTY, &ret_all);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret_in_use);                      // [確認_正常系] - 使用中の走査が成功すること。
    EXPECT_EQ(std::vector<uint64_t>({1u, 3u}), in_use);      // [確認_正常系] - 使用中のレコードだけを昇順に返すこと。
    EXPECT_EQ(COM_UTIL_OK, ret_deleted);                     // [確認_正常系] - 削除済みの走査が成功すること。
    EXPECT_EQ(std::vector<uint64_t>({2u}), deleted);         // [確認_正常系] - 削除済みのレコードだけを返すこと。
    EXPECT_EQ(COM_UTIL_OK, ret_empty);                       // [確認_正常系] - 空の走査が成功すること。
    EXPECT_EQ(std::vector<uint64_t>({4u}), empty);           // [確認_正常系] - 空のレコードだけを返すこと。
    EXPECT_EQ(COM_UTIL_OK, ret_all);                         // [確認_正常系] - 全状態の走査が成功すること。
    EXPECT_EQ(std::vector<uint64_t>({1u, 2u, 3u, 4u}), all); // [確認_正常系] - すべてのレコードを昇順に返すこと。
}

TEST_F(hashtableScanTest, reports_end_of_scan_without_error)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    uint64_t record = 12345;
    int has_record = -1;

    fill_config(&config, 4, 5); // [状態] - 空のテーブルを用意する。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_empty_table =
        com_util_hashtable_next_record(ht, 0, COM_UTIL_HASHTABLE_SCAN_IN_USE, &record, &has_record);
    int has_record_at_end = -1;
    uint64_t record_at_end = 54321;
    int actual_ret_at_end = com_util_hashtable_next_record(ht, 4, COM_UTIL_HASHTABLE_SCAN_EMPTY, &record_at_end,
                                                           &has_record_at_end); // [手順] - 終端から走査する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_empty_table); // [確認_正常系] - 該当が無くてもエラーにならないこと。
    EXPECT_EQ(0, has_record);                       // [確認_正常系] - 該当が無いことが出力引数で分かること。
    EXPECT_EQ(12345u, record);                      // [確認_正常系] - 該当が無いとき格納先を書き換えないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_at_end);      // [確認_正常系] - capacity と等しい位置からの走査が成功すること。
    EXPECT_EQ(0, has_record_at_end);                // [確認_正常系] - 終端では該当が無いこと。
    EXPECT_EQ(54321u, record_at_end);               // [確認_正常系] - 終端でも格納先を書き換えないこと。
}

TEST_F(hashtableScanTest, guards_reject_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    uint64_t record = 0;
    int has_record = 0;

    fill_config(&config, 4, 5);

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht =
        com_util_hashtable_next_record(NULL, 0, COM_UTIL_HASHTABLE_SCAN_IN_USE, &record, &has_record);
    int actual_ret_null_record =
        com_util_hashtable_next_record(ht, 0, COM_UTIL_HASHTABLE_SCAN_IN_USE, NULL, &has_record);
    int actual_ret_null_has = com_util_hashtable_next_record(ht, 0, COM_UTIL_HASHTABLE_SCAN_IN_USE, &record, NULL);
    int actual_ret_zero_mask = com_util_hashtable_next_record(ht, 0, 0, &record, &has_record);
    int actual_ret_unknown_mask =
        com_util_hashtable_next_record(ht, 0, 0x8u, &record, &has_record); // [手順] - 未定義のビットを渡す。
    int actual_ret_from_over =
        com_util_hashtable_next_record(ht, 5, COM_UTIL_HASHTABLE_SCAN_IN_USE, &record,
                                       &has_record); // [手順] - capacity を超える開始位置を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_null_ht); // [確認_異常系] - ht が NULL なら失敗すること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_record); // [確認_異常系] - レコード番号の格納先が NULL なら失敗すること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_has); // [確認_異常系] - 該当有無の格納先が NULL なら失敗すること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_zero_mask); // [確認_異常系] - マスクが 0 なら失敗すること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_unknown_mask); // [確認_異常系] - 未定義のビットを含むマスクが失敗すること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_from_over); // [確認_異常系] - capacity を超える開始位置が失敗すること。
}
