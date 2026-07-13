#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <TraceLoggingProvider.h>
    #include <testfw.h>
    #include <com_util/trace/etw.h>

    #include <mutex>
    #include <vector>
    #include <string>
    #include <utility>
    #include <cstdio>

COM_UTIL_ETW_DEFINE_PROVIDER(s_test_provider, "EtwSessionTest",
                             (0x0dfe6031, 0x5678, 0x4688, 0xae, 0xe8, 0x61, 0x13, 0x40, 0x99, 0x7c, 0xaa));

    #define TEST_PROVIDER_GUID "0dfe6031-5678-4688-aee8-611340997caa"

struct EventCollector
{
    std::mutex mtx;
    struct EventRecord
    {
        int level;
        uint32_t process_id;
        bool has_service;
        int64_t timestamp_100ns;
        std::string event_name;
        std::string service;
        std::string message;
    };
    std::vector<EventRecord> events;
};

static void collect_callback(const com_util_etw_event *event, void *context)
{
    EventCollector *collector = static_cast<EventCollector *>(context);
    std::lock_guard<std::mutex> lock(collector->mtx);
    ASSERT_NE((const com_util_etw_event *)NULL, event);
    EventCollector::EventRecord record;
    record.level = event->level;
    record.process_id = event->process_id;
    record.has_service = (event->service != nullptr);
    record.timestamp_100ns = event->timestamp_100ns;
    if (event->event_name != nullptr)
    {
        record.event_name = event->event_name;
    }
    if (event->service != nullptr)
    {
        record.service = event->service;
    }
    if (event->message != nullptr)
    {
        record.message = event->message;
    }
    collector->events.push_back(record);
}

static int s_session_counter = 0;

static std::string make_session_name()
{
    char buf[128];
    snprintf(buf, sizeof(buf), "EtwSessionTest_%lu_%d", (unsigned long)GetCurrentProcessId(), ++s_session_counter);
    return std::string(buf);
}

class etwSessionIntegrationTest : public Test
{
};

// NULL セッションで stop を呼んでも安全なことの確認
TEST_F(etwSessionIntegrationTest, test_session_stop_with_null)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_etw_session_stop(NULL); // [手順] - NULL セッションで com_util_etw_session_stop を呼び出す。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}

// session_start の必須引数 NULL が ERR_PARAM で拒否されることの確認
TEST_F(etwSessionIntegrationTest, test_session_start_null_params)
{
    // Arrange
    int status = COM_UTIL_ETW_SESSION_OK; // [状態] - status の受け取り先を OK で初期化する。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ((com_util_etw_session *)NULL,
              com_util_etw_session_start(NULL, TEST_PROVIDER_GUID, collect_callback, NULL,
                                         &status)); // [手順] - session_name に NULL を渡して session_start を呼び出す。
                                                    // [確認_異常系] - NULL が返ること。
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - status に ERR_PARAM が返ること。

    EXPECT_EQ(
        (com_util_etw_session *)NULL,
        com_util_etw_session_start("test", NULL, collect_callback, NULL,
                                   &status)); // [手順] - provider_guid に NULL を渡して session_start を呼び出す。
                                              // [確認_異常系] - NULL が返ること。
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - status に ERR_PARAM が返ること。

    EXPECT_EQ((com_util_etw_session *)NULL,
              com_util_etw_session_start("test", TEST_PROVIDER_GUID, NULL, NULL,
                                         &status));    // [手順] - callback に NULL を渡して session_start を呼び出す。
                                                       // [確認_異常系] - NULL が返ること。
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - status に ERR_PARAM が返ること。
}

// 不正な GUID 文字列が ERR_PARAM で拒否されることの確認
TEST_F(etwSessionIntegrationTest, test_session_start_invalid_guid)
{
    // Arrange
    int status = COM_UTIL_ETW_SESSION_OK; // [状態] - status の受け取り先を OK で初期化する。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        (com_util_etw_session *)NULL,
        com_util_etw_session_start("test", "not-a-guid", collect_callback, NULL,
                                   &status)); // [手順] - 不正な GUID 文字列 "not-a-guid" で session_start を呼び出す。
                                              // [確認_異常系] - NULL が返ること。
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - status に ERR_PARAM が返ること。
}

class etwSessionSubscribeIntegrationTest : public Test
{
  protected:
    void SetUp() override
    {
        int status = com_util_etw_session_check_access();
        ASSERT_NE(COM_UTIL_ETW_SESSION_ERR_ACCESS, status)
            << "ETW session の開始権限がありません。Administrators または "
               "\"Performance Log Users\" が必要です。\n"
               "対処方法: net localgroup \"Performance Log Users\" %USERNAME% /add\n"
               "          この操作後にサインアウト/サインインが必要です。";
        ASSERT_EQ(COM_UTIL_ETW_SESSION_OK, status)
            << "com_util_etw_session_check_access failed (status=" << status << ")";
    }
};

// ASCII メッセージが session で購読・受信できることの確認
TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_ascii)
{
    // Arrange
    std::string session_name = make_session_name(); // [状態] - プロセス固有の session 名を生成する。
    EventCollector collector;

    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    com_util_etw_session *session =
        com_util_etw_session_start(session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector,
                                   NULL); // [状態] - 収集 callback 付きで ETW session を開始する。
    ASSERT_NE((com_util_etw_session *)NULL, session);

    // Pre-Assert

    // Act
    Sleep(200);
    com_util_etw_provider_write(handle, 4, NULL, "hello world"); // [手順] - ASCII メッセージを書き込む。
    com_util_etw_session_stop(session);                          // [手順] - session を停止して受信を確定させる。

    // Assert
    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == "hello world")
        {
            EXPECT_EQ(4, evt.level);                                    // [確認_正常系] - INFO レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name);                         // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service);              // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected event 'hello world' not found";

    com_util_etw_provider_dispose(handle);
}

// 日本語 UTF-8 メッセージが session で購読・受信できることの確認
TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_utf8_japanese)
{
    // Arrange
    std::string session_name = make_session_name(); // [状態] - プロセス固有の session 名を生成する。
    EventCollector collector;
    const char *msg = "\xe8\xa8\x88\xe7\xae\x97\xe7\xb5\x90\xe6\x9e\x9c: "
                      "\xe6\x88\x90\xe5\x8a\x9f";

    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    com_util_etw_session *session =
        com_util_etw_session_start(session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector,
                                   NULL); // [状態] - 収集 callback 付きで ETW session を開始する。
    ASSERT_NE((com_util_etw_session *)NULL, session);

    // Pre-Assert

    // Act
    Sleep(200);
    com_util_etw_provider_write(handle, 3, NULL, msg); // [手順] - 日本語 UTF-8 メッセージを書き込む。
    com_util_etw_session_stop(session);                // [手順] - session を停止して受信を確定させる。

    // Assert
    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == msg)
        {
            EXPECT_EQ(3, evt.level); // [確認_正常系] - WARNING レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name);                         // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service);              // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected UTF-8 Japanese event not found";

    com_util_etw_provider_dispose(handle);
}

// ASCII と絵文字を含む混在 UTF-8 メッセージが購読・受信できることの確認
TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_utf8_mixed)
{
    // Arrange
    std::string session_name = make_session_name(); // [状態] - プロセス固有の session 名を生成する。
    EventCollector collector;
    const char *msg = "Hello "
                      "\xe4\xb8\x96\xe7\x95\x8c"
                      " "
                      "\xf0\x9f\x8c\x8d"
                      " World";

    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    com_util_etw_session *session =
        com_util_etw_session_start(session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector,
                                   NULL); // [状態] - 収集 callback 付きで ETW session を開始する。
    ASSERT_NE((com_util_etw_session *)NULL, session);

    // Pre-Assert

    // Act
    Sleep(200);
    com_util_etw_provider_write(handle, 2, NULL, msg); // [手順] - 混在 UTF-8 メッセージを書き込む。
    com_util_etw_session_stop(session);                // [手順] - session を停止して受信を確定させる。

    // Assert
    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == msg)
        {
            EXPECT_EQ(2, evt.level); // [確認_正常系] - ERROR レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name);                         // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service);              // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected UTF-8 mixed event not found";

    com_util_etw_provider_dispose(handle);
}

// 全レベルのイベントが購読・受信できることの確認
TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_multiple_levels)
{
    // Arrange
    std::string session_name = make_session_name(); // [状態] - プロセス固有の session 名を生成する。
    EventCollector collector;

    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    com_util_etw_session *session =
        com_util_etw_session_start(session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector,
                                   NULL); // [状態] - 収集 callback 付きで ETW session を開始する。
    ASSERT_NE((com_util_etw_session *)NULL, session);

    // Pre-Assert

    // Act
    Sleep(200);
    com_util_etw_provider_write(handle, 1, NULL, "critical_msg"); // [手順] - CRITICAL を書き込む。
    com_util_etw_provider_write(handle, 2, NULL, "error_msg");    // [手順] - ERROR を書き込む。
    com_util_etw_provider_write(handle, 3, NULL, "warning_msg");  // [手順] - WARNING を書き込む。
    com_util_etw_provider_write(handle, 4, NULL, "info_msg");     // [手順] - INFO を書き込む。
    com_util_etw_provider_write(handle, 5, NULL, "verbose_msg");  // [手順] - VERBOSE を書き込む。
    com_util_etw_session_stop(session);                           // [手順] - session を停止して受信を確定させる。

    // Assert
    bool saw_critical = false;
    bool saw_error = false;
    bool saw_warning = false;
    bool saw_info = false;
    bool saw_verbose = false;

    for (const auto &evt : collector.events)
    {
        if (evt.message == "critical_msg")
        {
            EXPECT_EQ(1, evt.level);
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id);
            EXPECT_EQ("Trace", evt.event_name);
            saw_critical = true;
        }
        else if (evt.message == "error_msg")
        {
            EXPECT_EQ(2, evt.level);
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id);
            EXPECT_EQ("Trace", evt.event_name);
            saw_error = true;
        }
        else if (evt.message == "warning_msg")
        {
            EXPECT_EQ(3, evt.level);
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id);
            EXPECT_EQ("Trace", evt.event_name);
            saw_warning = true;
        }
        else if (evt.message == "info_msg")
        {
            EXPECT_EQ(4, evt.level);
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id);
            EXPECT_EQ("Trace", evt.event_name);
            saw_info = true;
        }
        else if (evt.message == "verbose_msg")
        {
            EXPECT_EQ(5, evt.level);
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id);
            EXPECT_EQ("Trace", evt.event_name);
            saw_verbose = true;
        }
        EXPECT_FALSE(evt.has_service);              // [確認_正常系] - 既存ケースでは Service なしで受信されること。
        EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - 各イベントに ETW タイムスタンプが設定されること。
    }

    EXPECT_TRUE(saw_critical); // [確認_正常系] - CRITICAL が受信されること。
    EXPECT_TRUE(saw_error);    // [確認_正常系] - ERROR が受信されること。
    EXPECT_TRUE(saw_warning);  // [確認_正常系] - WARNING が受信されること。
    EXPECT_TRUE(saw_info);     // [確認_正常系] - INFO が受信されること。
    EXPECT_TRUE(saw_verbose);  // [確認_正常系] - VERBOSE が受信されること。

    com_util_etw_provider_dispose(handle);
}

// 空文字列メッセージが購読・受信できることの確認
TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_empty_string)
{
    // Arrange
    std::string session_name = make_session_name(); // [状態] - プロセス固有の session 名を生成する。
    EventCollector collector;

    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    com_util_etw_session *session =
        com_util_etw_session_start(session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector,
                                   NULL); // [状態] - 収集 callback 付きで ETW session を開始する。
    ASSERT_NE((com_util_etw_session *)NULL, session);

    // Pre-Assert

    // Act
    Sleep(200);
    com_util_etw_provider_write(handle, 5, NULL, ""); // [手順] - 空文字列を書き込む。
    com_util_etw_session_stop(session);               // [手順] - session を停止して受信を確定させる。

    // Assert
    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message.empty())
        {
            EXPECT_EQ(5, evt.level); // [確認_正常系] - 空文字列が VERBOSE で受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name);                         // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service);              // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected empty-string event not found";

    com_util_etw_provider_dispose(handle);
}

// Service と Message を持つイベントが購読・受信できることの確認
TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_service_and_message)
{
    // Arrange
    std::string session_name = make_session_name(); // [状態] - プロセス固有の session 名を生成する。
    EventCollector collector;

    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    com_util_etw_session *session =
        com_util_etw_session_start(session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector,
                                   NULL); // [状態] - 収集 callback 付きで ETW session を開始する。
    ASSERT_NE((com_util_etw_session *)NULL, session);

    // Pre-Assert

    // Act
    Sleep(200);
    com_util_etw_provider_write(handle, 4, "worker-1",
                                "service_msg"); // [手順] - Service と Message を持つイベントを書き込む。
    com_util_etw_session_stop(session);         // [手順] - session を停止して受信を確定させる。

    // Assert
    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == "service_msg")
        {
            EXPECT_EQ(4, evt.level);                                    // [確認_正常系] - INFO レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name);                         // [確認_正常系] - イベント名が受信されること。
            EXPECT_TRUE(evt.has_service);               // [確認_正常系] - Service フィールドが受信されること。
            EXPECT_EQ("worker-1", evt.service);         // [確認_正常系] - Service 名が復元されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected event with service field not found";

    com_util_etw_provider_dispose(handle);
}

#elif defined(PLATFORM_LINUX)

    #include <testfw.h>

TEST(etwSessionIntegrationTest, not_supported)
{
    GTEST_SKIP() << "ETW session integration is not supported on this platform";
}

#endif /* PLATFORM_ */
