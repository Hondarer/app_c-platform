#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <mock_com_util.h>

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
    config->timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD;
    config->key_size = key_size;
    config->record_size = record_size;
    config->lifetime = lifetime;
}

void fill_value(std::vector<unsigned char> *buf, const char *text)
{
    buf->assign(buf->size(), 0);
    if (text != nullptr)
    {
        std::memcpy(buf->data(), text, std::strlen(text));
    }
}

void set_timestamp(com_util_timespec *ts, time_t sec, int64_t nsec)
{
    ts->tv_sec = sec;
    ts->tv_nsec = nsec;
}

/* hashtable.c の hash_key と同じ djb2。同一バケットの別キーを用意するために使う。 */
unsigned long hash_string_mod(const char *key, size_t capacity)
{
    unsigned long hash = 5381;
    const unsigned char *p = reinterpret_cast<const unsigned char *>(key);
    int c = 0;

    while ((c = *p++) != 0)
    {
        hash = ((hash << 5) + hash) + static_cast<unsigned long>(c);
    }
    return hash % capacity;
}

const char *find_colliding_key(const char *base, size_t capacity)
{
    static const char *const candidates[] = {"b", "c", "d", "e", "f", "g", "h", "i", "j", "k", nullptr};
    const unsigned long target = hash_string_mod(base, capacity);
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

class hashtableTimestampTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
    time_t next_sec_ = 1000;

    void SetUp() override
    {
        ON_CALL(mock_com_util_, com_util_get_realtime(_))
            .WillByDefault(
                [this](com_util_timespec *ts)
                {
                    set_timestamp(ts, next_sec_, 0);
                    next_sec_ += 10;
                }); // [状態] - com_util_get_realtime が呼び出された際に 10 秒ずつ進む時刻を返すようにモックを設定する。
    }
};

TEST_F(hashtableTimestampTest, add_update_delete_stamp_realtime)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    com_util_timespec added = {};
    com_util_timespec added_copy = {};
    com_util_timespec walked = {};
    com_util_timespec updated = {};
    com_util_timespec deleted = {};
    const com_util_timespec *added_ref = nullptr;
    const char *peer = nullptr;

    fill_config(&config, 4, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 5 の設定を用意する。
    fill_value(&value, "v1");
    peer = find_colliding_key("a", 4); // [状態] - "a" と同じバケットの別キーを探す。

    // Pre-Assert
    ASSERT_NE(nullptr, peer); // [Pre-Assert確認_正常系] - 同一バケットの別キーが見つかること。

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_add = com_util_hashtable_add(ht, "a", value.data());       // [手順] - キーを追加する。
    int actual_ret_add_time = com_util_hashtable_get_timestamp_val(ht, 1, &added); // [手順] - 追加後の時刻を読む。
    com_util_timespec table_added = {};
    int actual_ret_table_add =
        com_util_hashtable_get_table_timestamp_val(ht, &table_added); // [手順] - 追加後のテーブル時刻を読む。
    int actual_ret_find_ref = com_util_hashtable_find_timestamp_ref(ht, "a", &added_ref);  // [手順] - キーで時刻参照を取る。
    int actual_ret_find_val = com_util_hashtable_find_timestamp_val(ht, "a", &added_copy); // [手順] - キーで時刻を複製する。
    time_t added_ref_sec = (added_ref == nullptr) ? 0 : added_ref->tv_sec;
    (void)com_util_hashtable_add(ht, peer,
                                 value.data()); // [手順] - 同一バケットへ別キーを追加してチェイン先頭をずらす。
    int actual_ret_walk = com_util_hashtable_find_timestamp_val(ht, "a", &walked); // [手順] - チェインを辿って時刻を読む。
    fill_value(&value, "v2");
    int actual_ret_update = com_util_hashtable_update(ht, "a", value.data());      // [手順] - 値を更新する。
    int actual_ret_update_time = com_util_hashtable_get_timestamp_val(ht, 1, &updated); // [手順] - 更新後の時刻を読む。
    com_util_timespec table_updated = {};
    int actual_ret_table_update = com_util_hashtable_get_table_timestamp_val(ht, &table_updated);
    int actual_ret_delete = com_util_hashtable_delete(ht, "a");                     // [手順] - キーを削除する。
    int actual_ret_deleted_time = com_util_hashtable_get_timestamp_val(ht, 1, &deleted); // [手順] - 削除後の時刻を読む。
    com_util_timespec table_deleted = {};
    int actual_ret_table_delete = com_util_hashtable_get_table_timestamp_val(ht, &table_deleted);
    int actual_ret_find_deleted =
        com_util_hashtable_find_timestamp_val(ht, "a", &added); // [手順] - 削除済みキーで検索する。
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);          // [確認_正常系] - add が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add_time);     // [確認_正常系] - 追加後に時刻を読めること。
    EXPECT_EQ(1000, added.tv_sec);                   // [確認_正常系] - 追加時刻が最初の実時刻であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_table_add);    // [確認_正常系] - 追加後にテーブル時刻を読めること。
    EXPECT_EQ(1000, table_added.tv_sec);             // [確認_正常系] - テーブル時刻が追加時刻と一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find_ref);     // [確認_正常系] - find_timestamp_ref が成功すること。
    EXPECT_EQ(1000, added_ref_sec);                  // [確認_正常系] - 参照の秒が追加時刻と一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find_val);     // [確認_正常系] - find_timestamp_val が成功すること。
    EXPECT_EQ(1000, added_copy.tv_sec);              // [確認_正常系] - 複製の秒が追加時刻と一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_walk);         // [確認_正常系] - チェインを辿って時刻を読めること。
    EXPECT_EQ(1000, walked.tv_sec);                  // [確認_正常系] - チェイン走査後も追加時刻が保たれること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_update);       // [確認_正常系] - update が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_update_time);  // [確認_正常系] - 更新後に時刻を読めること。
    EXPECT_EQ(1020, updated.tv_sec);                 // [確認_正常系] - 更新で時刻が進むこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_table_update); // [確認_正常系] - 更新後にテーブル時刻を読めること。
    EXPECT_EQ(1020, table_updated.tv_sec);           // [確認_正常系] - テーブル時刻が更新時刻と一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete);       // [確認_正常系] - delete が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_deleted_time); // [確認_正常系] - 削除済みでも時刻を読めること。
    EXPECT_EQ(1030, deleted.tv_sec);                 // [確認_正常系] - 削除時刻が更新されていること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_table_delete); // [確認_正常系] - 削除後にテーブル時刻を読めること。
    EXPECT_EQ(1030, table_deleted.tv_sec);           // [確認_正常系] - テーブル時刻が削除時刻と一致すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_find_deleted); // [確認_異常系] - 削除済みキーの find_timestamp が NOT_FOUND であること。
}

TEST_F(hashtableTimestampTest, push_deleted_does_not_stamp)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    com_util_timespec before = {};
    com_util_timespec after = {};

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 5 の設定を用意する。
    fill_value(&value, "keep");

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data());
    (void)com_util_hashtable_delete(ht, "a");
    (void)com_util_hashtable_get_timestamp_val(ht, 1, &before);                 // [手順] - 加齢前の時刻を保存する。
    int actual_ret_push = com_util_hashtable_push_deleted(ht);             // [手順] - 削除済みを 1 段階加齢する。
    int actual_ret_after = com_util_hashtable_get_timestamp_val(ht, 1, &after); // [手順] - 加齢後の時刻を読む。
    int actual_ret_status = 0;
    int status = -1;
    actual_ret_status = com_util_hashtable_get_status(ht, 1, &status);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_push);  // [確認_正常系] - push_deleted が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after); // [確認_正常系] - 加齢後も時刻を読めること。
    EXPECT_EQ(before.tv_sec, after.tv_sec);   // [確認_正常系] - 加齢では時刻が変わらないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_status);
    EXPECT_EQ(3, status); // [確認_正常系] - 加齢で status が 3 になること。
}

TEST_F(hashtableTimestampTest, empty_slot_time_is_not_found_and_zeroed)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    com_util_timespec ts = {};
    ts.tv_sec = 9;
    com_util_timespec insert_timestamp = {};
    set_timestamp(&insert_timestamp, 50, 0);

    fill_config(&config, 2, 8, 8, 2, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 削除で直ちに空へ戻る設定を用意する。
    fill_value(&value, "gone");

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_empty = com_util_hashtable_get_timestamp_val(ht, 1, &ts); // [手順] - 空スロットの時刻を読む。
    (void)com_util_hashtable_add(ht, "a", value.data());
    int actual_ret_delete = com_util_hashtable_delete(ht, "a");         // [手順] - lifetime 2 で直ちに空へ戻す。
    int actual_ret_after = com_util_hashtable_get_timestamp_val(ht, 1, &ts); // [手順] - 空へ戻したスロットの時刻を読む。
    int actual_ret_insert = com_util_hashtable_insert_direct(ht, 1, "b", 1, value.data(), &insert_timestamp);
    int actual_ret_direct_time = com_util_hashtable_get_timestamp_val(ht, 1, &ts);
    (void)com_util_hashtable_delete(ht, "b");
    (void)com_util_hashtable_insert_direct(ht, 2, "c", 3, value.data(), &insert_timestamp);
    int actual_ret_purge = com_util_hashtable_purge_deleted(ht); // [手順] - 削除済みを空へ戻す。
    int actual_ret_purged = com_util_hashtable_get_timestamp_val(ht, 2, &ts);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_empty);  // [確認_異常系] - 空の get_time が NOT_FOUND であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete);            // [確認_正常系] - 直ちに空へ戻る削除が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_after);  // [確認_異常系] - 空へ戻したスロットの時刻が無効であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_insert);            // [確認_正常系] - insert_direct が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_direct_time);       // [確認_正常系] - 指定時刻を読めること。
    EXPECT_EQ(50, ts.tv_sec);                             // [確認_正常系] - insert_direct が渡した時刻を保持すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_purge);             // [確認_正常系] - purge_deleted が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_purged); // [確認_異常系] - 回収後の時刻が無効であること。
}

TEST_F(hashtableTimestampTest, accessors_reject_invalid_arguments)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_timespec ts = {};
    const com_util_timespec *ref = nullptr;
    std::vector<unsigned char> value(8, 0);
    char too_long[16];

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - 妥当な設定を用意する。
    fill_value(&value, "a");
    std::memset(too_long, 'x', sizeof(too_long)); // [状態] - NUL が無いキーを用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data());
    int actual_ret_get_null_ht = com_util_hashtable_get_timestamp_ref(NULL, 1, &ref);
    int actual_ret_get_null_out = com_util_hashtable_get_timestamp_ref(ht, 1, NULL);
    int actual_ret_get_rec0 = com_util_hashtable_get_timestamp_val(ht, 0, &ts);
    int actual_ret_get_rec_hi = com_util_hashtable_get_timestamp_val(ht, 3, &ts);
    int actual_ret_get_val_null = com_util_hashtable_get_timestamp_val(ht, 1, NULL);
    int actual_ret_find_null_ht = com_util_hashtable_find_timestamp_ref(NULL, "a", &ref);
    int actual_ret_find_null_key = com_util_hashtable_find_timestamp_ref(ht, NULL, &ref);
    int actual_ret_find_null_out = com_util_hashtable_find_timestamp_ref(ht, "a", NULL);
    int actual_ret_find_long = com_util_hashtable_find_timestamp_val(ht, too_long, &ts);
    int actual_ret_find_val_null = com_util_hashtable_find_timestamp_val(ht, "a", NULL);
    int actual_ret_find_missing = com_util_hashtable_find_timestamp_val(ht, "b", &ts);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_null_out); // [確認_異常系] - NULL 出力が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_rec0); // [確認_異常系] - レコード番号 0 が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_rec_hi); // [確認_異常系] - 範囲外レコードが INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_val_null); // [確認_異常系] - NULL 複製先が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_null_ht); // [確認_異常系] - find の NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_null_key); // [確認_異常系] - find の NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_null_out); // [確認_異常系] - find の NULL 出力が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE,
              actual_ret_find_long); // [確認_異常系] - 長すぎるキーが OUT_OF_RANGE であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_val_null); // [確認_異常系] - find_val の NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_find_missing); // [確認_異常系] - 無いキーが NOT_FOUND であること。
}

TEST_F(hashtableTimestampTest, table_timestamp_tracks_content_changes)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    com_util_timespec table = {};
    com_util_timespec older = {};
    com_util_timespec newer = {};
    const com_util_timespec *table_ref = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING); // [状態] - lifetime 5 の設定を用意する。
    fill_value(&value, "v");
    set_timestamp(&older, 50, 0);
    set_timestamp(&newer, 2000, 0);

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_create =
        com_util_hashtable_get_table_timestamp_val(ht, &table); // [手順] - 構築直後のテーブル時刻を読む。
    time_t created_sec = table.tv_sec;
    (void)com_util_hashtable_add(ht, "a", value.data());
    (void)com_util_hashtable_delete(ht, "a");
    (void)com_util_hashtable_get_table_timestamp_val(ht, &table); // [手順] - 削除後のテーブル時刻を保存する。
    time_t after_delete = table.tv_sec;
    int actual_ret_push = com_util_hashtable_push_deleted(ht); // [手順] - 削除済みを加齢する。
    int actual_ret_after_push = com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_push = table.tv_sec;
    int actual_ret_purge = com_util_hashtable_purge_deleted(ht); // [手順] - 削除済みを空へ戻す。
    int actual_ret_after_purge = com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_purge = table.tv_sec;
    int actual_ret_old = com_util_hashtable_insert_direct(ht, 1, "b", 1, value.data(), &older);
    int actual_ret_after_old = com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_old = table.tv_sec;
    int actual_ret_new = com_util_hashtable_insert_direct(ht, 2, "c", 1, value.data(), &newer);
    int actual_ret_after_new = com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_new = table.tv_sec;
    int actual_ret_ref = com_util_hashtable_get_table_timestamp_ref(ht, &table_ref);
    time_t ref_sec = (table_ref == nullptr) ? 0 : table_ref->tv_sec;
    int actual_ret_clear = com_util_hashtable_clear(ht); // [手順] - テーブルを空にする。
    int actual_ret_after_clear = com_util_hashtable_get_table_timestamp_val(ht, &table);
    int actual_ret_null_ht = com_util_hashtable_get_table_timestamp_ref(NULL, &table_ref);
    int actual_ret_null_out = com_util_hashtable_get_table_timestamp_ref(ht, NULL);
    int actual_ret_val_null = com_util_hashtable_get_table_timestamp_val(ht, NULL);
    int actual_ret_val_null_ht = com_util_hashtable_get_table_timestamp_val(NULL, &table);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);      // [確認_正常系] - 構築直後でもテーブル時刻を読めること。
    EXPECT_EQ(0, created_sec);                      // [確認_正常系] - 構築直後のテーブル時刻が 0 であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_push);        // [確認_正常系] - push_deleted が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_push);  // [確認_正常系] - 加齢後もテーブル時刻を読めること。
    EXPECT_EQ(after_delete, after_push);            // [確認_正常系] - 加齢ではテーブル時刻が変わらないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_purge);       // [確認_正常系] - purge_deleted が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_purge); // [確認_正常系] - 回収後もテーブル時刻を読めること。
    EXPECT_EQ(after_delete, after_purge);           // [確認_正常系] - 回収ではテーブル時刻が変わらないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_old);         // [確認_正常系] - 古い時刻の insert_direct が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_old);   // [確認_正常系] - 古い時刻の書き込み後もテーブル時刻を読めること。
    EXPECT_EQ(after_delete, after_old);             // [確認_正常系] - 古い時刻ではテーブル時刻が進まないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_new);         // [確認_正常系] - 新しい時刻の insert_direct が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_new); // [確認_正常系] - 新しい時刻の書き込み後にテーブル時刻を読めること。
    EXPECT_EQ(2000, after_new);                   // [確認_正常系] - 新しい時刻でテーブル時刻が進むこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ref);       // [確認_正常系] - get_table_timestamp_ref が成功すること。
    EXPECT_EQ(2000, ref_sec);                     // [確認_正常系] - 参照の秒がテーブル時刻と一致すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_clear);     // [確認_正常系] - clear が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_clear); // [確認_正常系] - clear 後にテーブル時刻を読めること。
    EXPECT_EQ(1020, table.tv_sec);                  // [確認_正常系] - clear でテーブル時刻が進むこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_ht); // [確認_異常系] - NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_null_out); // [確認_異常系] - NULL 出力が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_val_null); // [確認_異常系] - NULL 複製先が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_val_null_ht); // [確認_異常系] - val の NULL ht が INVALID_ARGUMENT であること。
}

TEST_F(hashtableTimestampTest, scope_table_record_timestamp_apis_are_unsupported)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable_config read_config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    com_util_timespec ts = {};
    com_util_timespec table = {};
    const com_util_timespec *ref = nullptr;
    const com_util_timespec *table_ref = nullptr;

    fill_config(&config, 2, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING);
    config.timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE; // [状態] - テーブル横断のみの粒度にする。
    fill_value(&value, "v");

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    (void)com_util_hashtable_add(ht, "a", value.data());
    int actual_ret_get_ref = com_util_hashtable_get_timestamp_ref(ht, 1, &ref);
    int actual_ret_get_val = com_util_hashtable_get_timestamp_val(ht, 1, &ts);
    int actual_ret_find_ref = com_util_hashtable_find_timestamp_ref(ht, "a", &ref);
    int actual_ret_find_val = com_util_hashtable_find_timestamp_val(ht, "a", &ts);
    int actual_ret_get_null_ht = com_util_hashtable_get_timestamp_ref(NULL, 1, &ref);
    int actual_ret_get_null_out = com_util_hashtable_get_timestamp_ref(ht, 1, NULL);
    int actual_ret_get_val_null = com_util_hashtable_get_timestamp_val(ht, 1, NULL);
    int actual_ret_find_null_ht = com_util_hashtable_find_timestamp_ref(NULL, "a", &ref);
    int actual_ret_find_null_key = com_util_hashtable_find_timestamp_ref(ht, NULL, &ref);
    int actual_ret_find_null_out = com_util_hashtable_find_timestamp_ref(ht, "a", NULL);
    int actual_ret_find_val_null = com_util_hashtable_find_timestamp_val(ht, "a", NULL);
    int actual_ret_table_ref = com_util_hashtable_get_table_timestamp_ref(ht, &table_ref);
    int actual_ret_table_val = com_util_hashtable_get_table_timestamp_val(ht, &table);
    int actual_ret_config = com_util_hashtable_get_config_val(ht, &read_config);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              actual_ret_get_ref); // [確認_異常系] - SCOPE_TABLE の get_timestamp_ref が UNSUPPORTED であること。
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              actual_ret_get_val); // [確認_異常系] - SCOPE_TABLE の get_timestamp_val が UNSUPPORTED であること。
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              actual_ret_find_ref); // [確認_異常系] - SCOPE_TABLE の find_timestamp_ref が UNSUPPORTED であること。
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              actual_ret_find_val); // [確認_異常系] - SCOPE_TABLE の find_timestamp_val が UNSUPPORTED であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_null_ht); // [確認_異常系] - NULL ht が UNSUPPORTED より INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_null_out); // [確認_異常系] - NULL 出力が UNSUPPORTED より INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_get_val_null); // [確認_異常系] - NULL 複製先が UNSUPPORTED より INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_null_ht); // [確認_異常系] - find の NULL ht が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_null_key); // [確認_異常系] - find の NULL key が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_null_out); // [確認_異常系] - find の NULL 出力が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_find_val_null); // [確認_異常系] - find_val の NULL が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_table_ref); // [確認_正常系] - SCOPE_TABLE でもテーブル時刻参照が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_table_val); // [確認_正常系] - SCOPE_TABLE でもテーブル時刻複製が成功すること。
    EXPECT_EQ(1000, table.tv_sec);               // [確認_正常系] - add でテーブル時刻が進むこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_config);   // [確認_正常系] - get_config_val が成功すること。
    EXPECT_EQ(COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE,
              read_config.timestamp_scope); // [確認_正常系] - 構築時の粒度が設定へ残ること。
}

TEST_F(hashtableTimestampTest, scope_table_insert_direct_and_table_timestamp)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(8, 0);
    com_util_timespec supplied = {};
    com_util_timespec table = {};

    fill_config(&config, 4, 8, 8, 5, COM_UTIL_HASHTABLE_KEY_STRING);
    config.timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE; // [状態] - テーブル横断のみの粒度にする。
    fill_value(&value, "v");
    set_timestamp(&supplied, 50, 0);

    // Pre-Assert

    // Act
    (void)com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht);
    int actual_ret_add = com_util_hashtable_add(ht, "a", value.data());
    (void)com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_add = table.tv_sec;
    int actual_ret_update = com_util_hashtable_update(ht, "a", value.data());
    (void)com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_update = table.tv_sec;
    int actual_ret_delete = com_util_hashtable_delete(ht, "a");
    (void)com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_delete = table.tv_sec;
    int actual_ret_nonnull =
        com_util_hashtable_insert_direct(ht, 2, "b", 1, value.data(), &supplied); // [手順] - SCOPE_TABLE で時刻を渡す。
    int actual_ret_null =
        com_util_hashtable_insert_direct(ht, 2, "b", 1, value.data(), NULL); // [手順] - SCOPE_TABLE で時刻を省略する。
    int actual_ret_after_direct = com_util_hashtable_get_table_timestamp_val(ht, &table);
    time_t after_direct = table.tv_sec;
    int actual_ret_clear = com_util_hashtable_clear(ht);
    int actual_ret_after_clear = com_util_hashtable_get_table_timestamp_val(ht, &table);
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_add);    // [確認_正常系] - add が成功すること。
    EXPECT_EQ(1000, after_add);                // [確認_正常系] - add でテーブル時刻が進むこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_update); // [確認_正常系] - update が成功すること。
    EXPECT_EQ(1010, after_update);             // [確認_正常系] - update でテーブル時刻が進むこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_delete); // [確認_正常系] - delete が成功すること。
    EXPECT_EQ(1020, after_delete);             // [確認_正常系] - delete でテーブル時刻が進むこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              actual_ret_nonnull); // [確認_異常系] - SCOPE_TABLE で時刻を渡すと INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_null);        // [確認_正常系] - SCOPE_TABLE で時刻省略の insert_direct が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_direct); // [確認_正常系] - insert_direct 後もテーブル時刻を読めること。
    EXPECT_EQ(after_delete, after_direct); // [確認_正常系] - SCOPE_TABLE の insert_direct ではテーブル時刻が進まないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_clear);       // [確認_正常系] - clear が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_after_clear); // [確認_正常系] - clear 後にテーブル時刻を読めること。
    EXPECT_EQ(1030, table.tv_sec);                  // [確認_正常系] - clear でテーブル時刻が進むこと。
}
