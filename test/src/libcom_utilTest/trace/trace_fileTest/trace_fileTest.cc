#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/trace/trace_file.h>
#include <com_util/crt/file.h>
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

static com_util_realtime_timestamp_t make_fixed_timestamp(void)
{
    com_util_realtime_timestamp_t timestamp;
    timestamp.tv_sec = 1714100645LL;
    timestamp.tv_nsec = 678000000;
    timestamp.reserved = 0;
    return timestamp;
}

static uint32_t open_flags_default(void)
{
    return COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
           COM_UTIL_FILE_OPEN_WRITE_THROUGH | COM_UTIL_FILE_OPEN_SHARE_READ |
           COM_UTIL_FILE_OPEN_SHARE_DELETE;
}

static uint32_t open_flags_truncate(void)
{
    return COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE |
           COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH |
           COM_UTIL_FILE_OPEN_SHARE_READ | COM_UTIL_FILE_OPEN_SHARE_DELETE;
}

} // namespace

class trace_fileTest : public Test
{
protected:
    NiceMock<Mock_com_util> mock_;

    void SetUp() override
    {
        ON_CALL(mock_, com_util_get_realtime_deadline_ms(_, _))
            .WillByDefault([](uint64_t, struct timespec *abs_timeout) {
                set_valid_deadline(abs_timeout);
            });
        ON_CALL(mock_, com_util_get_realtime(_, _))
            .WillByDefault([](int64_t *tv_sec, int32_t *tv_nsec) {
                set_fixed_realtime(tv_sec, tv_nsec);
            });
        ON_CALL(mock_, com_util_format_realtime_iso8601_local(_, _, _, _))
            .WillByDefault([](char *buf, size_t buf_size, int64_t, int32_t) {
                snprintf(buf, buf_size, "%s", "2026-04-26T03:04:05.678+09:00");
                return 0;
            });
        ON_CALL(mock_, com_util_file_open(_, _, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_file_get_size(_, _))
            .WillByDefault([](com_util_file_t *, size_t *size_out) {
                *size_out = 0;
                return 0;
            });
        ON_CALL(mock_, com_util_file_write(_, _, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_file_close(_))
            .WillByDefault(Return());
        ON_CALL(mock_, com_util_remove(_))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_rename(_, _))
            .WillByDefault(Return(0));
    }
};

// NULL path では create が失敗することの確認
TEST_F(trace_fileTest, test_create_returns_null_for_null_path)
{
    // Act
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create(NULL, 0, 0); // [手順] - NULL path で create を呼ぶ。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink_t *)NULL, handle); // [確認_異常系] - NULL が返ること。
}

// create が既定 open flags でファイルを開くことの確認
TEST_F(trace_fileTest, test_create_opens_file_with_default_flags)
{
    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 既定 open flags でファイルを開くこと。
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce([](com_util_file_t *, size_t *size_out) {
            *size_out = 123;
            return 0;
        }); // [Pre-Assert確認_正常系] - 既存サイズ取得が 1 回呼ばれること。
    EXPECT_CALL(mock_, com_util_file_close(_)).Times(AtLeast(1));

    // Act
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 0, 0); // [手順] - 既定値で create を呼ぶ。

    // Assert
    EXPECT_NE((com_util_trace_file_sink_t *)NULL, handle); // [確認_正常系] - ハンドルが生成されること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// INFO 行が固定タイムスタンプと I marker で書き込まれることの確認
TEST_F(trace_fileTest, test_write_formats_info_line)
{
    // Arrange
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 0, 0);
    ASSERT_NE((com_util_trace_file_sink_t *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce([](com_util_file_t *, const void *buf, size_t len) {
            std::string actual((const char *)buf, len);
            EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I hello\n", actual);
            return 0;
        }); // [Pre-Assert確認_正常系] - INFO 行が期待フォーマットで書き込まれること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "hello"); // [手順] - INFO 行を書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// DEBUG 行が D marker で書き込まれることの確認
TEST_F(trace_fileTest, test_write_formats_debug_marker)
{
    // Arrange
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 0, 0);
    ASSERT_NE((com_util_trace_file_sink_t *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce([](com_util_file_t *, const void *buf, size_t len) {
            std::string actual((const char *)buf, len);
            EXPECT_EQ("2026-04-26T03:04:05.678+09:00 D debug line\n", actual);
            return 0;
        }); // [Pre-Assert確認_正常系] - DEBUG 行が D marker で書き込まれること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL, "debug line"); // [手順] - DEBUG 行を書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 明示タイムスタンプ指定時に内部の現在時刻取得を行わずに書き込むことの確認
TEST_F(trace_fileTest, test_write_uses_explicit_timestamp_without_internal_clock)
{
    // Arrange
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 0, 0);
    com_util_realtime_timestamp_t timestamp = make_fixed_timestamp();
    ASSERT_NE((com_util_trace_file_sink_t *)NULL, handle);

    EXPECT_CALL(mock_, com_util_get_realtime(_, _)).Times(0); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定時は現在時刻を取得しないこと。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce([](com_util_file_t *, const void *buf, size_t len) {
            std::string actual((const char *)buf, len);
            EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I explicit hello\n", actual);
            return 0;
        }); // [Pre-Assert確認_正常系] - 明示タイムスタンプがそのまま書式化されること。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp, "explicit hello"); // [手順] - 明示タイムスタンプ付きで書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 明示タイムスタンプ付き書き込みが成功すること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// ファイル書き込み失敗時に -1 が返ることの確認
TEST_F(trace_fileTest, test_write_returns_minus_one_on_file_error)
{
    // Arrange
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 0, 0);
    ASSERT_NE((com_util_trace_file_sink_t *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce(Return(-1)); // [Pre-Assert確認_異常系] - 低レベル書き込みが -1 を返すこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "write error"); // [手順] - 書き込み失敗を発生させる。

    // Assert
    EXPECT_EQ(-1, result); // [確認_異常系] - com_util_trace_file_sink_write が -1 を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 不正な明示タイムスタンプ指定時に現在時刻へ代替して書き込みつつ -1 を返すことの確認
TEST_F(trace_fileTest, test_write_falls_back_from_invalid_explicit_timestamp)
{
    // Arrange
    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 0, 0);
    com_util_realtime_timestamp_t invalid_timestamp = {1714100645LL, 1000000000, 0};
    ASSERT_NE((com_util_trace_file_sink_t *)NULL, handle);

    EXPECT_CALL(mock_, com_util_get_realtime(_, _)).Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
    EXPECT_CALL(mock_, com_util_file_write(_, _, _))
        .WillOnce([](com_util_file_t *, const void *buf, size_t len) {
            std::string actual((const char *)buf, len);
            EXPECT_EQ("2026-04-26T03:04:05.678+09:00 I invalid\n", actual);
            return 0;
        }); // [Pre-Assert確認_異常系] - 代替時刻で低レベル書き込みを行うこと。

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp, "invalid"); // [手順] - 不正タイムスタンプで書き込む。

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

    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("trace.log"), open_flags_default()))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_file_get_size(_, _))
        .WillOnce([](com_util_file_t *, size_t *size_out) {
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

    com_util_trace_file_sink_t *handle = com_util_trace_file_sink_create("trace.log", 1, 2);
    ASSERT_NE((com_util_trace_file_sink_t *)NULL, handle);

    // Act
    int result = com_util_trace_file_sink_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "rotate me"); // [手順] - 上限 1 byte のファイルへ 1 行書き込む。

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
