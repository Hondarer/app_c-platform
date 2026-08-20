#include <testfw.h>

#include <com_util/base/result.h>
#include <com_util/hashtable/hashtable.h>
#include <com_util/mmap/mmap.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{

constexpr size_t k_key_size = 512;
constexpr size_t k_record_size = 512;
constexpr unsigned char k_lifetime = 5;

void fill_value(std::vector<unsigned char> *buf, const char *text)
{
    buf->assign(k_record_size, 0);
    if (text != nullptr)
    {
        std::memcpy(buf->data(), text, std::strlen(text));
    }
}

} // namespace

class hashtableIntegrationTest : public Test
{
};

TEST_F(hashtableIntegrationTest, string_mode_demo_scenarios)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    std::vector<unsigned char> value(k_record_size, 0);
    uint64_t apple_rec = 0;
    uint64_t banana_rec = 0;
    int status = -1;
    size_t in_use = 0;
    size_t deleted = 0;
    size_t empty = 0;
    const void *found = nullptr;
    char too_long[k_key_size + 1];
    com_util_timespec rec_timestamp = {};

    config.capacity = 4;
    config.key_type = COM_UTIL_HASHTABLE_KEY_STRING;
    config.key_size = k_key_size;
    config.record_size = k_record_size;
    config.lifetime = k_lifetime; // [状態] - デモと同じ文字列モード設定を用意する。
    std::memset(too_long, 'x', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';

    // Pre-Assert

    // Act
    int actual_ret_create = com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - テーブルを構築する。
    fill_value(&value, "りんご");
    (void)com_util_hashtable_add(ht, "apple", value.data());
    fill_value(&value, "バナナ");
    (void)com_util_hashtable_add(ht, "banana", value.data());
    fill_value(&value, "さくらんぼ");
    (void)com_util_hashtable_add(ht, "cherry", value.data());
    fill_value(&value, "ドリアン");
    (void)com_util_hashtable_add(ht, "durian", value.data());
    int actual_ret_timestamp = com_util_hashtable_get_timestamp_val(ht, 1, &rec_timestamp);
    (void)com_util_hashtable_find_recno(ht, "apple", &apple_rec);
    (void)com_util_hashtable_find_recno(ht, "banana", &banana_rec);
    (void)com_util_hashtable_delete(ht, "banana");
    int actual_ret_find_deleted = com_util_hashtable_find_value_ref(ht, "banana", &found);
    (void)com_util_hashtable_get_status(ht, banana_rec, &status);
    fill_value(&value, "エルダーベリー");
    int actual_ret_full = com_util_hashtable_add(ht, "elderberry", value.data());
    for (int i = 0; i < k_lifetime - 2; ++i)
    {
        (void)com_util_hashtable_push_deleted(ht);
    }
    fill_value(&value, "エルダーベリー");
    int actual_ret_reuse = com_util_hashtable_add(ht, "elderberry", value.data());
    (void)com_util_hashtable_count_status(ht, &in_use, &deleted, &empty);
    fill_value(&value, "long");
    int actual_ret_long = com_util_hashtable_add(ht, too_long, value.data());
    (void)com_util_hashtable_clear(ht);
    fill_value(&value, "空");
    int actual_ret_empty_key = com_util_hashtable_add(ht, "", value.data());
    com_util_hashtable_destroy(ht);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create); // [確認_正常系] - create が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_UNSUPPORTED,
              actual_ret_timestamp); // [確認_異常系] - 既定の SCOPE_TABLE ではレコード時刻が取れないこと。
    EXPECT_EQ(1u, apple_rec);        // [確認_正常系] - apple のレコード番号が 1 であること。
    EXPECT_EQ(2u, banana_rec);       // [確認_正常系] - banana のレコード番号が 2 であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_find_deleted); // [確認_正常系] - 削除後の banana が見つからないこと。
    EXPECT_EQ(2, status);                                       // [確認_正常系] - 削除直後の状態が 2 であること。
    EXPECT_EQ(COM_UTIL_ERR_LIMIT_EXCEEDED, actual_ret_full);    // [確認_異常系] - 削除直後は満杯であること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_reuse);                   // [確認_正常系] - 寿命到達後に追加できること。
    EXPECT_EQ(4u, in_use);                                      // [確認_正常系] - 再利用後は 4 件実装中であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_RANGE, actual_ret_long);      // [確認_異常系] - 長すぎるキーが拒否されること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_empty_key);               // [確認_正常系] - 空文字列キーが追加できること。
}

TEST_F(hashtableIntegrationTest, binary_and_persist_demo_scenarios)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *reattached = nullptr;
    std::vector<unsigned char> value(k_record_size, 0);
    unsigned char key1[k_key_size] = {0};
    unsigned char key3[k_key_size] = {0};
    unsigned char zero_key[k_key_size] = {0};
    const void *found = nullptr;
    size_t mgmt_needed = 0;
    size_t data_needed = 0;

    config.capacity = 4;
    config.key_type = COM_UTIL_HASHTABLE_KEY_BINARY;
    config.key_size = k_key_size;
    config.record_size = k_record_size;
    config.lifetime = k_lifetime; // [状態] - デモと同じバイナリ モード設定を用意する。
    key1[0] = 1;
    key1[1] = 2;
    key1[2] = 3;
    key3[0] = 1;
    key3[1] = 2;
    key3[2] = 3;
    key3[k_key_size - 1] = 0xFF;

    // Pre-Assert

    // Act
    int actual_ret_create =
        com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - バイナリ テーブルを構築する。
    fill_value(&value, "binary-value-1");
    (void)com_util_hashtable_add(ht, key1, value.data());
    fill_value(&value, "全ゼロ");
    int actual_ret_zero = com_util_hashtable_add(ht, zero_key, value.data());
    int actual_ret_tail = com_util_hashtable_find_value_ref(ht, key3, &found);
    com_util_hashtable_destroy(ht);

    config.key_type = COM_UTIL_HASHTABLE_KEY_STRING;
    int actual_ret_size = com_util_hashtable_required_size(&config, &mgmt_needed, &data_needed);
    std::vector<unsigned char> buf_mgmt(mgmt_needed, 0);
    std::vector<unsigned char> buf_data(data_needed, 0);
    std::vector<unsigned char> buf_mgmt2(mgmt_needed, 0);
    std::vector<unsigned char> buf_data2(data_needed, 0);
    int actual_ret_ext =
        com_util_hashtable_create(&config, buf_mgmt.data(), buf_mgmt.size(), buf_data.data(), buf_data.size(), &ht);
    fill_value(&value, "いちじく");
    (void)com_util_hashtable_add(ht, "fig", value.data());
    std::memcpy(buf_mgmt2.data(), buf_mgmt.data(), mgmt_needed);
    std::memcpy(buf_data2.data(), buf_data.data(), data_needed);
    int actual_ret_attach =
        com_util_hashtable_attach(buf_mgmt2.data(), buf_mgmt2.size(), buf_data2.data(), buf_data2.size(), &reattached);
    int actual_ret_validate = com_util_hashtable_validate(reattached);
    int actual_ret_find = com_util_hashtable_find_value_ref(reattached, "fig", &found);
    com_util_hashtable_destroy(ht);
    com_util_hashtable_destroy(reattached);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);          // [確認_正常系] - バイナリ create が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_zero);            // [確認_正常系] - 全ゼロ キーが追加できること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_tail); // [確認_正常系] - 末尾差のキーが見つからないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_size);            // [確認_正常系] - required_size が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ext);             // [確認_正常系] - 外部バッファー create が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attach);          // [確認_正常系] - attach が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate);        // [確認_正常系] - validate が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);            // [確認_正常系] - 再接続後に fig が見つかること。
    EXPECT_STREQ("いちじく", static_cast<const char *>(found)); // [確認_正常系] - 再接続後の値が一致すること。
}

TEST_F(hashtableIntegrationTest, migrate_records_by_number)
{
    // Arrange
    com_util_hashtable_config src_config = {};
    com_util_hashtable_config dest_config = {};
    com_util_hashtable *src = nullptr;
    com_util_hashtable *dest_keep = nullptr;
    com_util_hashtable *dest_skip = nullptr;
    std::vector<unsigned char> value(k_record_size, 0);
    std::vector<unsigned char> key_buf(k_key_size, 0);
    std::vector<unsigned char> value_buf(k_record_size, 0);
    uint64_t apple_rec = 0;
    uint64_t banana_rec = 0;
    uint64_t cherry_rec = 0;
    int banana_status = -1;
    int dest_banana_status = -1;
    uint64_t dest_apple_rec = 0;
    const void *found = nullptr;
    size_t in_use = 0;
    size_t deleted = 0;
    size_t empty = 0;

    src_config.capacity = 4;
    src_config.key_type = COM_UTIL_HASHTABLE_KEY_STRING;
    src_config.timestamp_scope = COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD; // [状態] - 移行はレコード時刻を保つ。
    src_config.key_size = k_key_size;
    src_config.record_size = k_record_size;
    src_config.lifetime = k_lifetime; // [状態] - 移行元は lifetime 5 とする。
    dest_config = src_config;
    dest_config.lifetime = 4; // [状態] - 削除済みを受け取れる lifetime 4 の移行先を用意する。

    // Pre-Assert

    // Act
    int actual_ret_src = com_util_hashtable_create(&src_config, NULL, 0, NULL, 0, &src); // [手順] - 移行元を構築する。
    fill_value(&value, "りんご");
    (void)com_util_hashtable_add(src, "apple", value.data());
    fill_value(&value, "バナナ");
    (void)com_util_hashtable_add(src, "banana", value.data());
    fill_value(&value, "さくらんぼ");
    (void)com_util_hashtable_add(src, "cherry", value.data());
    (void)com_util_hashtable_find_recno(src, "apple", &apple_rec);
    (void)com_util_hashtable_find_recno(src, "banana", &banana_rec);
    (void)com_util_hashtable_find_recno(src, "cherry", &cherry_rec);
    (void)com_util_hashtable_delete(src, "banana");
    (void)com_util_hashtable_get_status(src, banana_rec, &banana_status);

    int actual_ret_keep = com_util_hashtable_create(&dest_config, NULL, 0, NULL, 0,
                                                    &dest_keep); // [手順] - lifetime 4 の移行先を構築する。
    std::vector<int> keep_copies;
    for (uint64_t rec = 1; rec <= 4; ++rec)
    {
        int status = 0;
        (void)com_util_hashtable_get_status(src, rec, &status);
        if (status == 0)
        {
            continue;
        }
        (void)com_util_hashtable_get_key_val(src, rec, key_buf.data());
        (void)com_util_hashtable_get_value_val(src, rec, value_buf.data());
        com_util_timespec rec_timestamp = {};
        (void)com_util_hashtable_get_timestamp_val(src, rec, &rec_timestamp);
        keep_copies.push_back(
            com_util_hashtable_insert_direct(dest_keep, rec, key_buf.data(), status, value_buf.data(), &rec_timestamp));
    }
    int actual_ret_keep_find = com_util_hashtable_find_recno(dest_keep, "apple", &dest_apple_rec);
    int actual_ret_keep_deleted = com_util_hashtable_find_value_ref(dest_keep, "banana", &found);
    (void)com_util_hashtable_get_status(dest_keep, banana_rec, &dest_banana_status);
    int actual_ret_keep_validate = com_util_hashtable_validate(dest_keep);

    dest_config.lifetime = 2; // [状態] - 削除済みを受け取れない lifetime 2 の移行先へ切り替える。
    int actual_ret_skip = com_util_hashtable_create(&dest_config, NULL, 0, NULL, 0,
                                                    &dest_skip); // [手順] - lifetime 2 の移行先を構築する。
    std::vector<int> skip_copies;
    for (uint64_t rec = 1; rec <= 4; ++rec)
    {
        int status = 0;
        (void)com_util_hashtable_get_status(src, rec, &status);
        if (status == 0)
        {
            continue;
        }
        (void)com_util_hashtable_get_key_val(src, rec, key_buf.data());
        (void)com_util_hashtable_get_value_val(src, rec, value_buf.data());
        com_util_timespec rec_timestamp = {};
        (void)com_util_hashtable_get_timestamp_val(src, rec, &rec_timestamp);
        skip_copies.push_back(
            com_util_hashtable_insert_direct(dest_skip, rec, key_buf.data(), status, value_buf.data(), &rec_timestamp));
    }
    (void)com_util_hashtable_count_status(dest_skip, &in_use, &deleted, &empty);
    int actual_ret_skip_validate = com_util_hashtable_validate(dest_skip);
    int actual_ret_skip_find = com_util_hashtable_find_value_ref(dest_skip, "banana", &found);

    com_util_hashtable_destroy(src);
    com_util_hashtable_destroy(dest_keep);
    com_util_hashtable_destroy(dest_skip);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_src);       // [確認_正常系] - 移行元の構築が成功すること。
    EXPECT_EQ(1u, apple_rec);                     // [確認_正常系] - apple がレコード 1 であること。
    EXPECT_EQ(2u, banana_rec);                    // [確認_正常系] - banana がレコード 2 であること。
    EXPECT_EQ(3u, cherry_rec);                    // [確認_正常系] - cherry がレコード 3 であること。
    EXPECT_EQ(2, banana_status);                  // [確認_正常系] - 移行元の banana が削除済みであること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_keep);      // [確認_正常系] - lifetime 4 の移行先構築が成功すること。
    ASSERT_EQ(3u, keep_copies.size());            // [確認_正常系] - 空以外の 3 件を移行すること。
    EXPECT_EQ(COM_UTIL_OK, keep_copies[0]);       // [確認_正常系] - lifetime 4 では 1 件目を置けること。
    EXPECT_EQ(COM_UTIL_OK, keep_copies[1]);       // [確認_正常系] - lifetime 4 では 2 件目を置けること。
    EXPECT_EQ(COM_UTIL_OK, keep_copies[2]);       // [確認_正常系] - lifetime 4 では 3 件目を置けること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_keep_find); // [確認_正常系] - 移行後に apple が見つかること。
    EXPECT_EQ(apple_rec, dest_apple_rec);         // [確認_正常系] - apple のレコード番号が保たれること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              actual_ret_keep_deleted); // [確認_正常系] - 移行後の banana は検索対象にならないこと。
    EXPECT_EQ(2, dest_banana_status);   // [確認_正常系] - banana の加齢値が移行先でも 2 であること。
    EXPECT_EQ(COM_UTIL_OK,
              actual_ret_keep_validate);         // [確認_正常系] - 削除済みを含む移行先の validate が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_skip);     // [確認_正常系] - lifetime 2 の移行先構築が成功すること。
    ASSERT_EQ(3u, skip_copies.size());           // [確認_正常系] - lifetime 2 でも空以外の 3 件を試すこと。
    EXPECT_EQ(COM_UTIL_OK, skip_copies[0]);      // [確認_正常系] - apple は lifetime 2 でも置けること。
    EXPECT_EQ(COM_UTIL_SKIPPED, skip_copies[1]); // [確認_正常系] - banana の削除済みが SKIPPED になること。
    EXPECT_EQ(COM_UTIL_OK, skip_copies[2]);      // [確認_正常系] - cherry は lifetime 2 でも置けること。
    EXPECT_EQ(2u, in_use);                       // [確認_正常系] - lifetime 2 の移行先は実装中 2 件であること。
    EXPECT_EQ(0u, deleted);                      // [確認_正常系] - lifetime 2 の移行先に削除済みが残らないこと。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_skip_validate); // [確認_正常系] - SKIPPED 後の移行先の validate が成功すること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND, actual_ret_skip_find); // [確認_正常系] - SKIPPED した banana が移行先に無いこと。
}

TEST_F(hashtableIntegrationTest, mmap_backed_data_region_round_trip)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *reattached = nullptr;
    com_util_mmap *map = nullptr;
    com_util_mmap *map2 = nullptr;
    size_t mgmt_size = 0;
    size_t data_size = 0;
    std::vector<unsigned char> value(k_record_size, 0);
    const void *found = nullptr;
    std::string ws = findWorkspaceRoot();
    std::string path =
        ws + "/app/com_util/test/src/integration/hashtable/hashtableIntegrationTest/results/hashtable_data.map";

    remove(path.c_str());

    config.capacity = 4;
    config.key_type = COM_UTIL_HASHTABLE_KEY_STRING;
    config.key_size = k_key_size;
    config.record_size = k_record_size;
    config.lifetime = k_lifetime; // [状態] - mmap 検証用のテーブル設定を用意する。

    // Pre-Assert

    // Act
    (void)com_util_hashtable_required_size(&config, &mgmt_size, &data_size);
    std::vector<unsigned char> buf_mgmt(mgmt_size, 0);
    int actual_ret_mmap_create = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, data_size, &map,
                                                      nullptr); // [手順] - データ領域用のファイルを新規に mmap する。
    int actual_ret_create = com_util_hashtable_create(
        &config, buf_mgmt.data(), buf_mgmt.size(), com_util_mmap_get_address(map), com_util_mmap_get_size(map),
        &ht); // [手順] - 管理領域は通常確保、データ領域は mmap したファイルで構築する。
    fill_value(&value, "mapped-value");
    (void)com_util_hashtable_add(ht, "grape", value.data());
    int actual_ret_flush =
        com_util_mmap_flush(map, nullptr, 0, nullptr); // [手順] - mmap した内容をディスクへ反映する。
    com_util_hashtable_destroy(ht);
    (void)com_util_mmap_detach(map, nullptr);

    std::vector<unsigned char> buf_mgmt2 = buf_mgmt;
    int actual_ret_mmap_reopen = com_util_mmap_attach(path.c_str(), COM_UTIL_MMAP_ACCESS_READ_WRITE, data_size, &map2,
                                                      nullptr); // [手順] - 同じファイルを別ハンドルで再度 mmap する。
    int actual_ret_attach = com_util_hashtable_attach(
        buf_mgmt2.data(), buf_mgmt2.size(), com_util_mmap_get_address(map2), com_util_mmap_get_size(map2),
        &reattached); // [手順] - 複製した管理領域と再マップしたデータ領域で再接続する。
    int actual_ret_find =
        com_util_hashtable_find_value_ref(reattached, "grape", &found); // [手順] - 再接続後に検索する。
    std::string found_text =
        (found == nullptr) ? "" : static_cast<const char *>(found); // [手順] - unmap 前に値を複製する。

    com_util_hashtable_destroy(reattached);
    (void)com_util_mmap_detach(map2, nullptr);
    remove(path.c_str());

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_mmap_create); // [確認_正常系] - データ領域ファイルの mmap が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);      // [確認_正常系] - mmap 領域をデータ領域として構築できること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_flush);       // [確認_正常系] - flush が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_mmap_reopen); // [確認_正常系] - 同じファイルの再 mmap が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attach);      // [確認_正常系] - 再マップしたデータ領域で再接続できること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);        // [確認_正常系] - 再接続後に mmap 経由で値を検索できること。
    EXPECT_EQ("mapped-value", found_text);          // [確認_正常系] - mmap 経由の値が一致すること。
}

TEST_F(hashtableIntegrationTest, internal_buffers_round_trip_through_file)
{
    // Arrange
    com_util_hashtable_config config = {};
    com_util_hashtable *ht = nullptr;
    com_util_hashtable *reattached = nullptr;
    std::vector<unsigned char> value(k_record_size, 0);
    const void *mgmt = nullptr;
    const void *data = nullptr;
    size_t mgmt_size = 0;
    size_t data_size = 0;
    const void *found = nullptr;
    std::string ws = findWorkspaceRoot();
    std::string path =
        ws + "/app/com_util/test/src/integration/hashtable/hashtableIntegrationTest/results/hashtable_dump.bin";

    remove(path.c_str());

    config.capacity = 4;
    config.key_type = COM_UTIL_HASHTABLE_KEY_STRING;
    config.key_size = k_key_size;
    config.record_size = k_record_size;
    config.lifetime = k_lifetime; // [状態] - 内部確保で永続化するテーブル設定を用意する。

    // Pre-Assert

    // Act
    int actual_ret_create =
        com_util_hashtable_create(&config, NULL, 0, NULL, 0, &ht); // [手順] - 内部確保でテーブルを構築する。
    fill_value(&value, "persisted-apple");
    (void)com_util_hashtable_add(ht, "apple", value.data());
    fill_value(&value, "persisted-banana");
    (void)com_util_hashtable_add(ht, "banana", value.data());

    int actual_ret_ref = com_util_hashtable_buffer_ref(ht, &mgmt, &data); // [手順] - 管理中の 2 領域の先頭を得る。
    int actual_ret_size = com_util_hashtable_buffer_size(ht, &mgmt_size, &data_size);

    FILE *out = fopen(path.c_str(), "wb");
    ASSERT_NE(nullptr, out);                             // [状態確認] - 書き出し先を開けること。
    size_t wrote_mgmt = fwrite(mgmt, 1, mgmt_size, out); // [手順] - 管理領域を書き出す。
    size_t wrote_data = fwrite(data, 1, data_size, out); // [手順] - データ領域を書き出す。
    (void)fclose(out);
    com_util_hashtable_destroy(ht); // [手順] - 元のテーブルを破棄する。

    /* uint64_t の配列で確保し、管理領域に必要なアラインメントを満たす。 */
    std::vector<uint64_t> load_mgmt((mgmt_size + sizeof(uint64_t) - 1u) / sizeof(uint64_t), 0);
    std::vector<unsigned char> load_data(data_size, 0);
    FILE *in = fopen(path.c_str(), "rb");
    ASSERT_NE(nullptr, in);                                       // [状態確認] - 読み戻し元を開けること。
    size_t read_mgmt = fread(load_mgmt.data(), 1, mgmt_size, in); // [手順] - 管理領域を読み戻す。
    size_t read_data = fread(load_data.data(), 1, data_size, in); // [手順] - データ領域を読み戻す。
    (void)fclose(in);

    int actual_ret_attach = com_util_hashtable_attach(load_mgmt.data(), mgmt_size, load_data.data(), load_data.size(),
                                                      &reattached); // [手順] - 読み戻した 2 領域へ再接続する。
    int actual_ret_validate = com_util_hashtable_validate(reattached);
    int actual_ret_find = com_util_hashtable_find_value_ref(reattached, "banana", &found);
    std::string found_text = (found == nullptr) ? "" : static_cast<const char *>(found);

    com_util_hashtable_destroy(reattached);
    remove(path.c_str());

    // Assert
    EXPECT_EQ(COM_UTIL_OK, actual_ret_create);   // [確認_正常系] - 内部確保の構築が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_ref);      // [確認_正常系] - buffer_ref が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_size);     // [確認_正常系] - buffer_size が成功すること。
    EXPECT_EQ(mgmt_size, wrote_mgmt);            // [確認_正常系] - 管理領域を全量書き出せること。
    EXPECT_EQ(data_size, wrote_data);            // [確認_正常系] - データ領域を全量書き出せること。
    EXPECT_EQ(mgmt_size, read_mgmt);             // [確認_正常系] - 管理領域を全量読み戻せること。
    EXPECT_EQ(data_size, read_data);             // [確認_正常系] - データ領域を全量読み戻せること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_attach);   // [確認_正常系] - 読み戻した領域へ再接続できること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_validate); // [確認_正常系] - 再接続後の整合性検査が成功すること。
    EXPECT_EQ(COM_UTIL_OK, actual_ret_find);     // [確認_正常系] - 再接続後に値を検索できること。
    EXPECT_EQ("persisted-banana", found_text);   // [確認_正常系] - 永続化した値が保たれること。
}
