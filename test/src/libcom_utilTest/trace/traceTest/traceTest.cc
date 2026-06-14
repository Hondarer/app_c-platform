#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/trace/tracer.h>
#include <com_util/trace/trace_file.h>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdint>

#include <com_util/trace/tracer_internal.h>

#if defined(PLATFORM_LINUX)
    #include <syslog.h>
#endif

using testing::_;
using testing::AtLeast;
using testing::HasSubstr;
using testing::MatchesRegex;
using testing::NiceMock;
using testing::NotNull;
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

} // namespace

class traceTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_;
    com_util_trace_file_sink *file_handle_ =
        reinterpret_cast<com_util_trace_file_sink *>(static_cast<uintptr_t>(0x2200));

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink *os_handle_ = reinterpret_cast<com_util_syslog_sink *>(static_cast<uintptr_t>(0x1100));
#elif defined(PLATFORM_WINDOWS)
    com_util_etw_provider *os_handle_ = reinterpret_cast<com_util_etw_provider *>(static_cast<uintptr_t>(0x1100));
    com_util_eventlog_sink *eventlog_handle_ =
        reinterpret_cast<com_util_eventlog_sink *>(static_cast<uintptr_t>(0x1300));
#endif

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
        ON_CALL(mock_, com_util_trace_file_sink_create(_, _, _, _)).WillByDefault(Return(file_handle_));
        ON_CALL(mock_, com_util_trace_file_sink_write(_, _, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_trace_file_sink_dispose(_)).WillByDefault(Return());
        // デフォルト パス解決を決定的にするため、実行ファイル パスを固定する
        ON_CALL(mock_, com_util_process_get_executable_path(_, _))
            .WillByDefault(
                [](char *out_path, size_t out_path_sz)
                {
                    snprintf(out_path, out_path_sz, "%s", "/opt/bin/myapp");
                    return 0;
                });

#if defined(PLATFORM_LINUX)
        ON_CALL(mock_, com_util_syslog_sink_create(_, _)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_, com_util_syslog_sink_write(_, _, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_syslog_sink_rename(_, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_syslog_sink_dispose(_)).WillByDefault(Return());
#elif defined(PLATFORM_WINDOWS)
        ON_CALL(mock_, com_util_etw_provider_create(_)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_, com_util_etw_provider_write(_, _, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_etw_provider_dispose(_)).WillByDefault(Return());
        ON_CALL(mock_, com_util_eventlog_sink_create(_)).WillByDefault(Return(eventlog_handle_));
        ON_CALL(mock_, com_util_eventlog_sink_write(_, _, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_, com_util_eventlog_sink_dispose(_)).WillByDefault(Return());
#endif
    }

    com_util_tracer *create_logger()
    {
        com_util_tracer *handle = com_util_tracer_create();
        EXPECT_NE((com_util_tracer *)NULL, handle);
        return handle;
    }
};

// 初期化と破棄が成功することの確認
TEST_F(traceTest, test_init_and_dispose)
{
    // Arrange & Act
    com_util_tracer *handle = create_logger(); // [手順] - トレース ハンドルを初期化する。

    // Assert
    EXPECT_EQ((size_t)1, trace_registry_count()); // [確認_正常系] - registry に 1 件登録されること。

    // Cleanup
    com_util_tracer_dispose(handle);
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_正常系] - dispose 後に registry が空になること。
}

// get_state が create/start/stop の状態遷移を返すことの確認
TEST_F(traceTest, test_get_state_reports_stopped_started_stopped)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Act
    com_util_tracer_state_t created_state = com_util_tracer_get_state(handle); // [手順] - create 直後の状態を取得する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    com_util_tracer_state_t started_state = com_util_tracer_get_state(handle); // [手順] - start 後の状態を取得する。
    ASSERT_EQ(0, com_util_tracer_stop(handle));
    com_util_tracer_state_t stopped_state = com_util_tracer_get_state(handle); // [手順] - stop 後の状態を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STOPPED, created_state); // [確認_正常系] - create 直後は stopped を返すこと。
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STARTED, started_state); // [確認_正常系] - start 後は started を返すこと。
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STOPPED, stopped_state); // [確認_正常系] - stop 後は stopped を返すこと。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// get_state が NULL に対して disposed を返すことの確認
TEST_F(traceTest, test_get_state_returns_disposed_for_null)
{
    // Act
    com_util_tracer_state_t state = com_util_tracer_get_state(NULL); // [手順] - NULL ハンドルの状態を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_TRACER_STATE_DISPOSED, state); // [確認_異常系] - NULL では disposed を返すこと。
}

// registry が live handle 数と容量拡張を追跡できることの確認
TEST_F(traceTest, test_registry_tracks_and_expands)
{
    // Arrange
    const size_t create_count = 12; // [状態] - 初期容量 8 を超える 12 個のハンドルを生成する。
    com_util_tracer *handles[create_count] = {};

    // Act
    for (size_t i = 0; i < create_count; i++)
    {
        handles[i] = create_logger(); // [手順] - com_util_tracer_create() を 12 回呼び出す。
    }

    // Assert
    EXPECT_EQ(create_count, trace_registry_count());    // [確認_正常系] - registry 件数が 12 であること。
    EXPECT_GE(trace_registry_capacity(), create_count); // [確認_正常系] - registry 容量が 12 以上へ拡張されること。

    // Cleanup
    for (com_util_tracer *handle : handles)
    {
        com_util_tracer_dispose(handle);
    }
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_正常系] - すべて破棄後に registry が空になること。
}

// started 状態で INFO 出力が OS backend へ送られることの確認
TEST_F(traceTest, test_macro_write_prefixes_source_location)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] macro message")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 公開マクロが source location を付けて backend へ渡すこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] macro message")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 公開マクロが source location を付けて backend へ渡すこと。
#endif

    // Act
    int result = com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                       "macro message"); // [手順] - 公開マクロ経由で INFO を書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 公開マクロ経由の書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 公開マクロが明示タイムスタンプを backend へ渡すことの確認
TEST_F(traceTest, test_macro_write_passes_explicit_timestamp)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_realtime_timestamp timestamp = make_fixed_timestamp();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] explicit macro timestamp")))
        .WillOnce(
            [&timestamp](com_util_syslog_sink *, int, const com_util_realtime_timestamp *actual_timestamp, const char *)
            {
                EXPECT_EQ(timestamp.tv_sec, actual_timestamp->tv_sec);
                EXPECT_EQ(timestamp.tv_nsec, actual_timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - 公開マクロ経由でも明示タイムスタンプがそのまま渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] explicit macro timestamp")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 公開マクロ経由でも ETW backend へメッセージが渡ること。
#endif

    // Act
    int result =
        com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                              "explicit macro timestamp"); // [手順] - 明示タイムスタンプ付きで公開マクロを呼ぶ。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 明示タイムスタンプ付きの公開マクロ呼び出しが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 公開マクロが NULL メッセージでもソース位置だけを backend へ渡すことの確認
TEST_F(traceTest, test_macro_write_with_null_message_emits_source_location_only)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("^\\[traceTest\\.cc:[0-9]+\\]$")))
        .WillOnce(Return(
            0)); // [Pre-Assert確認_正常系] - NULL メッセージでも公開マクロが source location を付けて backend へ渡すこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("^\\[traceTest\\.cc:\\d+\\]$")))
        .WillOnce(Return(
            0)); // [Pre-Assert確認_正常系] - NULL メッセージでも公開マクロが source location だけを ETW backend へ渡すこと。
#endif

    // Act
    int result = com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                       NULL); // [手順] - NULL メッセージで公開マクロを呼ぶ。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - NULL メッセージでも公開マクロ呼び出しが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// started 状態で INFO 出力が OS backend へ送られることの確認
TEST_F(traceTest, test_write_routes_info_to_os_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq("test message")))
        .WillOnce(
            [](com_util_syslog_sink *, int, const com_util_realtime_timestamp *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - INFO が解決済み時刻付きで syslog backend へ 1 回送られること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("test message")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - INFO が ETW backend へ 1 回送られること。
#endif

    // Act
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "test message"); // [手順] - INFO メッセージを書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// ETW トレース (etw_level) と OS トレース (os_level) が独立にゲートされることの確認
TEST_F(traceTest, test_etw_and_os_levels_are_independent)
{
#if defined(PLATFORM_WINDOWS)
    // Arrange: ETW は既定 (VERBOSE) のまま、OS トレース (EventLog) は無効にする。
    com_util_tracer *handle = create_logger();
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_VERBOSE,
              com_util_tracer_get_etw_level(handle)); // [確認_正常系] - ETW は既定で有効 (VERBOSE)。
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert: ETW へは送られるが、EventLog へは送られないこと。
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("only etw"))).WillOnce(Return(0));
    EXPECT_CALL(mock_, com_util_eventlog_sink_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - os_level=NONE のため EventLog へは送られないこと。

    // Act
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "only etw");

    // Assert
    EXPECT_EQ(0, result);

    // Cleanup
    com_util_tracer_dispose(handle);
#else
    // Linux では ETW が存在しないため、etw_level は常に NONE を返し、設定は no-op となる。
    com_util_tracer *handle = create_logger();
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_NONE, com_util_tracer_get_etw_level(handle)); // [確認_正常系] - Linux は常に NONE。
    EXPECT_EQ(0, com_util_tracer_set_etw_level(handle,
                                               COM_UTIL_TRACE_LEVEL_WARNING)); // [確認_正常系] - 設定は no-op で成功。
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_NONE,
              com_util_tracer_get_etw_level(handle)); // [確認_正常系] - 設定後も NONE のまま。
    com_util_tracer_dispose(handle);
#endif
}

#if defined(PLATFORM_WINDOWS)
// OS トレース (os_level) が EventLog backend へ送られることの確認 (Windows)
TEST_F(traceTest, test_write_routes_info_to_eventlog_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert: INFO が EventLog backend へ 1 回送られること。
    EXPECT_CALL(mock_, com_util_eventlog_sink_write(eventlog_handle_, (int)COM_UTIL_TRACE_LEVEL_INFO, NotNull(),
                                                    StrEq("to eventlog")))
        .WillOnce(Return(0));

    // Act
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "to eventlog");

    // Assert
    EXPECT_EQ(0, result);

    // Cleanup
    com_util_tracer_dispose(handle);
}
#endif /* PLATFORM_WINDOWS */

// 明示タイムスタンプ付き INFO 出力が OS backend へ渡ることの確認
TEST_F(traceTest, test_write_routes_explicit_timestamp_to_os_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_realtime_timestamp timestamp = make_fixed_timestamp();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq("explicit timestamp")))
        .WillOnce(
            [&timestamp](com_util_syslog_sink *, int, const com_util_realtime_timestamp *actual_timestamp, const char *)
            {
                EXPECT_EQ(timestamp.tv_sec, actual_timestamp->tv_sec);
                EXPECT_EQ(timestamp.tv_nsec, actual_timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - 明示タイムスタンプが syslog backend へそのまま渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   StrEq("explicit timestamp")))
        .WillOnce(
            Return(0)); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定でも ETW backend へメッセージが渡ること。
#endif

    // Act
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                                        "explicit timestamp"); // [手順] - 明示タイムスタンプ付き INFO を書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// NULL ハンドルと NULL メッセージが安全に無視されることの確認
TEST_F(traceTest, test_write_is_null_safe)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Act
    int null_handle_result =
        _com_util_tracer_write(NULL, COM_UTIL_TRACE_LEVEL_INFO, NULL, "ignored"); // [手順] - NULL ハンドルで書き込む。
    int null_message_result =
        _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, NULL); // [手順] - NULL メッセージで書き込む。

    // Assert
    EXPECT_EQ(0, null_handle_result);  // [確認_異常系] - NULL ハンドルで 0 が返ること。
    EXPECT_EQ(0, null_message_result); // [確認_異常系] - NULL メッセージで 0 が返ること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 1024 バイト超の UTF-8 文字列が安全な境界で切り詰められることの確認
TEST_F(traceTest, test_write_truncates_utf8_boundary)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    char msg[1025];
    unsigned char utf8[] = {0xE3, 0x81, 0x82};
    memset(msg, 'A', 1021);
    memcpy(&msg[1021], utf8, 3);
    msg[1024] = '\0';

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), _))
        .WillOnce(
            [](com_util_syslog_sink *, int, const com_util_realtime_timestamp *timestamp, const char *actual)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                EXPECT_EQ((size_t)1021, strlen(actual));
                EXPECT_EQ(std::string(1021, 'A'), std::string(actual));
                return 0;
            }); // [Pre-Assert確認_正常系] - UTF-8 境界直前の 1021 バイトだけが backend へ渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(), _))
        .WillOnce(
            [](com_util_etw_provider *, int, const char *, const char *actual)
            {
                EXPECT_EQ((size_t)1021, strlen(actual));
                EXPECT_EQ(std::string(1021, 'A'), std::string(actual));
                return 0;
            }); // [Pre-Assert確認_正常系] - UTF-8 境界直前の 1021 バイトだけが backend へ渡ること。
#endif

    // Act
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        msg); // [手順] - UTF-8 境界を跨ぐ長いメッセージを書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 切り詰め後も書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// writef が format 展開後の文字列を backend へ渡すことの確認
TEST_F(traceTest, test_writef_formats_message)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  StrEq("user=alice count=42")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - format 展開後の文字列が時刻付きで backend へ渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("user=alice count=42")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - format 展開後の文字列が backend へ渡ること。
#endif

    // Act
    int result = _com_util_tracer_writef(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "user=%s count=%d", "alice",
                                         42); // [手順] - writef を呼び出す。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// HEX 書き込みがラベル付きテキストへ変換されることの確認
TEST_F(traceTest, test_write_hex_formats_payload)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));
    unsigned char data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  StrEq("Data: 48 65 6C 6C 6F")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - HEX テキストが時刻付きで backend へ渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("Data: 48 65 6C 6C 6F")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - HEX テキストが backend へ渡ること。
#endif

    // Act
    int result = _com_util_tracer_write_hex(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data),
                                            "Data"); // [手順] - ラベル付き HEX 書き込みを行う。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// HEX 書き込みでデータ本体を出力できない残り長の場合に省略記号だけが付与されることの確認
TEST_F(traceTest, test_write_hex_appends_ellipsis_when_only_ellipsis_fits)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));
    unsigned char data[] = {0x48, 0x69};
    std::string label(COM_UTIL_TRACER_MESSAGE_MAX_BYTES - 6, 'L');
    std::string expected = label + ": ...";

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  StrEq(expected.c_str())))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - HEX データ本体なしで省略記号だけが backend へ渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq(expected.c_str())))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - HEX データ本体なしで省略記号だけが backend へ渡ること。
#endif

    // Act
    int result =
        _com_util_tracer_write_hex(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data),
                                   label.c_str()); // [手順] - 省略記号だけが収まるラベル長で HEX 書き込みを行う。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// started 中は設定関数が失敗することの確認
TEST_F(traceTest, test_config_fails_when_started)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Act
    int name_result = com_util_tracer_set_name(handle, "running", 0); // [手順] - started 中に set_name を呼ぶ。
    int os_result = com_util_tracer_set_os_level(
        handle, COM_UTIL_TRACE_LEVEL_VERBOSE); // [手順] - started 中に set_os_level を呼ぶ。
    int file_result = com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                     0); // [手順] - started 中に set_file_level を呼ぶ。

    // Assert
    EXPECT_EQ(-1, name_result); // [確認_異常系] - started 中の set_name が失敗すること。
    EXPECT_EQ(-1, os_result);   // [確認_異常系] - started 中の set_os_level が失敗すること。
    EXPECT_EQ(-1, file_result); // [確認_異常系] - started 中の set_file_level が失敗すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// stopped 中の file level 設定が file backend 作成と書き込みへ反映されることの確認
TEST_F(traceTest, test_file_level_routes_to_file_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - file sink が指定パスで初期化されること。
    EXPECT_CALL(mock_,
                com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("file info")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - INFO が file sink へ 1 回送られること。

    // Act
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                0)); // [手順] - file trace を有効化する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "file info"); // [手順] - INFO メッセージを書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - file backend 経由の書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_file_level の flags が start 時の file sink 生成へ引き渡されることの確認
TEST_F(traceTest, test_set_file_level_passes_flags_to_file_sink)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - flags がそのまま create へ渡ること。

    // Act
    int result = com_util_tracer_set_file_level(
        handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
        COM_UTIL_TRACE_FILE_SINK_SHARED); // [手順] - 共有フラグ付きで set_file_level を呼ぶ。
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start で file sink を生成させる。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 設定が成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_file_level が sink を生成せず start まで遅延されることの確認
TEST_F(traceTest, test_set_file_level_defers_sink_creation_until_start)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Act & Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - set_file_level の時点では file sink が生成されないこと。
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                0)); // [手順] - stopped 中に set_file_level を呼ぶ。
    ::testing::Mock::VerifyAndClearExpectations(&mock_);

    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [確認_正常系] - start 時に設定済みパスで file sink が生成されること。
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start で file sink を生成させる。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 明示タイムスタンプ指定時に file backend と stderr が同じ時刻を使うことの確認
TEST_F(traceTest, test_explicit_timestamp_is_shared_by_file_and_stderr)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_realtime_timestamp timestamp = make_fixed_timestamp();

    EXPECT_CALL(mock_, com_util_get_realtime(_, _))
        .Times(0); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定時は現在時刻取得を行わないこと。
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - file sink が初期化されること。
    EXPECT_CALL(
        mock_, com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("explicit ts")))
        .WillOnce(
            [](com_util_trace_file_sink *, int, const com_util_realtime_timestamp *actual, const char *)
            {
                EXPECT_NE(nullptr, actual);
                EXPECT_EQ(1714100645LL, actual->tv_sec);
                EXPECT_EQ(678000000, actual->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - file sink へ明示タイムスタンプがそのまま渡ること。

    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0));
    ASSERT_EQ(0, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Act
    testing::internal::CaptureStderr();
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                                        "explicit ts"); // [手順] - 明示タイムスタンプ付きで書き込む。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 明示タイムスタンプ付き書き込みが成功すること。
    EXPECT_NE(
        std::string::npos,
        captured.find(
            "2026-04-26T03:04:05.678+09:00 I explicit ts")); // [確認_正常系] - stderr でも同じ時刻文字列が使われること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// file level NONE でファイル トレースが無効化されることの確認
TEST_F(traceTest, test_file_level_none_disables_file_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, NULL, COM_UTIL_TRACE_LEVEL_NONE, 0, 0,
                                                0)); // [手順] - level NONE でファイル トレースを無効化する。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - file sink が生成されないこと。
    EXPECT_CALL(mock_, com_util_trace_file_sink_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - file sink へは 1 回も送られないこと。

    // Act
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_CRITICAL, NULL,
                                        "no file output"); // [手順] - ファイル トレース無効のまま書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - file 無効でもエラーにならないこと。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_name が識別子付き名称を反映することの確認
TEST_F(traceTest, test_set_name_with_identifier_updates_backend_name)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_rename(os_handle_, StrEq("worker-2")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - syslog ident が worker-2 へ更新されること。
#endif

    // Act
    int result = com_util_tracer_set_name(handle, "worker", 2); // [手順] - identifier = 2 で set_name を呼ぶ。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - set_name が成功すること。

#if defined(PLATFORM_WINDOWS)
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, StrEq("worker-2"), StrEq("running as worker-2")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - ETW サービス名が worker-2 に更新されること。
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "running as worker-2"));
#endif

    // Cleanup
    com_util_tracer_dispose(handle);
}

// COM_UTIL_TRACE_LEVEL_NONE では OS backend が呼ばれないことの確認
TEST_F(traceTest, test_os_level_none_suppresses_output)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - syslog backend が呼ばれないこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_eventlog_sink_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - EventLog backend が呼ばれないこと。
#endif

    // Act
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_CRITICAL, NULL,
                                        "suppressed"); // [手順] - OS level NONE のまま書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - 出力抑止でもエラーにならないこと。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// stderr level DEBUG で V と D の marker が出力されることの確認
TEST_F(traceTest, test_stderr_level_debug_outputs_markers)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_DEBUG));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    // Act
    testing::internal::CaptureStderr();
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_VERBOSE, NULL,
                                        "verbose to stderr")); // [手順] - VERBOSE を stderr へ出力する。
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL,
                                        "debug to stderr")); // [手順] - DEBUG を stderr へ出力する。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_NE(
        std::string::npos,
        captured.find(
            "2026-04-26T03:04:05.678+09:00 V verbose to stderr")); // [確認_正常系] - VERBOSE 行が V で出力されること。
    EXPECT_NE(
        std::string::npos,
        captured.find(
            "2026-04-26T03:04:05.678+09:00 D debug to stderr")); // [確認_正常系] - DEBUG 行が D で出力されること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 不正な明示タイムスタンプ指定時に現在時刻へ代替して各出力先へ書き込みつつ -1 を返すことの確認
TEST_F(traceTest, test_invalid_explicit_timestamp_falls_back_and_returns_minus_one)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_realtime_timestamp invalid_timestamp = {1714100645LL, 1000000000, 0};

    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0));
    ASSERT_EQ(0, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    EXPECT_CALL(mock_, com_util_get_realtime(_, _))
        .Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq("invalid ts")))
        .WillOnce(
            [](com_util_syslog_sink *, int, const com_util_realtime_timestamp *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_異常系] - syslog backend へ代替時刻で渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_, com_util_etw_provider_write(os_handle_, 4, _, StrEq("invalid ts")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - ETW backend への出力は継続すること。
#endif
    EXPECT_CALL(mock_,
                com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("invalid ts")))
        .WillOnce(
            [](com_util_trace_file_sink *, int, const com_util_realtime_timestamp *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_異常系] - file backend へ代替時刻で渡ること。

    // Act
    testing::internal::CaptureStderr();
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp,
                                        "invalid ts"); // [手順] - 不正タイムスタンプで書き込む。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_EQ(-1, result); // [確認_異常系] - 代替出力後も -1 を返すこと。
    EXPECT_NE(std::string::npos,
              captured.find(
                  "2026-04-26T03:04:05.678+09:00 I invalid ts")); // [確認_異常系] - stderr も代替時刻で出力すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 不正な明示タイムスタンプ指定時に write_hex でも現在時刻へ代替して -1 を返すことの確認
TEST_F(traceTest, test_write_hex_invalid_explicit_timestamp_falls_back_and_returns_minus_one)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_realtime_timestamp invalid_timestamp = {1714100645LL, 1000000000, 0};
    unsigned char data[] = {0x48, 0x69};

    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0));
    ASSERT_EQ(0, com_util_tracer_start(handle));

    EXPECT_CALL(mock_, com_util_get_realtime(_, _))
        .Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
    EXPECT_CALL(
        mock_, com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("Data: 48 69")))
        .WillOnce(
            [](com_util_trace_file_sink *, int, const com_util_realtime_timestamp *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_異常系] - HEX 書き込みでも代替時刻が file backend へ渡ること。

    // Act
    int result = _com_util_tracer_write_hex(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp, data, sizeof(data),
                                            "Data"); // [手順] - 不正タイムスタンプ付きで HEX 書き込みを行う。

    // Assert
    EXPECT_EQ(-1, result); // [確認_異常系] - HEX 書き込みでも代替出力後に -1 を返すこと。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// stopped 状態では出力関数が失敗することの確認
TEST_F(traceTest, test_write_fails_when_stopped)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    unsigned char data[] = {0x01, 0x02};

    // Act
    int write_result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                              "stopped message"); // [手順] - stopped 状態で write を呼ぶ。
    int writef_result = _com_util_tracer_writef(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "stopped %s",
                                                "msg"); // [手順] - stopped 状態で writef を呼ぶ。
    int hex_result = _com_util_tracer_write_hex(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data),
                                                "hex"); // [手順] - stopped 状態で write_hex を呼ぶ。
    int hexf_result = _com_util_tracer_write_hexf(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data), "hex %d",
                                                  1); // [手順] - stopped 状態で write_hexf を呼ぶ。

    // Assert
    EXPECT_EQ(-1, write_result);  // [確認_異常系] - stopped 状態の write が失敗すること。
    EXPECT_EQ(-1, writef_result); // [確認_異常系] - stopped 状態の writef が失敗すること。
    EXPECT_EQ(-1, hex_result);    // [確認_異常系] - stopped 状態の write_hex が失敗すること。
    EXPECT_EQ(-1, hexf_result);   // [確認_異常系] - stopped 状態の write_hexf が失敗すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// start と stop の二重呼び出しが冪等であることの確認
TEST_F(traceTest, test_start_and_stop_are_idempotent)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Act
    int first_start = com_util_tracer_start(handle);  // [手順] - 1 回目の start を呼ぶ。
    int second_start = com_util_tracer_start(handle); // [手順] - 2 回目の start を呼ぶ。
    int first_stop = com_util_tracer_stop(handle);    // [手順] - 1 回目の stop を呼ぶ。
    int second_stop = com_util_tracer_stop(handle);   // [手順] - 2 回目の stop を呼ぶ。

    // Assert
    EXPECT_EQ(0, first_start);  // [確認_正常系] - 1 回目の start が成功すること。
    EXPECT_EQ(0, second_start); // [確認_正常系] - 2 回目の start も成功すること。
    EXPECT_EQ(0, first_stop);   // [確認_正常系] - 1 回目の stop が成功すること。
    EXPECT_EQ(0, second_stop);  // [確認_正常系] - 2 回目の stop も成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_file_level 未呼び出しの start でデフォルト パスのファイル トレースが有効になることの確認
TEST_F(traceTest, test_start_creates_default_file_sink)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(
            file_handle_)); // [Pre-Assert確認_正常系] - 実行ファイルのディレクトリ配下の log/<有効名>.log を開くこと。
    EXPECT_CALL(mock_, com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(),
                                                      StrEq("default file")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - デフォルトの file sink へ INFO が送られること。

    // Act
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - 未設定のまま start する。
    int result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "default file"); // [手順] - INFO メッセージを書き込む。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - デフォルトのファイル トレースで書き込みが成功すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_name (インスタンス名とインスタンス識別) がデフォルトのトレースファイル名に影響しないことの確認
TEST_F(traceTest, test_set_name_does_not_affect_default_file_path)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_name(handle, "worker", 3)); // [手順] - インスタンス名を worker-3 に変更する。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(
            file_handle_)); // [Pre-Assert確認_正常系] - set_name に関わらずプロセス名のデフォルト パスを開くこと。

    // Act & Assert
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start でデフォルト パスを解決させる。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_file_name のファイル名とファイル識別がデフォルト パスへ反映されることの確認
TEST_F(traceTest, test_set_file_name_reflects_to_default_file_path)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_name(handle, "custom", 2)); // [手順] - ファイル名を custom-2 に変更する。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("/opt/bin/log/custom-2.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - custom-2 のデフォルト パスを開くこと。

    // Act & Assert
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start でデフォルト パスを解決させる。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// set_file_name(NULL, 0) でデフォルト (プロセス名、識別なし) に戻ることの確認
TEST_F(traceTest, test_set_file_name_null_restores_process_name_default)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_name(handle, "custom", 2)); // [手順] - 一度ファイル名を変更する。
    ASSERT_EQ(0, com_util_tracer_set_file_name(handle, NULL, 0));     // [手順] - NULL でデフォルトに戻す。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - プロセス名のデフォルト パスへ戻ること。

    // Act & Assert
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start でデフォルト パスを解決させる。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 名前と識別の getter がインスタンス側とファイル側を独立して返すことの確認
TEST_F(traceTest, test_getters_report_instance_and_file_settings_independently)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    char name_buf[64];
    char file_buf[64];

    // Act & Assert (デフォルト値)
    ASSERT_EQ(0, com_util_tracer_get_name(handle, name_buf, sizeof(name_buf))); // [手順] - インスタンス名を取得する。
    EXPECT_STREQ("myapp", name_buf); // [確認_正常系] - デフォルトのインスタンス名はプロセス名であること。
    EXPECT_EQ(0, com_util_tracer_get_identifier(handle)); // [確認_正常系] - デフォルトのインスタンス識別は 0。
    ASSERT_EQ(0, com_util_tracer_get_file_name(handle, file_buf, sizeof(file_buf))); // [手順] - ファイル名を取得する。
    EXPECT_STREQ("myapp", file_buf); // [確認_正常系] - デフォルトのファイル名はプロセス名であること。
    EXPECT_EQ(0, com_util_tracer_get_file_identifier(handle)); // [確認_正常系] - デフォルトのファイル識別は 0。

    // Act & Assert (設定後: インスタンス側とファイル側が独立していること)
    ASSERT_EQ(0, com_util_tracer_set_name(handle, "worker", 2));      // [手順] - インスタンス側を設定する。
    ASSERT_EQ(0, com_util_tracer_set_file_name(handle, "custom", 5)); // [手順] - ファイル側を設定する。
    ASSERT_EQ(0, com_util_tracer_get_name(handle, name_buf, sizeof(name_buf)));
    EXPECT_STREQ("worker-2", name_buf);                   // [確認_正常系] - インスタンス名が識別込みで返ること。
    EXPECT_EQ(2, com_util_tracer_get_identifier(handle)); // [確認_正常系] - インスタンス識別が返ること。
    ASSERT_EQ(0, com_util_tracer_get_file_name(handle, file_buf, sizeof(file_buf)));
    EXPECT_STREQ("custom-5", file_buf);                        // [確認_正常系] - ファイル名が識別込みで返ること。
    EXPECT_EQ(5, com_util_tracer_get_file_identifier(handle)); // [確認_正常系] - ファイル識別が返ること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// started 中の set_file_name と負の識別番号が失敗することの確認
TEST_F(traceTest, test_set_file_name_fails_when_started_or_identifier_negative)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Act & Assert
    EXPECT_EQ(-1, com_util_tracer_set_file_name(handle, "x", -1)); // [確認_異常系] - 負の識別番号は失敗すること。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_EQ(-1, com_util_tracer_set_file_name(handle, "x", 0)); // [確認_異常系] - started 中は失敗すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// 名前と識別の getter が NULL や不足バッファーに対して安全に失敗することの確認
TEST_F(traceTest, test_name_getters_fail_safely_for_invalid_arguments)
{
    // Arrange
    char buf[64];
    char small_buf[2];
    com_util_tracer *handle = create_logger();

    // Act & Assert
    EXPECT_EQ(-1, com_util_tracer_get_name(NULL, buf, sizeof(buf))); // [確認_異常系] - NULL ハンドルで失敗すること。
    EXPECT_EQ(-1, com_util_tracer_get_file_name(NULL, buf, sizeof(buf)));
    EXPECT_EQ(-1, com_util_tracer_get_identifier(NULL));      // [確認_異常系] - NULL ハンドルで -1 が返ること。
    EXPECT_EQ(-1, com_util_tracer_get_file_identifier(NULL)); // [確認_異常系] - NULL ハンドルで -1 が返ること。
    EXPECT_EQ(-1,
              com_util_tracer_get_name(handle, NULL, sizeof(buf))); // [確認_異常系] - NULL バッファーで失敗すること。
    EXPECT_EQ(-1, com_util_tracer_get_file_name(handle, buf, 0));   // [確認_異常系] - サイズ 0 で失敗すること。
    EXPECT_EQ(-1, com_util_tracer_get_name(handle, small_buf,
                                           sizeof(small_buf))); // [確認_異常系] - バッファー不足で失敗すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

#if defined(PLATFORM_WINDOWS)
// Windows でプロセス名由来の有効名から .exe が除去されることの確認
TEST_F(traceTest, test_default_file_path_strips_exe_suffix_on_windows)
{
    // Arrange
    ON_CALL(mock_, com_util_process_get_executable_path(_, _))
        .WillByDefault(
            [](char *out_path, size_t out_path_sz)
            {
                snprintf(out_path, out_path_sz, "%s", "C:/bin/myapp.exe");
                return 0;
            });
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_file_name(handle, NULL, 7)); // [手順] - ファイル識別 7 を設定する。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("C:/bin/log/myapp-7.log"), 0, 0, 0))
        .WillOnce(
            Return(file_handle_)); // [Pre-Assert確認_正常系] - .exe を除いたプロセス名でデフォルト パスを開くこと。

    // Act & Assert
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start でデフォルト パスを解決させる。

    // Cleanup
    com_util_tracer_dispose(handle);
}
#endif /* PLATFORM_WINDOWS */

// 実行ファイル パス取得失敗時に相対 log パスへフォールバックすることの確認
TEST_F(traceTest, test_default_file_path_falls_back_to_relative_log)
{
    // Arrange
    ON_CALL(mock_, com_util_process_get_executable_path(_, _))
        .WillByDefault(Return(-1));            // [状態] - 実行ファイル パスの取得が失敗する。
    com_util_tracer *handle = create_logger(); // [手順] - 有効名はフォールバックの unknown になる。
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("log/unknown.log"), 0, 0, 0))
        .WillOnce(
            Return(file_handle_)); // [Pre-Assert確認_異常系] - 相対パス log/unknown.log へフォールバックすること。

    // Act & Assert
    ASSERT_EQ(0, com_util_tracer_start(handle)); // [手順] - start でデフォルト パスを解決させる。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// トレース ファイルを開けない場合に start が -1 を返しつつ started 状態になることの確認
TEST_F(traceTest, test_start_returns_minus_one_but_starts_when_file_sink_create_fails)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO));

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(_, _, _, _))
        .WillOnce(
            Return((com_util_trace_file_sink *)NULL)); // [Pre-Assert確認_異常系] - file sink の生成が失敗すること。

    // Act
    int start_result = com_util_tracer_start(handle); // [手順] - file sink 生成が失敗する状態で start する。
    testing::internal::CaptureStderr();
    int write_result = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                              "still works"); // [手順] - ファイル以外の出力を確認する。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_EQ(-1, start_result); // [確認_異常系] - start が -1 を返すこと。
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STARTED,
              com_util_tracer_get_state(handle)); // [確認_異常系] - それでも started 状態へ遷移すること。
    EXPECT_EQ(0, write_result);                   // [確認_異常系] - ファイル以外の書き込みは成功すること。
    EXPECT_NE(std::string::npos,
              captured.find("I still works")); // [確認_異常系] - stderr 出力が継続すること。

    // Cleanup
    com_util_tracer_dispose(handle);
}

// stop がトレース ファイルを閉じ、再 start で新しいファイル名のデフォルト パスを開き直すことの確認
TEST_F(traceTest, test_stop_disposes_file_sink_and_restart_uses_new_name)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - 初回 start で従来のファイル名のパスを開くこと。
    EXPECT_CALL(mock_, com_util_trace_file_sink_create(StrEq("/opt/bin/log/renamed.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - 再 start で新しいファイル名のパスを開くこと。
    EXPECT_CALL(mock_, com_util_trace_file_sink_dispose(file_handle_))
        .Times(2); // [Pre-Assert確認_正常系] - stop 時と dispose 時にトレース ファイルが閉じられること。

    // Act & Assert
    ASSERT_EQ(0, com_util_tracer_start(handle));                       // [手順] - 初回 start。
    ASSERT_EQ(0, com_util_tracer_stop(handle));                        // [手順] - stop でトレース ファイルを閉じる。
    ASSERT_EQ(0, com_util_tracer_set_file_name(handle, "renamed", 0)); // [手順] - stopped 中にファイル名を変更する。
    ASSERT_EQ(0, com_util_tracer_start(handle));                       // [手順] - 再 start。

    // Cleanup
    com_util_tracer_dispose(handle);
}
