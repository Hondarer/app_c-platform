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

static void set_fixed_realtime(com_util_timespec *ts)
{
    ts->tv_sec = 1714100645LL;
    ts->tv_nsec = 678000000;
}

static com_util_timespec make_fixed_timestamp(void)
{
    com_util_timespec timestamp;
    timestamp.tv_sec = 1714100645LL;
    timestamp.tv_nsec = 678000000;
    return timestamp;
}

static uint32_t open_flags_default(void)
{
    return COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH;
}

static uint32_t open_flags_truncate(void)
{
    return open_flags_default() | COM_UTIL_FILE_OPEN_TRUNCATE;
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
        ON_CALL(mock_, com_util_get_realtime(_)).WillByDefault([](com_util_timespec *ts) { set_fixed_realtime(ts); });
        ON_CALL(mock_, com_util_format_realtime_iso8601_local(_, _, _))
            .WillByDefault(
                [](char *buf, size_t buf_size, const com_util_timespec *)
                {
                    snprintf(buf, buf_size, "%s", "2026-04-26T03:04:05.678+09:00");
                    return 0;
                });
        ON_CALL(mock_, com_util_file_open(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_file_get_size(_, _, _))
            .WillByDefault(
                [](const com_util_file *, size_t *size_out, com_util_error *)
                {
                    *size_out = 0;
                    return 0;
                });
        ON_CALL(mock_, com_util_file_write(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_file_close(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_remove(_, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_rename(_, _, _)).WillByDefault(Return(0));

        // 共有モード用の既定動作: ハンドルとパスの同一性は常に一致させる
        ON_CALL(mock_, com_util_file_get_id(_, _, _))
            .WillByDefault(
                [](const com_util_file *, com_util_file_id *id_out, com_util_error *)
                {
                    set_file_id(id_out, kDefaultFileIndex);
                    return 0;
                });
        ON_CALL(mock_, com_util_file_get_path_id(_, _, _))
            .WillByDefault(
                [](const char *, com_util_file_id *id_out, com_util_error *)
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
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_, com_util_interprocess_lock_try_lock(_)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_interprocess_lock_unlock(_)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_interprocess_lock_destroy(_)).WillByDefault(Return());
    }
};

// NULL path では create が失敗することの確認
TEST_F(trace_fileTest, test_create_returns_null_for_null_path)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create(NULL, 0, 0, 0); // [手順] - NULL path で create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              handle); // [確認_異常系] - com_util_trace_file_sink_create の戻り値が NULL であること。
}

// create が既定 open flags でファイルを開くことの確認
TEST_F(trace_fileTest, test_create_opens_file_with_default_flags)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 既定 open flags でファイルを開くこと。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 123;
                return 0;
            }); // [Pre-Assert確認_正常系] - 既存サイズ取得が 1 回呼ばれること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));

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
    // Arrange

    // Pre-Assert
    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_正常系] - 初回のファイル オープンが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - 3 秒相当の待機を行うこと。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - リトライでファイル オープンが成功すること。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));

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
    // Arrange

    // Pre-Assert
    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 初回のファイル オープンが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_異常系] - 1 回目のリトライ前に待機すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 1 回目のリトライが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_異常系] - 2 回目のリトライ前に待機すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 2 回目のリトライが失敗すること。
    EXPECT_CALL(mock_, com_util_sleep_ms(3000))
        .WillOnce(Return()); // [Pre-Assert確認_異常系] - 3 回目のリトライ前に待機すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 3 回目のリトライが失敗すること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - open 失敗を継続させる。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              handle); // [確認_異常系] - com_util_trace_file_sink_create の戻り値が NULL であること。
}

// INFO 行が固定タイムスタンプと I marker で書き込まれることの確認
TEST_F(trace_fileTest, test_write_formats_info_line)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len, com_util_error *)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I hello\n", actual);
                return 0;
            }); // [Pre-Assert確認_正常系] - INFO 行が期待フォーマットで書き込まれること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "hello"); // [手順] - INFO 行を書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、書き込みが成功したと判断できること。

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
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len, com_util_error *)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 D debug line\n", actual);
                return 0;
            }); // [Pre-Assert確認_正常系] - DEBUG 行が D marker で書き込まれること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL,
                                                "debug line"); // [手順] - DEBUG 行を書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 明示タイムスタンプ指定時に内部の現在時刻取得を行わずに書き込むことの確認
TEST_F(trace_fileTest, test_write_uses_explicit_timestamp_without_internal_clock)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    com_util_timespec timestamp = make_fixed_timestamp();
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    EXPECT_CALL(mock_, com_util_get_realtime(_))
        .Times(0); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定時は現在時刻を取得しないこと。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len, com_util_error *)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I explicit hello\n", actual);
                return 0;
            }); // [Pre-Assert確認_正常系] - 明示タイムスタンプがそのまま書式化されること。

    // Pre-Assert

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                                                "explicit hello"); // [手順] - 明示タイムスタンプ付きで書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、明示タイムスタンプ付き書き込みが成功したと判断できること。

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
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 低レベル書き込みが -1 を返すこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "write error"); // [手順] - 書き込み失敗を発生させる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_trace_file_sink_write が COM_UTIL_ERR_UNKNOWN を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 不正な明示タイムスタンプ指定時に現在時刻へ代替して書き込みつつ -1 を返すことの確認
TEST_F(trace_fileTest, test_write_falls_back_from_invalid_explicit_timestamp)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    com_util_timespec invalid_timestamp = {1714100645LL, 1000000000};
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    EXPECT_CALL(mock_, com_util_get_realtime(_))
        .Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len, com_util_error *)
            {
                std::string actual((const char *)buf, len);
                EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I invalid\n", actual);
                return 0;
            }); // [Pre-Assert確認_異常系] - 代替時刻で低レベル書き込みを行うこと。

    // Pre-Assert

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp,
                                                "invalid"); // [手順] - 不正タイムスタンプで書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, result); // [確認_異常系] - 代替出力後も COM_UTIL_ERR_UNKNOWN を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// サイズ上限超過時にローテーションが実行されることの確認
TEST_F(trace_fileTest, test_write_rotates_when_size_limit_is_reached)
{
    // Arrange
    InSequence seq;

    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 元ファイルへの書き込みが成功すること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1);
    EXPECT_CALL(mock_, com_util_remove(StrEq("trace.log.2"), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 最古世代ファイル削除が 1 回呼ばれること。
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log.1"), StrEq("trace.log.2"), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 旧 .1 が .2 へ順送りされること。
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log"), StrEq("trace.log.1"), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 現在ファイルが .1 へリネームされること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_truncate(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 新規世代ファイルが truncate 付きで開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1);

    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 1, 2, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "rotate me"); // [手順] - 上限 1 byte のファイルへ 1 行書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - 書き込み後にローテーションが完了すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 単一プロセス モードのローテーション後にファイル オープンが失敗しても再試行しないことの確認
TEST_F(trace_fileTest, test_write_does_not_retry_open_after_rotation)
{
    // Arrange
    {
        InSequence seq;

        EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _)).WillOnce(Return(0));
        EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
            .WillOnce(
                [](const com_util_file *, size_t *size_out, com_util_error *)
                {
                    *size_out = 0;
                    return 0;
                });
        EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
            .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - ローテーション前の書き込みが成功すること。
        EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1);
        EXPECT_CALL(mock_, com_util_remove(StrEq("trace.log.2"), _)).WillOnce(Return(0));
        EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log.1"), StrEq("trace.log.2"), _)).WillOnce(Return(0));
        EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log"), StrEq("trace.log.1"), _)).WillOnce(Return(0));
        EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_truncate(), _))
            .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - ローテーション後のファイル オープンが失敗すること。
        EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1);
    }

    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("trace.log", 1, 2, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_sleep_ms(_))
        .Times(0); // [Pre-Assert確認_異常系] - ローテーション後のオープン失敗時に待機しないこと。

    // Act
    int result = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "rotate once"); // [手順] - 上限 1 byte のファイルへ書き込み、ローテーション後の open を失敗させる。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_異常系] - com_util_trace_file_sink_write は完了済みの書き込み結果として COM_UTIL_OK を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// dispose が NULL ハンドルでも安全であることの確認
TEST_F(trace_fileTest, test_dispose_with_null_handle_is_safe)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_trace_file_sink_dispose(NULL); // [手順] - NULL ハンドルで dispose を呼び出す。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}

// パスに区切り文字が含まれる場合に makedirs が親ディレクトリ パスで呼ばれることの確認
TEST_F(trace_fileTest, test_create_calls_makedirs_for_path_with_separator)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_makedirs(StrEq("sub"), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 親ディレクトリ "sub" で makedirs が呼ばれること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("sub/trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 親ディレクトリ生成後にファイルが開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));

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
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_makedirs(StrEq("sub/dir"), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 正規化した親ディレクトリで makedirs が呼ばれること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("sub\\dir\\trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 呼び出し元のパス文字列でファイルが開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));

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
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - ロック ファイルが開かれないこと。
    EXPECT_CALL(mock_, com_util_file_get_path_id(_, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - 同一性チェックが行われないこと。
    EXPECT_CALL(mock_, com_util_file_get_id(_, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - ハンドル同一性の取得が行われないこと。

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - flags なしの create を呼ぶ。
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "single"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_NE((com_util_trace_file_sink *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードの create がロック ファイルを開くことの確認
TEST_F(trace_fileTest, test_create_shared_opens_lock_file)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - ファイルが開かれること。
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(StrEq("trace.log.lock"), _))
        .Times(1); // [Pre-Assert確認_正常系] - "<path>.lock" でプロセス間ロックが開かれること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));
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
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(StrEq("trace.log.lock"), _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - ロックのオープンが失敗すること。
    EXPECT_CALL(mock_, com_util_file_close(_, _))
        .Times(AtLeast(1)); // [Pre-Assert確認_異常系] - オープン済みファイルが閉じられること。

    // Act
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED); // [手順] - 共有モードで create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              handle); // [確認_異常系] - com_util_trace_file_sink_create の戻り値が NULL であること。
}

// 負の flags では create が NULL を返すことの確認
TEST_F(trace_fileTest, test_create_returns_null_for_negative_flags)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, -1); // [手順] - 負の flags で create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              handle); // [確認_異常系] - com_util_trace_file_sink_create の戻り値が NULL であること。
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
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _, _))
        .WillOnce(
            [](const char *, com_util_file_id *id_out, com_util_error *)
            {
                set_file_id(id_out, kDefaultFileIndex + 1); // 別実体を示す
                return 0;
            });                                             // [Pre-Assert確認_正常系] - path が別実体を指していること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1); // [Pre-Assert確認_正常系] - 旧ハンドルが閉じられること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 新しい path が開き直されること。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(Return(0));                               // [Pre-Assert確認_正常系] - 開き直し後に書き込まれること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1); // dispose 分

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "after rotate"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードの開き直しでファイル オープンに失敗した場合に再試行しないことの確認
TEST_F(trace_fileTest, test_shared_write_reopen_does_not_retry_after_external_rotation)
{
    // Arrange
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    {
        InSequence seq;
        EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _, _))
            .WillOnce(
                [](const char *, com_util_file_id *id_out, com_util_error *)
                {
                    set_file_id(id_out, kDefaultFileIndex + 1);
                    return 0;
                }); // [Pre-Assert確認_異常系] - path が別実体を指していること。
        EXPECT_CALL(mock_, com_util_file_close(_, _))
            .Times(1); // [Pre-Assert確認_異常系] - 旧ハンドルが閉じられること。
        EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
            .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 開き直しのファイル オープンが失敗すること。
        EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1); // dispose 分
    }
    EXPECT_CALL(mock_, com_util_sleep_ms(_))
        .Times(0); // [Pre-Assert確認_異常系] - ファイル オープンの失敗後に待機しないこと。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - ファイルを開き直せない場合は書き込まないこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "after rotate"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        result); // [確認_異常系] - com_util_trace_file_sink_write の戻り値が直ちに COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有モードで実サイズが閾値以上のときプロセス間ロック下でローテーションすることの確認
TEST_F(trace_fileTest, test_shared_write_rotates_under_interprocess_lock)
{
    // Arrange
    InSequence seq;

    // create: オープン → サイズ取得 → 同一性キャッシュ → ロック ファイル オープン
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_get_id(_, _, _)).Times(1);
    EXPECT_CALL(mock_, com_util_interprocess_lock_open(StrEq("trace.log.lock"), _)).Times(1);

    // write: 同一性チェック → 書き込み → 実サイズ超過
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _, _)).Times(1);
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 書き込みが成功すること。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_正常系] - 実サイズが閾値以上であること。

    // プロセス間ロック下の再確認 → ローテーション
    EXPECT_CALL(mock_, com_util_interprocess_lock_try_lock(_))
        .WillOnce(
            Return(COM_UTIL_OK)); // [Pre-Assert確認_正常系] - ローテーション前にプロセス間ロックを即時取得すること。
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _, _)).Times(1);
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_正常系] - ロック下で実サイズを再確認すること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1);
    EXPECT_CALL(mock_, com_util_remove(StrEq("trace.log.2"), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log.1"), StrEq("trace.log.2"), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_rename(StrEq("trace.log"), StrEq("trace.log.1"), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 切り詰めずに追記モードで開き直すこと。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 0;
                return 0;
            });
    EXPECT_CALL(mock_, com_util_file_get_id(_, _, _)).Times(1);
    EXPECT_CALL(mock_, com_util_interprocess_lock_unlock(_))
        .WillOnce(Return(COM_UTIL_OK)); // [Pre-Assert確認_正常系] - ローテーション後にロックを解放すること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1); // dispose 分

    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 1, 2, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "rotate me"); // [手順] - 上限 1 byte のファイルへ 1 行書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - 書き込み後にローテーションが完了すること。

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
    EXPECT_CALL(mock_, com_util_rename(_, _, _)).Times(0); // [Pre-Assert確認_正常系] - リネームは行われないこと。

    InSequence seq;
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _, _)).Times(1); // 既定動作 (一致) を使う
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _)).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_正常系] - 実サイズが閾値以上であること。
    EXPECT_CALL(mock_, com_util_interprocess_lock_try_lock(_)).WillOnce(Return(COM_UTIL_OK));
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("trace.log"), _, _))
        .WillOnce(
            [](const char *, com_util_file_id *id_out, com_util_error *)
            {
                set_file_id(id_out, kDefaultFileIndex + 1); // 他プロセスがローテーション済み
                return 0;
            }); // [Pre-Assert確認_正常系] - ロック下の再確認で別実体を検知すること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1);
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 開き直しのみ行われること。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 0;
                return 0;
            });                                                 // 開き直し時の初期サイズ取得
    EXPECT_CALL(mock_, com_util_file_get_id(_, _, _)).Times(1); // 開き直し時の同一性キャッシュ
    EXPECT_CALL(mock_, com_util_interprocess_lock_unlock(_)).WillOnce(Return(COM_UTIL_OK));
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(1); // dispose 分

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "already rotated"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// プロセス間ロックがビジー状態の場合にローテーションを見送ることの確認
TEST_F(trace_fileTest, test_shared_write_skips_rotate_when_lock_is_busy)
{
    // Arrange
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("trace.log", 1, 2, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 10;
                return 0;
            }); // [Pre-Assert確認_異常系] - 実サイズが閾値以上であること。
    EXPECT_CALL(mock_, com_util_interprocess_lock_try_lock(_))
        .WillOnce(Return(COM_UTIL_ERR_BUSY)); // [Pre-Assert確認_異常系] - プロセス間ロックがビジー状態であること。
    EXPECT_CALL(mock_, com_util_rename(_, _, _)).Times(0); // [Pre-Assert確認_異常系] - リネームは行われないこと。
    EXPECT_CALL(mock_, com_util_interprocess_lock_unlock(_))
        .Times(0); // [Pre-Assert確認_異常系] - ロック解放は呼ばれないこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "lock busy"); // [手順] - 1 行書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_異常系] - ローテーションを見送っても書き込みは成功扱いであること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 同一プロセス内で同一パスの create が同一ハンドルを共有することの確認 (プロセス内調停)
TEST_F(trace_fileTest, test_create_same_path_shares_handle_in_single_process)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - ファイル オープンは 1 回だけ行われること。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink *first =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - 1 回目の create を呼ぶ。
    com_util_trace_file_sink *second =
        com_util_trace_file_sink_create("trace.log", 0, 0, 0); // [手順] - 同一パスで 2 回目の create を呼ぶ。

    // Assert
    ASSERT_NE((com_util_trace_file_sink *)NULL, first); // [確認_正常系] - 1 回目のハンドルが生成されること。
    EXPECT_EQ(
        first,
        second); // [確認_正常系] - com_util_trace_file_sink_create の戻り値として、2 回目は同一ハンドルが返ること。

    // Cleanup
    com_util_trace_file_sink_dispose(second);
    com_util_trace_file_sink_dispose(first);
}

// 参照カウントにより最後の dispose までハンドルが有効であることの確認
TEST_F(trace_fileTest, test_shared_handle_survives_until_last_dispose)
{
    // Arrange
    com_util_trace_file_sink *first = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    com_util_trace_file_sink *second = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, first);
    ASSERT_EQ(first, second);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(
            [](com_util_file *, const void *buf, size_t len, com_util_error *)
            {
                std::string actual((const char *)buf, len);
                EXPECT_NE(std::string::npos, actual.find("after first dispose"));
                return 0;
            }); // [Pre-Assert確認_正常系] - 1 回目の dispose 後も書き込みできること。

    // Act
    com_util_trace_file_sink_dispose(first); // [手順] - 1 人目の利用者が解放する。
    int result = com_util_trace_file_sink_write(second, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                "after first dispose"); // [手順] - 2 人目の利用者が書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値から、参照が残っている間は書き込みが成功したと判断できること。

    // Cleanup
    com_util_trace_file_sink_dispose(second);
}

// 共有モード設定が一致しない同一パスの create が失敗することの確認
TEST_F(trace_fileTest, test_create_same_path_with_mismatched_shared_flag_returns_null)
{
    // Arrange
    com_util_trace_file_sink *first = com_util_trace_file_sink_create("trace.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, first); // [状態] - 占有モードの sink が存在する。

    // Pre-Assert

    // Act
    com_util_trace_file_sink *second = com_util_trace_file_sink_create(
        "trace.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED); // [手順] - 同一パスを共有モードで create する。

    // Assert
    EXPECT_EQ(
        (com_util_trace_file_sink *)NULL,
        second); // [確認_異常系] - com_util_trace_file_sink_create の戻り値として、モード不一致では NULL が返ること。

    // Cleanup
    com_util_trace_file_sink_dispose(first);
}

// 異なるパスの create は独立したハンドルを生成することの確認
TEST_F(trace_fileTest, test_create_different_paths_returns_distinct_handles)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("first.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 1 つ目のパスでファイルを開くこと。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("second.log"), open_flags_default(), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 2 つ目のパスでファイルを開くこと。
    EXPECT_CALL(mock_, com_util_file_close(_, _)).Times(AtLeast(2));

    // Act
    com_util_trace_file_sink *first =
        com_util_trace_file_sink_create("first.log", 0, 0, 0); // [手順] - 1 つ目のパスで create を呼ぶ。
    com_util_trace_file_sink *second =
        com_util_trace_file_sink_create("second.log", 0, 0, 0); // [手順] - 2 つ目のパスで create を呼ぶ。

    // Assert
    ASSERT_NE((com_util_trace_file_sink *)NULL, first);  // [確認_正常系] - 1 つ目のハンドルが生成されること。
    ASSERT_NE((com_util_trace_file_sink *)NULL, second); // [確認_正常系] - 2 つ目のハンドルが生成されること。
    EXPECT_NE(first, second);                            // [確認_正常系] - 異なるパスでは別ハンドルになること。

    // Cleanup
    com_util_trace_file_sink_dispose(first);
    com_util_trace_file_sink_dispose(second);
}
