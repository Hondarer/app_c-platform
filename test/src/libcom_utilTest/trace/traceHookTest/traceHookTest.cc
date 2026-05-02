/**
 *******************************************************************************
 *  @file           traceHookTest.cc
 *  @brief          com_util_tracer フック API の単体テスト。
 *  @author         Tetsuo Honda
 *  @date           2026/05/02
 *  @version        1.0.0
 *******************************************************************************
 */
#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/trace/tracer.h>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdint>
#include <vector>

#include <com_util/trace/tracer_internal.h>

#if defined(PLATFORM_LINUX)
    #include <syslog.h>
#endif

using testing::_;
using testing::NiceMock;
using testing::NotNull;
using testing::Return;

namespace
{

static void set_valid_deadline(struct timespec *abs_timeout)
{
    abs_timeout->tv_sec = (time_t)(time(NULL) + 1);
    abs_timeout->tv_nsec = 0;
}

static void set_fixed_realtime(int64_t *tv_sec, int32_t *tv_nsec)
{
    *tv_sec  = 1714100645LL;
    *tv_nsec = 678000000;
}

static com_util_realtime_timestamp_t make_fixed_timestamp(void)
{
    com_util_realtime_timestamp_t ts;
    ts.tv_sec  = 1714100645LL;
    ts.tv_nsec = 678000000;
    ts.reserved = 0;
    return ts;
}

struct HookRecord
{
    com_util_tracer_hook_entry_t *prev;
    com_util_tracer_t            *handle;
    com_util_realtime_timestamp_t timestamp;
    std::string                   message;
    void                         *context;
    com_util_trace_level_t        level;
};

static std::vector<HookRecord> g_hook_records;

static void reset_hook_records()
{
    g_hook_records.clear();
}

static void recording_hook(com_util_tracer_hook_entry_t *prev,
                           com_util_tracer_t *handle,
                           com_util_trace_level_t level,
                           const com_util_realtime_timestamp_t *timestamp,
                           const char *message,
                           void *context)
{
    HookRecord rec;
    rec.prev    = prev;
    rec.handle  = handle;
    rec.level   = level;
    rec.timestamp = *timestamp;
    rec.message = (message != NULL) ? message : "";
    rec.context = context;
    g_hook_records.push_back(rec);
}

} // namespace

class traceHookTest : public Test
{
protected:
    NiceMock<Mock_com_util> mock_;
    com_util_trace_file_sink_t *file_handle_ =
        reinterpret_cast<com_util_trace_file_sink_t *>(static_cast<uintptr_t>(0x2200));

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink_t *os_handle_ =
        reinterpret_cast<com_util_syslog_sink_t *>(static_cast<uintptr_t>(0x1100));
#elif defined(PLATFORM_WINDOWS)
    com_util_etw_provider_t *os_handle_ =
        reinterpret_cast<com_util_etw_provider_t *>(static_cast<uintptr_t>(0x1100));
#endif

    void SetUp() override
    {
        reset_hook_records();

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
        ON_CALL(mock_, com_util_trace_file_sink_create(_, _, _))
            .WillByDefault(Return(file_handle_));
        ON_CALL(mock_, com_util_trace_file_sink_write(_, _, _, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_trace_file_sink_dispose(_))
            .WillByDefault(Return());

#if defined(PLATFORM_LINUX)
        ON_CALL(mock_, com_util_syslog_sink_create(_, _))
            .WillByDefault(Return(os_handle_));
        ON_CALL(mock_, com_util_syslog_sink_write(_, _, _, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_syslog_sink_rename(_, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_syslog_sink_dispose(_))
            .WillByDefault(Return());
#elif defined(PLATFORM_WINDOWS)
        ON_CALL(mock_, com_util_etw_provider_create(_))
            .WillByDefault(Return(os_handle_));
        ON_CALL(mock_, com_util_etw_provider_write(_, _, _, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_, com_util_etw_provider_dispose(_))
            .WillByDefault(Return());
#endif
    }

    com_util_tracer_t *create_tracer()
    {
        com_util_tracer_t *handle = com_util_tracer_create();
        EXPECT_NE((com_util_tracer_t *)NULL, handle);
        return handle;
    }
};

// フックが未設定のとき set_hook は有効なエントリを返す
TEST_F(traceHookTest, test_set_hook_returns_non_null)
{
    // Arrange
    com_util_tracer_t *tracer = create_tracer();

    // Act
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);

    // Assert
    EXPECT_NE((com_util_tracer_hook_entry_t *)NULL, entry);

    com_util_tracer_remove_hook(tracer, entry);
    com_util_tracer_dispose(tracer);
}

// handle が NULL のとき set_hook は NULL を返す
TEST_F(traceHookTest, test_set_hook_null_handle_returns_null)
{
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(nullptr, recording_hook, nullptr);
    EXPECT_EQ((com_util_tracer_hook_entry_t *)NULL, entry);
}

// fn が NULL のとき set_hook は NULL を返す
TEST_F(traceHookTest, test_set_hook_null_fn_returns_null)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, nullptr, nullptr);
    EXPECT_EQ((com_util_tracer_hook_entry_t *)NULL, entry);
    com_util_tracer_dispose(tracer);
}

// started 状態では set_hook は NULL を返す
TEST_F(traceHookTest, test_set_hook_while_started_returns_null)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_start(tracer);

    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);
    EXPECT_EQ((com_util_tracer_hook_entry_t *)NULL, entry);

    com_util_tracer_stop(tracer);
    com_util_tracer_dispose(tracer);
}

// フックを登録してから write するとコールバックが呼ばれる
TEST_F(traceHookTest, test_hook_is_called_on_write)
{
    // Arrange
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, reinterpret_cast<void *>(0xABCD));
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, entry);
    com_util_tracer_start(tracer);

    com_util_realtime_timestamp_t ts = make_fixed_timestamp();

    // Act
    int rc = _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, &ts, "hello hook");

    // Assert
    EXPECT_EQ(0, rc);
    ASSERT_EQ(1u, g_hook_records.size());
    EXPECT_EQ(tracer,                    g_hook_records[0].handle);
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_INFO, g_hook_records[0].level);
    EXPECT_EQ("hello hook",              g_hook_records[0].message);
    EXPECT_EQ(reinterpret_cast<void *>(0xABCD), g_hook_records[0].context);
    EXPECT_EQ(ts.tv_sec,                 g_hook_records[0].timestamp.tv_sec);
    EXPECT_EQ(ts.tv_nsec,                g_hook_records[0].timestamp.tv_nsec);

    com_util_tracer_stop(tracer);
    com_util_tracer_remove_hook(tracer, entry);
    com_util_tracer_dispose(tracer);
}

// COM_UTIL_TRACE_LEVEL_NONE で要求した場合もフックは呼ばれる
TEST_F(traceHookTest, test_hook_is_called_for_none_level)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, entry);
    com_util_tracer_start(tracer);

    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_NONE, &ts, "none level message");

    ASSERT_EQ(1u, g_hook_records.size());
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_NONE, g_hook_records[0].level);
    EXPECT_EQ("none level message",      g_hook_records[0].message);

    com_util_tracer_stop(tracer);
    com_util_tracer_remove_hook(tracer, entry);
    com_util_tracer_dispose(tracer);
}

// フックが設定されていない場合は write が通常通り成功する (性能パス)
TEST_F(traceHookTest, test_no_hook_write_succeeds)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_start(tracer);

    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    int rc = _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, &ts, "no hook");

    EXPECT_EQ(0, rc);
    EXPECT_EQ(0u, g_hook_records.size());

    com_util_tracer_stop(tracer);
    com_util_tracer_dispose(tracer);
}

// remove_hook 後はコールバックが呼ばれない
TEST_F(traceHookTest, test_hook_not_called_after_remove)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, entry);

    // 解除
    com_util_tracer_remove_hook(tracer, entry);

    com_util_tracer_start(tracer);
    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, &ts, "after remove");

    EXPECT_EQ(0u, g_hook_records.size());

    com_util_tracer_stop(tracer);
    com_util_tracer_dispose(tracer);
}

// 複数フックのチェーン: 最後に登録したものから順に呼ばれる
TEST_F(traceHookTest, test_hook_chain_order)
{
    static std::vector<int> call_order;
    call_order.clear();

    struct HookCtx
    {
        int id;
    };
    static HookCtx ctx1 = {1};
    static HookCtx ctx2 = {2};

    auto chain_fn = [](com_util_tracer_hook_entry_t *prev,
                       com_util_tracer_t *handle,
                       com_util_trace_level_t level,
                       const com_util_realtime_timestamp_t *timestamp,
                       const char *message,
                       void *context) {
        HookCtx *c = reinterpret_cast<HookCtx *>(context);
        call_order.push_back(c->id);
        com_util_tracer_call_next_hook(prev, handle, level, timestamp, message);
    };

    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *e1 =
        com_util_tracer_set_hook(tracer, chain_fn, &ctx1);
    com_util_tracer_hook_entry_t *e2 =
        com_util_tracer_set_hook(tracer, chain_fn, &ctx2);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, e1);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, e2);

    com_util_tracer_start(tracer);
    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, &ts, "chain test");

    // 最後に登録した e2 (id=2) が最初に呼ばれ、次に e1 (id=1) が呼ばれる
    ASSERT_EQ(2u, call_order.size());
    EXPECT_EQ(2, call_order[0]);
    EXPECT_EQ(1, call_order[1]);

    com_util_tracer_stop(tracer);
    com_util_tracer_remove_hook(tracer, e2);
    com_util_tracer_remove_hook(tracer, e1);
    com_util_tracer_dispose(tracer);
}

// call_next_hook に NULL を渡しても何も起きない
TEST_F(traceHookTest, test_call_next_hook_null_prev)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_start(tracer);

    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    EXPECT_NO_FATAL_FAILURE(
        com_util_tracer_call_next_hook(nullptr, tracer, COM_UTIL_TRACE_LEVEL_INFO, &ts, "test")
    );

    com_util_tracer_stop(tracer);
    com_util_tracer_dispose(tracer);
}

// writef 経由でもフックが呼ばれる
TEST_F(traceHookTest, test_hook_called_via_writef)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, entry);
    com_util_tracer_start(tracer);

    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    _com_util_tracer_writef(tracer, COM_UTIL_TRACE_LEVEL_WARNING, &ts, "fmt %d", 42);

    ASSERT_EQ(1u, g_hook_records.size());
    EXPECT_EQ(COM_UTIL_TRACE_LEVEL_WARNING, g_hook_records[0].level);
    // _com_util_tracer_writef はフォーマット展開後の文字列が渡される
    EXPECT_NE(std::string::npos, g_hook_records[0].message.find("42"));

    com_util_tracer_stop(tracer);
    com_util_tracer_remove_hook(tracer, entry);
    com_util_tracer_dispose(tracer);
}

// タイムスタンプが NULL でも write_dual 内で解決されたタイムスタンプがフックに渡る
TEST_F(traceHookTest, test_hook_receives_resolved_timestamp)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, entry);
    com_util_tracer_start(tracer);

    // timestamp=NULL で呼び出す → 内部で現在時刻を取得
    _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, nullptr, "ts resolve test");

    ASSERT_EQ(1u, g_hook_records.size());
    // set_fixed_realtime によりモックで固定された値と一致するはず
    EXPECT_EQ(1714100645LL, g_hook_records[0].timestamp.tv_sec);
    EXPECT_EQ(678000000,    g_hook_records[0].timestamp.tv_nsec);

    com_util_tracer_stop(tracer);
    com_util_tracer_remove_hook(tracer, entry);
    com_util_tracer_dispose(tracer);
}

// started 状態では remove_hook は何もしない
TEST_F(traceHookTest, test_remove_hook_while_started_does_nothing)
{
    com_util_tracer_t *tracer = create_tracer();
    com_util_tracer_hook_entry_t *entry =
        com_util_tracer_set_hook(tracer, recording_hook, nullptr);
    ASSERT_NE((com_util_tracer_hook_entry_t *)NULL, entry);
    com_util_tracer_start(tracer);

    // started 中は解除できない
    com_util_tracer_remove_hook(tracer, entry);

    com_util_realtime_timestamp_t ts = make_fixed_timestamp();
    _com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, &ts, "hook still active");

    // フックはまだ有効
    EXPECT_EQ(1u, g_hook_records.size());

    com_util_tracer_stop(tracer);
    com_util_tracer_remove_hook(tracer, entry);
    com_util_tracer_dispose(tracer);
}
