#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include "hashtable.inject.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace
{

void fill_config(com_util_hashtable_config *config, size_t capacity, size_t key_size, size_t record_size,
                 unsigned char lifetime, com_util_hashtable_key_type key_type)
{
    *config = {};
    config->capacity = capacity;
    config->key_type = key_type;
    config->key_size = key_size;
    config->record_size = record_size;
    config->lifetime = lifetime;
}

} // namespace

class hashtableAttachTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableAttachTest, rejects_misaligned_buffer)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *attached = reinterpret_cast<com_util_hashtable *>(1);
    size_t mgmt_needed = 0;
    size_t data_needed = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    (void)com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed + 1, 0);
    std::vector<unsigned char> buf_data(data_needed, 0);
    (void)com_util_hashtable_create(&config, buf_mgmt.data(), mgmt_needed, buf_data.data(), buf_data.size(),
                                    &ht); // [状態] - 先頭が整列した妥当な管理領域を構築する。

    // Pre-Assert

    // Act
    int actual_ret = com_util_hashtable_attach(buf_mgmt.data() + 1, mgmt_needed, buf_data.data(), buf_data.size(),
                                               &attached); // [手順] - 1 バイトずれた管理領域へ再接続する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              actual_ret);        // [確認_異常系] - 未整列の領域が BUFFER_TOO_SMALL であること。
    EXPECT_EQ(nullptr, attached); // [確認_異常系] - 失敗後の ht_out が NULL であること。
}

TEST_F(hashtableAttachTest, rejects_null_ht_out)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    size_t mgmt_needed = 0;
    size_t data_needed = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    (void)com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed, 0);
    std::vector<unsigned char> buf_data(data_needed, 0);
    (void)com_util_hashtable_create(&config, buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(),
                                    &ht); // [状態] - 妥当な領域を構築する。

    // Pre-Assert

    // Act
    int actual_ret = com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(),
                                               NULL); // [手順] - ht_out に NULL を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret); // [確認_異常系] - NULL ht_out が INVALID_ARGUMENT であること。
}

TEST_F(hashtableAttachTest, accepts_binary_key_type)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *attached = nullptr;
    size_t mgmt_needed = 0;
    size_t data_needed = 0;
    unsigned char key[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<unsigned char> value(8, 9);
    const void *found = nullptr;

    fill_config(&config, 2, 8, 8, 5,
                COM_UTIL_HASHTABLE_KEY_BINARY); // [状態] - バイナリ キー種別の設定を用意する。
    (void)com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed, 0);
    std::vector<unsigned char> buf_data(data_needed, 0);

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(),
                                    &ht); // [手順] - バイナリ キーのテーブルを構築する。
    (void)com_util_hashtable_add(ht, key, value.data(), COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_attach =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(),
                                  &attached); // [手順] - バイナリ キーのテーブルへ再接続する。
    int actual_ret_find = com_util_hashtable_find_value_ref(attached, key, &found);
    com_util_hashtable_dispose(ht);
    com_util_hashtable_dispose(attached);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attach); // [確認_正常系] - バイナリ キー種別の再接続が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);   // [確認_正常系] - 再接続後も検索できること。
}

TEST_F(hashtableAttachTest, rejects_corrupted_config_fields)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable_config bad_config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *attached = nullptr;
    size_t mgmt_needed = 0;
    size_t data_needed = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    (void)com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed, 0);
    std::vector<unsigned char> buf_data(data_needed, 0);

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(),
                                    &ht); // [手順] - 外部バッファーへ構築する。

    bad_config = config;
    {
        int invalid_key_type = 2;
        std::memcpy(&bad_config.key_type, &invalid_key_type, sizeof(bad_config.key_type));
    }
    test_hashtable_set_config(ht, &bad_config); // [手順] - key_type を不正な値へ書き換える。
    int actual_ret_key_type =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    bad_config = config;
    {
        int invalid_scope = 2;
        std::memcpy(&bad_config.timestamp_scope, &invalid_scope, sizeof(bad_config.timestamp_scope));
    }
    test_hashtable_set_config(ht, &bad_config); // [手順] - timestamp_scope を不正な値へ書き換える。
    int actual_ret_scope =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    bad_config = config;
    bad_config.key_size = 0; // [手順] - key_size を 0 へ書き換える。
    test_hashtable_set_config(ht, &bad_config);
    int actual_ret_key_size =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    bad_config = config;
    bad_config.record_size = 0; // [手順] - record_size を 0 へ書き換える。
    test_hashtable_set_config(ht, &bad_config);
    int actual_ret_record_size =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    bad_config = config;
    bad_config.lifetime = 1; // [手順] - lifetime を 1 へ書き換える。
    test_hashtable_set_config(ht, &bad_config);
    int actual_ret_lifetime =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    bad_config = config;
    bad_config.capacity = 0; // [手順] - capacity を 0 へ書き換える。
    test_hashtable_set_config(ht, &bad_config);
    int actual_ret_capacity =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    test_hashtable_set_config(ht, &config);
    test_hashtable_set_next_empty(ht, 99); // [手順] - next_empty を capacity 超へ書き換える。
    int actual_ret_next_empty =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    test_hashtable_set_next_empty(ht, 1);
    test_hashtable_set_counts(ht, 3, 0); // [手順] - in_use_count を capacity 超へ書き換える。
    int actual_ret_in_use_count =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    test_hashtable_set_counts(ht, 0, 3); // [手順] - deleted_count を capacity 超へ書き換える。
    int actual_ret_deleted_count =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    test_hashtable_set_counts(ht, 0, 0);
    bad_config = config;
    bad_config.capacity = (SIZE_MAX / sizeof(uint64_t)) + 1u; // [手順] - レイアウト計算が破綻する capacity を書き込む。
    test_hashtable_set_config(ht, &bad_config);
    int actual_ret_layout =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &attached);

    test_hashtable_set_config(ht, &config);
    int actual_ret_mgmt_too_small =
        com_util_hashtable_attach(buf_mgmt.data(), mgmt_needed - 1, buf_data.data(), buf_data.size(),
                                  &attached); // [手順] - 管理領域の buf_mgmt_size 不足で再接続する。
    std::vector<unsigned char> small_data(1, 0);
    int actual_ret_data_too_small =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), small_data.data(), small_data.size(),
                                  &attached); // [手順] - データ領域の buf_data_size 不足で再接続する。

    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_key_type); // [確認_異常系] - 不正な key_type が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_scope); // [確認_異常系] - 不正な timestamp_scope が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_key_size); // [確認_異常系] - key_size 0 が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_record_size);                             // [確認_異常系] - record_size 0 が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_lifetime); // [確認_異常系] - lifetime 1 が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_capacity); // [確認_異常系] - capacity 0 が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_next_empty); // [確認_異常系] - capacity を超える next_empty が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_in_use_count); // [確認_異常系] - capacity を超える in_use_count が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_deleted_count); // [確認_異常系] - capacity を超える deleted_count が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_layout); // [確認_異常系] - レイアウト計算の破綻が拒否されること。
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              actual_ret_mgmt_too_small); // [確認_異常系] - 管理領域不足が BUFFER_TOO_SMALL であること。
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              actual_ret_data_too_small); // [確認_異常系] - データ領域不足が BUFFER_TOO_SMALL であること。
}
