#include <testfw.h>

#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include <cstring>
#include <string>
#include <vector>

class hashtableVariableStringTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util;
};

static com_util_hashtable_config variable_config(size_t key_storage_size, size_t value_storage_size)
{
    com_util_hashtable_config config = {};

    config.capacity = 4;
    config.key_type = COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING;
    config.value_type = COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING;
    config.key_storage_size = key_storage_size;
    config.value_storage_size = value_storage_size;
    config.lifetime = 5;
    return config;
}

TEST_F(hashtableVariableStringTest, persists_variable_key_and_value_and_copies_with_size_query)
{
    // Arrange
    com_util_hashtable_config config = variable_config(64, 64);
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *attached = nullptr;
    const void *mgmt = nullptr;
    const void *data = nullptr;
    const void *found = nullptr;
    size_t mgmt_size = 0;
    size_t data_size = 0;
    size_t required_size = 0;
    char copied[32] = {};

    // Pre-Assert

    // Act
    int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_add =
        com_util_hashtable_add(ht, "variable-key", "variable-value", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_find = com_util_hashtable_find_value_ref(ht, "variable-key", &found);
    int actual_ret_query = com_util_hashtable_find_value_copy(ht, "variable-key", NULL, 0, &required_size);
    int actual_ret_copy =
        com_util_hashtable_find_value_copy(ht, "variable-key", copied, sizeof(copied), &required_size);
    (void)com_util_hashtable_buffer_size(ht, &mgmt_size, &data_size);
    (void)com_util_hashtable_buffer_ref(ht, &mgmt, &data);
    std::vector<unsigned char> mgmt_copy(static_cast<const unsigned char *>(mgmt),
                                         static_cast<const unsigned char *>(mgmt) + mgmt_size);
    std::vector<unsigned char> data_copy(static_cast<const unsigned char *>(data),
                                         static_cast<const unsigned char *>(data) + data_size);
    int actual_ret_attach =
        com_util_hashtable_attach(mgmt_copy.data(), mgmt_copy.size(), data_copy.data(), data_copy.size(), &attached);
    const void *attached_value = nullptr;
    int actual_ret_attached_find = com_util_hashtable_find_value_ref(attached, "variable-key", &attached_value);
    std::string found_text = static_cast<const char *>(found);
    std::string attached_text = static_cast<const char *>(attached_value);
    com_util_hashtable_dispose(attached);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);
    EXPECT_EQ("variable-value", found_text);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_query);
    EXPECT_EQ(std::strlen("variable-value") + 1u, required_size);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_copy);
    EXPECT_STREQ("variable-value", copied);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attach);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attached_find);
    EXPECT_EQ("variable-value", attached_text);
}

TEST_F(hashtableVariableStringTest, fragmented_update_returns_storage_full_and_preserves_value)
{
    // Arrange
    com_util_hashtable_config config = variable_config(32, 12);
    com_util_hashtable *ht = nullptr;
    const void *found = nullptr;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_add_a = com_util_hashtable_add(ht, "a", "1111", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_add_b = com_util_hashtable_add(ht, "b", "2222", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_update = com_util_hashtable_update(ht, "a", "123456");
    int actual_ret_find = com_util_hashtable_find_value_ref(ht, "a", &found);
    int actual_ret_validate = com_util_hashtable_validate(ht);
    std::string found_text = static_cast<const char *>(found);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add_a);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add_b);
    EXPECT_EQ(COM_UTIL_ERR_STORAGE_FULL, actual_ret_update);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);
    EXPECT_EQ("1111", found_text);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate);
}

TEST_F(hashtableVariableStringTest, explicit_compaction_enables_fragmented_add_and_invalidates_moved_reference)
{
    // Arrange
    com_util_hashtable_config config = variable_config(32, 20);
    com_util_hashtable *ht = nullptr;
    const void *before_compact = nullptr;
    const void *after_compact = nullptr;
    const void *key_before_compact = nullptr;
    const void *key_after_compact = nullptr;
    const void *added = nullptr;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", "1111", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "b", "2222", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "c", "3333", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_find_value_ref(ht, "c", &before_compact);
    uint64_t c_record = 0;
    (void)com_util_hashtable_find_recno(ht, "c", &c_record);
    (void)com_util_hashtable_get_key_ref(ht, c_record, &key_before_compact);
    (void)com_util_hashtable_delete(ht, "b");
    (void)com_util_hashtable_purge_deleted(ht);
    int actual_ret_fragmented =
        com_util_hashtable_add(ht, "d", "55555", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    const void *after_failed_add = nullptr;
    (void)com_util_hashtable_find_value_ref(ht, "c", &after_failed_add);
    int actual_ret_compact = com_util_hashtable_compact(ht);
    (void)com_util_hashtable_find_value_ref(ht, "c", &after_compact);
    (void)com_util_hashtable_get_key_ref(ht, c_record, &key_after_compact);
    bool vacated_key_zeroed = static_cast<const unsigned char *>(key_before_compact)[0] == 0 &&
                              static_cast<const unsigned char *>(key_before_compact)[1] == 0;
    bool vacated_value_zeroed = true;
    for (size_t i = 0; i < std::strlen("3333") + 1u; i++)
    {
        vacated_value_zeroed = vacated_value_zeroed && (static_cast<const unsigned char *>(before_compact)[i] == 0);
    }
    int actual_ret_retry = com_util_hashtable_add(ht, "d", "55555", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_find = com_util_hashtable_find_value_ref(ht, "d", &added);
    int actual_ret_validate = com_util_hashtable_validate(ht);
    std::string compacted_text = static_cast<const char *>(after_compact);
    std::string added_text = static_cast<const char *>(added);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_STORAGE_FULL, actual_ret_fragmented);
    EXPECT_EQ(before_compact, after_failed_add);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_compact);
    EXPECT_NE(before_compact, after_compact);
    EXPECT_NE(key_before_compact, key_after_compact);
    EXPECT_TRUE(vacated_key_zeroed);
    EXPECT_TRUE(vacated_value_zeroed);
    EXPECT_EQ("3333", compacted_text);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_retry);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);
    EXPECT_EQ("55555", added_text);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate);
}

TEST_F(hashtableVariableStringTest, purge_zero_fills_released_variable_key_and_value)
{
    // Arrange
    com_util_hashtable_config config = variable_config(32, 32);
    com_util_hashtable *ht = nullptr;
    const void *key_ref = nullptr;
    const void *value_ref = nullptr;
    uint64_t record = 0;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "secret-key", "secret-value", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_find_recno(ht, "secret-key", &record);
    (void)com_util_hashtable_get_key_ref(ht, record, &key_ref);
    (void)com_util_hashtable_get_value_ref(ht, record, &value_ref);
    (void)com_util_hashtable_delete(ht, "secret-key");
    bool retained_while_deleted = std::strcmp(static_cast<const char *>(key_ref), "secret-key") == 0 &&
                                  std::strcmp(static_cast<const char *>(value_ref), "secret-value") == 0;
    int actual_ret_purge = com_util_hashtable_purge_deleted(ht);
    bool key_zeroed = true;
    bool value_zeroed = true;
    for (size_t i = 0; i < std::strlen("secret-key") + 1u; i++)
    {
        key_zeroed = key_zeroed && (static_cast<const unsigned char *>(key_ref)[i] == 0);
    }
    for (size_t i = 0; i < std::strlen("secret-value") + 1u; i++)
    {
        value_zeroed = value_zeroed && (static_cast<const unsigned char *>(value_ref)[i] == 0);
    }
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_TRUE(retained_while_deleted);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_purge);
    EXPECT_TRUE(key_zeroed);
    EXPECT_TRUE(value_zeroed);
}

TEST_F(hashtableVariableStringTest, lifetime_expiration_zero_fills_released_variable_storage)
{
    // Arrange
    com_util_hashtable_config config = variable_config(32, 32);
    com_util_hashtable *ht = nullptr;
    const void *key_ref = nullptr;
    const void *value_ref = nullptr;
    uint64_t record = 0;

    config.lifetime = 3;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "expired", "private", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_find_recno(ht, "expired", &record);
    (void)com_util_hashtable_get_key_ref(ht, record, &key_ref);
    (void)com_util_hashtable_get_value_ref(ht, record, &value_ref);
    (void)com_util_hashtable_delete(ht, "expired");
    int actual_ret_push = com_util_hashtable_push_deleted(ht);
    bool key_zeroed = true;
    bool value_zeroed = true;
    for (size_t i = 0; i < std::strlen("expired") + 1u; i++)
    {
        key_zeroed = key_zeroed && (static_cast<const unsigned char *>(key_ref)[i] == 0);
    }
    for (size_t i = 0; i < std::strlen("private") + 1u; i++)
    {
        value_zeroed = value_zeroed && (static_cast<const unsigned char *>(value_ref)[i] == 0);
    }
    int status = -1;
    (void)com_util_hashtable_get_status(ht, record, &status);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_push);
    EXPECT_EQ(0, status);
    EXPECT_TRUE(key_zeroed);
    EXPECT_TRUE(value_zeroed);
}

TEST_F(hashtableVariableStringTest, compact_accepts_null_and_is_noop_for_fixed_fields)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *before = nullptr;
    const void *after = nullptr;

    config.capacity = 1;
    config.key_type = COM_UTIL_HASHTABLE_FIELD_FIXED_STRING;
    config.value_type = COM_UTIL_HASHTABLE_FIELD_FIXED_STRING;
    config.key_size = 8;
    config.value_size = 8;
    config.lifetime = 5;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "key", "value", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_find_value_ref(ht, "key", &before);
    int actual_ret_null = com_util_hashtable_compact(NULL);
    int actual_ret_compact = com_util_hashtable_compact(ht);
    (void)com_util_hashtable_find_value_ref(ht, "key", &after);
    std::string after_text = static_cast<const char *>(after);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_null);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_compact);
    EXPECT_EQ(before, after);
    EXPECT_EQ("value", after_text);
}

TEST_F(hashtableVariableStringTest, supports_variable_key_with_fixed_binary_value)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const unsigned char value[] = {0, 1, 2, 3};
    unsigned char copied[sizeof(value)] = {};
    size_t required_size = 0;

    config.capacity = 2;
    config.key_type = COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING;
    config.value_type = COM_UTIL_HASHTABLE_FIELD_FIXED_BINARY;
    config.key_storage_size = 32;
    config.value_size = sizeof(value);
    config.lifetime = 5;

    // Pre-Assert

    // Act
    int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_add = com_util_hashtable_add(ht, "key", value, COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_copy = com_util_hashtable_find_value_copy(ht, "key", copied, sizeof(copied), &required_size);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_copy);
    EXPECT_EQ(sizeof(value), required_size);
    EXPECT_EQ(0, std::memcmp(value, copied, sizeof(value)));
}

TEST_F(hashtableVariableStringTest, fixed_strings_accept_literals_and_zero_fill_unused_storage)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *key_ref = nullptr;
    const void *value_ref = nullptr;
    char key_copy[8] = {};
    char value_copy[8] = {};
    unsigned char stored_key[8] = {};
    unsigned char stored_value[8] = {};
    size_t key_required_size = 0;
    size_t value_required_size = 0;
    uint64_t record = 0;
    const unsigned char expected_key[8] = {'k', 0, 0, 0, 0, 0, 0, 0};
    const unsigned char expected_value[8] = {'n', 'e', 'w', 0, 0, 0, 0, 0};

    config.capacity = 2;
    config.key_type = COM_UTIL_HASHTABLE_FIELD_FIXED_STRING;
    config.value_type = COM_UTIL_HASHTABLE_FIELD_FIXED_STRING;
    config.key_size = sizeof(key_copy);
    config.value_size = sizeof(value_copy);
    config.lifetime = 5;

    // Pre-Assert

    // Act
    int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_add = com_util_hashtable_add(ht, "k", "old", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_update = com_util_hashtable_update(ht, "k", "new");
    (void)com_util_hashtable_find_recno(ht, "k", &record);
    (void)com_util_hashtable_get_key_ref(ht, record, &key_ref);
    (void)com_util_hashtable_get_value_ref(ht, record, &value_ref);
    int actual_ret_key_copy =
        com_util_hashtable_get_key_copy(ht, record, key_copy, sizeof(key_copy), &key_required_size);
    int actual_ret_value_copy =
        com_util_hashtable_get_value_copy(ht, record, value_copy, sizeof(value_copy), &value_required_size);
    std::memcpy(stored_key, key_ref, sizeof(stored_key));
    std::memcpy(stored_value, value_ref, sizeof(stored_value));
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_update);
    EXPECT_EQ(0, std::memcmp(expected_key, stored_key, sizeof(expected_key)));
    EXPECT_EQ(0, std::memcmp(expected_value, stored_value, sizeof(expected_value)));
    EXPECT_EQ(COM_UTIL_OK, actual_ret_key_copy);
    EXPECT_EQ(2u, key_required_size);
    EXPECT_STREQ("k", key_copy);
    EXPECT_EQ(COM_UTIL_OK, actual_ret_value_copy);
    EXPECT_EQ(4u, value_required_size);
    EXPECT_STREQ("new", value_copy);
}

TEST_F(hashtableVariableStringTest, supports_all_key_and_value_field_type_combinations)
{
    const com_util_hashtable_field_type field_types[] = {COM_UTIL_HASHTABLE_FIELD_FIXED_BINARY,
                                                         COM_UTIL_HASHTABLE_FIELD_FIXED_STRING,
                                                         COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING};
    const unsigned char binary_key[] = {'k', 'e', 'y', 0};
    const unsigned char binary_value[] = {'v', 'a', 'l', 0};

    for (com_util_hashtable_field_type key_type : field_types)
    {
        for (com_util_hashtable_field_type value_type : field_types)
        {
            // Arrange
            com_util_hashtable_config config = {};
            com_util_hashtable *ht = nullptr;
            unsigned char copied[16] = {};
            size_t required_size = 0;
            const void *key = key_type == COM_UTIL_HASHTABLE_FIELD_FIXED_BINARY ? static_cast<const void *>(binary_key)
                                                                                : static_cast<const void *>("key");
            const void *value = value_type == COM_UTIL_HASHTABLE_FIELD_FIXED_BINARY
                                    ? static_cast<const void *>(binary_value)
                                    : static_cast<const void *>("val");

            config.capacity = 2;
            config.key_type = key_type;
            config.value_type = value_type;
            config.key_size = key_type == COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING ? 0 : sizeof(binary_key);
            config.value_size = value_type == COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING ? 0 : sizeof(binary_value);
            config.key_storage_size = key_type == COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING ? 16 : 0;
            config.value_storage_size = value_type == COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING ? 16 : 0;
            config.lifetime = 5;

            // Pre-Assert

            // Act
            int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
            int actual_ret_add = com_util_hashtable_add(ht, key, value, COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
            int actual_ret_query = com_util_hashtable_find_value_copy(ht, key, NULL, 0, &required_size);
            int actual_ret_too_small =
                com_util_hashtable_find_value_copy(ht, key, copied, required_size - 1, &required_size);
            bool too_small_unchanged = true;
            for (unsigned char byte : copied)
            {
                too_small_unchanged = too_small_unchanged && (byte == 0);
            }
            int actual_ret_copy = com_util_hashtable_find_value_copy(ht, key, copied, sizeof(copied), &required_size);
            com_util_hashtable_dispose(ht);

            // Assert
            EXPECT_EQ(COM_UTIL_OK, actual_ret_create);
            EXPECT_EQ(COM_UTIL_OK, actual_ret_add);
            EXPECT_EQ(COM_UTIL_OK, actual_ret_query);
            EXPECT_EQ(sizeof(binary_value), required_size);
            EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL, actual_ret_too_small);
            EXPECT_TRUE(too_small_unchanged);
            EXPECT_EQ(COM_UTIL_OK, actual_ret_copy);
            EXPECT_EQ(0, std::memcmp(binary_value, copied, sizeof(binary_value)));
        }
    }
}

/*
 *  以下は可変長ストレージの配置に関する特性化テストです。
 *  先着適合の探索順と圧縮の詰め直し順を、オフセットの実測値で固定します。
 *  ストレージ アロケーターの内部実装を差し替えても、配置が変わらないことを保証します。
 */

static com_util_hashtable_config variable_config_with_capacity(size_t capacity, size_t key_storage_size,
                                                               size_t value_storage_size)
{
    com_util_hashtable_config config = {};

    config.capacity = capacity;
    config.key_type = COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING;
    config.value_type = COM_UTIL_HASHTABLE_FIELD_VARIABLE_STRING;
    config.key_storage_size = key_storage_size;
    config.value_storage_size = value_storage_size;
    config.lifetime = 5;
    return config;
}

/**
 *  可変長ストレージの先頭は、キーが管理領域の末尾、値がデータ領域の末尾に置かれます。
 *  この位置関係は公開 API の buffer_ref と buffer_size から算出できます。
 */
class storage_origin
{
  public:
    storage_origin(const com_util_hashtable *ht, const com_util_hashtable_config &config)
    {
        const void *mgmt = nullptr;
        const void *data = nullptr;
        size_t mgmt_size = 0;
        size_t data_size = 0;

        (void)com_util_hashtable_buffer_ref(ht, &mgmt, &data);
        (void)com_util_hashtable_buffer_size(ht, &mgmt_size, &data_size);
        key_base_ = static_cast<const unsigned char *>(mgmt) + mgmt_size - config.key_storage_size;
        value_base_ = static_cast<const unsigned char *>(data) + data_size - config.value_storage_size;
    }

    /** キーが見つからない場合は -1 を返します。 */
    long key_offset(const com_util_hashtable *ht, const char *key) const
    {
        const void *ref = nullptr;
        uint64_t record = 0;

        if (com_util_hashtable_find_recno(ht, key, &record) != COM_UTIL_OK)
        {
            return -1;
        }
        if (com_util_hashtable_get_key_ref(ht, record, &ref) != COM_UTIL_OK)
        {
            return -1;
        }
        return static_cast<long>(static_cast<const unsigned char *>(ref) - key_base_);
    }

    /** キーが見つからない場合は -1 を返します。 */
    long value_offset(const com_util_hashtable *ht, const char *key) const
    {
        const void *ref = nullptr;

        if (com_util_hashtable_find_value_ref(ht, key, &ref) != COM_UTIL_OK)
        {
            return -1;
        }
        return static_cast<long>(static_cast<const unsigned char *>(ref) - value_base_);
    }

  private:
    const unsigned char *key_base_;
    const unsigned char *value_base_;
};

TEST_F(hashtableVariableStringTest, first_fit_placement_is_stable_across_add_purge_and_update)
{
    // Arrange
    com_util_hashtable_config config =
        variable_config_with_capacity(8, 64, 32); // [状態] - キー 64 バイト、値 32 バイトの可変長ストレージを用意する。
    com_util_hashtable *ht = nullptr;
    std::vector<long> actual_value_offsets;
    std::vector<long> actual_key_offsets;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    storage_origin origin(ht, config);
    (void)com_util_hashtable_add(ht, "k1", "aaaa", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k2", "bbbb", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k3", "cccc", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k4", "dddd",
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - 5 バイト値を 4 件詰める。
    (void)com_util_hashtable_delete(ht, "k2");
    (void)com_util_hashtable_purge_deleted(ht); // [手順] - 2 件目を回収し、途中に穴を作る。
    int actual_ret_reuse_hole = com_util_hashtable_add(ht, "k5", "ee",
                                                       COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    long actual_offset_reuse_hole = origin.value_offset(ht, "k5"); // [手順] - 穴に収まる 3 バイト値を追加する。
    int actual_ret_skip_hole = com_util_hashtable_add(ht, "k6", "ffffff",
                                                      COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    long actual_offset_skip_hole = origin.value_offset(ht, "k6"); // [手順] - 穴に収まらない 7 バイト値を追加する。
    (void)com_util_hashtable_add(ht, "k7", "gg", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_shrink = com_util_hashtable_update(ht, "k1", "hh");
    long actual_offset_shrink = origin.value_offset(ht, "k1"); // [手順] - 先頭の値を短い値へ更新する。
    int actual_ret_merge_own = com_util_hashtable_update(ht, "k3", "iiiiii");
    long actual_offset_merge_own = origin.value_offset(ht, "k3"); // [手順] - 自ブロックと隣接する穴の結合が要る更新を行う。
    for (const char *key : {"k1", "k3", "k4", "k5", "k6", "k7"})
    {
        actual_value_offsets.push_back(origin.value_offset(ht, key));
        actual_key_offsets.push_back(origin.key_offset(ht, key));
    }
    int actual_ret_validate = com_util_hashtable_validate(ht);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_reuse_hole); // [確認_正常系] - 穴に収まる add が成功すること。
    EXPECT_EQ(5, actual_offset_reuse_hole); // [確認_正常系] - 穴に収まる add が、回収済みの穴の先頭へ配置されること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_skip_hole); // [確認_正常系] - 穴に収まらない add が成功すること。
    EXPECT_EQ(20, actual_offset_skip_hole); // [確認_正常系] - 穴に収まらない add が、小さすぎる穴を読み飛ばして末尾側へ配置されること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_shrink); // [確認_正常系] - 短い値への update が成功すること。
    EXPECT_EQ(0, actual_offset_shrink); // [確認_正常系] - 短い値への update が、自ブロックの先頭を維持すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_merge_own); // [確認_正常系] - 自ブロックと隣接穴の結合が要る update が成功すること。
    EXPECT_EQ(8, actual_offset_merge_own); // [確認_正常系] - 当該 update が、直前の穴と自ブロックを結合した位置へ配置されること。
    EXPECT_EQ(std::vector<long>({0, 8, 15, 5, 20, 27}),
              actual_value_offsets); // [確認_正常系] - 一連の操作後の値オフセットが記録どおりであること。
    EXPECT_EQ(std::vector<long>({0, 6, 9, 3, 12, 15}),
              actual_key_offsets); // [確認_正常系] - 一連の操作後のキー オフセットが記録どおりであること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate); // [確認_正常系] - 一連の操作後の validate が成功すること。
}

TEST_F(hashtableVariableStringTest, compaction_packs_blocks_in_offset_order_and_frees_the_tail)
{
    // Arrange
    com_util_hashtable_config config =
        variable_config_with_capacity(8, 64, 32); // [状態] - キー 64 バイト、値 32 バイトの可変長ストレージを用意する。
    com_util_hashtable *ht = nullptr;
    std::vector<long> actual_value_offsets;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    storage_origin origin(ht, config);
    (void)com_util_hashtable_add(ht, "k1", "aaaa", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k2", "bbbb", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k3", "cccc", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k4", "dddd", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_delete(ht, "k2");
    (void)com_util_hashtable_purge_deleted(ht);
    (void)com_util_hashtable_add(ht, "k5", "ee", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k6", "ffffff", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k7", "gg", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_update(ht, "k1", "hh");
    (void)com_util_hashtable_update(ht, "k3", "iiiiii"); // [手順] - 穴が残る状態を作る。
    int actual_ret_compact = com_util_hashtable_compact(ht); // [手順] - 明示的に圧縮する。
    for (const char *key : {"k1", "k5", "k3", "k4", "k6", "k7"})
    {
        actual_value_offsets.push_back(origin.value_offset(ht, key));
    }
    int actual_ret_add_after_compact =
        com_util_hashtable_add(ht, "k8", "jjj",
                               COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - 圧縮で空いた末尾へ追加する。
    long actual_offset_after_compact = origin.value_offset(ht, "k8");
    int actual_ret_validate = com_util_hashtable_validate(ht);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_compact); // [確認_正常系] - compact が成功すること。
    EXPECT_EQ(std::vector<long>({0, 3, 6, 13, 18, 25}),
              actual_value_offsets); // [確認_正常系] - compact がオフセット順に隙間なく詰め直すこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add_after_compact); // [確認_正常系] - 圧縮後の add が成功すること。
    EXPECT_EQ(28, actual_offset_after_compact); // [確認_正常系] - 圧縮後の add が、詰め直した末尾へ配置されること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate); // [確認_正常系] - 圧縮と追加の後の validate が成功すること。
}

TEST_F(hashtableVariableStringTest, exact_fit_consumes_the_last_hole_and_the_next_add_reports_storage_full)
{
    // Arrange
    com_util_hashtable_config config =
        variable_config_with_capacity(8, 64, 16); // [状態] - 値ストレージを 16 バイトに絞る。
    com_util_hashtable *ht = nullptr;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    storage_origin origin(ht, config);
    (void)com_util_hashtable_add(ht, "k1", "aaaaaaa", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_exact_fit = com_util_hashtable_add(ht, "k2", "bbbbbbb",
                                                      COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    long actual_offset_exact_fit = origin.value_offset(ht, "k2"); // [手順] - 残り 8 バイトへ 8 バイト値を追加する。
    int actual_ret_full = com_util_hashtable_add(ht, "k3", "c",
                                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - さらに追加する。
    int actual_ret_validate = com_util_hashtable_validate(ht);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_exact_fit); // [確認_正常系] - 残り容量ちょうどの add が成功すること。
    EXPECT_EQ(8, actual_offset_exact_fit); // [確認_正常系] - 残り容量ちょうどの add が、末尾の穴の先頭へ配置されること。
    EXPECT_EQ(COM_UTIL_ERR_STORAGE_FULL, actual_ret_full); // [確認_異常系] - 空きが無くなった後の add が STORAGE_FULL であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate); // [確認_正常系] - 満杯状態の validate が成功すること。
}

TEST_F(hashtableVariableStringTest, resize_repacks_variable_storage_and_moves_references)
{
    // Arrange
    com_util_hashtable_config config =
        variable_config_with_capacity(4, 64, 32); // [状態] - キー 64 バイト、値 32 バイトの可変長ストレージを用意する。
    com_util_hashtable_config grown =
        variable_config_with_capacity(8, 64, 32); // [状態] - capacity だけを 8 へ広げた設定を用意する。
    com_util_hashtable *ht = nullptr;
    const void *before_resize = nullptr;
    const void *after_resize = nullptr;
    std::vector<long> actual_offsets_before;
    std::vector<long> actual_offsets_after;

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(ht, "k1", "aaaa", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k2", "bbbb", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, "k3", "cccc", COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_delete(ht, "k2");
    (void)com_util_hashtable_purge_deleted(ht); // [手順] - 中間の 1 件を回収し、ストレージの途中に穴を作る。
    {
        storage_origin origin(ht, config);

        for (const char *key : {"k1", "k3"})
        {
            actual_offsets_before.push_back(origin.value_offset(ht, key));
        }
        (void)com_util_hashtable_find_value_ref(ht, "k3", &before_resize);
    }
    int actual_ret_resize = com_util_hashtable_resize(ht, &grown); // [手順] - capacity を広げる。
    {
        storage_origin origin(ht, grown);

        for (const char *key : {"k1", "k3"})
        {
            actual_offsets_after.push_back(origin.value_offset(ht, key));
        }
        (void)com_util_hashtable_find_value_ref(ht, "k3", &after_resize);
    }
    std::string moved_text = static_cast<const char *>(after_resize);
    int actual_ret_validate = com_util_hashtable_validate(ht);
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(std::vector<long>({0, 10}),
              actual_offsets_before); // [確認_正常系] - resize 前は回収済みの穴がそのまま残ること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_resize); // [確認_正常系] - resize が成功すること。
    EXPECT_EQ(std::vector<long>({0, 5}),
              actual_offsets_after); // [確認_正常系] - resize が残すレコードをレコード番号順に隙間なく詰め直すこと。
    EXPECT_NE(before_resize, after_resize); // [確認_正常系] - resize が取得済みの可変長参照を移動させること。
    EXPECT_EQ("cccc", moved_text); // [確認_正常系] - 詰め直した後も値の内容が変わらないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate); // [確認_正常系] - resize 後の validate が成功すること。
}
