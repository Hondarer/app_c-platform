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

com_util_timespec k_insert_timestamp = {1, 0};

void fill_config(com_util_hashtable_config *config, size_t capacity, size_t key_size, size_t value_size,
                 unsigned char lifetime, com_util_hashtable_key_type key_type)
{
    *config = {};
    config->capacity = capacity;
    config->key_type = key_type;
    config->timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD;
    config->key_size = key_size;
    config->value_size = value_size;
    config->lifetime = lifetime;
}

/* hashtable.c の hash_key と同じ djb2。バケットが変わる/一致するキーを探すために使う。 */
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

class hashtableValidateTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtableValidateTest, rejects_null_handle)
{
    // Arrange

    // Pre-Assert

    // Act
    int actual_ret = com_util_hashtable_validate(NULL); // [手順] - NULL で validate を呼ぶ。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, actual_ret); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
}

TEST_F(hashtableValidateTest, detects_bucket_link_out_of_range)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 何も追加せずテーブルを構築する。
    test_hashtable_bucket_head(ht)[0] = 5;            // [手順] - バケット先頭リンクへ capacity 超の値を書く。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - capacity を超えるバケット リンクが CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_link_cycle)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    const char *peer = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。
    peer = find_colliding_key("a", 2);                               // [状態] - "a" と同じバケットへ落ちるキーを探す。

    // Pre-Assert
    ASSERT_NE(nullptr, peer); // [Pre-Assert確認_正常系] - 同一バケットの別キーが見つかること。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_insert_direct(ht, 1, "a", 1, value.data(), &k_insert_timestamp,
                                           1); // [手順] - レコード 1 を使用中にする。
    (void)com_util_hashtable_insert_direct(ht, 2, peer, 1, value.data(), &k_insert_timestamp,
                                           2); // [手順] - 同一バケットのレコード 2 を使用中にする。
    *test_hashtable_entry_next(ht, 0) = 2; // [手順] - レコード 1 の次リンクをレコード 2 へ向け、2 件の循環にする。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - チェインの循環が CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_duplicate_visit)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data(),
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - レコード 1 を使用中にする。
    test_hashtable_bucket_head(ht)[0] = 1;            // [手順] - バケット 0 からもレコード 1 を指させる。
    test_hashtable_bucket_head(ht)[1] = 1;            // [手順] - バケット 1 からもレコード 1 を指させる。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - 同じレコードへの二重リンクが CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_linked_empty_slot)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 何も追加せずテーブルを構築する。
    test_hashtable_bucket_head(ht)[0] = 1;            // [手順] - 空のレコード 1 をバケット 0 から直接リンクする。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - 空スロットへのリンクが CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_key_without_terminator)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - key_size 8 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(
        ht, "a", value.data(),
        COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE);        // [手順] - レコード 1 に文字列キーを格納する。
    std::memset(test_hashtable_entry_key(ht, 0), 'x', 8); // [手順] - 格納キーを NUL 無しで埋め尽くす。
    int actual_ret = com_util_hashtable_validate(ht);     // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - NUL の無いキーが CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_hash_mismatch)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    char mismatched_key[2] = {0, 0};
    size_t idx_a = 0;
    int i = 0;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。
    idx_a = hash_string_mod("a", 2);
    for (i = 0; i < 26; i++) // [状態] - "a" と異なるバケットへ落ちる 1 文字キーを探す。
    {
        char candidate_key[2] = {static_cast<char>('b' + i), 0};

        if (hash_string_mod(candidate_key, 2) != idx_a)
        {
            mismatched_key[0] = candidate_key[0];
            break;
        }
    }

    // Pre-Assert
    ASSERT_NE(0, mismatched_key[0]); // [Pre-Assert確認_正常系] - 異なるバケットへ落ちるキーが見つかること。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data(),
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - レコード 1 に "a" を格納する。
    std::memset(test_hashtable_entry_key(ht, 0), 0, 8);
    std::memcpy(test_hashtable_entry_key(ht, 0), mismatched_key,
                std::strlen(mismatched_key) + 1);     // [手順] - バケットは変えず格納キーだけを差し替える。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - バケットと再計算ハッシュの不一致が CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_next_link_out_of_range)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data(),
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - レコード 1 を使用中にする。
    *test_hashtable_entry_next(ht, 0) = 99;           // [手順] - 次リンクへ capacity を超える値を書き込む。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - capacity を超える次リンクが CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_unlinked_in_use_record)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 何も追加せずテーブルを構築する。
    *test_hashtable_entry_status(ht, 0) = 1; // [手順] - どのバケットからもリンクせずレコード 1 を使用中にする。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - 未リンクの使用中レコードが CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_next_empty_mismatch)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0,
                                    &ht); // [手順] - 何も追加せずテーブルを構築する(next_empty は 1)。
    test_hashtable_set_next_empty(ht, 2); // [手順] - next_empty を実際の最小空きスロットと異なる値へ書き換える。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - next_empty の不一致が CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_in_use_count_mismatch)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 1);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(
        ht, "a", value.data(),
        COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - 1 件追加する(in_use_count は 1 になる)。
    test_hashtable_set_counts(ht, 2, 0);           // [手順] - in_use_count を実際のスロット状態と異なる値へ書き換える。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - 実装中件数の不一致が CORRUPT_DESCRIPTOR であること。
}

TEST_F(hashtableValidateTest, detects_deleted_count_mismatch)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 1);

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - capacity 2 の設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    (void)com_util_hashtable_add(ht, "a", value.data(),
                                 COM_UTIL_HASHTABLE_ADD_DELETED_OVERWRITE); // [手順] - 1 件追加する。
    (void)com_util_hashtable_delete(ht, "a"); // [手順] - 削除する(in_use_count は 0、deleted_count は 1 になる)。
    test_hashtable_set_counts(ht, 0, 2); // [手順] - in_use_count は正しいまま、deleted_count だけ異なる値へ書き換える。
    int actual_ret = com_util_hashtable_validate(ht); // [手順] - 整合性を検証する。
    com_util_hashtable_dispose(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              actual_ret); // [確認_異常系] - 削除済み件数の不一致が CORRUPT_DESCRIPTOR であること。
}
