#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

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

class hashtableConfigTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableConfigTest, get_config_ref_rejects_null_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const com_util_hashtable_config *out = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_get_config_ref(NULL, &out); // [手順] - ht に NULL を渡す。
    int actual_ret_null_out = com_util_hashtable_get_config_ref(ht, NULL);  // [手順] - config_out に NULL を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL config_out が INVALID_ARGUMENT であること。
}

TEST_F(hashtableConfigTest, get_config_val_rejects_null_out_and_propagates_ref_failure)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable_config out = {};
    com_util_hashtable *ht = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_out = com_util_hashtable_get_config_val(ht, NULL);  // [手順] - config_out に NULL を渡す。
    int actual_ret_null_ht = com_util_hashtable_get_config_val(NULL, &out); // [手順] - ht に NULL を渡す。
    int actual_ret_ok = com_util_hashtable_get_config_val(ht, &out);        // [手順] - 妥当な引数で呼び出す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL config_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht);         // [確認_異常系] - NULL ht が get_config_ref の失敗として伝播すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ok); // [確認_正常系] - 妥当な引数で複製できること。
    EXPECT_EQ(2u, out.capacity);           // [確認_正常系] - 複製内容が一致すること。
}

TEST_F(hashtableConfigTest, buffer_size_rejects_null_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    size_t mgmt_size = 0;
    size_t data_size = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht =
        com_util_hashtable_buffer_size(NULL, &mgmt_size, &data_size);  // [手順] - ht に NULL を渡す。
    int actual_ret_null_both = com_util_hashtable_buffer_size(ht, NULL, NULL); // [手順] - 両方の出力先に NULL を渡す。
    int actual_ret_only_mgmt =
        com_util_hashtable_buffer_size(ht, &mgmt_size, NULL); // [手順] - データ側だけ NULL を渡す。
    int actual_ret_only_data =
        com_util_hashtable_buffer_size(ht, NULL, &data_size); // [手順] - 管理側だけ NULL を渡す。
    int actual_ret_ok =
        com_util_hashtable_buffer_size(ht, &mgmt_size, &data_size); // [手順] - 妥当な引数で呼び出す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_both);              // [確認_異常系] - 両方 NULL のとき INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_only_mgmt); // [確認_正常系] - 管理側だけの問い合わせが成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_only_data); // [確認_正常系] - データ側だけの問い合わせが成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ok);        // [確認_正常系] - 妥当な引数で取得できること。
    EXPECT_GT(mgmt_size, 0u);                 // [確認_正常系] - 管理領域の必要サイズが 0 より大きいこと。
    EXPECT_GT(data_size, 0u);                 // [確認_正常系] - データ領域の必要サイズが 0 より大きいこと。
}

TEST_F(hashtableConfigTest, buffer_ref_rejects_null_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *mgmt = nullptr;
    const void *data = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_buffer_ref(NULL, &mgmt, &data); // [手順] - ht に NULL を渡す。
    int actual_ret_null_both = com_util_hashtable_buffer_ref(ht, NULL, NULL); // [手順] - 両方の出力先に NULL を渡す。
    int actual_ret_only_mgmt =
        com_util_hashtable_buffer_ref(ht, &mgmt, NULL); // [手順] - データ側だけ NULL を渡す。
    int actual_ret_only_data = com_util_hashtable_buffer_ref(ht, NULL, &data); // [手順] - 管理側だけ NULL を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_both);              // [確認_異常系] - 両方 NULL のとき INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_only_mgmt); // [確認_正常系] - 管理側だけの問い合わせが成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_only_data); // [確認_正常系] - データ側だけの問い合わせが成功すること。
}

TEST_F(hashtableConfigTest, buffer_ref_returns_internal_regions_contiguously)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *mgmt = nullptr;
    const void *data = nullptr;
    size_t mgmt_size = 0;
    size_t data_size = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);          // [手順] - 内部確保で構築する。
    int actual_ret = com_util_hashtable_buffer_ref(ht, &mgmt, &data); // [手順] - 両領域の先頭を取得する。
    (void)com_util_hashtable_buffer_size(ht, &mgmt_size, &data_size);
    const unsigned char *mgmt_bytes = static_cast<const unsigned char *>(mgmt);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret); // [確認_正常系] - buffer_ref が成功すること。
    EXPECT_NE(nullptr, mgmt);           // [確認_正常系] - 管理領域の先頭が非 NULL であること。
    EXPECT_NE(nullptr, data);           // [確認_正常系] - データ領域の先頭が非 NULL であること。
    EXPECT_EQ(static_cast<const void *>(mgmt_bytes + mgmt_size),
              data); // [確認_正常系] - 内部確保では管理領域の直後にデータ領域が続くこと。
}

TEST_F(hashtableConfigTest, buffer_ref_returns_external_regions_as_supplied)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *attached = nullptr;
    const void *mgmt = nullptr;
    const void *data = nullptr;
    const void *attached_mgmt = nullptr;
    const void *attached_data = nullptr;
    size_t mgmt_size = 0;
    size_t data_size = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    (void)com_util_hashtable_required_size(&config, &mgmt_size, &data_size);
    std::vector<uint64_t> buf_mgmt((mgmt_size + sizeof(uint64_t) - 1u) / sizeof(uint64_t), 0);
    std::vector<unsigned char> buf_data(data_size, 0); // [状態] - 外部指定用の 2 領域を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, buf_mgmt.data(), mgmt_size, buf_data.data(), buf_data.size(),
                                    &ht); // [手順] - 外部指定で構築する。
    int actual_ret_create = com_util_hashtable_buffer_ref(ht, &mgmt, &data);
    (void)com_util_hashtable_attach(buf_mgmt.data(), mgmt_size, buf_data.data(), buf_data.size(),
                                    &attached); // [手順] - 同じ領域へ再接続する。
    int actual_ret_attach = com_util_hashtable_buffer_ref(attached, &attached_mgmt, &attached_data);
    com_util_hashtable_dispose(ht);
    com_util_hashtable_dispose(attached);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create); // [確認_正常系] - 外部指定でも buffer_ref が成功すること。
    EXPECT_EQ(static_cast<const void *>(buf_mgmt.data()),
              mgmt); // [確認_正常系] - 渡した管理領域がそのまま返ること。
    EXPECT_EQ(static_cast<const void *>(buf_data.data()),
              data);                       // [確認_正常系] - 渡したデータ領域がそのまま返ること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attach); // [確認_正常系] - 再接続後も buffer_ref が成功すること。
    EXPECT_EQ(static_cast<const void *>(buf_mgmt.data()),
              attached_mgmt); // [確認_正常系] - 再接続後も管理領域が一致すること。
    EXPECT_EQ(static_cast<const void *>(buf_data.data()),
              attached_data); // [確認_正常系] - 再接続がデータ領域アドレスを渡した値で上書きすること。
}
