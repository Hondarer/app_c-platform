#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/trace/trace_file.h>
#include <com_util/crt/file.h>
#include <com_util/sync/sync.h>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdio>

using testing::_;
using testing::AtLeast;
using testing::HasSubstr;
using testing::InSequence;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;

namespace
{

static void set_valid_deadline(struct timespec *abs_timeout)
{
    abs_timeout->tv_sec = (time_t)(time(NULL) + 1);
    abs_timeout->tv_nsec = 0;
}

static void set_fixed_realtime(int64_t *tv_sec, int32_t *tv_nsec)
{
    *tv_sec = 1714100645LL;
    *tv_nsec = 678000000;
}

static com_util_realtime_timestamp make_fixed_timestamp(void)
{
    com_util_realtime_timestamp timestamp;
    timestamp.tv_sec = 1714100645LL;
    timestamp.tv_nsec = 678000000;
    timestamp.reserved = 0;
    return timestamp;
}

static uint32_t open_flags_default(void)
{
    // 単一プロセス モードは共有書き込み (SHARE_WRITE) を許可しない
    return COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH |
           COM_UTIL_FILE_OPEN_SHARE_READ | COM_UTIL_FILE_OPEN_SHARE_DELETE;
}

static uint32_t open_flags_truncate(void)
{
    return open_flags_default() | COM_UTIL_FILE_OPEN_TRUNCATE;
}

static uint32_t open_flags_shared(void)
{
    return open_flags_default() | COM_UTIL_FILE_OPEN_SHARE_WRITE;
}

// 共有モード テストで使う既定のファイル同一性インデックス
constexpr uint64_t kDefaultFileIndex = 100;

static void set_file_id(com_util_file_id *id_out, uint64_t index)
{
    id_out->volume = 1;
    id_out->index = index;
}

} // namespace

class trace_fileTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_;

    void SetUp() override
    {
        ON_CALL(mock_, com_util_get_realtime_deadline_ms(_, _))
            .WillByDefault([](uint64_t, struct timespec *abs_timeout) { set_valid_deadline(abs_timeout); });
        ON_CALL(mock_, com_util_get_realtime(_, _))
            .WillByDefault([](int64_t *tv_sec, int32_t *tv_nsec) { set_fixed_realtime(tv_sec, tv_nsec); });
        ON_CALL(mock_, com_util_format_realtime_iso8601_local(_, _, _, _))
            .WillByDefault(
                [](char *buf, size_t buf_size, int64_t, int32_t)
                {
                    snprintf(buf, buf_size, "%s", "2026-04-26T03:04:05.678+09:00");
                    return 0;
                });
        ON_CALL(mock_, com_util_file_open(_, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_file_get_size(_, _))
            .WillByDefault(
                [](const com_util_file *, size_t *size_out)
                {
                    *size_out = 0;
                    return 0;
                });
        ON_CALL(mock_, com_util_file_write(_, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_file_close(_)).WillByDefault(Return());
        ON_CALL(mock_, com_util_remove(_)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_rename(_, _)).WillByDefault(Return(0));

        // 共有モード用の既定動作: ハンドルとパスの同一性は常に一致させる
        ON_CALL(mock_, com_util_file_get_id(_, _))
            .WillByDefault(
                [](const com_util_file *, com_util_file_id *id_out)
                {
                    set_file_id(id_out, kDefaultFileIndex);
                    return 0;
                });
        ON_CALL(mock_, com_util_file_get_path_id(_, _))
            .WillByDefault(
                [](const char *, com_util_file_id *id_out)
                {
                    set_file_id(id_out, kDefaultFileIndex);
                    return 0;
                });

        // プロセス間ロックはダミー ハンドルで成功させる (実体は作らない)
        ON_CALL(mock_, com_util_interprocess_lock_open(_, _))
            .WillByDefault(
                [](const char *, com_util_interprocess_lock **lock)
                {
                    static int dummy_lock = 0;
                    *lock = (com_util_interprocess_lock *)&dummy_lock;
                    return COM_UTIL_SYNC_OK;
                });
        ON_CALL(mock_, com_util_interprocess_lock_lock(_, _)).WillByDefault(Return(COM_UTIL_SYNC_OK));
        ON_CALL(mock_, com_util_interprocess_lock_unlock(_)).WillByDefault(Return(COM_UTIL_SYNC_OK));
        ON_CALL(mock_, com_util_interprocess_lock_destroy(_)).WillByDefault(Return());
    }
};

// NULL path では create が失敗することの確認
TEST_F(trace_fileTest, test_create_returns_null_for_null_path)
{
    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create(NULL, 0, 0, 0); // [手順] - NULL path で create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL, handle); // [確認_異常系] - NULL が返ること。
}

// create が既定 open flags でファイルを開くことの確認
TEST_F(trace_fileTest, test_create_opens_file_with_default_flags)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 既定 open flags でファイルを開くこと。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 123;
                return 0;
            }); // [Pre-Assert確認_正常系] - 既存サイズ取得が 1 回呼ばれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - 既定値で create を呼ぶ。

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// create のファイル オープンが初回失敗後にリトライされることの確認
TEST_F(trace_fileTest, test_create_retries_file_open_after_initial_failure)
{
    // Pre-Assert
    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(-1)); // [Pre-Assert確認_正常系] - 初回のファイル オープンが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - 3 秒相当の待機を行うこと。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - リトライでファイル オープンが成功すること。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - 初回 open 失敗後に create を継続する。

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// create のファイル オープンがリトライ上限まで失敗した場合に NULL を返すことの確認
TEST_F(trace_fileTest, test_create_returns_null_after_file_open_retry_exhausted)
{
    // Pre-Assert
    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 初回のファイル オープンが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_異常系] - 1 回目のリトライ前に待機すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 1 回目のリトライが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_異常系] - 2 回目のリトライ前に待機すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 2 回目のリトライが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_異常系] - 3 回目のリトライ前に待機すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 3 回目のリトライが失敗すること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - open 失敗を継続させる。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL, handle); // [確認_異常系] - NULL が返ること。
}

// INFO 行が固定タイムスタンプと I marker で書き込まれることの確認
TEST_F(trace_fileTest, test_write_formats_info_line)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I hello\n", actual);
                return 0;
            }); // [Pre-Assert確認_正常系] - INFO 行が期待フォーマットで書き込まれること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "hello"); // [手順] - INFO 行を書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// DEBUG 行が D marker で書き込まれることの確認
TEST_F(trace_fileTest, test_write_formats_debug_marker)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 D debug line\n", actual);
                return 0;
            }); // [Pre-Assert確認_正常系] - DEBUG 行が D marker で書き込まれること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL,
                                                "debug line"); // [手順] - DEBUG 行を書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 明示タイムスタンプ指定時に内部の現在時刻取得を行わずに書き込むことの確認
TEST_F(trace_fileTest, test_write_uses_explicit_timestamp_without_internal_clock)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    com_util_realtime_timestamp timestamp = make_fixed_timestamp();
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    EXPECT_CALL(mock_, com_util_get_realtime(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定時は現在時刻を取得しないこと。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I explicit hello\n", actual);
                return 0;
            }); // [Pre-Assert確認_正常系] - 明示タイムスタンプがそのまま書式化されること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                                                "explicit hello"); // [手順] - 明示タイムスタンプ付きで書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 明示タイムスタンプ付き書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// ファイル書き込み失敗時に -1 が返ることの確認
TEST_F(trace_fileTest, test_write_returns_minus_one_on_file_error)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 低レベル書き込みが -1 を返すこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "write error"); // [手順] - 書き込み失敗を発生させる。

    // Assert
    EXPECT_EQ(-1, result); // [確認_異常系] - com_util_trace_file_sink_write が -1 を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 不正な明示タイムスタンプ指定時に現在時刻へ代替して書き込みつつ -1 を返すことの確認
TEST_F(trace_fileTest, test_write_falls_back_from_invalid_explicit_timestamp)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    com_util_realtime_timestamp invalid_timestamp = {1714100645LL, 1000000000, 0};
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    EXPECT_CALL(mock_, com_util_get_realtime(_, _))
        .Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I invalid\n", actual);
                return 0;
            }); // [Pre-Assert確認_異常系] - 代替時刻で低レベル書き込みを行うこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp,
                                                "invalid"); // [手順] - 不正タイムスタンプで書き込む。

    // Assert
    EXPECT_EQ(-1, result); // [確認_異常系] - 代替出力後も -1 を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// サイズ上限超過時にローテーションが実行されることの確認
TEST_F(trace_fileTest, test_write_rotates_when_size_limit_is_reached)
{
    // Arrange
    InSequence seq;

    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default())).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 元ファイルへの書き込みが成功すること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1);
    EXPECT_CALL(mock_, com_util_remove(StrEq("trace.log.2")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 最古世代ファイル削除が 1 回呼ばれること。
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log.1"), StrEq("trace.log.2")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 旧 .1 が .2 へ順送りされること。
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log"), StrEq("trace.log.1")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 現在ファイルが .1 へリネームされること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_truncate()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 新規世代ファイルが truncate 付きで開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1);

    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 1, 2, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "rotate me"); // [手順] - 上限 1 byte のファイルへ 1 行書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込み後にローテーションが完了すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// dispose が NULL ハンドルでも安全であることの確認
TEST_F(trace_fileTest, test_dispose_with_null_handle_is_safe)
{
    // Act & Assert
    com_util_trace_file_sink_dispose(NULL); // [手順] - NULL ハンドルで dispose を呼ぶ。
}

// パスに区切り文字が含まれる場合に makedirs が親ディレクトリ パスで呼ばれることの確認
TEST_F(trace_fileTest, test_create_calls_makedirs_for_path_with_separator)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_makedirs(StrEq("sub")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 親ディレクトリ "sub" で makedirs が呼ばれること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("sub/trace.log"), open_flags_default()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 親ディレクトリ生成後にファイルが開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("sub/trace.log", 0, 0, 0); // [手順] - 区切り文字を含むパスで create を呼ぶ。

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

#if defined(PLATFORM_WINDOWS)
// Windows スタイル区切りのパスでも makedirs が親ディレクトリ パスで呼ばれることの確認
TEST_F(trace_fileTest, test_create_normalizes_windows_separator_for_parent_directory)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_makedirs(StrEq("sub/dir")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 正規化した親ディレクトリで makedirs が呼ばれること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("sub\\dir\\trace.log"), open_flags_default()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 呼び出し元のパス文字列でファイルが開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("sub\\dir\\trace.log", 0, 0, 0);

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}
#endif /* PLATFORM_WINDOWS */

// 単一プロセス モード (flags 0) ではプロセス間ロックも同一性チェックも使わないことの確認
TEST_F(trace_fileTest, test_single_mode_does_not_use_interprocess_lock_or_identity_check)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - ロック ファイルが開かれないこと。
    EXPECT_CALL(mock_, com_util_file_get_path_id(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - 同一性チェックが行われないこと。
    EXPECT_CALL(mock_, com_util_file_get_id(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - ハンドル同一性の取得が行われないこと。

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - flags なしの create を呼ぶ。
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "single"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。
    EXPECT_EQ(0, result);                                // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードの create がロック ファイルを開き、共有書き込みフラグでファイルを開くことの確認
TEST_F(trace_fileTest, test_create_shared_opens_lock_file)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 共有書き込みフラグでファイルが開かれること。
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(StrEq("trace.log.lock"), _))
        .Times(1); // [Pre-Assert確認_正常系] - "<path>.lock" でプロセス間ロックが開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));
    EXPECT_CALL(mock_, com_util_interprocess_lock_destroy(_))
        .Times(1); // [Pre-Assert確認_正常系] - dispose でプロセス間ロックが破棄されること。

    // Act
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED); // [手順] - 共有モードで create を呼ぶ。

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードでプロセス間ロックのオープンに失敗した場合に create が NULL を返すことの確認
TEST_F(trace_fileTest, test_create_shared_returns_null_when_lock_open_fails)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(StrEq("trace.log.lock"), _))
        .WillOnce(Return(COM_UTIL_SYNC_SYSTEM_ERROR)); // [Pre-Assert確認_異常系] - ロックのオープンが失敗すること。
    EXPECT_CALL(mock_, com_util_file_close(_))
        .Times(AtLeast(1)); // [Pre-Assert確認_異常系] - オープン済みファイルが閉じられること。

    // Act
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED); // [手順] - 共有モードで create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL, handle); // [確認_異常系] - NULL が返ること。
}

// 負の flags では create が NULL を返すことの確認
TEST_F(trace_fileTest, test_create_returns_null_for_negative_flags)
{
    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, -1); // [手順] - 負の flags で create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL, handle); // [確認_異常系] - NULL が返ること。
}

// 他プロセスのローテーションで path の実体が変わった場合に書き込み前に開き直すことの確認
TEST_F(trace_fileTest, test_shared_write_reopens_after_external_rotation)
{
    // Arrange
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _))
        .WillOnce(
            [](const char *, com_util_file_id *id_out)
            {
                set_file_id(id_out, kDefaultFileIndex + 1); // 別実体を示す
                return 0;
            });                                          // [Pre-Assert確認_正常系] - path が別実体を指していること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1); // [Pre-Assert確認_正常系] - 旧ハンドルが閉じられること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 新しい path が開き直されること。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(Return(0));                            // [Pre-Assert確認_正常系] - 開き直し後に書き込まれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1); // dispose 分

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "after rotate"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードの開き直しでもファイル オープンがリトライされることの確認
TEST_F(trace_fileTest, test_shared_write_reopen_retries_file_open_after_external_rotation)
{
    // Arrange
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _))
        .WillOnce(
            [](const char *, com_util_file_id *id_out)
            {
                set_file_id(id_out, kDefaultFileIndex + 1);
                return 0;
            });                                          // [Pre-Assert確認_正常系] - path が別実体を指していること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1); // [Pre-Assert確認_正常系] - 旧ハンドルが閉じられること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared()))
        .WillOnce(Return(-1)); // [Pre-Assert確認_正常系] - 開き直しの初回 open が失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - 3 秒相当の待機を行うこと。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - リトライで開き直しが成功すること。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(Return(0));                            // [Pre-Assert確認_正常系] - 開き直し後に書き込まれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1); // dispose 分

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "after rotate"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードで実サイズが閾値以上のときプロセス間ロック下でローテーションすることの確認
TEST_F(trace_fileTest, test_shared_write_rotates_under_interprocess_lock)
{
    // Arrange
    InSequence seq;

    // create: オープン → サイズ取得 → 同一性キャッシュ → ロック ファイル オープン
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared())).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_get_id(_, _)).Times(1);
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(StrEq("trace.log.lock"), _)).Times(1);

    // write: 同一性チェック → 書き込み → 実サイズ超過
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _)).Times(1);
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 書き込みが成功すること。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_正常系] - 実サイズが閾値以上であること。

    // プロセス間ロック下の再確認 → ローテーション
    EXPECT_CALL(mock_, com_util_interprocess_lock_lock(_, _))
        .WillOnce(
            Return(COM_UTIL_SYNC_OK)); // [Pre-Assert確認_正常系] - ローテーション前にプロセス間ロックを取得すること。
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _)).Times(1);
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_正常系] - ロック下で実サイズを再確認すること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1);
    EXPECT_CALL(mock_, com_util_remove(StrEq("trace.log.2"))).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log.1"), StrEq("trace.log.2"))).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log"), StrEq("trace.log.1"))).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 切り詰めずに追記モードで開き直すこと。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_get_id(_, _)).Times(1);
    EXPECT_CALL(mock_, com_util_interprocess_lock_unlock(_))
        .WillOnce(Return(COM_UTIL_SYNC_OK)); // [Pre-Assert確認_正常系] - ローテーション後にロックを解放すること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1); // dispose 分

    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 1, 2, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "rotate me"); // [手順] - 上限 1 byte のファイルへ 1 行書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込み後にローテーションが完了すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// ロック下の再確認で他プロセスのローテーション済みを検知した場合に開き直すだけになることの確認
TEST_F(trace_fileTest, test_shared_write_skips_rotate_when_other_process_already_rotated)
{
    // Arrange
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 1, 2, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_rename(_, _)).Times(0); // [Pre-Assert確認_正常系] - リネームは行われないこと。

    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _)).Times(1); // 既定動作 (一致) を使う
    EXPECT_CALL(mock_, com_util_file_write(_, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_正常系] - 実サイズが閾値以上であること。
    EXPECT_CALL(mock_, com_util_interprocess_lock_lock(_, _)).WillOnce(Return(COM_UTIL_SYNC_OK));
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _))
        .WillOnce(
            [](const char *, com_util_file_id *id_out)
            {
                set_file_id(id_out, kDefaultFileIndex + 1); // 他プロセスがローテーション済み
                return 0;
            }); // [Pre-Assert確認_正常系] - ロック下の再確認で別実体を検知すること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1);
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_shared()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 開き直しのみ行われること。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 0;
                return 0;
            });                                              // 開き直し時の初期サイズ取得
    EXPECT_CALL(mock_, com_util_file_get_id(_, _)).Times(1); // 開き直し時の同一性キャッシュ
    EXPECT_CALL(mock_, com_util_interprocess_lock_unlock(_)).WillOnce(Return(COM_UTIL_SYNC_OK));
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(1); // dispose 分

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "already rotated"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// プロセス間ロックの取得がタイムアウトした場合にローテーションを見送ることの確認
TEST_F(trace_fileTest, test_shared_write_skips_rotate_on_lock_timeout)
{
    // Arrange
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 1, 2, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_異常系] - 実サイズが閾値以上であること。
    EXPECT_CALL(mock_, com_util_interprocess_lock_lock(_, _))
        .WillOnce(Return(COM_UTIL_SYNC_TIMEOUT));       // [Pre-Assert確認_異常系] - ロック取得がタイムアウトすること。
    EXPECT_CALL(mock_, com_util_rename(_, _)).Times(0); // [Pre-Assert確認_異常系] - リネームは行われないこと。
    EXPECT_CALL(mock_, com_util_interprocess_lock_unlock(_))
        .Times(0); // [Pre-Assert確認_異常系] - ロック解放は呼ばれないこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "lock busy"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_異常系] - ローテーションを見送っても書き込みは成功扱いであること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}
