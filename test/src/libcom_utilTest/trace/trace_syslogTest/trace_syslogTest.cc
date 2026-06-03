#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <cstdlib>
    #include <cstdio>
    #include <ctime>
    #include <string>
    #include <unistd.h>
    #include <syslog.h>
    #include <testfw.h>
    #include <com_util/trace/syslog.h>

class trace_syslogTest : public Test
{
};

// プロバイダを初期化し、有効なハンドルが返されることの確認
TEST_F(trace_syslogTest, test_init_and_dispose)
{
    com_util_syslog_sink *handle =
        com_util_syslog_sink_create("syslog_test", LOG_USER); // [手順] - syslog sink を初期化する。
    EXPECT_NE((com_util_syslog_sink *)NULL, handle);          // [確認_正常系] - ハンドルが NULL でないこと。
    com_util_syslog_sink_dispose(handle);
}

// INFO レベルでメッセージを書き込み、戻り値が 0 であることの確認
TEST_F(trace_syslogTest, test_write_returns_zero)
{
    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslog_test", LOG_USER);
    ASSERT_NE((com_util_syslog_sink *)NULL, handle);

    int result = com_util_syslog_sink_write(handle, LOG_INFO, NULL, "test message"); // [手順] - INFO レベルで書き込む。

    EXPECT_EQ(0, result); // [確認_正常系] - 戻り値が 0 であること。
    com_util_syslog_sink_dispose(handle);
}

// 全レベルで書き込みが成功することの確認
TEST_F(trace_syslogTest, test_write_all_levels)
{
    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslog_test", LOG_USER);
    ASSERT_NE((com_util_syslog_sink *)NULL, handle);

    EXPECT_EQ(0, com_util_syslog_sink_write(handle, LOG_CRIT, NULL,
                                            "critical")); // [確認_正常系] - CRIT レベルで書き込めること。
    EXPECT_EQ(
        0, com_util_syslog_sink_write(handle, LOG_ERR, NULL, "error")); // [確認_正常系] - ERR レベルで書き込めること。
    EXPECT_EQ(0, com_util_syslog_sink_write(handle, LOG_WARNING, NULL,
                                            "warning")); // [確認_正常系] - WARNING レベルで書き込めること。
    EXPECT_EQ(
        0, com_util_syslog_sink_write(handle, LOG_INFO, NULL, "info")); // [確認_正常系] - INFO レベルで書き込めること。
    EXPECT_EQ(0, com_util_syslog_sink_write(handle, LOG_DEBUG, NULL,
                                            "debug")); // [確認_正常系] - DEBUG レベルで書き込めること。

    com_util_syslog_sink_dispose(handle);
}

// SYSLOG_TEST_FD 経路ではタイムスタンプ付き 1 行が書き込まれることの確認
TEST_F(trace_syslogTest, test_write_to_test_fd_prefixes_timestamp)
{
    int pipe_fds[2];
    char fd_text[32];
    char actual[256];
    char expected[256];
    ssize_t nread;
    const char *saved_tz = getenv("TZ");
    const char *saved_fd = getenv("SYSLOG_TEST_FD");
    std::string saved_tz_value = saved_tz != NULL ? saved_tz : "";
    std::string saved_fd_value = saved_fd != NULL ? saved_fd : "";
    com_util_realtime_timestamp timestamp = {1412916640LL, 0, 0};

    ASSERT_EQ(0, pipe(pipe_fds));
    ASSERT_EQ(0, setenv("TZ", "Asia/Tokyo", 1));
    tzset();
    snprintf(fd_text, sizeof(fd_text), "%d", pipe_fds[1]);
    ASSERT_EQ(0, setenv("SYSLOG_TEST_FD", fd_text, 1));

    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslog_test", LOG_USER);
    ASSERT_NE((com_util_syslog_sink *)NULL, handle);

    EXPECT_EQ(0, com_util_syslog_sink_write(handle, LOG_INFO, &timestamp, "test message"));

    close(pipe_fds[1]);
    pipe_fds[1] = -1;
    nread = read(pipe_fds[0], actual, sizeof(actual) - 1);
    ASSERT_GT(nread, 0);
    actual[nread] = '\0';

    snprintf(expected, sizeof(expected), "2014-10-10T13:50:40.000+09:00 <14>syslog_test[%d]: test message\n",
             (int)getpid());
    EXPECT_STREQ(expected, actual);

    com_util_syslog_sink_dispose(handle);
    close(pipe_fds[0]);
    if (saved_fd != NULL)
    {
        setenv("SYSLOG_TEST_FD", saved_fd_value.c_str(), 1);
    }
    else
    {
        unsetenv("SYSLOG_TEST_FD");
    }
    if (saved_tz != NULL)
    {
        setenv("TZ", saved_tz_value.c_str(), 1);
    }
    else
    {
        unsetenv("TZ");
    }
    tzset();
}

// 不正な明示タイムスタンプ指定時に現在時刻へ代替して出力しつつ -1 を返すことの確認
TEST_F(trace_syslogTest, test_write_to_test_fd_falls_back_from_invalid_explicit_timestamp)
{
    int pipe_fds[2];
    char fd_text[32];
    char actual[256];
    char expected[256];
    ssize_t nread;
    const char *saved_tz = getenv("TZ");
    const char *saved_fd = getenv("SYSLOG_TEST_FD");
    std::string saved_tz_value = saved_tz != NULL ? saved_tz : "";
    std::string saved_fd_value = saved_fd != NULL ? saved_fd : "";
    com_util_realtime_timestamp invalid_timestamp = {1714100645LL, 1000000000, 0};

    ASSERT_EQ(0, pipe(pipe_fds));
    ASSERT_EQ(0, setenv("TZ", "Asia/Tokyo", 1));
    tzset();
    snprintf(fd_text, sizeof(fd_text), "%d", pipe_fds[1]);
    ASSERT_EQ(0, setenv("SYSLOG_TEST_FD", fd_text, 1));

    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslog_test", LOG_USER);
    ASSERT_NE((com_util_syslog_sink *)NULL, handle);

    EXPECT_EQ(-1, com_util_syslog_sink_write(handle, LOG_INFO, &invalid_timestamp, "invalid ts"));

    close(pipe_fds[1]);
    pipe_fds[1] = -1;
    nread = read(pipe_fds[0], actual, sizeof(actual) - 1);
    ASSERT_GT(nread, 0);
    actual[nread] = '\0';

    snprintf(expected, sizeof(expected), "<14>syslog_test[%d]: invalid ts\n", (int)getpid());
    EXPECT_NE(std::string::npos, std::string(actual).find(expected)); // [確認_異常系] - syslog 本文が出力されること。
    EXPECT_NE('<', actual[0]); // [確認_異常系] - 先頭にタイムスタンプが付与されること。

    com_util_syslog_sink_dispose(handle);
    close(pipe_fds[0]);
    if (saved_fd != NULL)
    {
        setenv("SYSLOG_TEST_FD", saved_fd_value.c_str(), 1);
    }
    else
    {
        unsetenv("SYSLOG_TEST_FD");
    }
    if (saved_tz != NULL)
    {
        setenv("TZ", saved_tz_value.c_str(), 1);
    }
    else
    {
        unsetenv("TZ");
    }
    tzset();
}

// NULL 引数が安全に無視されることの確認
TEST_F(trace_syslogTest, test_null_arguments_are_safe)
{
    com_util_syslog_sink *handle = com_util_syslog_sink_create("syslog_test", LOG_USER);

    EXPECT_EQ((com_util_syslog_sink *)NULL,
              com_util_syslog_sink_create(NULL, LOG_USER)); // [確認_異常系] - NULL ident で create が失敗すること。
    EXPECT_EQ(0, com_util_syslog_sink_write(NULL, LOG_INFO, NULL,
                                            "test message")); // [確認_異常系] - NULL ハンドルが安全であること。
    EXPECT_EQ(
        0, com_util_syslog_sink_write(handle, LOG_INFO, NULL, NULL)); // [確認_異常系] - NULL message が安全であること。

    com_util_syslog_sink_dispose(handle);
    com_util_syslog_sink_dispose(NULL); // [手順] - NULL ハンドルで dispose を呼ぶ。
}

#elif defined(PLATFORM_WINDOWS)

    #include <testfw.h>

TEST(trace_syslogTest, not_supported)
{
    GTEST_SKIP() << "syslog is not supported on this platform";
}

#endif /* PLATFORM_ */
