#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

#include <cstring>
#include <string>
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

class hashtableLifetimeInfiniteTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableLifetimeInfiniteTest, create_accepts_infinite_lifetime)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    const com_util_hashtable_config *got = nullptr;

    fill_config(&config, 2, 8, 8, COM_UTIL_HASHTABLE_LIFETIME_INFINITE,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 255 の設定を用意する。

    // Pre-Assert

    // Act
    int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 寿命無限で構築する。
    int actual_ret_config = com_util_hashtable_get_config_ref(ht, &got);               // [手順] - 設定を読む。
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create); // [確認_正常系] - lifetime 255 で構築できること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_config); // [確認_正常系] - 設定参照が成功すること。
    ASSERT_NE(nullptr, got);                   // [確認_正常系] - 設定ポインターが非 NULL であること。
    EXPECT_EQ(COM_UTIL_HASHTABLE_LIFETIME_INFINITE, got->lifetime); // [確認_正常系] - lifetime が 255 であること。
}

TEST_F(hashtableLifetimeInfiniteTest, push_stops_at_terminal_status)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 1);
    int status_after_age = -1;
    int status_at_terminal = -1;
    int status_after_hold = -1;
    size_t empty = 99;
    size_t deleted = 0;
    const void *found = nullptr;
    int i = 0;

    fill_config(&config, 2, 16, 8, COM_UTIL_HASHTABLE_LIFETIME_INFINITE,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 寿命無限の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "keep", value.data());
    int actual_ret_delete = com_util_hashtable_delete(ht, "keep"); // [手順] - キーを削除して status 2 にする。
    for (i = 0; i < (COM_UTIL_HASHTABLE_LIFETIME_INFINITE - 3); ++i)
    {
        (void)com_util_hashtable_push_deleted(ht); // [手順] - status が 254 になるまで加齢する。
    }
    (void)com_util_hashtable_get_status(ht, 1, &status_after_age);
    int actual_ret_to_terminal = com_util_hashtable_push_deleted(ht); // [手順] - 254 から 255 へ加齢する。
    (void)com_util_hashtable_get_status(ht, 1, &status_at_terminal);
    int actual_ret_hold1 = com_util_hashtable_push_deleted(ht); // [手順] - 255 のままさらに加齢する。
    int actual_ret_hold2 = com_util_hashtable_push_deleted(ht);
    (void)com_util_hashtable_get_status(ht, 1, &status_after_hold);
    (void)com_util_hashtable_empty_count(ht, &empty);
    (void)com_util_hashtable_deleted_count(ht, &deleted);
    int actual_ret_find = com_util_hashtable_find_value_ref(ht, "keep", &found);
    int actual_ret_validate = com_util_hashtable_validate(ht);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete);      // [確認_正常系] - delete が成功すること。
    EXPECT_EQ(254, status_after_age);               // [確認_正常系] - 252 回の push 後に status が 254 であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_to_terminal); // [確認_正常系] - 254 からの push が成功すること。
    EXPECT_EQ(COM_UTIL_HASHTABLE_LIFETIME_INFINITE,
              status_at_terminal);            // [確認_正常系] - 次の status が 255 であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_hold1); // [確認_正常系] - 終端後の push が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_hold2); // [確認_正常系] - 終端後の再 push が成功すること。
    EXPECT_EQ(COM_UTIL_HASHTABLE_LIFETIME_INFINITE, status_after_hold); // [確認_正常系] - その後も 255 を維持すること。
    EXPECT_EQ(1u, empty);                               // [確認_正常系] - 削除済みが空へ戻っていないこと。
    EXPECT_EQ(1u, deleted);                             // [確認_正常系] - 削除済みが 1 件残ること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_find); // [確認_正常系] - 終端の削除済みは検索対象にならないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate);        // [確認_正常系] - 終端の削除済みでも validate が成功すること。
}

TEST_F(hashtableLifetimeInfiniteTest, insert_direct_accepts_terminal_status)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 2);
    int status = -1;
    const void *found = nullptr;
    const void *key_out = nullptr;

    fill_config(&config, 2, 16, 8, COM_UTIL_HASHTABLE_LIFETIME_INFINITE,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 寿命無限の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_direct =
        com_util_hashtable_insert_direct(ht, 1, "gone", COM_UTIL_HASHTABLE_LIFETIME_INFINITE, value.data(),
                                         &k_insert_timestamp); // [手順] - status 255 を直接書く。
    int actual_ret_find = com_util_hashtable_find_value_ref(ht, "gone", &found);
    int actual_ret_status = com_util_hashtable_get_status(ht, 1, &status);
    int actual_ret_key = com_util_hashtable_get_key_ref(ht, 1, &key_out);
    int actual_ret_validate = com_util_hashtable_validate(ht);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_direct);               // [確認_正常系] - 寿命無限先へ status 255 を置けること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_find);      // [確認_正常系] - 終端の削除済みは検索対象にならないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_status);               // [確認_正常系] - get_status が成功すること。
    EXPECT_EQ(COM_UTIL_HASHTABLE_LIFETIME_INFINITE, status); // [確認_正常系] - status が 255 であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_key);                  // [確認_正常系] - キーを読めること。
    EXPECT_STREQ("gone", static_cast<const char *>(key_out)); // [確認_正常系] - キーが一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate);              // [確認_正常系] - validate が成功すること。
}

TEST_F(hashtableLifetimeInfiniteTest, insert_direct_skips_beyond_finite_max)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 3);
    int status = -1;

    fill_config(&config, 2, 16, 8, 254, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 有限の最大寿命 254 を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_eq = com_util_hashtable_insert_direct(
        ht, 1, "a", 254, value.data(), &k_insert_timestamp); // [手順] - status 254 を lifetime 254 へ置く。
    int actual_ret_over =
        com_util_hashtable_insert_direct(ht, 1, "a", COM_UTIL_HASHTABLE_LIFETIME_INFINITE, value.data(),
                                         &k_insert_timestamp); // [手順] - status 255 を lifetime 254 へ置く。
    (void)com_util_hashtable_get_status(ht, 1, &status);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_SKIPPED, actual_ret_eq); // [確認_正常系] - lifetime 254 では status 254 が SKIPPED であること。
    EXPECT_EQ(COM_UTIL_SKIPPED,
              actual_ret_over); // [確認_正常系] - lifetime 254 では status 255 が SKIPPED であること。
    EXPECT_EQ(0, status);       // [確認_正常系] - SKIPPED 後もスロットが空であること。
}

TEST_F(hashtableLifetimeInfiniteTest, add_reuses_and_purge_expires_terminal_status)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    const void *found = nullptr;
    int status_after_add = -1;
    int status_after_purge = -1;
    size_t empty = 0;

    fill_config(&config, 2, 16, 8, COM_UTIL_HASHTABLE_LIFETIME_INFINITE,
                COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 寿命無限の設定を用意する。
    std::memcpy(value.data(), "old", 4);

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_insert_direct(ht, 1, "reuse", COM_UTIL_HASHTABLE_LIFETIME_INFINITE, value.data(),
                                           &k_insert_timestamp);
    std::memcpy(value.data(), "new", 4);
    int actual_ret_add = com_util_hashtable_add(ht, "reuse", value.data()); // [手順] - 終端の削除済みキーを再追加する。
    int actual_ret_find = com_util_hashtable_find_value_ref(ht, "reuse", &found);
    std::string found_text = (found == nullptr) ? "" : static_cast<const char *>(found);
    (void)com_util_hashtable_get_status(ht, 1, &status_after_add);
    (void)com_util_hashtable_delete(ht, "reuse");
    (void)com_util_hashtable_insert_direct(ht, 2, "drop", COM_UTIL_HASHTABLE_LIFETIME_INFINITE, value.data(),
                                           &k_insert_timestamp);
    int actual_ret_purge = com_util_hashtable_purge_deleted(ht); // [手順] - 終端を含む削除済みを回収する。
    (void)com_util_hashtable_get_status(ht, 2, &status_after_purge);
    (void)com_util_hashtable_empty_count(ht, &empty);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);   // [確認_正常系] - 終端の削除済みキーを add で再利用できること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);  // [確認_正常系] - 再利用後にキーが見つかること。
    EXPECT_EQ("new", found_text);             // [確認_正常系] - 再利用後の値が new であること。
    EXPECT_EQ(1, status_after_add);           // [確認_正常系] - 再利用後の状態が実装中であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_purge); // [確認_正常系] - purge が成功すること。
    EXPECT_EQ(0, status_after_purge);         // [確認_正常系] - purge が status 255 も空へ戻すこと。
    EXPECT_GE(empty, 1u);                     // [確認_正常系] - 回収後に空スロットがあること。
}
