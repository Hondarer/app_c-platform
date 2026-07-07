#include <testfw.h>
#include <com_util/crt/time.h>
#include <mock_time.h>
#include <string.h>

class timeTest : public Test
{
};

TEST_F(timeTest, gmtime_success_epoch)
{
    struct tm utc_tm;
    time_t epoch = 0;

    memset(&utc_tm, 0xff, sizeof(utc_tm));

    int ret = com_util_gmtime(&utc_tm, &epoch);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(70, utc_tm.tm_year);
    EXPECT_EQ(0, utc_tm.tm_mon);
    EXPECT_EQ(1, utc_tm.tm_mday);
    EXPECT_EQ(0, utc_tm.tm_hour);
    EXPECT_EQ(0, utc_tm.tm_min);
    EXPECT_EQ(0, utc_tm.tm_sec);
}

TEST_F(timeTest, gmtime_null_tm)
{
    time_t epoch = 0;

    int ret = com_util_gmtime(NULL, &epoch);

    EXPECT_EQ(-1, ret);
}

TEST_F(timeTest, gmtime_null_time)
{
    struct tm utc_tm;

    int ret = com_util_gmtime(&utc_tm, NULL);

    EXPECT_EQ(-1, ret);
}

TEST_F(timeTest, gmtime_zeroes_tm_when_platform_conversion_fails)
{
    struct tm utc_tm;
    time_t epoch = 0;

    utc_tm.tm_year = 1;
    utc_tm.tm_mon = 2;
    utc_tm.tm_mday = 3;
    utc_tm.tm_hour = 4;
    utc_tm.tm_min = 5;
    utc_tm.tm_sec = 6;

    Mock_time mock_time;

#ifndef _WIN32
    EXPECT_CALL(mock_time, gmtime_r(_, _, _, _, _))
        .WillOnce([](const char *, const int, const char *, const time_t *timep, struct tm *result)
        {
            EXPECT_EQ((time_t)0, *timep);
            EXPECT_NE((struct tm *)NULL, result);
            return (struct tm *)NULL;
        });
#else
    EXPECT_CALL(mock_time, gmtime_s(_, _, _, _, _))
        .WillOnce([](const char *, const int, const char *, struct tm *result, const time_t *timep)
        {
            EXPECT_EQ((time_t)0, *timep);
            EXPECT_NE((struct tm *)NULL, result);
            return 1;
        });
#endif

    int ret = com_util_gmtime(&utc_tm, &epoch);

    EXPECT_EQ(-1, ret);
    EXPECT_EQ(0, utc_tm.tm_year);
    EXPECT_EQ(0, utc_tm.tm_mon);
    EXPECT_EQ(0, utc_tm.tm_mday);
    EXPECT_EQ(0, utc_tm.tm_hour);
    EXPECT_EQ(0, utc_tm.tm_min);
    EXPECT_EQ(0, utc_tm.tm_sec);
}

TEST_F(timeTest, localtime_matches_platform_result)
{
    struct tm expected_tm;
    struct tm actual_tm;
    time_t epoch = 0;

    memset(&expected_tm, 0, sizeof(expected_tm));
    memset(&actual_tm, 0xff, sizeof(actual_tm));

#if defined(PLATFORM_LINUX)
    ASSERT_NE((struct tm *)NULL, localtime_r(&epoch, &expected_tm));
#elif defined(PLATFORM_WINDOWS)
    ASSERT_EQ(0, localtime_s(&expected_tm, &epoch));
#endif

    EXPECT_EQ(0, com_util_localtime(&actual_tm, &epoch));
    EXPECT_EQ(expected_tm.tm_year, actual_tm.tm_year);
    EXPECT_EQ(expected_tm.tm_mon, actual_tm.tm_mon);
    EXPECT_EQ(expected_tm.tm_mday, actual_tm.tm_mday);
    EXPECT_EQ(expected_tm.tm_hour, actual_tm.tm_hour);
    EXPECT_EQ(expected_tm.tm_min, actual_tm.tm_min);
    EXPECT_EQ(expected_tm.tm_sec, actual_tm.tm_sec);
}

TEST_F(timeTest, localtime_null_tm)
{
    time_t epoch = 0;

    EXPECT_EQ(-1, com_util_localtime(NULL, &epoch));
}

TEST_F(timeTest, localtime_null_time)
{
    struct tm local_tm;

    EXPECT_EQ(-1, com_util_localtime(&local_tm, NULL));
}

TEST_F(timeTest, ctime_success_epoch)
{
    char buf[26];
    time_t epoch = 0;
    size_t len;

    memset(buf, 0xff, sizeof(buf));

    int ret = com_util_ctime(buf, sizeof(buf), &epoch);

    EXPECT_EQ(0, ret);

    /* ctime はローカル時刻依存のため固定文字列比較は行わず、形式のみを確認する */
    len = strlen(buf);
    EXPECT_GE(len, (size_t)24);
    EXPECT_LE(len, (size_t)25);
    EXPECT_EQ('\n', buf[len - 1]);
}

TEST_F(timeTest, ctime_matches_platform_result)
{
    char expected[26];
    char actual[26];
    time_t epoch = 0;

    memset(expected, 0, sizeof(expected));
    memset(actual, 0xff, sizeof(actual));

#if defined(PLATFORM_LINUX)
    ASSERT_NE((char *)NULL, ctime_r(&epoch, expected));
#elif defined(PLATFORM_WINDOWS)
    ASSERT_EQ(0, ctime_s(expected, sizeof(expected), &epoch));
#endif

    EXPECT_EQ(0, com_util_ctime(actual, sizeof(actual), &epoch));
    EXPECT_STREQ(expected, actual);
}

TEST_F(timeTest, ctime_null_buf)
{
    time_t epoch = 0;

    EXPECT_EQ(-1, com_util_ctime(NULL, 26, &epoch));
}

TEST_F(timeTest, ctime_null_time_zeroes_buf)
{
    char buf[26];

    memset(buf, 0xff, sizeof(buf));

    EXPECT_EQ(-1, com_util_ctime(buf, sizeof(buf), NULL));

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        EXPECT_EQ('\0', buf[i]);
    }
}

TEST_F(timeTest, ctime_small_buf_zeroes_buf)
{
    char buf[25];
    time_t epoch = 0;

    memset(buf, 0xff, sizeof(buf));

    EXPECT_EQ(-1, com_util_ctime(buf, sizeof(buf), &epoch));

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        EXPECT_EQ('\0', buf[i]);
    }
}

TEST_F(timeTest, ctime_zero_buf_size_fails_without_write)
{
    char buf[1];
    time_t epoch = 0;

    buf[0] = 0x7f;

    EXPECT_EQ(-1, com_util_ctime(buf, 0, &epoch));

    /* buf_size が 0 のため memset も書き込みを行わないこと */
    EXPECT_EQ(0x7f, buf[0]);
}

TEST_F(timeTest, ctime_zeroes_buf_when_platform_conversion_fails)
{
    char buf[26];
    time_t epoch = 0;

    memset(buf, 0xff, sizeof(buf));

    Mock_time mock_time;

#ifndef _WIN32
    EXPECT_CALL(mock_time, ctime_r(_, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, const time_t *timep, char *result)
            {
                EXPECT_EQ((time_t)0, *timep);
                EXPECT_NE((char *)NULL, result);
                return (char *)NULL;
            });
#else
    EXPECT_CALL(mock_time, ctime_s(_, _, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, char *result, size_t size, const time_t *timep)
            {
                EXPECT_EQ((time_t)0, *timep);
                EXPECT_EQ((size_t)26, size);
                EXPECT_NE((char *)NULL, result);
                return 1;
            });
#endif

    int ret = com_util_ctime(buf, sizeof(buf), &epoch);

    EXPECT_EQ(-1, ret);

    for (size_t i = 0; i < sizeof(buf); i++)
    {
        EXPECT_EQ('\0', buf[i]);
    }
}
