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
#include "traceSyncMock.h"

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

} // namespace

class traceTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util;
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
        set_trace_sync_mock_defaults(mock_com_util);
        ON_CALL(mock_com_util, com_util_shutdown_register(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util, com_util_get_realtime_deadline_ms(_, _))
            .WillByDefault([](uint64_t, struct timespec *abs_timeout) { set_valid_deadline(abs_timeout); });
        ON_CALL(mock_com_util, com_util_get_realtime(_)).WillByDefault([](com_util_timespec *ts) { set_fixed_realtime(ts); });
        ON_CALL(mock_com_util, com_util_format_realtime_iso8601_local(_, _, _))
            .WillByDefault(
                [](char *buf, size_t buf_size, const com_util_timespec *)
                {
                    snprintf(buf, buf_size, "%s", "2026-04-26T03:04:05.678+09:00");
                    return 0;
                });
        ON_CALL(mock_com_util, com_util_trace_file_sink_create(_, _, _, _)).WillByDefault(Return(file_handle_));
        ON_CALL(mock_com_util, com_util_trace_file_sink_write(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util, com_util_trace_file_sink_dispose(_)).WillByDefault(Return());
        // デフォルト パス解決を決定的にするため、実行ファイル パスを固定する
        ON_CALL(mock_com_util, com_util_process_get_executable_path(_, _))
            .WillByDefault(
                [](char *out_path, size_t out_path_sz)
                {
                    snprintf(out_path, out_path_sz, "%s", "/opt/bin/myapp");
                    return 0;
                });

#if defined(PLATFORM_LINUX)
        ON_CALL(mock_com_util, com_util_syslog_sink_create(_, _)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_com_util, com_util_syslog_sink_write(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util, com_util_syslog_sink_rename(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util, com_util_syslog_sink_dispose(_)).WillByDefault(Return());
#elif defined(PLATFORM_WINDOWS)
        ON_CALL(mock_com_util, com_util_etw_provider_create(_)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_com_util, com_util_etw_provider_write(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util, com_util_etw_provider_dispose(_)).WillByDefault(Return());
        ON_CALL(mock_com_util, com_util_eventlog_sink_create(_)).WillByDefault(Return(eventlog_handle_));
        ON_CALL(mock_com_util, com_util_eventlog_sink_write(_, _, _, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util, com_util_eventlog_sink_dispose(_)).WillByDefault(Return());
#endif
    }

    com_util_tracer *create_logger()
    {
        com_util_tracer *handle = com_util_tracer_create(COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED);
        EXPECT_NE((com_util_tracer *)NULL, handle);
        return handle;
    }
};

// 初期化と破棄が成功することの確認
TEST_F(traceTest, init_and_dispose)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_tracer *handle = create_logger(); // [手順] - トレース ハンドルを初期化する。
    size_t registry_count_after_create = trace_registry_count();
    com_util_tracer_dispose(&handle); // [手順] - トレース ハンドルを破棄する。
    size_t registry_count_after_dispose = trace_registry_count();

    // Assert
    EXPECT_EQ((size_t)1, registry_count_after_create);  // [確認_正常系] - create 後に registry へ 1 件登録されること。
    EXPECT_EQ((size_t)0, registry_count_after_dispose); // [確認_正常系] - dispose 後に registry が空になること。
}

// caller-managed モードではハンドル専用 rwlock を使用しないことの確認
TEST_F(traceTest, caller_managed_mode_does_not_use_handle_rwlock)
{
    // Arrange
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_create(_)).Times(0);
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_lock_shared(_, _)).Times(0);
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_lock_exclusive(_, _)).Times(0);
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_unlock_shared(_)).Times(0);
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_unlock_exclusive(_)).Times(0);
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_destroy(_)).Times(0);

    // Pre-Assert

    // Act
    com_util_tracer *handle = com_util_tracer_create(COM_UTIL_TRACER_CONCURRENCY_CALLER_MANAGED);
    ASSERT_NE(nullptr, handle);
    int start_result = com_util_tracer_start(handle); // [手順] - caller-managed モードの tracer を開始する。
    int stop_result = com_util_tracer_stop(handle);   // [手順] - caller-managed モードの tracer を停止する。
    com_util_tracer_dispose(&handle);                 // [手順] - caller-managed モードの tracer を破棄する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, start_result); // [確認_正常系] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, stop_result);  // [確認_正常系] - com_util_tracer_stop の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(nullptr, handle);           // [確認_正常系] - com_util_tracer_dispose がハンドルを NULL にすること。
}

// 不正な並行処理管理モードを副作用なしで拒否することの確認
TEST_F(traceTest, create_rejects_invalid_concurrency_mode_before_side_effects)
{
    // Arrange
    com_util_tracer_concurrency_mode invalid_mode;
    memset(&invalid_mode, 0x7F, sizeof(invalid_mode));
    EXPECT_CALL(mock_com_util, com_util_shutdown_register(_, _)).Times(0);
    EXPECT_CALL(mock_com_util, com_util_local_rwlock_create(_)).Times(0);

    // Pre-Assert

    // Act
    com_util_tracer *handle =
        com_util_tracer_create(invalid_mode); // [手順] - 未定義の並行処理管理モードで tracer を生成する。

    // Assert
    EXPECT_EQ(nullptr, handle); // [確認_異常系] - com_util_tracer_create が NULL を返すこと。
}

// get_state が create/start/stop の状態遷移を返すことの確認
TEST_F(traceTest, get_state_reports_stopped_started_stopped)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert

    // Act
    com_util_tracer_state created_state = com_util_tracer_get_state(handle); // [手順] - create 直後の状態を取得する。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle));
    com_util_tracer_state started_state = com_util_tracer_get_state(handle); // [手順] - start 後の状態を取得する。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_stop(handle));
    com_util_tracer_state stopped_state = com_util_tracer_get_state(handle); // [手順] - stop 後の状態を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STOPPED, created_state); // [確認_正常系] - create 直後は stopped を返すこと。
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STARTED, started_state); // [確認_正常系] - start 後は started を返すこと。
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STOPPED, stopped_state); // [確認_正常系] - stop 後は stopped を返すこと。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// get_state が NULL に対して disposed を返すことの確認
TEST_F(traceTest, get_state_returns_disposed_for_null)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_tracer_state state = com_util_tracer_get_state(NULL); // [手順] - NULL ハンドルの状態を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_TRACER_STATE_DISPOSED, state); // [確認_異常系] - NULL では disposed を返すこと。
}

// registry が live handle 数と容量拡張を追跡できることの確認
TEST_F(traceTest, registry_tracks_and_expands)
{
    // Arrange
    const size_t create_count = 12; // [状態] - 初期容量 8 を超える 12 個のハンドルを生成する。
    com_util_tracer *handles[create_count] = {};

    // Pre-Assert

    // Act
    for (size_t i = 0; i < create_count; i++)
    {
        handles[i] =
            create_logger(); // [手順] - com_util_tracer_create(COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED) を 12 回呼び出す。
    }

    // Assert
    EXPECT_EQ(create_count, trace_registry_count());    // [確認_正常系] - registry 件数が 12 であること。
    EXPECT_GE(trace_registry_capacity(), create_count); // [確認_正常系] - registry 容量が 12 以上へ拡張されること。

    // Cleanup
    for (com_util_tracer *handle : handles)
    {
        com_util_tracer_dispose(&handle);
    }
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_正常系] - すべて破棄後に registry が空になること。
}

// started 状態で INFO 出力が OS backend へ送られることの確認
TEST_F(traceTest, macro_write_prefixes_source_location)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 公開マクロが source location を付けて backend へ渡すこと。
    // [Pre-Assert手順] - backend 書き込みから 0 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] macro message")))
        .WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] macro message")))
        .WillOnce(Return(0));
#endif

    // Act
    int result = com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                       "macro message"); // [手順] - 公開マクロ経由で INFO を書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write の戻り値から、公開マクロ経由の書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 公開マクロが明示タイムスタンプを backend へ渡すことの確認
TEST_F(traceTest, macro_write_passes_explicit_timestamp)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_timespec timestamp = make_fixed_timestamp();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] explicit macro timestamp")))
        .WillOnce(
            [&timestamp](com_util_syslog_sink *, int, const com_util_timespec *actual_timestamp, const char *)
            {
                EXPECT_EQ(timestamp.tv_sec, actual_timestamp->tv_sec);
                EXPECT_EQ(timestamp.tv_nsec, actual_timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - 公開マクロ経由でも明示タイムスタンプがそのまま渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] explicit macro timestamp")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 公開マクロ経由でも ETW backend へメッセージが渡ること。
#endif

    // Act
    int result =
        com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                              "explicit macro timestamp"); // [手順] - 明示タイムスタンプ付きで公開マクロを呼ぶ。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write の戻り値から、明示タイムスタンプ付きの公開マクロ呼び出しが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 公開マクロが NULL メッセージでもソース位置だけを backend へ渡すことの確認
TEST_F(traceTest, macro_write_with_null_message_emits_source_location_only)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("^\\[traceTest\\.cc:[0-9]+\\]$")))
        .WillOnce(Return(
            0)); // [Pre-Assert確認_正常系] - NULL メッセージでも公開マクロが source location を付けて backend へ渡すこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("^\\[traceTest\\.cc:\\d+\\]$")))
        .WillOnce(Return(
            0)); // [Pre-Assert確認_正常系] - NULL メッセージでも公開マクロが source location だけを ETW backend へ渡すこと。
#endif

    // Act
    int result = com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                       NULL); // [手順] - NULL メッセージで公開マクロを呼ぶ。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write の戻り値から、NULL メッセージでも公開マクロ呼び出しが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 公開マクロが source location にファイルの basename を使うことの確認
TEST_F(traceTest, public_macros_prefix_source_location_with_basename)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    unsigned char data[] = {0x48, 0x69};
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - write マクロが basename を使うこと。
    // [Pre-Assert確認_正常系] - writef マクロが basename を使うこと。
    // [Pre-Assert確認_正常系] - write_hex マクロが basename を使うこと。
    // [Pre-Assert確認_正常系] - write_hexf マクロが basename を使うこと。
    // [Pre-Assert手順] - 各 backend 書き込みから 0 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] direct write")))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] value=7")))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] hex label: 48 69")))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  MatchesRegex("\\[traceTest\\.cc:[0-9]+\\] hex 7: 48 69")))
        .WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] direct write")))
        .WillOnce(Return(0));
    EXPECT_CALL(
        mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), MatchesRegex("\\[traceTest\\.cc:\\d+\\] value=7")))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] hex label: 48 69")))
        .WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   MatchesRegex("\\[traceTest\\.cc:\\d+\\] hex 7: 48 69")))
        .WillOnce(Return(0));
#endif

    // Act
    int write_result = com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "direct write");
    int writef_result = com_util_tracer_writef(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "value=%d", 7);
    int hex_result =
        com_util_tracer_write_hex(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data), "hex label");
    int hexf_result =
        com_util_tracer_write_hexf(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data), "hex %d", 7);

    // Assert
    EXPECT_EQ(
        0,
        write_result); // [確認_正常系] - com_util_tracer_write の戻り値から、write マクロの書き込みが成功したと判断できること。
    EXPECT_EQ(
        0,
        writef_result); // [確認_正常系] - com_util_tracer_writef の戻り値から、writef マクロの書き込みが成功したと判断できること。
    EXPECT_EQ(
        0,
        hex_result); // [確認_正常系] - com_util_tracer_write_hex の戻り値から、write_hex マクロの書き込みが成功したと判断できること。
    EXPECT_EQ(
        0,
        hexf_result); // [確認_正常系] - com_util_tracer_write_hexf の戻り値から、write_hexf マクロの書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 状態で INFO 出力が OS backend へ送られることの確認
TEST_F(traceTest, write_routes_info_to_os_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq("test message")))
        .WillOnce(
            [](com_util_syslog_sink *, int, const com_util_timespec *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - INFO が解決済み時刻付きで syslog backend へ 1 回送られること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("test message")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - INFO が ETW backend へ 1 回送られること。
#endif

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "test message"); // [手順] - INFO メッセージを書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_write_at の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

#if defined(PLATFORM_WINDOWS)
// ETW トレース (etw_level) と OS トレース (os_level) が独立にゲートされることの確認
TEST_F(traceTest, etw_and_os_levels_are_independent)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_VERBOSE,
              com_util_tracer_get_etw_level(handle)); // [状態] - ETW レベルは既定の VERBOSE のままとする。
    ASSERT_EQ(COM_UTIL_OK,
              com_util_tracer_set_os_level(
                  handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS トレース (EventLog) を NONE で無効にする。
                                                       // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("only etw")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - "only etw" が ETW backend へ 1 回送られること。
    EXPECT_CALL(mock_com_util, com_util_eventlog_sink_write(_, _, _, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - os_level=NONE のため EventLog へは送られないこと。

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "only etw"); // [手順] - INFO レベルで "only etw" を書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - com_util_tracer_write_at の戻り値が COM_UTIL_OK であること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}
#elif defined(PLATFORM_LINUX)
// Linux では etw_level が常に NONE を返し、設定が no-op となることの確認
TEST_F(traceTest, etw_level_is_none_and_noop_on_linux)
{
    // Arrange
    com_util_tracer *handle = create_logger(); // [状態] - 生成済みの tracer を用意する。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_NONE,
              com_util_tracer_get_etw_level(handle)); // [確認_正常系] - Linux では etw_level が常に NONE であること。
    int actual_ret_tracer_set_etw_level = com_util_tracer_set_etw_level(
        handle, COM_UTIL_TRACE_LEVEL_WARNING); // [手順] - etw_level に WARNING の設定を試みる。
    EXPECT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_etw_level); // [確認_正常系] - com_util_tracer_set_etw_level の戻り値から、設定が no-op として成功したと判断できること。
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_NONE,
              com_util_tracer_get_etw_level(handle)); // [確認_正常系] - 設定後も etw_level が NONE のままであること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}
#endif /* PLATFORM_ */

#if defined(PLATFORM_WINDOWS)
// OS トレース (os_level) が EventLog backend へ送られることの確認 (Windows)
TEST_F(traceTest, write_routes_info_to_eventlog_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_eventlog_sink_write(eventlog_handle_, (int)COM_UTIL_TRACE_LEVEL_INFO, 0, StrEq("myapp"),
                                                    0, StrEq("to eventlog")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - INFO が EventLog backend へ 1 回送られること。
                              // [Pre-Assert手順] - com_util_eventlog_sink_write から 0 を返却する。

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "to eventlog");

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result);

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// EventLog backend へファイル識別子とインスタンス識別子が個別に渡されることの確認 (Windows)
TEST_F(traceTest, write_routes_eventlog_identity_fields)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_name(handle, "worker", 3)); // [状態] - インスタンス名を worker_3 とする。
                                                                           // [状態確認] - com_util_tracer_set_name の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_name(handle, "trace-file", 7)); // [状態] - ファイル名を trace-file_7 とする。
                                                                                    // [状態確認] - com_util_tracer_set_file_name の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_eventlog_sink_write(eventlog_handle_, (int)COM_UTIL_TRACE_LEVEL_INFO, 7,
                                                    StrEq("worker"), 3, StrEq("identity fields")))
        .WillOnce(Return(
            0)); // [Pre-Assert確認_正常系] - EventLog へファイル識別子、元のインスタンス名、インスタンス識別子が個別に送られること。
                 // [Pre-Assert手順] - com_util_eventlog_sink_write から 0 を返却する。

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "identity fields");

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result);

    // Cleanup
    com_util_tracer_dispose(&handle);
}
#endif /* PLATFORM_WINDOWS */

// 明示タイムスタンプ付き INFO 出力が OS backend へ渡ることの確認
TEST_F(traceTest, write_routes_explicit_timestamp_to_os_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_timespec timestamp = make_fixed_timestamp();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq("explicit timestamp")))
        .WillOnce(
            [&timestamp](com_util_syslog_sink *, int, const com_util_timespec *actual_timestamp, const char *)
            {
                EXPECT_EQ(timestamp.tv_sec, actual_timestamp->tv_sec);
                EXPECT_EQ(timestamp.tv_nsec, actual_timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - 明示タイムスタンプが syslog backend へそのまま渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(),
                                                   StrEq("explicit timestamp")))
        .WillOnce(
            Return(0)); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定でも ETW backend へメッセージが渡ること。
#endif

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                                        "explicit timestamp"); // [手順] - 明示タイムスタンプ付き INFO を書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_write_at の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// NULL ハンドルと NULL メッセージが安全に無視されることの確認
TEST_F(traceTest, write_is_null_safe)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert

    // Act
    int null_handle_result =
        com_util_tracer_write_at(NULL, COM_UTIL_TRACE_LEVEL_INFO, NULL, "ignored"); // [手順] - NULL ハンドルで書き込む。
    int null_message_result =
        com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, NULL); // [手順] - NULL メッセージで書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        null_handle_result); // [確認_異常系] - com_util_tracer_write_at の戻り値として、NULL ハンドルで 0 が返ること。
    EXPECT_EQ(
        COM_UTIL_OK,
        null_message_result); // [確認_異常系] - com_util_tracer_write_at の戻り値として、NULL メッセージで 0 が返ること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 1024 バイト超の UTF-8 文字列が安全な境界で切り詰められることの確認
TEST_F(traceTest, write_truncates_utf8_boundary)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    char msg[1025];
    unsigned char utf8[] = {0xE3, 0x81, 0x82};
    memset(msg, 'A', 1021);
    memcpy(&msg[1021], utf8, 3);
    msg[1024] = '\0';

    // Pre-Assert
    // [Pre-Assert確認_正常系] - UTF-8 境界直前の 1021 バイトだけが backend へ渡ること。
    // [Pre-Assert手順] - 切り詰め後の 1021 バイト文字列を確認し、0 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), _))
        .WillOnce(
            [](com_util_syslog_sink *, int, const com_util_timespec *timestamp, const char *actual)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                EXPECT_EQ((size_t)1021, strlen(actual));
                EXPECT_EQ(std::string(1021, 'A'), std::string(actual));
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), _))
        .WillOnce(
            [](com_util_etw_provider *, int, const char *, const char *actual)
            {
                EXPECT_EQ((size_t)1021, strlen(actual));
                EXPECT_EQ(std::string(1021, 'A'), std::string(actual));
                return 0;
            });
#endif

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        msg); // [手順] - UTF-8 境界を跨ぐ長いメッセージを書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write_at の戻り値から、切り詰め後も書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// writef が format 展開後の文字列を backend へ渡すことの確認
TEST_F(traceTest, writef_formats_message)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  StrEq("user=alice count=42")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - format 展開後の文字列が時刻付きで backend へ渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("user=alice count=42")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - format 展開後の文字列が backend へ渡ること。
#endif

    // Act
    int result = com_util_tracer_writef_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "user=%s count=%d", "alice",
                                         42); // [手順] - writef を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_writef_at の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// HEX 書き込みがラベル付きテキストへ変換されることの確認
TEST_F(traceTest, write_hex_formats_payload)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    unsigned char data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(),
                                                  StrEq("Data: 48 65 6C 6C 6F")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - HEX テキストが時刻付きで backend へ渡ること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq("Data: 48 65 6C 6C 6F")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - HEX テキストが backend へ渡ること。
#endif

    // Act
    int result = com_util_tracer_write_hex_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data),
                                            "Data"); // [手順] - ラベル付き HEX 書き込みを行う。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_write_hex_at の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// HEX 書き込みでデータ本体を出力できない残り長の場合に省略記号だけが付与されることの確認
TEST_F(traceTest, write_hex_appends_ellipsis_when_only_ellipsis_fits)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    unsigned char data[] = {0x48, 0x69};
    std::string label(COM_UTIL_TRACER_MESSAGE_MAX_BYTES - 6, 'L');
    std::string expected = label + ": ...";

    // Pre-Assert
    // [Pre-Assert確認_正常系] - HEX データ本体なしで省略記号だけが backend へ渡ること。
    // [Pre-Assert手順] - backend 書き込みから 0 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq(expected.c_str())))
        .WillOnce(Return(0));
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, NotNull(), StrEq(expected.c_str())))
        .WillOnce(Return(0));
#endif

    // Act
    int result =
        com_util_tracer_write_hex_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data),
                                   label.c_str()); // [手順] - 省略記号だけが収まるラベル長で HEX 書き込みを行う。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_write_hex_at の戻り値から、書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中は識別子・ファイル名の設定関数が失敗することの確認
TEST_F(traceTest, identity_config_fails_when_started)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    int name_result = com_util_tracer_set_name(handle, "running", 0); // [手順] - started 中に set_name を呼ぶ。
    int file_name_result =
        com_util_tracer_set_file_name(handle, "running", 0); // [手順] - started 中に set_file_name を呼ぶ。

    // Assert
    EXPECT_EQ(
        -1,
        name_result); // [確認_異常系] - com_util_tracer_set_name の戻り値から、started 中の set_name が失敗したと判断できること。
    EXPECT_EQ(
        -1,
        file_name_result); // [確認_異常系] - com_util_tracer_set_file_name の戻り値から、started 中の set_file_name が失敗したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中でも os / etw / stderr のレベル変更が成功し反映されることの確認
TEST_F(traceTest, level_change_allowed_when_started)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - stderr レベルを INFO とする。
                                                                                                 // [状態確認] - com_util_tracer_set_stderr_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    int os_result = com_util_tracer_set_os_level(
        handle, COM_UTIL_TRACE_LEVEL_VERBOSE); // [手順] - started 中に os レベルを変更する。
    int etw_result = com_util_tracer_set_etw_level(
        handle, COM_UTIL_TRACE_LEVEL_WARNING); // [手順] - started 中に etw レベルを変更する。
    int stderr_result = com_util_tracer_set_stderr_level(
        handle, COM_UTIL_TRACE_LEVEL_ERROR); // [手順] - started 中に stderr レベルを変更する。

    // Assert
    EXPECT_EQ(
        0,
        os_result); // [確認_正常系] - com_util_tracer_set_os_level の戻り値から、started 中の set_os_level が成功したと判断できること。
    EXPECT_EQ(
        0,
        etw_result); // [確認_正常系] - com_util_tracer_set_etw_level の戻り値から、Linux では no-op となる set_etw_level が started 中に成功したと判断できること。
    EXPECT_EQ(
        0,
        stderr_result); // [確認_正常系] - com_util_tracer_set_stderr_level の戻り値から、started 中の set_stderr_level が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_VERBOSE,
              com_util_tracer_get_os_level(handle)); // [確認_正常系] - 変更後の os レベルが反映されること。
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_ERROR,
              com_util_tracer_get_stderr_level(handle)); // [確認_正常系] - 変更後の stderr レベルが反映されること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中の os レベル引き上げが即座に出力へ反映されることの確認 (連続性)
TEST_F(traceTest, os_level_raise_takes_effect_while_started)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    // 旧閾値 INFO では VERBOSE は OS backend へ送られない。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_DEBUG, NotNull(), HasSubstr("verbose before")))
        .Times(0); // [Pre-Assert確認_正常系] - しきい値引き上げ前は VERBOSE が backend へ送られないこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_eventlog_sink_write(eventlog_handle_, (int)COM_UTIL_TRACE_LEVEL_VERBOSE, _, _, _,
                                                    HasSubstr("verbose before")))
        .Times(0); // [Pre-Assert確認_正常系] - しきい値引き上げ前は VERBOSE が EventLog へ送られないこと。
#endif

    // Act
    com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_VERBOSE, NULL,
                          "verbose before"); // [手順] - しきい値引き上げ前に VERBOSE で "verbose before" を書き込む。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    // Pre-Assert_2
    // 停止せずにしきい値を VERBOSE へ引き上げると VERBOSE が送られる。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_DEBUG, NotNull(), HasSubstr("verbose after")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - しきい値引き上げ後は VERBOSE が backend へ送られること。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_eventlog_sink_write(eventlog_handle_, (int)COM_UTIL_TRACE_LEVEL_VERBOSE, 0,
                                                    StrEq("myapp"), 0, HasSubstr("verbose after")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - しきい値引き上げ後は VERBOSE が EventLog へ送られること。
#endif

    // Act_2
    int actual_ret_tracer_set_os_level = com_util_tracer_set_os_level(
        handle, COM_UTIL_TRACE_LEVEL_VERBOSE); // [手順] - started のまましきい値を VERBOSE へ引き上げる。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_os_level); // [確認_正常系] - started のまましきい値を VERBOSE へ引き上げた com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_VERBOSE, NULL,
                          "verbose after"); // [手順] - 引き上げ後に VERBOSE で "verbose after" を書き込む。

    // Assert

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中のしきい値のみ変更では file sink を開き直さないことの確認 (ケース 2)
TEST_F(traceTest, set_file_level_threshold_only_no_reopen_while_started)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0)); // [状態] - ファイル レベルを INFO、パスを "trace.log" とする。
                                                                                                                     // [状態確認] - com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    // パスとパラメーターが同一でしきい値のみ変える場合は再オープンしない。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - しきい値のみ変更では file sink を再生成しないこと。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_dispose(_))
        .Times(0); // [Pre-Assert確認_正常系] - しきい値のみ変更では file sink を破棄しないこと。

    // Act
    int result = com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_VERBOSE, 0, 0, 0);

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_set_file_level の戻り値から、しきい値のみの変更が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_VERBOSE,
              com_util_tracer_get_file_level(handle)); // [確認_正常系] - 変更後の file レベルが反映されること。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中にパスを変更すると file sink を開き直すことの確認 (ケース 3)
TEST_F(traceTest, set_file_level_reopen_on_path_change_while_started)
{
    // Arrange
    com_util_trace_file_sink *file_handle2 =
        reinterpret_cast<com_util_trace_file_sink *>(static_cast<uintptr_t>(0x2300));
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。

    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [状態確認] - start 時に初期パスで file sink を開くこと。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0)); // [状態] - ファイル レベルを INFO、パスを "trace.log" とする。
                                                                                                                     // [状態確認] - com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    // Pre-Assert
    // パス変更時は新パスで開き、旧ハンドルを破棄する。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("other.log"), 0, 0, 0))
        .WillOnce(Return(file_handle2)); // [Pre-Assert確認_正常系] - 新パスで file sink を開くこと。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_dispose(file_handle_))
        .Times(1); // [Pre-Assert確認_正常系] - 旧 file sink を破棄すること。

    // Act
    int result = com_util_tracer_set_file_level(handle, "other.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0);

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_set_file_level の戻り値から、パス変更を伴う変更が成功したと判断できること。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中に level=NONE を指定するとファイル出力を無効化することの確認 (ケース 1)
TEST_F(traceTest, set_file_level_disable_while_started)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。

    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [状態確認] - start 時に file sink を開くこと。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0)); // [状態] - ファイル レベルを INFO、パスを "trace.log" とする。
                                                                                                                     // [状態確認] - com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    // Pre-Assert
    // 無効化では既存ハンドルを破棄し、再オープンしない。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_dispose(file_handle_))
        .Times(1); // [Pre-Assert確認_正常系] - 既存の file sink を破棄すること。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - 無効化では file sink を再生成しないこと。

    // Act
    int result = com_util_tracer_set_file_level(handle, NULL, COM_UTIL_TRACE_LEVEL_NONE, 0, 0, 0);

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_set_file_level の戻り値から、ファイル出力の無効化が成功したと判断できること。
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_NONE,
              com_util_tracer_get_file_level(handle)); // [確認_正常系] - file レベルが NONE になること。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// stopped 中の file level 設定が file backend 作成と書き込みへ反映されることの確認
TEST_F(traceTest, file_level_routes_to_file_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - file sink が指定パスで初期化されること。
    EXPECT_CALL(mock_com_util,
                com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("file info")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - INFO が file sink へ 1 回送られること。

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    int actual_ret_tracer_set_file_level = com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                                   0); // [手順] - file trace を有効化する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_level); // [確認_正常系] - file trace を有効化した com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle));
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "file info"); // [手順] - INFO メッセージを書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write_at の戻り値から、file backend 経由の書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_file_level の flags が start 時の file sink 生成へ引き渡されることの確認
TEST_F(traceTest, set_file_level_passes_flags_to_file_sink)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - flags がそのまま create へ渡ること。

    // Act
    int result = com_util_tracer_set_file_level(
        handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
        COM_UTIL_TRACE_FILE_SINK_SHARED);                 // [手順] - 共有フラグ付きで set_file_level を呼ぶ。
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start で file sink を生成させる。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - file sink を生成した com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_set_file_level の戻り値から、設定が成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_file_level が sink を生成せず start まで遅延されることの確認
TEST_F(traceTest, set_file_level_defers_sink_creation_until_start)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - set_file_level の時点では file sink が生成されないこと。

    // Act
    int actual_ret_tracer_set_file_level = com_util_tracer_set_file_level(
        handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0); // [手順] - stopped 中に set_file_level を呼び出す。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_level); // [確認_正常系] - stopped 中に set_file_level を呼び出した com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ::testing::Mock::VerifyAndClearExpectations(&mock_com_util);

    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(
            file_handle_)); // [Pre-Assert確認_正常系] - start 時に設定済みパス "trace.log" で file sink が生成されること。
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start で file sink を生成させる。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - file sink を生成した com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 明示タイムスタンプ指定時に file backend と stderr が同じ時刻を使うことの確認
TEST_F(traceTest, explicit_timestamp_is_shared_by_file_and_stderr)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_timespec timestamp = make_fixed_timestamp();

    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("trace.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [状態確認] - start 時に file sink を初期化すること。

    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0)); // [状態] - ファイル レベルを INFO、パスを "trace.log" とする。
                                                                                                                     // [状態確認] - com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - stderr レベルを INFO とする。
                                                                                                 // [状態確認] - com_util_tracer_set_stderr_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(_))
        .Times(0); // [Pre-Assert確認_正常系] - 明示タイムスタンプ指定時は現在時刻取得を行わないこと。
    EXPECT_CALL(
        mock_com_util, com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("explicit ts")))
        .WillOnce(
            [](com_util_trace_file_sink *, int, const com_util_timespec *actual, const char *)
            {
                EXPECT_NE(nullptr, actual);
                EXPECT_EQ(1714100645LL, actual->tv_sec);
                EXPECT_EQ(678000000, actual->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_正常系] - file sink へ明示タイムスタンプがそのまま渡ること。
                // [Pre-Assert手順] - 渡された時刻を確認し、0 を返却する。

    // Act
    testing::internal::CaptureStderr();
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, &timestamp,
                                        "explicit ts"); // [手順] - 明示タイムスタンプ付きで書き込む。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write_at の戻り値から、明示タイムスタンプ付き書き込みが成功したと判断できること。
    EXPECT_NE(
        std::string::npos,
        captured.find(
            "2026-04-26T03:04:05.678+09:00 I explicit ts")); // [確認_正常系] - stderr でも同じ時刻文字列が使われること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// file level NONE でファイル トレースが無効化されることの確認
TEST_F(traceTest, file_level_none_disables_file_backend)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_set_file_level = com_util_tracer_set_file_level(
        handle, NULL, COM_UTIL_TRACE_LEVEL_NONE, 0, 0, 0); // [状態] - level NONE でファイル トレースを無効化する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_level); // [状態確認] - level NONE で無効化した com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - file sink が生成されないこと。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - file sink へは 1 回も送られないこと。

    // Act
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle));
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_CRITICAL, NULL,
                                        "no file output"); // [手順] - ファイル トレース無効のまま書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - file 無効でもエラーにならないこと。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_name が識別子付き名称を反映することの確認
TEST_F(traceTest, set_name_with_identifier_updates_backend_name)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_rename(os_handle_, StrEq("worker_2")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - syslog ident が worker_2 へ更新されること。
#endif

    // Act
    int result = com_util_tracer_set_name(handle, "worker", 2); // [手順] - identifier = 2 で set_name を呼ぶ。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_tracer_set_name の戻り値から、set_name が成功したと判断できること。

#if defined(PLATFORM_WINDOWS)
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO));
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle));
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, StrEq("worker_2"), StrEq("running as worker_2")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - ETW サービス名が worker_2 に更新されること。
    EXPECT_EQ(COM_UTIL_OK, com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "running as worker_2"));
#endif

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// COM_UTIL_TRACE_LEVEL_NONE では OS backend が呼ばれないことの確認
TEST_F(traceTest, os_level_none_suppresses_output)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - syslog backend が呼ばれないこと。
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_eventlog_sink_write(_, _, _, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - EventLog backend が呼ばれないこと。
#endif

    // Act
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_CRITICAL, NULL,
                                        "suppressed"); // [手順] - OS level NONE のまま書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result); // [確認_正常系] - 出力抑止でもエラーにならないこと。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// stderr level DEBUG で V と D の marker が出力されることの確認
TEST_F(traceTest, stderr_level_debug_outputs_markers)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_DEBUG)); // [状態] - stderr レベルを DEBUG とする。
                                                                                                  // [状態確認] - com_util_tracer_set_stderr_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert

    // Act
    testing::internal::CaptureStderr();
    int actual_ret_tracer_write = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_VERBOSE, NULL,
                                                  "verbose to stderr"); // [手順] - VERBOSE を stderr へ出力する。
    EXPECT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_write); // [確認_正常系] - VERBOSE を stderr へ出力した com_util_tracer_write_at の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_write_2 = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL,
                                                    "debug to stderr"); // [手順] - DEBUG を stderr へ出力する。
    EXPECT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_write_2); // [確認_正常系] - DEBUG を stderr へ出力した com_util_tracer_write_at の戻り値が COM_UTIL_OK であること。
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
    com_util_tracer_dispose(&handle);
}

// 不正な明示タイムスタンプ指定時に現在時刻へ代替して各出力先へ書き込みつつ -1 を返すことの確認
TEST_F(traceTest, invalid_explicit_timestamp_falls_back_and_returns_minus_one)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_timespec invalid_timestamp = {1714100645LL, 1000000000};

    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - OS レベルを INFO とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0)); // [状態] - ファイル レベルを INFO、パスを "trace.log" とする。
                                                                                                                     // [状態確認] - com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - stderr レベルを INFO とする。
                                                                                                 // [状態確認] - com_util_tracer_set_stderr_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(_))
        .Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
    // [Pre-Assert確認_異常系] - OS backend へ代替時刻で渡ること。
    // [Pre-Assert手順] - OS backend へ代替時刻を渡して 0 を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_write(os_handle_, LOG_INFO, NotNull(), StrEq("invalid ts")))
        .WillOnce(
            [](com_util_syslog_sink *, int, const com_util_timespec *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    EXPECT_CALL(mock_com_util, com_util_etw_provider_write(os_handle_, 4, _, StrEq("invalid ts"))).WillOnce(Return(0));
#endif
    EXPECT_CALL(mock_com_util,
                com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("invalid ts")))
        .WillOnce(
            [](com_util_trace_file_sink *, int, const com_util_timespec *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_異常系] - file backend へ代替時刻で渡ること。
                // [Pre-Assert手順] - file backend へ代替時刻を渡して 0 を返却する。

    // Act
    testing::internal::CaptureStderr();
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp,
                                        "invalid ts"); // [手順] - 不正タイムスタンプで書き込む。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, result); // [確認_異常系] - 代替出力後も COM_UTIL_ERR_UNKNOWN を返すこと。
    EXPECT_NE(std::string::npos,
              captured.find(
                  "2026-04-26T03:04:05.678+09:00 I invalid ts")); // [確認_異常系] - stderr も代替時刻で出力すること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 不正な明示タイムスタンプ指定時に write_hex でも現在時刻へ代替して -1 を返すことの確認
TEST_F(traceTest, write_hex_invalid_explicit_timestamp_falls_back_and_returns_minus_one)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    com_util_timespec invalid_timestamp = {1714100645LL, 1000000000};
    unsigned char data[] = {0x48, 0x69};

    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_file_level(handle, "trace.log", COM_UTIL_TRACE_LEVEL_INFO, 0, 0, 0)); // [状態] - ファイル レベルを INFO、パスを "trace.log" とする。
                                                                                                                     // [状態確認] - com_util_tracer_set_file_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_start(handle)); // [状態] - tracer を started 状態とする。
                                                           // [状態確認] - com_util_tracer_start の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(_))
        .Times(1); // [Pre-Assert確認_異常系] - 不正時刻では現在時刻へ代替すること。
    EXPECT_CALL(
        mock_com_util, com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(), StrEq("Data: 48 69")))
        .WillOnce(
            [](com_util_trace_file_sink *, int, const com_util_timespec *timestamp, const char *)
            {
                EXPECT_EQ(1714100645LL, timestamp->tv_sec);
                EXPECT_EQ(678000000, timestamp->tv_nsec);
                return 0;
            }); // [Pre-Assert確認_異常系] - HEX 書き込みでも代替時刻が file backend へ渡ること。
                // [Pre-Assert手順] - 代替時刻を確認し、0 を返却する。

    // Act
    int result = com_util_tracer_write_hex_at(handle, COM_UTIL_TRACE_LEVEL_INFO, &invalid_timestamp, data, sizeof(data),
                                            "Data"); // [手順] - 不正タイムスタンプ付きで HEX 書き込みを行う。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - HEX 書き込みでも代替出力後に COM_UTIL_ERR_UNKNOWN を返すこと。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// stopped 状態では出力関数が失敗することの確認
TEST_F(traceTest, write_fails_when_stopped)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    unsigned char data[] = {0x01, 0x02};

    // Pre-Assert

    // Act
    int write_result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                              "stopped message"); // [手順] - stopped 状態で write を呼ぶ。
    int writef_result = com_util_tracer_writef_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "stopped %s",
                                                "msg"); // [手順] - stopped 状態で writef を呼ぶ。
    int hex_result = com_util_tracer_write_hex_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data),
                                                "hex"); // [手順] - stopped 状態で write_hex を呼ぶ。
    int hexf_result = com_util_tracer_write_hexf_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, data, sizeof(data), "hex %d",
                                                  1); // [手順] - stopped 状態で write_hexf を呼ぶ。

    // Assert
    EXPECT_EQ(
        -1,
        write_result); // [確認_異常系] - com_util_tracer_write_at の戻り値から、stopped 状態の write が失敗したと判断できること。
    EXPECT_EQ(
        -1,
        writef_result); // [確認_異常系] - com_util_tracer_writef_at の戻り値から、stopped 状態の writef が失敗したと判断できること。
    EXPECT_EQ(
        -1,
        hex_result); // [確認_異常系] - com_util_tracer_write_hex_at の戻り値から、stopped 状態の write_hex が失敗したと判断できること。
    EXPECT_EQ(
        -1,
        hexf_result); // [確認_異常系] - com_util_tracer_write_hexf_at の戻り値から、stopped 状態の write_hexf が失敗したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// start と stop の二重呼び出しがべき等であることの確認
TEST_F(traceTest, start_and_stop_are_idempotent)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert

    // Act
    int first_start = com_util_tracer_start(handle);  // [手順] - 1 回目の start を呼ぶ。
    int second_start = com_util_tracer_start(handle); // [手順] - 2 回目の start を呼ぶ。
    int first_stop = com_util_tracer_stop(handle);    // [手順] - 1 回目の stop を呼ぶ。
    int second_stop = com_util_tracer_stop(handle);   // [手順] - 2 回目の stop を呼ぶ。

    // Assert
    EXPECT_EQ(
        0,
        first_start); // [確認_正常系] - com_util_tracer_start の戻り値から、1 回目の start が成功したと判断できること。
    EXPECT_EQ(
        0,
        second_start); // [確認_正常系] - com_util_tracer_start の戻り値から、2 回目の start も成功したと判断できること。
    EXPECT_EQ(
        0, first_stop); // [確認_正常系] - com_util_tracer_stop の戻り値から、1 回目の stop が成功したと判断できること。
    EXPECT_EQ(
        0,
        second_stop); // [確認_正常系] - com_util_tracer_stop の戻り値から、2 回目の stop も成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_file_level 未呼び出しの start でデフォルト パスのファイル トレースが有効になることの確認
TEST_F(traceTest, start_creates_default_file_sink)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(
            file_handle_)); // [Pre-Assert確認_正常系] - 実行ファイルのディレクトリ配下の log/<有効名>.log を開くこと。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_write(file_handle_, COM_UTIL_TRACE_LEVEL_INFO, NotNull(),
                                                      StrEq("default file")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - デフォルトの file sink へ INFO が送られること。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - 未設定のまま start する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - 未設定のまま start した com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    int result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "default file"); // [手順] - INFO メッセージを書き込む。

    // Assert
    EXPECT_EQ(
        COM_UTIL_OK,
        result); // [確認_正常系] - com_util_tracer_write_at の戻り値から、デフォルトのファイル トレースで書き込みが成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_name (インスタンス名とインスタンス識別) がデフォルトのトレース ファイル名に影響しないことの確認
TEST_F(traceTest, set_name_does_not_affect_default_file_path)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(
        0, com_util_tracer_set_name(handle, "worker", 3)); // [状態] - インスタンス名を worker_3 に変更した状態とする。
                                                           // [状態確認] - com_util_tracer_set_name の戻り値が 0 であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(
            file_handle_)); // [Pre-Assert確認_正常系] - set_name に関わらずプロセス名のデフォルト パスを開くこと。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start でデフォルト パスを解決させる。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - com_util_tracer_start の戻り値から、start が成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_file_name のファイル名とファイル識別がデフォルト パスへ反映されることの確認
TEST_F(traceTest, set_file_name_reflects_to_default_file_path)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(
        0, com_util_tracer_set_file_name(handle, "custom", 2)); // [状態] - ファイル名を custom_2 に変更した状態とする。
                                                                // [状態確認] - com_util_tracer_set_file_name の戻り値が 0 であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("/opt/bin/log/custom_2.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - custom_2 のデフォルト パスを開くこと。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start でデフォルト パスを解決させる。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - com_util_tracer_start の戻り値から、start が成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// set_file_name(NULL, 0) でデフォルト (プロセス名、識別なし) に戻ることの確認
TEST_F(traceTest, set_file_name_null_restores_process_name_default)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_set_file_name =
        com_util_tracer_set_file_name(handle, "custom", 2); // [状態] - 一度ファイル名を変更する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_name); // [状態確認] - 一度ファイル名を変更した com_util_tracer_set_file_name の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_set_file_name_2 =
        com_util_tracer_set_file_name(handle, NULL, 0); // [状態] - ファイル名を NULL でデフォルトに戻す。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_name_2); // [状態確認] - NULL でデフォルトに戻した com_util_tracer_set_file_name の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - プロセス名のデフォルト パスへ戻ること。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start でデフォルト パスを解決させる。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - com_util_tracer_start の戻り値から、start が成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 名前と識別の getter がインスタンス側とファイル側を独立して返すことの確認
TEST_F(traceTest, getters_report_instance_and_file_settings_independently)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    char name_buf[64];
    char file_buf[64];

    // Pre-Assert

    // Act
    // Assert
    // デフォルト値の確認
    int actual_ret_tracer_get_name =
        com_util_tracer_get_name(handle, name_buf, sizeof(name_buf)); // [手順] - インスタンス名を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_get_name); // [確認_正常系] - インスタンス名を取得した com_util_tracer_get_name の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("myapp", name_buf); // [確認_正常系] - デフォルトのインスタンス名がプロセス名 "myapp" であること。
    EXPECT_EQ(
        0, com_util_tracer_get_identifier(handle)); // [確認_正常系] - デフォルトのインスタンス識別番号が 0 であること。
    int actual_ret_tracer_get_file_name =
        com_util_tracer_get_file_name(handle, file_buf, sizeof(file_buf)); // [手順] - ファイル名を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_get_file_name); // [確認_正常系] - ファイル名を取得した com_util_tracer_get_file_name の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("myapp", file_buf); // [確認_正常系] - デフォルトのファイル名がプロセス名 "myapp" であること。
    EXPECT_EQ(COM_UTIL_OK, com_util_tracer_get_file_identifier(
                               handle)); // [確認_正常系] - デフォルトのファイル識別番号が 0 であること。

    // 設定後: インスタンス側とファイル側が独立していることの確認
    int actual_ret_tracer_set_name =
        com_util_tracer_set_name(handle, "worker", 2); // [手順] - インスタンス側を "worker", 2 に設定する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_name); // [確認_正常系] - インスタンス側を "worker", 2 に設定した com_util_tracer_set_name の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_set_file_name =
        com_util_tracer_set_file_name(handle, "custom", 5); // [手順] - ファイル側を "custom", 5 に設定する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_name); // [確認_正常系] - ファイル側を "custom", 5 に設定した com_util_tracer_set_file_name の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_get_name(handle, name_buf, sizeof(name_buf)));
    EXPECT_STREQ("worker_2", name_buf); // [確認_正常系] - インスタンス名が識別込みの "worker_2" で返ること。
    EXPECT_EQ(
        2,
        com_util_tracer_get_identifier(
            handle)); // [確認_正常系] - com_util_tracer_get_identifier の戻り値として、インスタンス識別番号 2 が返ること。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_get_file_name(handle, file_buf, sizeof(file_buf)));
    EXPECT_STREQ("custom_5", file_buf); // [確認_正常系] - ファイル名が識別込みの "custom_5" で返ること。
    EXPECT_EQ(
        5,
        com_util_tracer_get_file_identifier(
            handle)); // [確認_正常系] - com_util_tracer_get_file_identifier の戻り値として、ファイル識別番号 5 が返ること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// started 中の set_file_name と負の識別番号が失敗することの確認
TEST_F(traceTest, set_file_name_fails_when_started_or_identifier_negative)
{
    // Arrange
    com_util_tracer *handle = create_logger();

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_tracer_set_file_name(
            handle, "x",
            -1)); // [確認_異常系] - com_util_tracer_set_file_name の戻り値として、負の識別番号 -1 では COM_UTIL_ERR_INVALID_ARGUMENT が返ること。
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - tracer を started 状態にする。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - tracer を started 状態にした com_util_tracer_start の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        com_util_tracer_set_file_name(
            handle, "x",
            0)); // [確認_異常系] - com_util_tracer_set_file_name の戻り値として、started 中の set_file_name では COM_UTIL_ERR_UNKNOWN が返ること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// 名前と識別の getter が NULL や不足バッファーに対して安全に失敗することの確認
TEST_F(traceTest, name_getters_fail_safely_for_invalid_arguments)
{
    // Arrange
    char buf[64];
    char small_buf[2];
    com_util_tracer *handle = create_logger();

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        -1,
        com_util_tracer_get_name(
            NULL, buf,
            sizeof(
                buf))); // [確認_異常系] - com_util_tracer_get_name の戻り値から、get_name が NULL ハンドルで失敗したと判断できること。
    EXPECT_EQ(
        -1,
        com_util_tracer_get_file_name(
            NULL, buf,
            sizeof(
                buf))); // [確認_異常系] - com_util_tracer_get_file_name の戻り値から、get_file_name が NULL ハンドルで失敗したと判断できること。
    EXPECT_EQ(
        -1, com_util_tracer_get_identifier(NULL)); // [確認_異常系] - get_identifier が NULL ハンドルで -1 を返すこと。
    EXPECT_EQ(-1, com_util_tracer_get_file_identifier(
                      NULL)); // [確認_異常系] - get_file_identifier が NULL ハンドルで -1 を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_tracer_get_name(
            handle, NULL,
            sizeof(
                buf))); // [確認_異常系] - com_util_tracer_get_name の戻り値から、NULL バッファーで失敗したと判断できること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_tracer_get_file_name(
            handle, buf,
            0)); // [確認_異常系] - com_util_tracer_get_file_name の戻り値から、サイズ 0 で失敗したと判断できること。
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        com_util_tracer_get_name(
            handle, small_buf,
            sizeof(
                small_buf))); // [確認_異常系] - com_util_tracer_get_name の戻り値から、バッファー不足で失敗したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

#if defined(PLATFORM_WINDOWS)
// Windows でプロセス名由来の有効名から .exe が除去されることの確認
TEST_F(traceTest, default_file_path_strips_exe_suffix_on_windows)
{
    // Arrange
    ON_CALL(mock_com_util, com_util_process_get_executable_path(_, _))
        .WillByDefault(
            [](char *out_path, size_t out_path_sz)
            {
                snprintf(out_path, out_path_sz, "%s", "C:/bin/myapp.exe");
                return 0;
            }); // [状態] - com_util_process_get_executable_path が呼び出された際に "C:/bin/myapp.exe" を返すようにモックを設定する。
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_set_file_name =
        com_util_tracer_set_file_name(handle, NULL, 7); // [状態] - ファイル識別 7 を設定する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_name); // [状態確認] - ファイル識別 7 を設定した com_util_tracer_set_file_name の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("C:/bin/log/myapp_7.log"), 0, 0, 0))
        .WillOnce(
            Return(file_handle_)); // [Pre-Assert確認_正常系] - .exe を除いたプロセス名でデフォルト パスを開くこと。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start でデフォルト パスを解決させる。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - com_util_tracer_start の戻り値から、start が成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}
#endif /* PLATFORM_WINDOWS */

// 実行ファイル パス取得失敗時に相対 log パスへフォールバックすることの確認
TEST_F(traceTest, default_file_path_falls_back_to_relative_log)
{
    // Arrange
    ON_CALL(mock_com_util, com_util_process_get_executable_path(_, _))
        .WillByDefault(Return(
            -1)); // [状態] - com_util_process_get_executable_path が呼び出された際に -1 を返すようにモックを設定する。
    com_util_tracer *handle = create_logger(); // [手順] - 有効名はフォールバックの unknown になる。
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("log/unknown.log"), 0, 0, 0))
        .WillOnce(
            Return(file_handle_)); // [Pre-Assert確認_異常系] - 相対パス log/unknown.log へフォールバックすること。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - start でデフォルト パスを解決させる。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - com_util_tracer_start の戻り値から、start が成功したと判断できること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// トレース ファイルを開けない場合に start が -1 を返しつつ started 状態になることの確認
TEST_F(traceTest, start_returns_minus_one_but_starts_when_file_sink_create_fails)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_stderr_level(handle, COM_UTIL_TRACE_LEVEL_INFO)); // [状態] - stderr レベルを INFO とする。
                                                                                                 // [状態確認] - com_util_tracer_set_stderr_level の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(_, _, _, _))
        .WillOnce(
            Return((com_util_trace_file_sink *)NULL)); // [Pre-Assert確認_異常系] - file sink の生成が失敗すること。

    // Act
    int start_result = com_util_tracer_start(handle); // [手順] - file sink 生成が失敗する状態で start する。
    testing::internal::CaptureStderr();
    int write_result = com_util_tracer_write_at(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                              "still works"); // [手順] - ファイル以外の出力を確認する。
    std::string captured = testing::internal::GetCapturedStderr();

    // Assert
    EXPECT_EQ(-1, start_result); // [確認_異常系] - start が -1 を返すこと。
    EXPECT_EQ(COM_UTIL_TRACER_STATE_STARTED,
              com_util_tracer_get_state(handle)); // [確認_異常系] - それでも started 状態へ遷移すること。
    EXPECT_EQ(
        0,
        write_result); // [確認_異常系] - com_util_tracer_write_at の戻り値から、ファイル以外の書き込みは成功したと判断できること。
    EXPECT_NE(std::string::npos,
              captured.find("I still works")); // [確認_異常系] - stderr 出力が継続すること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}

// stop がトレース ファイルを閉じ、再 start で新しいファイル名のデフォルト パスを開き直すことの確認
TEST_F(traceTest, stop_disposes_file_sink_and_restart_uses_new_name)
{
    // Arrange
    com_util_tracer *handle = create_logger();
    ASSERT_EQ(COM_UTIL_OK, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE)); // [状態] - OS レベルを NONE とする。
                                                                                             // [状態確認] - com_util_tracer_set_os_level の戻り値が COM_UTIL_OK であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("/opt/bin/log/myapp.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - 初回 start で従来のファイル名のパスを開くこと。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_create(StrEq("/opt/bin/log/renamed.log"), 0, 0, 0))
        .WillOnce(Return(file_handle_)); // [Pre-Assert確認_正常系] - 再 start で新しいファイル名のパスを開くこと。
    EXPECT_CALL(mock_com_util, com_util_trace_file_sink_dispose(file_handle_))
        .Times(2); // [Pre-Assert確認_正常系] - stop 時と dispose 時にトレース ファイルが閉じられること。

    // Act
    int actual_ret_tracer_start = com_util_tracer_start(handle); // [手順] - 初回の start を行う。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start); // [確認_正常系] - com_util_tracer_start の戻り値として、初回の start を行った結果が COM_UTIL_OK であること。
    int actual_ret_tracer_stop = com_util_tracer_stop(handle); // [手順] - stop でトレース ファイルを閉じる。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_stop); // [確認_正常系] - com_util_tracer_stop の戻り値として、stop でトレース ファイルを閉じた結果が COM_UTIL_OK であること。
    int actual_ret_tracer_set_file_name =
        com_util_tracer_set_file_name(handle, "renamed", 0); // [手順] - stopped 中にファイル名を "renamed" に変更する。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_set_file_name); // [確認_正常系] - stopped 中にファイル名を "renamed" に変更した com_util_tracer_set_file_name の戻り値が COM_UTIL_OK であること。
    int actual_ret_tracer_start_2 = com_util_tracer_start(handle); // [手順] - 再度 start を行う。
    ASSERT_EQ(
        COM_UTIL_OK,
        actual_ret_tracer_start_2); // [確認_正常系] - com_util_tracer_start の戻り値として、再度 start を行った結果が COM_UTIL_OK であること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}
