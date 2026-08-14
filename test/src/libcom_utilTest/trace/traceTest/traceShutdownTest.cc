#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/trace/tracer.h>
#include <com_util/trace/tracer_internal.h>

#include "tracer.inject.h"
#include "traceSyncMock.h"

using testing::_;
using testing::NiceMock;
using testing::Return;

namespace
{

class traceShutdownTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_;
    com_util_shutdown_fn shutdown_callback_ = nullptr;
    void *shutdown_context_ = nullptr;

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink *os_handle_ = reinterpret_cast<com_util_syslog_sink *>(static_cast<uintptr_t>(0x1100));
#elif defined(PLATFORM_WINDOWS)
    com_util_etw_provider *os_handle_ = reinterpret_cast<com_util_etw_provider *>(static_cast<uintptr_t>(0x1100));
#endif

    void SetUp() override
    {
        set_trace_sync_mock_defaults(mock_);
        test_trace_registry_reset_shutdown_state();
        ON_CALL(mock_, com_util_shutdown_register(_, _))
            .WillByDefault(
                [this](com_util_shutdown_fn callback, void *context)
                {
                    shutdown_callback_ = callback;
                    shutdown_context_ = context;
                    return COM_UTIL_OK;
                });

#if defined(PLATFORM_LINUX)
        ON_CALL(mock_, com_util_syslog_sink_create(_, _)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_, com_util_syslog_sink_dispose(_)).WillByDefault(Return());
        ON_CALL(mock_, com_util_syslog_sink_rename(_, _)).WillByDefault(Return(COM_UTIL_OK));
#elif defined(PLATFORM_WINDOWS)
        ON_CALL(mock_, com_util_etw_provider_create(_)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_, com_util_etw_provider_dispose(_)).WillByDefault(Return());
#endif
    }

    void TearDown() override
    {
        // shutdown_started は本来戻らないため、同一バイナリの他テストが tracer を生成できるよう初期状態へ戻す。
        test_trace_registry_reset_shutdown_state();
    }
};

} // namespace

// 共通 shutdown で registry が破棄され、以後の tracer 生成が拒否されることの確認
TEST_F(traceShutdownTest, shutdown_disposes_registry_and_rejects_new_create)
{
    // Arrange
    com_util_tracer *handle = com_util_tracer_create(
        COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED); // [状態] - tracer を 1 件生成し registry に登録された状態とする。
    ASSERT_NE((com_util_tracer *)NULL, handle); // [状態確認] - ハンドルが非 NULL であること。
    EXPECT_EQ((size_t)1, trace_registry_count()); // [状態確認] - registry の登録件数が 1 件であること。

    com_util_shutdown_event event = {COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                                     0}; // [状態] - 通常終了 (NORMAL_EXIT) の shutdown イベントを用意する。

    // Pre-Assert
    ASSERT_NE(nullptr, shutdown_callback_); // [Pre-Assert確認_正常系] - tracer の shutdown callback が登録されること。

    // Act
    shutdown_callback_(&event, shutdown_context_); // [手順] - 登録された tracer shutdown callback を直接呼び出す。
    com_util_tracer *created_after_shutdown = com_util_tracer_create(
        COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED); // [手順] - shutdown 後に新しい tracer の生成を試みる。

    // Assert
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_正常系] - shutdown 後に registry が空になること。
    EXPECT_EQ(
        (com_util_tracer *)NULL,
        created_after_shutdown); // [確認_正常系] - com_util_tracer_create の戻り値として、shutdown 開始後は新規 tracer 作成が拒否され NULL が返ること。
}
