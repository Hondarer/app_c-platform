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

COM_UTIL_ETW_DEFINE_PROVIDER(
    s_test_provider,
    "EtwSessionTest",
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

static void collect_callback(const com_util_etw_event_t *event, void *context)
{
    EventCollector *collector = static_cast<EventCollector *>(context);
    std::lock_guard<std::mutex> lock(collector->mtx);
    ASSERT_NE((const com_util_etw_event_t *)NULL, event);
    collector->events.push_back(EventCollector::EventRecord{
        event->level,
        event->process_id,
        event->service != nullptr,
        event->timestamp_100ns,
        event->event_name ? event->event_name : "",
        event->service ? event->service : "",
        event->message ? event->message : ""});
}

static int s_session_counter = 0;

static std::string make_session_name()
{
    char buf[128];
    snprintf(buf, sizeof(buf), "EtwSessionTest_%lu_%d",
             (unsigned long)GetCurrentProcessId(), ++s_session_counter);
    return std::string(buf);
}

class etwSessionIntegrationTest : public Test
{
};

TEST_F(etwSessionIntegrationTest, test_session_stop_with_null)
{
    com_util_etw_session_stop(NULL); // [手順] - NULL セッションで stop を呼ぶ。
}

TEST_F(etwSessionIntegrationTest, test_session_start_null_params)
{
    int status = COM_UTIL_ETW_SESSION_OK;

    EXPECT_EQ((com_util_etw_session_t *)NULL,
        com_util_etw_session_start(NULL, TEST_PROVIDER_GUID, collect_callback, NULL, &status));
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - session_name NULL で PARAM が返ること。

    EXPECT_EQ((com_util_etw_session_t *)NULL,
        com_util_etw_session_start("test", NULL, collect_callback, NULL, &status));
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - provider_guid NULL で PARAM が返ること。

    EXPECT_EQ((com_util_etw_session_t *)NULL,
        com_util_etw_session_start("test", TEST_PROVIDER_GUID, NULL, NULL, &status));
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - callback NULL で PARAM が返ること。
}

TEST_F(etwSessionIntegrationTest, test_session_start_invalid_guid)
{
    int status = COM_UTIL_ETW_SESSION_OK;

    EXPECT_EQ((com_util_etw_session_t *)NULL,
        com_util_etw_session_start("test", "not-a-guid", collect_callback, NULL, &status));
    EXPECT_EQ(COM_UTIL_ETW_SESSION_ERR_PARAM, status); // [確認_異常系] - 不正 GUID で PARAM が返ること。
}

class etwSessionSubscribeIntegrationTest : public Test
{
protected:
    void SetUp() override
    {
        int status = com_util_etw_session_check_access();
        ASSERT_NE(COM_UTIL_ETW_SESSION_ERR_ACCESS, status)
            << "ETW session requires 'Performance Log Users' group membership.\n"
               "Run: net localgroup \"Performance Log Users\" %USERNAME% /add";
        ASSERT_EQ(COM_UTIL_ETW_SESSION_OK, status)
            << "com_util_etw_session_check_access failed (status=" << status << ")";
    }
};

TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_ascii)
{
    std::string session_name = make_session_name();
    EventCollector collector;

    com_util_etw_provider_t *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider_t *)NULL, handle);

    com_util_etw_session_t *session = com_util_etw_session_start(
        session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector, NULL);
    ASSERT_NE((com_util_etw_session_t *)NULL, session);

    Sleep(200);
    com_util_etw_provider_write(handle, 4, NULL, "hello world"); // [手順] - ASCII メッセージを書き込む。
    com_util_etw_session_stop(session);

    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == "hello world")
        {
            EXPECT_EQ(4, evt.level); // [確認_正常系] - INFO レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name); // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service); // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected event 'hello world' not found";

    com_util_etw_provider_dispose(handle);
}

TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_utf8_japanese)
{
    std::string session_name = make_session_name();
    EventCollector collector;
    const char *msg = "\xe8\xa8\x88\xe7\xae\x97\xe7\xb5\x90\xe6\x9e\x9c: "
                      "\xe6\x88\x90\xe5\x8a\x9f";

    com_util_etw_provider_t *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider_t *)NULL, handle);

    com_util_etw_session_t *session = com_util_etw_session_start(
        session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector, NULL);
    ASSERT_NE((com_util_etw_session_t *)NULL, session);

    Sleep(200);
    com_util_etw_provider_write(handle, 3, NULL, msg); // [手順] - 日本語 UTF-8 メッセージを書き込む。
    com_util_etw_session_stop(session);

    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == msg)
        {
            EXPECT_EQ(3, evt.level); // [確認_正常系] - WARNING レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name); // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service); // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected UTF-8 Japanese event not found";

    com_util_etw_provider_dispose(handle);
}

TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_utf8_mixed)
{
    std::string session_name = make_session_name();
    EventCollector collector;
    const char *msg = "Hello "
                      "\xe4\xb8\x96\xe7\x95\x8c"
                      " "
                      "\xf0\x9f\x8c\x8d"
                      " World";

    com_util_etw_provider_t *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider_t *)NULL, handle);

    com_util_etw_session_t *session = com_util_etw_session_start(
        session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector, NULL);
    ASSERT_NE((com_util_etw_session_t *)NULL, session);

    Sleep(200);
    com_util_etw_provider_write(handle, 2, NULL, msg); // [手順] - 混在 UTF-8 メッセージを書き込む。
    com_util_etw_session_stop(session);

    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == msg)
        {
            EXPECT_EQ(2, evt.level); // [確認_正常系] - ERROR レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name); // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service); // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected UTF-8 mixed event not found";

    com_util_etw_provider_dispose(handle);
}

TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_multiple_levels)
{
    std::string session_name = make_session_name();
    EventCollector collector;

    com_util_etw_provider_t *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider_t *)NULL, handle);

    com_util_etw_session_t *session = com_util_etw_session_start(
        session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector, NULL);
    ASSERT_NE((com_util_etw_session_t *)NULL, session);

    Sleep(200);
    com_util_etw_provider_write(handle, 1, NULL, "critical_msg"); // [手順] - CRITICAL を書き込む。
    com_util_etw_provider_write(handle, 2, NULL, "error_msg");    // [手順] - ERROR を書き込む。
    com_util_etw_provider_write(handle, 3, NULL, "warning_msg");  // [手順] - WARNING を書き込む。
    com_util_etw_provider_write(handle, 4, NULL, "info_msg");     // [手順] - INFO を書き込む。
    com_util_etw_provider_write(handle, 5, NULL, "verbose_msg");  // [手順] - VERBOSE を書き込む。
    com_util_etw_session_stop(session);

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
        EXPECT_FALSE(evt.has_service); // [確認_正常系] - 既存ケースでは Service なしで受信されること。
        EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - 各イベントに ETW タイムスタンプが設定されること。
    }

    EXPECT_TRUE(saw_critical); // [確認_正常系] - CRITICAL が受信されること。
    EXPECT_TRUE(saw_error);    // [確認_正常系] - ERROR が受信されること。
    EXPECT_TRUE(saw_warning);  // [確認_正常系] - WARNING が受信されること。
    EXPECT_TRUE(saw_info);     // [確認_正常系] - INFO が受信されること。
    EXPECT_TRUE(saw_verbose);  // [確認_正常系] - VERBOSE が受信されること。

    com_util_etw_provider_dispose(handle);
}

TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_empty_string)
{
    std::string session_name = make_session_name();
    EventCollector collector;

    com_util_etw_provider_t *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider_t *)NULL, handle);

    com_util_etw_session_t *session = com_util_etw_session_start(
        session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector, NULL);
    ASSERT_NE((com_util_etw_session_t *)NULL, session);

    Sleep(200);
    com_util_etw_provider_write(handle, 5, NULL, ""); // [手順] - 空文字列を書き込む。
    com_util_etw_session_stop(session);

    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message.empty())
        {
            EXPECT_EQ(5, evt.level); // [確認_正常系] - 空文字列が VERBOSE で受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name); // [確認_正常系] - イベント名が受信されること。
            EXPECT_FALSE(evt.has_service); // [確認_正常系] - Service なしで受信されること。
            EXPECT_NE((int64_t)0, evt.timestamp_100ns); // [確認_正常系] - ETW タイムスタンプが設定されること。
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected empty-string event not found";

    com_util_etw_provider_dispose(handle);
}

TEST_F(etwSessionSubscribeIntegrationTest, test_subscribe_service_and_message)
{
    std::string session_name = make_session_name();
    EventCollector collector;

    com_util_etw_provider_t *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider_t *)NULL, handle);

    com_util_etw_session_t *session = com_util_etw_session_start(
        session_name.c_str(), TEST_PROVIDER_GUID, collect_callback, &collector, NULL);
    ASSERT_NE((com_util_etw_session_t *)NULL, session);

    Sleep(200);
    com_util_etw_provider_write(handle, 4, "worker-1", "service_msg"); // [手順] - Service と Message を持つイベントを書き込む。
    com_util_etw_session_stop(session);

    bool found = false;
    for (const auto &evt : collector.events)
    {
        if (evt.message == "service_msg")
        {
            EXPECT_EQ(4, evt.level); // [確認_正常系] - INFO レベルで受信されること。
            EXPECT_EQ((uint32_t)GetCurrentProcessId(), evt.process_id); // [確認_正常系] - 発行元 PID が受信されること。
            EXPECT_EQ("Trace", evt.event_name); // [確認_正常系] - イベント名が受信されること。
            EXPECT_TRUE(evt.has_service); // [確認_正常系] - Service フィールドが受信されること。
            EXPECT_EQ("worker-1", evt.service); // [確認_正常系] - Service 名が復元されること。
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
