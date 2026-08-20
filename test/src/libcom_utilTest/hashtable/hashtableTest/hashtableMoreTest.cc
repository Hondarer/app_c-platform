#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include <cstdint>
#include <cstring>
#include <vector>

namespace
{

com_util_timespec k_insert_timestamp = {1, 0};

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

class hashtableMoreTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableMoreTest, attach_rejects_invalid_buffers)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_needed = 0;
    size_t data_needed = 0;
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *attached = reinterpret_cast<com_util_hashtable *>(1);
    std::vector<unsigned char> value(8, 1);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 外部バッファー用の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed, 0);
    std::vector<unsigned char> buf_data(data_needed, 0);
    (void)com_util_hashtable_create(&config, buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &ht);
    (void)com_util_hashtable_add(ht, "a", value.data());
    int actual_ret_null = com_util_hashtable_attach(NULL, mgmt_needed, buf_data.data(), buf_data.size(),
                                                    &attached); // [手順] - buf_mgmt に NULL を渡す。
    int actual_ret_small = com_util_hashtable_attach(buf_mgmt.data(), 4, buf_data.data(), buf_data.size(),
                                                     &attached); // [手順] - 短すぎる buf_mgmt_size を渡す。
    std::vector<unsigned char> bad_magic = buf_mgmt;
    bad_magic[0] ^= 0xFF;
    int actual_ret_magic =
        com_util_hashtable_attach(bad_magic.data(), bad_magic.size(), buf_data.data(), buf_data.size(),
                                  &attached); // [手順] - マジックを壊して再接続する。
    std::vector<unsigned char> bad_version = buf_mgmt;
    bad_version[4] = 9;
    int actual_ret_version =
        com_util_hashtable_attach(bad_version.data(), bad_version.size(), buf_data.data(), buf_data.size(),
                                  &attached); // [手順] - 版番号を壊して再接続する。
    std::vector<unsigned char> small_data(1, 0);
    int actual_ret_small_data =
        com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), small_data.data(), small_data.size(),
                                  &attached); // [手順] - 短すぎる buf_data_size を渡す。
    int actual_ret_ok = com_util_hashtable_attach(buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(),
                                                  &attached); // [手順] - 正常な複製で再接続する。
    com_util_hashtable_destroy(ht);
    com_util_hashtable_destroy(attached);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null); // [確認_異常系] - NULL buf_mgmt が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              actual_ret_small); // [確認_異常系] - 短すぎる管理領域が BUFFER_TOO_SMALL であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_magic); // [確認_異常系] - マジック不一致が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_version); // [確認_異常系] - 版不一致が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              actual_ret_small_data);      // [確認_異常系] - 短すぎるデータ領域が BUFFER_TOO_SMALL であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ok); // [確認_正常系] - 正常な attach が成功すること。
}

TEST_F(hashtableMoreTest, delete_rec_and_purge)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 2);
    uint64_t rec = 0;
    int status = -1;
    size_t empty = 0;

    fill_config(&config, 4, 16, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 5 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(ht, "keep", value.data());          // [手順] - 残すキーを追加する。
    (void)com_util_hashtable_add(ht, "drop", value.data());          // [手順] - 削除するキーを追加する。
    (void)com_util_hashtable_find_recno(ht, "drop", &rec);
    int actual_ret_delete = com_util_hashtable_delete_rec(ht, rec); // [手順] - レコード番号で削除する。
    (void)com_util_hashtable_get_status(ht, rec, &status);
    int actual_ret_missing = com_util_hashtable_update(ht, "none", value.data()); // [手順] - 無いキーを更新する。
    int actual_ret_purge = com_util_hashtable_purge_deleted(ht);                  // [手順] - 削除済みを即時回収する。
    (void)com_util_hashtable_get_status(ht, rec, &status);
    (void)com_util_hashtable_empty_count(ht, &empty);
    const void *key_out = nullptr;
    int actual_ret_bad_rec = com_util_hashtable_get_key_ref(ht, 0, &key_out); // [手順] - レコード番号 0 を読む。
    int actual_ret_delete_missing = com_util_hashtable_delete(ht, "drop");    // [手順] - 回収後のキーを削除する。
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete); // [確認_正常系] - delete_rec が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_missing);            // [確認_異常系] - 無いキーの update が NOT_FOUND であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_purge); // [確認_正常系] - purge が成功すること。
    EXPECT_EQ(0, status);                     // [確認_正常系] - purge 後に空へ戻ること。
    EXPECT_GE(empty, 1u);                     // [確認_正常系] - 空件数が増えること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_bad_rec); // [確認_異常系] - レコード番号 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_delete_missing); // [確認_異常系] - 回収後の削除が NOT_FOUND であること。
}

TEST_F(hashtableMoreTest, delete_with_lifetime_two_expires_immediately)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 4);
    int status = -1;
    size_t empty = 0;

    fill_config(&config, 2, 16, 8, 2, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(ht, "a", value.data());
    int actual_ret_delete = com_util_hashtable_delete(ht, "a"); // [手順] - lifetime 2 で削除する。
    (void)com_util_hashtable_get_status(ht, 1, &status);
    (void)com_util_hashtable_empty_count(ht, &empty);
    int actual_ret_add = com_util_hashtable_add(ht, "b", value.data()); // [手順] - 別キーを直ちに追加する。
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete); // [確認_正常系] - delete が成功すること。
    EXPECT_EQ(0, status);                      // [確認_正常系] - 削除直後に空へ戻ること。
    EXPECT_GE(empty, 1u);                      // [確認_正常系] - 空スロットがあること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);    // [確認_正常系] - 別キーを直ちに追加できること。
}

TEST_F(hashtableMoreTest, delete_with_lifetime_two_sets_next_empty_when_table_full)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 5);
    size_t empty_before = 99;

    fill_config(&config, 2, 16, 8, 2,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 2、capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(ht, "a", value.data());
    (void)com_util_hashtable_add(ht, "b", value.data()); // [手順] - 満杯にする(next_empty が 0 になる)。
    (void)com_util_hashtable_empty_count(ht, &empty_before);
    int actual_ret_delete = com_util_hashtable_delete(ht, "a"); // [手順] - 満杯の状態で lifetime 2 の削除をする。
    int actual_ret_add = com_util_hashtable_add(ht, "c", value.data()); // [手順] - 空いた直後にすぐ追加する。
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(0u, empty_before);               // [確認_正常系] - 削除前は満杯であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete); // [確認_正常系] - 満杯からの delete が成功すること。
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret_add); // [確認_正常系] - next_empty==0 の分岐で更新された空きへ追加できること。
}

TEST_F(hashtableMoreTest, delete_with_lifetime_two_keeps_earlier_next_empty_hint)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 6);
    uint64_t next_empty_rec = 0;

    fill_config(&config, 3, 16, 8, 2,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 2、capacity 3 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_insert_direct(ht, 1, "a", 1, value.data(),
                                           &k_insert_timestamp); // [手順] - レコード 1 を占有する(next_empty は 2 になる)。
    (void)com_util_hashtable_insert_direct(ht, 3, "c", 1, value.data(),
                                           &k_insert_timestamp); // [手順] - レコード 2 を空けたままレコード 3 を占有する。
    int actual_ret_delete =
        com_util_hashtable_delete(ht, "c"); // [手順] - レコード 3(next_empty より大きい)を lifetime 2 で削除する。
    int actual_ret_add = com_util_hashtable_add(ht, "d", value.data()); // [手順] - 別キーを追加する。
    (void)com_util_hashtable_find_recno(ht, "d", &next_empty_rec);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete); // [確認_正常系] - delete が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);    // [確認_正常系] - 追加が成功すること。
    EXPECT_EQ(2u,
              next_empty_rec); // [確認_正常系] - next_empty がレコード 3 で上書きされず、レコード 2 のままだったこと。
}

TEST_F(hashtableMoreTest, required_size_rejects_overflowing_layout)
{
    // Arrange
    com_util_hashtable_config config = {};
    size_t mgmt_size = 1;
    size_t data_size = 1;
    com_util_hashtable *ht = reinterpret_cast<com_util_hashtable *>(1);

    fill_config(&config, (SIZE_MAX / sizeof(uint64_t)) + 1u, 8, 8, 5,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 乗算がオーバーフローする capacity を用意する。

    // Pre-Assert

    // Act
    int actual_ret_size =
        com_util_hashtable_required_size(&config, &mgmt_size, &data_size);     // [手順] - 必要サイズを求める。
    int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 同じ設定で構築する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_size); // [確認_異常系] - オーバーフローする設定が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_create); // [確認_異常系] - create も同じ設定を拒否すること。
    EXPECT_EQ(1u, mgmt_size); // [確認_異常系] - 失敗時に mgmt_size を書き換えないこと。
    EXPECT_EQ(1u, data_size); // [確認_異常系] - 失敗時に data_size を書き換えないこと。
}

TEST_F(hashtableMoreTest, validate_returns_out_of_memory_when_calloc_fails)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 3);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_calloc(_, _))
        .WillOnce(DoDefault())
        .WillOnce(Return(nullptr)); // [Pre-Assert確認_異常系] - validate 用の calloc が失敗すること。
                                    // [Pre-Assert手順] - 2 回目の com_util_calloc から NULL を返却する。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(ht, "a", value.data());
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - validate を呼び出す。
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              actual_ret); // [確認_異常系] - validate の確保失敗が OUT_OF_MEMORY であること。
}
