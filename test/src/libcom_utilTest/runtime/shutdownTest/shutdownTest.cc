#include <testfw.h>
#include <com_util/runtime/shutdown.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#if defined(PLATFORM_LINUX)
#include <signal.h>
#elif defined(PLATFORM_WINDOWS)
#include <com_util/base/windows_sdk.h>
#endif

namespace
{

int g_call_count = 0;
int g_order[8] = {};
com_util_shutdown_event_t g_last_event = {};

void reset_records()
{
    g_call_count = 0;
    memset(g_order, 0, sizeof(g_order));
    g_last_event.reason = COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT;
    g_last_event.code_kind = COM_UTIL_SHUTDOWN_CODE_KIND_NONE;
    g_last_event.code = 0;
}

com_util_shutdown_event_t make_event(com_util_shutdown_reason_t reason,
                                     com_util_shutdown_code_kind_t code_kind,
                                     int code)
{
    com_util_shutdown_event_t event;
    event.reason = reason;
    event.code_kind = code_kind;
    event.code = code;
    return event;
}

void record_callback(const com_util_shutdown_event_t *event, void *context)
{
    int id = *static_cast<int *>(context);
    g_order[g_call_count++] = id;
    g_last_event = *event;
}

void print_callback(const com_util_shutdown_event_t *event, void *)
{
    fprintf(stderr, "reason=%d kind=%d code=%d\n",
            (int)event->reason, (int)event->code_kind, event->code);
}

void print_count_callback(const com_util_shutdown_event_t *, void *)
{
    static int count = 0;
    count++;
    fprintf(stderr, "count=%d\n", count);
}

} // namespace

class shutdownTest : public Test
{
protected:
    void SetUp() override
    {
        _com_util_shutdown_reset_for_test();
        reset_records();
    }

    void TearDown() override
    {
        _com_util_shutdown_reset_for_test();
    }
};

TEST_F(shutdownTest, test_callbacks_are_invoked_in_lifo_order)
{
    // Arrange
    int first = 1;
    int second = 2;
    int third = 3;
    com_util_shutdown_event_t event =
        make_event(COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                   COM_UTIL_SHUTDOWN_CODE_KIND_EXIT_CODE,
                   7); // [状態] - 終了コード 7 を持つ通常終了イベントを使う。

    ASSERT_EQ(0, com_util_shutdown_register(record_callback, &first));
    ASSERT_EQ(0, com_util_shutdown_register(record_callback, &second));
    ASSERT_EQ(0, com_util_shutdown_register(record_callback, &third));

    // Act
    int result = _com_util_shutdown_invoke_for_test(&event); // [手順] - 3 件の callback を shutdown イベントで実行する。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - shutdown 実行が成功すること。
    ASSERT_EQ(3, g_call_count); // [確認_正常系] - callback が 3 回呼ばれること。
    EXPECT_EQ(3, g_order[0]); // [確認_正常系] - 最後に登録した callback から実行されること。
    EXPECT_EQ(2, g_order[1]); // [確認_正常系] - 2 番目に登録した callback が続くこと。
    EXPECT_EQ(1, g_order[2]); // [確認_正常系] - 最初に登録した callback が最後に実行されること。
    EXPECT_EQ(COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, g_last_event.reason); // [確認_正常系] - 終了理由が callback に渡ること。
    EXPECT_EQ(COM_UTIL_SHUTDOWN_CODE_KIND_EXIT_CODE, g_last_event.code_kind); // [確認_正常系] - code_kind が callback に渡ること。
    EXPECT_EQ(7, g_last_event.code); // [確認_正常系] - 終了コード 7 が callback に渡ること。
}

TEST_F(shutdownTest, test_multiple_invoke_runs_callbacks_only_once)
{
    // Arrange
    int id = 1;
    com_util_shutdown_event_t event =
        make_event(COM_UTIL_SHUTDOWN_REASON_PROCESS_TERMINATING,
                   COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                   0); // [状態] - 終了中イベントを使う。

    ASSERT_EQ(0, com_util_shutdown_register(record_callback, &id));

    // Act
    int first_result = _com_util_shutdown_invoke_for_test(&event);  // [手順] - 1 回目の shutdown を実行する。
    int second_result = _com_util_shutdown_invoke_for_test(&event); // [手順] - 2 回目の shutdown を実行する。
    int register_after_shutdown = com_util_shutdown_register(record_callback, &id); // [手順] - shutdown 後に新規登録を試みる。

    // Assert
    EXPECT_EQ(0, first_result); // [確認_正常系] - 1 回目は実行されること。
    EXPECT_EQ(1, second_result); // [確認_正常系] - 2 回目は再実行されないこと。
    EXPECT_EQ(-1, register_after_shutdown); // [確認_正常系] - shutdown 開始後の登録が拒否されること。
    EXPECT_EQ(1, g_call_count); // [確認_正常系] - callback は 1 回だけ実行されること。
}

TEST_F(shutdownTest, test_request_callbacks_do_not_consume_final_shutdown)
{
    // Arrange
    int request_first = 1;
    int request_second = 2;
    int final_id = 3;
    com_util_shutdown_event_t request_event =
        make_event(COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT,
                   COM_UTIL_SHUTDOWN_CODE_KIND_SIGNAL_NUMBER,
                   2);
    com_util_shutdown_event_t final_event =
        make_event(COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                   COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                   0);

    ASSERT_EQ(0, com_util_shutdown_request_register(record_callback, &request_first));
    ASSERT_EQ(0, com_util_shutdown_request_register(record_callback, &request_second));
    ASSERT_EQ(0, com_util_shutdown_register(record_callback, &final_id));

    // Act
    int request_result = _com_util_shutdown_request_invoke_for_test(&request_event);

    // Assert
    EXPECT_EQ(0, request_result); // [確認_正常系] - 終了要求 callback の実行が成功すること。
    ASSERT_EQ(2, g_call_count); // [確認_正常系] - request callback が 2 件だけ実行されること。
    EXPECT_EQ(2, g_order[0]); // [確認_正常系] - request callback も LIFO 順で実行されること。
    EXPECT_EQ(1, g_order[1]); // [確認_正常系] - 最初の request callback が最後に実行されること。
    EXPECT_EQ(COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT, g_last_event.reason); // [確認_正常系] - request イベントが渡ること。

    reset_records();

    int final_result = _com_util_shutdown_invoke_for_test(&final_event);

    EXPECT_EQ(0, final_result); // [確認_正常系] - final shutdown が別途実行できること。
    ASSERT_EQ(1, g_call_count); // [確認_正常系] - final callback は独立して 1 件実行されること。
    EXPECT_EQ(3, g_order[0]); // [確認_正常系] - final callback が実行されること。
    EXPECT_EQ(COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT, g_last_event.reason); // [確認_正常系] - final shutdown では通常終了イベントが渡ること。
}

TEST_F(shutdownTest, test_request_callback_runs_only_once)
{
    // Arrange
    int id = 1;
    com_util_shutdown_event_t event =
        make_event(COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT,
                   COM_UTIL_SHUTDOWN_CODE_KIND_SIGNAL_NUMBER,
                   2);

    ASSERT_EQ(0, com_util_shutdown_request_register(record_callback, &id));

    // Act
    int first_result = _com_util_shutdown_request_invoke_for_test(&event);
    int second_result = _com_util_shutdown_request_invoke_for_test(&event);
    int register_after_request = com_util_shutdown_request_register(record_callback, &id);

    // Assert
    EXPECT_EQ(0, first_result); // [確認_正常系] - 1 回目は実行されること。
    EXPECT_EQ(1, second_result); // [確認_正常系] - 2 回目は再実行されないこと。
    EXPECT_EQ(-1, register_after_request); // [確認_正常系] - 通知後の登録は拒否されること。
    EXPECT_EQ(1, g_call_count); // [確認_正常系] - request callback は 1 回だけ実行されること。
}

TEST_F(shutdownTest, test_com_util_exit_preserves_exit_code)
{
    EXPECT_EXIT(
        {
            _com_util_shutdown_reset_for_test();
            com_util_shutdown_register(print_callback, NULL);
            com_util_exit(7);
        },
        ::testing::ExitedWithCode(7),
        "reason=0 kind=1 code=7");
}

TEST_F(shutdownTest, test_explicit_invoke_prevents_atexit_double_execution)
{
    EXPECT_EXIT(
        {
            com_util_shutdown_event_t event =
                make_event(COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT,
                           COM_UTIL_SHUTDOWN_CODE_KIND_NONE,
                           0);
            _com_util_shutdown_reset_for_test();
            com_util_shutdown_register(print_count_callback, NULL);
            _com_util_shutdown_invoke_for_test(&event);
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        "count=1");
}

#if defined(PLATFORM_LINUX)
TEST_F(shutdownTest, test_sigint_is_reported_to_callback)
{
    EXPECT_EXIT(
        {
            _com_util_shutdown_reset_for_test();
            com_util_shutdown_request_register(print_callback, NULL);
            raise(SIGINT);
            fprintf(stderr, "after-sigint\n");
            exit(0);
        },
        ::testing::ExitedWithCode(0),
        "reason=2 kind=2 code=2.*after-sigint");
}
#elif defined(PLATFORM_WINDOWS)
TEST_F(shutdownTest, test_console_event_is_reported_to_callback)
{
    // Arrange
    int id = 1;
    com_util_shutdown_event_t event =
        make_event(COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT,
                   COM_UTIL_SHUTDOWN_CODE_KIND_CONSOLE_CTRL_TYPE,
                   CTRL_C_EVENT); // [状態] - CTRL_C_EVENT を模擬したイベントを使う。

    ASSERT_EQ(0, com_util_shutdown_request_register(record_callback, &id));

    // Act
    int result = _com_util_shutdown_request_invoke_for_test(&event); // [手順] - CTRL_C_EVENT 相当の request callback を実行する。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - request callback 実行が成功すること。
    EXPECT_EQ(1, g_call_count); // [確認_正常系] - callback が 1 回実行されること。
    EXPECT_EQ(COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT, g_last_event.reason); // [確認_正常系] - コンソール イベントの理由が渡ること。
    EXPECT_EQ(COM_UTIL_SHUTDOWN_CODE_KIND_CONSOLE_CTRL_TYPE, g_last_event.code_kind); // [確認_正常系] - CTRL 種別として渡ること。
    EXPECT_EQ((int)CTRL_C_EVENT, g_last_event.code); // [確認_正常系] - CTRL_C_EVENT の値が渡ること。
}
#endif
