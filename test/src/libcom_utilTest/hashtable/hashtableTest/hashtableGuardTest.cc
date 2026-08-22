#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include <cstring>
#include <vector>

namespace
{

void fill_config(com_util_hashtable_config *config, size_t capacity, size_t key_size, size_t value_size,
                 unsigned char lifetime, com_util_hashtable_key_type key_type)
{
    *config = {};
    config->capacity = capacity;
    config->key_type = key_type;
    config->key_size = key_size;
    config->value_size = value_size;
    config->lifetime = lifetime;
}

/* hashtable.c の hash_key と同じ djb2。同一バケットの別キーを用意するために使う。 */
size_t hash_string_mod(const char *key, size_t capacity)
{
    uint64_t hash = 5381;
    const unsigned char *p = reinterpret_cast<const unsigned char *>(key);
    int c = 0;

    while ((c = *p++) != 0)
    {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    return static_cast<size_t>(hash) % capacity;
}

const char *find_colliding_key(const char *base, size_t capacity)
{
    static const char *const candidates[] = {"b", "c", "d", "e", "f", "g", "h", "i", "j", "k", nullptr};
    const size_t target = hash_string_mod(base, capacity);
    size_t i = 0;

    for (i = 0; candidates[i] != nullptr; ++i)
    {
        if ((std::strcmp(candidates[i], base) != 0) && (hash_string_mod(candidates[i], capacity) == target))
        {
            return candidates[i];
        }
    }
    return nullptr;
}

} // namespace

class hashtableGuardTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableGuardTest, add_rejects_null_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_add(
        NULL, "a", value.data(), COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - ht に NULL を渡す。
    int actual_ret_null_key = com_util_hashtable_add(
        ht, NULL, value.data(), COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - key に NULL を渡す。
    int actual_ret_null_value = com_util_hashtable_add(
        ht, "a", NULL, COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - value に NULL を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_key); // [確認_異常系] - NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_value); // [確認_異常系] - NULL value が INVALID_ARGUMENT であること。
}

TEST_F(hashtableGuardTest, update_rejects_invalid_arguments_and_walks_chain)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    char too_long[9];
    const char *peer = nullptr;

    fill_config(&config, 4, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 4 の設定を用意する。
    std::memset(too_long, 'x', sizeof(too_long));                    // [状態] - NUL が無いキーを用意する。
    peer = find_colliding_key("a", 4);                               // [状態] - "a" と同じバケットの別キーを探す。

    // Pre-Assert
    ASSERT_NE(nullptr, peer); // [Pre-Assert確認_正常系] - 同一バケットの別キーが見つかること。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_update(NULL, "a", value.data()); // [手順] - ht に NULL を渡す。
    int actual_ret_null_key = com_util_hashtable_update(ht, NULL, value.data()); // [手順] - key に NULL を渡す。
    int actual_ret_null_value = com_util_hashtable_update(ht, "a", NULL);        // [手順] - value に NULL を渡す。
    int actual_ret_too_long =
        com_util_hashtable_update(ht, too_long, value.data()); // [手順] - 長すぎるキーで更新する。
    (void)com_util_hashtable_add(ht, "a", value.data(),
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - "a" を先に追加する。
    (void)com_util_hashtable_add(
        ht, peer, value.data(),
        COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - 同一バケットの別キーを後から追加する。
    int actual_ret_walk =
        com_util_hashtable_update(ht, "a", value.data());       // [手順] - チェイン先頭ではないキーを更新する。
    int actual_ret_delete = com_util_hashtable_delete(ht, "a"); // [手順] - "a" を削除済みにする。
    int actual_ret_deleted = com_util_hashtable_update(ht, "a", value.data()); // [手順] - 削除済みキーを更新する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_key); // [確認_異常系] - NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_value); // [確認_異常系] - NULL value が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
              actual_ret_too_long);            // [確認_異常系] - 長すぎるキーが OUT_OF_RANGE であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_walk);   // [確認_正常系] - チェインを辿って更新できること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete); // [確認_正常系] - delete が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_deleted); // [確認_異常系] - 削除済みキーの更新が NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, update_rec_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_update_rec(NULL, 1, value.data()); // [手順] - ht に NULL を渡す。
    int actual_ret_null_value = com_util_hashtable_update_rec(ht, 1, NULL);        // [手順] - value に NULL を渡す。
    int actual_ret_rec0 = com_util_hashtable_update_rec(ht, 0, value.data());      // [手順] - レコード番号 0 を渡す。
    int actual_ret_rec_hi =
        com_util_hashtable_update_rec(ht, 3, value.data()); // [手順] - capacity 超のレコード番号を渡す。
    int actual_ret_empty = com_util_hashtable_update_rec(ht, 1, value.data()); // [手順] - 空きレコードを更新する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_value); // [確認_異常系] - NULL value が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec0); // [確認_異常系] - レコード番号 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec_hi);                        // [確認_異常系] - capacity 超が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_empty); // [確認_異常系] - 空きレコードの更新が NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, find_value_ref_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *found = nullptr;
    char too_long[9];

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    std::memset(too_long, 'x', sizeof(too_long));                    // [状態] - NUL が無いキーを用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_find_value_ref(NULL, "a", &found); // [手順] - ht に NULL を渡す。
    int actual_ret_null_key = com_util_hashtable_find_value_ref(ht, NULL, &found); // [手順] - key に NULL を渡す。
    int actual_ret_null_out = com_util_hashtable_find_value_ref(ht, "a", NULL); // [手順] - value_out に NULL を渡す。
    int actual_ret_too_long =
        com_util_hashtable_find_value_ref(ht, too_long, &found); // [手順] - 長すぎるキーで検索する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_key); // [確認_異常系] - NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL value_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
              actual_ret_too_long); // [確認_異常系] - 長すぎるキーが OUT_OF_RANGE であること。
}

TEST_F(hashtableGuardTest, find_value_copy_rejects_invalid_arguments_and_succeeds)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    std::vector<unsigned char> copied(8, 0);
    size_t required_size = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    std::memcpy(value.data(), "v", 2);                               // [状態] - 検索する値を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data(),
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - キーを追加する。
    int actual_ret_null_size = com_util_hashtable_find_value_copy(ht, "a", copied.data(), copied.size(),
                                                                  NULL); // [手順] - required_size_out に NULL を渡す。
    int actual_ret_bad_pair =
        com_util_hashtable_find_value_copy(ht, "a", NULL, 1, &required_size); // [手順] - 不正な照会指定を渡す。
    int actual_ret_not_found = com_util_hashtable_find_value_copy(
        ht, "missing", copied.data(), copied.size(), &required_size); // [手順] - 存在しないキーで検索する。
    int actual_ret_ok = com_util_hashtable_find_value_copy(ht, "a", copied.data(), copied.size(),
                                                           &required_size); // [手順] - 妥当な引数で検索する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_size); // [確認_異常系] - NULL required_size_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_bad_pair); // [確認_異常系] - 不正な照会指定が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_not_found);       // [確認_異常系] - find_value_ref の失敗が NOT_FOUND として伝播すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ok); // [確認_正常系] - 妥当な引数で複製できること。
    EXPECT_STREQ("v", reinterpret_cast<char *>(copied.data())); // [確認_正常系] - 複製内容が一致すること。
}

TEST_F(hashtableGuardTest, find_recno_rejects_invalid_arguments_and_walks_chain)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    char too_long[9];
    uint64_t rec = 0;
    const char *peer = nullptr;

    fill_config(&config, 4, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 4 の設定を用意する。
    std::memset(too_long, 'x', sizeof(too_long));                    // [状態] - NUL が無いキーを用意する。
    peer = find_colliding_key("a", 4);                               // [状態] - "a" と同じバケットの別キーを探す。

    // Pre-Assert
    ASSERT_NE(nullptr, peer); // [Pre-Assert確認_正常系] - 同一バケットの別キーが見つかること。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_find_recno(NULL, "a", &rec);     // [手順] - ht に NULL を渡す。
    int actual_ret_null_key = com_util_hashtable_find_recno(ht, NULL, &rec);     // [手順] - key に NULL を渡す。
    int actual_ret_null_out = com_util_hashtable_find_recno(ht, "a", NULL);      // [手順] - record_out に NULL を渡す。
    int actual_ret_too_long = com_util_hashtable_find_recno(ht, too_long, &rec); // [手順] - 長すぎるキーで検索する。
    int actual_ret_absent = com_util_hashtable_find_recno(ht, "a", &rec); // [手順] - 何も無いテーブルで検索する。
    (void)com_util_hashtable_add(ht, "a", value.data(), COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    (void)com_util_hashtable_add(ht, peer, value.data(), COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_walk = com_util_hashtable_find_recno(ht, "a", &rec); // [手順] - チェイン先頭ではないキーを検索する。
    (void)com_util_hashtable_delete(ht, "a");
    int actual_ret_deleted = com_util_hashtable_find_recno(ht, "a", &rec); // [手順] - 削除済みキーを検索する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_key); // [確認_異常系] - NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL record_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
              actual_ret_too_long); // [確認_異常系] - 長すぎるキーが OUT_OF_RANGE であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_absent);            // [確認_異常系] - 何も無いテーブルでの検索が NOT_FOUND であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_walk); // [確認_正常系] - チェインを辿って見つかること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_deleted); // [確認_異常系] - 削除済みキーが NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, get_key_ref_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *key_out = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_get_key_ref(NULL, 1, &key_out); // [手順] - ht に NULL を渡す。
    int actual_ret_null_out = com_util_hashtable_get_key_ref(ht, 1, NULL);      // [手順] - key_out に NULL を渡す。
    int actual_ret_rec_hi =
        com_util_hashtable_get_key_ref(ht, 3, &key_out); // [手順] - capacity 超のレコード番号を渡す。
    int actual_ret_empty = com_util_hashtable_get_key_ref(ht, 1, &key_out); // [手順] - 空きレコードを読む。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL key_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec_hi); // [確認_異常系] - capacity 超が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_empty); // [確認_異常系] - 空きレコードの読み出しが NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, get_key_copy_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> key(8, 0);
    size_t required_size = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_size =
        com_util_hashtable_get_key_copy(ht, 1, key.data(), key.size(), NULL); // [手順] - 必要量出力に NULL を渡す。
    int actual_ret_propagated =
        com_util_hashtable_get_key_copy(ht, 0, key.data(), key.size(),
                                        &required_size); // [手順] - 不正な record で内部エラーを伝播させる。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_size); // [確認_異常系] - NULL required_size_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_propagated); // [確認_異常系] - get_key_ref の失敗が伝播すること。
}

TEST_F(hashtableGuardTest, get_value_ref_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const void *value_out = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_get_value_ref(NULL, 1, &value_out); // [手順] - ht に NULL を渡す。
    int actual_ret_null_out = com_util_hashtable_get_value_ref(ht, 1, NULL);   // [手順] - value_out に NULL を渡す。
    int actual_ret_rec0 = com_util_hashtable_get_value_ref(ht, 0, &value_out); // [手順] - レコード番号 0 を渡す。
    int actual_ret_rec_hi =
        com_util_hashtable_get_value_ref(ht, 3, &value_out); // [手順] - capacity 超のレコード番号を渡す。
    int actual_ret_empty = com_util_hashtable_get_value_ref(ht, 1, &value_out); // [手順] - 空きレコードを読む。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL value_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec0); // [確認_異常系] - レコード番号 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec_hi); // [確認_異常系] - capacity 超が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_empty); // [確認_異常系] - 空きレコードの読み出しが NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, get_value_copy_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    size_t required_size = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_size = com_util_hashtable_get_value_copy(ht, 1, value.data(), value.size(),
                                                                 NULL); // [手順] - required_size_out に NULL を渡す。
    int actual_ret_propagated =
        com_util_hashtable_get_value_copy(ht, 0, value.data(), value.size(),
                                          &required_size); // [手順] - 不正な record で内部エラーを伝播させる。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_size); // [確認_異常系] - NULL required_size_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_propagated); // [確認_異常系] - get_value_ref の失敗が伝播すること。
}

TEST_F(hashtableGuardTest, get_status_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    int status = -1;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_get_status(NULL, 1, &status); // [手順] - ht に NULL を渡す。
    int actual_ret_null_out = com_util_hashtable_get_status(ht, 1, NULL);     // [手順] - status_out に NULL を渡す。
    int actual_ret_rec0 = com_util_hashtable_get_status(ht, 0, &status);      // [手順] - レコード番号 0 を渡す。
    int actual_ret_rec_hi = com_util_hashtable_get_status(ht, 3, &status); // [手順] - capacity 超のレコード番号を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL status_out が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec0); // [確認_異常系] - レコード番号 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec_hi); // [確認_異常系] - capacity 超が INVALID_ARGUMENT であること。
}

TEST_F(hashtableGuardTest, count_status_rejects_null_handle)
{
    // Arrange
    size_t in_use = 0;

    // Pre-Assert

    // Act
    int actual_ret = com_util_hashtable_count_status(NULL, &in_use, NULL, NULL); // [手順] - ht に NULL を渡す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
}

TEST_F(hashtableGuardTest, count_wrappers_reject_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    size_t count = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_count_null_ht = com_util_hashtable_count(NULL, &count); // [手順] - count: ht に NULL を渡す。
    int actual_ret_count_null_out = com_util_hashtable_count(ht, NULL);    // [手順] - count: count に NULL を渡す。
    int actual_ret_deleted_null_ht =
        com_util_hashtable_deleted_count(NULL, &count); // [手順] - deleted_count: ht に NULL を渡す。
    int actual_ret_deleted_null_out =
        com_util_hashtable_deleted_count(ht, NULL); // [手順] - deleted_count: count に NULL を渡す。
    int actual_ret_empty_null_ht =
        com_util_hashtable_empty_count(NULL, &count); // [手順] - empty_count: ht に NULL を渡す。
    int actual_ret_empty_null_out =
        com_util_hashtable_empty_count(ht, NULL); // [手順] - empty_count: count に NULL を渡す。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_count_null_ht);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_count_null_out);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_deleted_null_ht);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_deleted_null_out);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_empty_null_ht);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret_empty_null_out);
}

TEST_F(hashtableGuardTest, delete_rejects_invalid_arguments_and_already_deleted)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    char too_long[9];

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 5 の設定を用意する。
    std::memset(too_long, 'x', sizeof(too_long));                    // [状態] - NUL が無いキーを用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_delete(NULL, "a");     // [手順] - ht に NULL を渡す。
    int actual_ret_null_key = com_util_hashtable_delete(ht, NULL);     // [手順] - key に NULL を渡す。
    int actual_ret_too_long = com_util_hashtable_delete(ht, too_long); // [手順] - 長すぎるキーで削除する。
    (void)com_util_hashtable_add(ht, "a", value.data(), COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);
    int actual_ret_first = com_util_hashtable_delete(ht, "a");  // [手順] - 1 回目の削除。
    int actual_ret_second = com_util_hashtable_delete(ht, "a"); // [手順] - 削除済みキーへの 2 回目の削除。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_key); // [確認_異常系] - NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
              actual_ret_too_long);           // [確認_異常系] - 長すぎるキーが OUT_OF_RANGE であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_first); // [確認_正常系] - 1 回目の削除が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_second); // [確認_異常系] - 削除済みキーへの再削除が NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, delete_rec_rejects_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_null_ht = com_util_hashtable_delete_rec(NULL, 1); // [手順] - ht に NULL を渡す。
    int actual_ret_rec0 = com_util_hashtable_delete_rec(ht, 0);      // [手順] - レコード番号 0 を渡す。
    int actual_ret_rec_hi = com_util_hashtable_delete_rec(ht, 3);    // [手順] - capacity 超のレコード番号を渡す。
    int actual_ret_empty = com_util_hashtable_delete_rec(ht, 1);     // [手順] - 空きレコードを削除する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec0); // [確認_異常系] - レコード番号 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_rec_hi);                        // [確認_異常系] - capacity 超が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_empty); // [確認_異常系] - 空きレコードの削除が NOT_FOUND であること。
}

TEST_F(hashtableGuardTest, lifecycle_wrappers_reject_null_handle)
{
    // Arrange

    // Pre-Assert

    // Act
    int actual_ret_push = com_util_hashtable_push_deleted(NULL);   // [手順] - push_deleted に NULL を渡す。
    int actual_ret_purge = com_util_hashtable_purge_deleted(NULL); // [手順] - purge_deleted に NULL を渡す。
    int actual_ret_clear = com_util_hashtable_clear(NULL);         // [手順] - clear に NULL を渡す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_push); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_purge); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_clear); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
}
