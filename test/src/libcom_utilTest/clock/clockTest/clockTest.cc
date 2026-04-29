#include <testfw.h>
#include <com_util/clock/clock.h>
#include <mock_com_util.h>
#include <mock_time.h>
#if defined(PLATFORM_WINDOWS)
    #include <mock_windows.h>
#endif
#include <stdint.h>

class clockTest : public Test
{
};

static void expect_tm_equal(const struct tm *actual, const struct tm *expected)
{
    EXPECT_EQ(expected->tm_year, actual->tm_year);
    EXPECT_EQ(expected->tm_mon, actual->tm_mon);
    EXPECT_EQ(expected->tm_mday, actual->tm_mday);
    EXPECT_EQ(expected->tm_hour, actual->tm_hour);
    EXPECT_EQ(expected->tm_min, actual->tm_min);
    EXPECT_EQ(expected->tm_sec, actual->tm_sec);
}

static void set_tm(struct tm *tm_value, int year, int month, int day, int hour, int min, int sec)
{
    memset(tm_value, 0, sizeof(*tm_value));
    tm_value->tm_year = year - 1900;
    tm_value->tm_mon = month - 1;
    tm_value->tm_mday = day;
    tm_value->tm_hour = hour;
    tm_value->tm_min = min;
    tm_value->tm_sec = sec;
}

#if defined(PLATFORM_WINDOWS)
static FILETIME to_filetime(int64_t tv_sec, int32_t tv_nsec)
{
    const uint64_t filetime_units_per_sec = 10000000ULL;
    const int64_t filetime_epoch_offset_sec = 11644473600LL;
    ULARGE_INTEGER uli;
    FILETIME file_time;

    uli.QuadPart = (uint64_t)(tv_sec + filetime_epoch_offset_sec) * filetime_units_per_sec + (uint64_t)(tv_nsec / 100);
    file_time.dwLowDateTime = uli.LowPart;
    file_time.dwHighDateTime = uli.HighPart;

    return file_time;
}
#endif

// 単調増加クロックのミリ秒値がプラットフォーム値から正しく変換されることの確認
TEST_F(clockTest, monotonic_ms_converts_platform_value)
{
    // Arrange

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 単調増加クロック取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 単調増加クロック取得 API にて 12345 ミリ秒相当の値を返す。
#if defined(PLATFORM_LINUX)
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_MONOTONIC, clk_id);
                ts->tv_sec = 12;
                ts->tv_nsec = 345678901L;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetTickCount64(_, _, _)).WillOnce(Return(12345ULL));
#endif

    // Act
    uint64_t actual_ms = com_util_get_monotonic_ms(); // [手順] - com_util_get_monotonic_ms() を呼び出す。

    // Assert
    EXPECT_EQ(12345U, actual_ms); // [確認_正常系] - 単調増加クロックのミリ秒値が 12345 であること。
}

// 単調増加クロックの秒部とナノ秒部がプラットフォーム値から正しく分解されることの確認
TEST_F(clockTest, monotonic_returns_split_platform_value)
{
    // Arrange
    int64_t tv_sec = -1;  // [状態] - 秒部の出力先を未更新値 -1 で初期化する。
    int32_t tv_nsec = -1; // [状態] - ナノ秒部の出力先を未更新値 -1 で初期化する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 単調増加クロック取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 単調増加クロック取得 API にて 12 秒相当の時刻を返す。
#if defined(PLATFORM_LINUX)
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_MONOTONIC, clk_id);
                ts->tv_sec = 12;
                ts->tv_nsec = 345678901L;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetTickCount64(_, _, _)).WillOnce(Return(12345ULL));
#endif

    // Act
    com_util_get_monotonic(&tv_sec, &tv_nsec); // [手順] - com_util_get_monotonic(&tv_sec, &tv_nsec) を呼び出す。

    // Assert
    EXPECT_EQ(12, tv_sec); // [確認_正常系] - 秒部が 12 であること。
    // [確認_正常系] - ナノ秒部がプラットフォームごとの期待値 (Linux: 345678901, Windows: 345000000) と一致すること。
#if defined(PLATFORM_LINUX)
    EXPECT_EQ(345678901, tv_nsec);
#elif defined(PLATFORM_WINDOWS)
    EXPECT_EQ(345000000, tv_nsec);
#endif
}

// 実時刻クロックの秒部とナノ秒部がプラットフォーム値から正しく分解されることの確認
TEST_F(clockTest, realtime_returns_split_platform_value)
{
    // Arrange
    const int64_t expected_sec = 1712345678LL; // [状態] - 実時刻の秒部期待値を 1712345678 とする。
    int64_t tv_sec = -1;                       // [状態] - 秒部の出力先を未更新値 -1 で初期化する。
    // [状態] - プラットフォームに応じたナノ秒部期待値を expected_nsec に設定する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 実時刻取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 実時刻取得 API にて期待する秒部とナノ秒部を返す。
#if defined(PLATFORM_LINUX)
    const int32_t expected_nsec = 987654321;
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [&](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_REALTIME, clk_id);
                ts->tv_sec = expected_sec;
                ts->tv_nsec = expected_nsec;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    const int32_t expected_nsec = 987654300;
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetSystemTimeAsFileTime(_, _, _, _))
        .WillOnce([&](const char *, const int, const char *, LPFILETIME file_time)
                  { *file_time = to_filetime(expected_sec, expected_nsec); });
#endif
    int32_t tv_nsec = -1; // [状態] - ナノ秒部の出力先を未更新値 -1 で初期化する。

    // Act
    com_util_get_realtime(&tv_sec, &tv_nsec); // [手順] - com_util_get_realtime(&tv_sec, &tv_nsec) を呼び出す。

    // Assert
    EXPECT_EQ(expected_sec, tv_sec);   // [確認_正常系] - 秒部が期待値と一致すること。
    EXPECT_EQ(expected_nsec, tv_nsec); // [確認_正常系] - ナノ秒部がプラットフォームに応じた期待値と一致すること。
}

// 実時刻が UTC 分解結果とナノ秒部へ正しく変換されることの確認
TEST_F(clockTest, realtime_utc_uses_platform_conversion_result)
{
    // Arrange
    const int64_t expected_sec = 1712345678LL; // [状態] - 実時刻の秒部期待値を 1712345678 とする。
    const int32_t expected_nsec = 246800000;   // [状態] - 実時刻ナノ秒部として 246800000 を返す前提とする。
    struct tm expected_tm = {};                // [状態] - UTC 変換後の期待値を格納する構造体を 0 初期化する。
    struct tm actual_tm = {};                  // [状態] - 実際の UTC 変換結果を受け取る構造体を 0 初期化する。
    int32_t actual_nsec = -1;                  // [状態] - ナノ秒部の出力先を未更新値 -1 で初期化する。
    Mock_com_util mock_com_util;

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 実時刻取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 実時刻取得 API にて期待する秒部とナノ秒部を返す。
#if defined(PLATFORM_LINUX)
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [&](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_REALTIME, clk_id);
                ts->tv_sec = expected_sec;
                ts->tv_nsec = expected_nsec;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetSystemTimeAsFileTime(_, _, _, _))
        .WillOnce([&](const char *, const int, const char *, LPFILETIME file_time)
                  { *file_time = to_filetime(expected_sec, expected_nsec); });
#endif

    EXPECT_CALL(mock_com_util, com_util_gmtime(_, _))
        .WillOnce(
            [&](struct tm *utc_tm, const time_t *timep)
            {
                EXPECT_EQ((time_t)expected_sec, *timep);
                expected_tm.tm_year = 124;
                expected_tm.tm_mon = 3;
                expected_tm.tm_mday = 5;
                expected_tm.tm_hour = 6;
                expected_tm.tm_min = 7;
                expected_tm.tm_sec = 8;
                *utc_tm = expected_tm;
                return 0;
            }); // [Pre-Assert確認_正常系] - com_util_gmtime(&actual_tm, &realtime_time) が 1 回呼び出されること。
    // [Pre-Assert手順] - com_util_gmtime() にて UTC 分解結果として 2024-04-05 06:07:08 相当を設定し、成功を返す。

    // Act
    com_util_get_realtime_utc(
        &actual_tm, &actual_nsec); // [手順] - com_util_get_realtime_utc(&actual_tm, &actual_nsec) を呼び出す。

    // Assert
    expect_tm_equal(&actual_tm,
                    &expected_tm);         // [確認_正常系] - UTC 分解結果が com_util_gmtime() の設定値と一致すること。
    EXPECT_EQ(expected_nsec, actual_nsec); // [確認_正常系] - ナノ秒部が 246800000 のまま返ること。
}

// UTC 変換が失敗した場合に struct tm が 0 初期化されることの確認
TEST_F(clockTest, realtime_utc_zeroes_tm_when_com_util_gmtime_fails)
{
    // Arrange
    const int64_t expected_sec = 1712345678LL; // [状態] - 実時刻の秒部期待値を 1712345678 とする。
    const int32_t expected_nsec = 246800000;   // [状態] - 実時刻ナノ秒部として 246800000 を返す前提とする。
    struct tm actual_tm = {};                  // [状態] - 実際の UTC 変換結果を受け取る構造体を初期化する。
    struct tm expected_tm = {};                // [状態] - 失敗時に 0 初期化される期待値を用意する。
    int32_t actual_nsec = -1;                  // [状態] - ナノ秒部の出力先を未更新値 -1 で初期化する。
    Mock_com_util mock_com_util;

    actual_tm.tm_year = 1; // [状態] - 失敗時に上書きされたことが分かるよう、year を 1 に設定する。
    actual_tm.tm_mon = 2;  // [状態] - 失敗時に上書きされたことが分かるよう、mon を 2 に設定する。
    actual_tm.tm_mday = 3; // [状態] - 失敗時に上書きされたことが分かるよう、mday を 3 に設定する。
    actual_tm.tm_hour = 4; // [状態] - 失敗時に上書きされたことが分かるよう、hour を 4 に設定する。
    actual_tm.tm_min = 5;  // [状態] - 失敗時に上書きされたことが分かるよう、min を 5 に設定する。
    actual_tm.tm_sec = 6;  // [状態] - 失敗時に上書きされたことが分かるよう、sec を 6 に設定する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 実時刻取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 実時刻取得 API にて期待する秒部とナノ秒部を返す。
#if defined(PLATFORM_LINUX)
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [&](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_REALTIME, clk_id);
                ts->tv_sec = expected_sec;
                ts->tv_nsec = expected_nsec;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetSystemTimeAsFileTime(_, _, _, _))
        .WillOnce([&](const char *, const int, const char *, LPFILETIME file_time)
                  { *file_time = to_filetime(expected_sec, expected_nsec); });
#endif

    EXPECT_CALL(mock_com_util, com_util_gmtime(_, _))
        .WillOnce(
            [&](struct tm *utc_tm, const time_t *timep)
            {
                EXPECT_EQ((time_t)expected_sec, *timep);
                EXPECT_EQ(&actual_tm, utc_tm);
                return -1;
            }); // [Pre-Assert確認_異常系] - com_util_gmtime(&actual_tm, &realtime_time) が 1 回呼び出されること。
                // [Pre-Assert手順] - com_util_gmtime() にて失敗を返し、clock.c 側の 0 初期化処理へ進ませる。

    // Act
    com_util_get_realtime_utc(
        &actual_tm, &actual_nsec); // [手順] - com_util_get_realtime_utc(&actual_tm, &actual_nsec) を呼び出す。

    // Assert
    expect_tm_equal(&actual_tm, &expected_tm); // [確認_異常系] - UTC 分解結果がすべて 0 に初期化されること。
    EXPECT_EQ(expected_nsec, actual_nsec);     // [確認_異常系] - ナノ秒部は取得済みの値 246800000 を保持すること。
}

TEST_F(clockTest, format_realtime_iso8601_local_outputs_offset_and_milliseconds)
{
    const int64_t tv_sec = 1712297228LL;
    const int32_t tv_nsec = 246800000;
    char actual[COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1];
    struct tm local_tm;
    struct tm utc_tm;
    Mock_com_util mock_com_util;

    set_tm(&local_tm, 2024, 4, 5, 15, 7, 8);
    set_tm(&utc_tm, 2024, 4, 5, 6, 7, 8);

    EXPECT_CALL(mock_com_util, com_util_localtime(_, _))
        .WillOnce([&](struct tm *tm_value, const time_t *timep)
        {
            EXPECT_EQ((time_t)tv_sec, *timep);
            *tm_value = local_tm;
            return 0;
        });
    EXPECT_CALL(mock_com_util, com_util_gmtime(_, _))
        .WillOnce([&](struct tm *tm_value, const time_t *timep)
        {
            EXPECT_EQ((time_t)tv_sec, *timep);
            *tm_value = utc_tm;
            return 0;
        });

    EXPECT_EQ(0, com_util_format_realtime_iso8601_local(actual, sizeof(actual), tv_sec, tv_nsec));
    EXPECT_STREQ("2024-04-05T15:07:08.246+09:00", actual);
}

TEST_F(clockTest, format_realtime_iso8601_local_supports_negative_offset)
{
    const int64_t tv_sec = 1712297228LL;
    const int32_t tv_nsec = 135000000;
    char actual[COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1];
    struct tm local_tm;
    struct tm utc_tm;
    Mock_com_util mock_com_util;

    set_tm(&local_tm, 2024, 4, 5, 0, 37, 8);
    set_tm(&utc_tm, 2024, 4, 5, 6, 7, 8);

    EXPECT_CALL(mock_com_util, com_util_localtime(_, _))
        .WillOnce([&](struct tm *tm_value, const time_t *)
        {
            *tm_value = local_tm;
            return 0;
        });
    EXPECT_CALL(mock_com_util, com_util_gmtime(_, _))
        .WillOnce([&](struct tm *tm_value, const time_t *)
        {
            *tm_value = utc_tm;
            return 0;
        });

    EXPECT_EQ(0, com_util_format_realtime_iso8601_local(actual, sizeof(actual), tv_sec, tv_nsec));
    EXPECT_STREQ("2024-04-05T00:37:08.135-05:30", actual);
}

TEST_F(clockTest, format_realtime_iso8601_utc_outputs_z_suffix)
{
    const int64_t tv_sec = 1712297228LL;
    const int32_t tv_nsec = 987000000;
    char actual[COM_UTIL_CLOCK_ISO8601_UTC_MSEC_LEN + 1];
    struct tm utc_tm;
    Mock_com_util mock_com_util;

    set_tm(&utc_tm, 2024, 4, 5, 6, 7, 8);

    EXPECT_CALL(mock_com_util, com_util_gmtime(_, _))
        .WillOnce([&](struct tm *tm_value, const time_t *timep)
        {
            EXPECT_EQ((time_t)tv_sec, *timep);
            *tm_value = utc_tm;
            return 0;
        });

    EXPECT_EQ(0, com_util_format_realtime_iso8601_utc(actual, sizeof(actual), tv_sec, tv_nsec));
    EXPECT_STREQ("2024-04-05T06:07:08.987Z", actual);
}

TEST_F(clockTest, format_realtime_iso8601_local_falls_back_when_nsec_is_invalid)
{
    char actual[COM_UTIL_CLOCK_ISO8601_LOCAL_MSEC_LEN + 1];

    EXPECT_EQ(-1, com_util_format_realtime_iso8601_local(actual, sizeof(actual), 0, 1000000000));
    EXPECT_STREQ("0000-00-00T00:00:00.000+00:00", actual);
}

TEST_F(clockTest, format_realtime_iso8601_utc_falls_back_when_gmtime_fails)
{
    const int64_t tv_sec = 1712297228LL;
    char actual[COM_UTIL_CLOCK_ISO8601_UTC_MSEC_LEN + 1];
    Mock_com_util mock_com_util;

    EXPECT_CALL(mock_com_util, com_util_gmtime(_, _))
        .WillOnce([&](struct tm *tm_value, const time_t *timep)
        {
            EXPECT_EQ((time_t)tv_sec, *timep);
            EXPECT_NE((struct tm *)NULL, tm_value);
            return -1;
        });

    EXPECT_EQ(-1, com_util_format_realtime_iso8601_utc(actual, sizeof(actual), tv_sec, 123000000));
    EXPECT_STREQ("0000-00-00T00:00:00.000Z", actual);
}

// 実時刻 deadline 計算でナノ秒 overflow がない場合にそのまま加算されることの確認
TEST_F(clockTest, realtime_deadline_ms_adds_timeout_without_nsec_carry)
{
    // Arrange
    const uint64_t timeout_ms = 250;      // [状態] - deadline に加算するタイムアウトを 250 ミリ秒とする。
    const time_t expected_sec = 100;      // [状態] - deadline の秒部期待値を 100 とする。
    const long expected_nsec = 350000000; // [状態] - deadline のナノ秒部期待値を 350000000 とする。
    struct timespec abs_timeout = {};     // [状態] - 計算結果の格納先を 0 初期化する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 実時刻取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 実時刻取得 API にて 100 秒台の時刻を返す。
#if defined(PLATFORM_LINUX)
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_REALTIME, clk_id);
                ts->tv_sec = 100;
                ts->tv_nsec = 100000000L;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetSystemTimeAsFileTime(_, _, _, _))
        .WillOnce([](const char *, const int, const char *, LPFILETIME file_time)
                  { *file_time = to_filetime(100, 100000000); });
#endif

    // Act
    com_util_get_realtime_deadline_ms(
        timeout_ms, &abs_timeout); // [手順] - com_util_get_realtime_deadline_ms(timeout_ms, &abs_timeout) を呼び出す。

    // Assert
    EXPECT_EQ(expected_sec, abs_timeout.tv_sec);   // [確認_正常系] - 秒繰り上がりなしで秒部が 100 のままであること。
    EXPECT_EQ(expected_nsec, abs_timeout.tv_nsec); // [確認_正常系] - ナノ秒部が 350000000 になること。
}

// 実時刻 deadline 計算でナノ秒 overflow が発生した場合に秒繰り上がりすることの確認
TEST_F(clockTest, realtime_deadline_ms_carries_nsec_overflow)
{
    // Arrange
    const uint64_t timeout_ms = 250;     // [状態] - deadline に加算するタイムアウトを 250 ミリ秒とする。
    const time_t expected_sec = 101;     // [状態] - overflow 後の秒部期待値を 101 とする。
    const long expected_nsec = 50000000; // [状態] - overflow 後のナノ秒部期待値を 50000000 とする。
    struct timespec abs_timeout = {};    // [状態] - 計算結果の格納先を 0 初期化する。

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 実時刻取得 API が 1 回呼び出されること。
    // [Pre-Assert手順] - 実時刻取得 API にて 100 秒台の時刻を返す。
#if defined(PLATFORM_LINUX)
    Mock_time mock_time;

    EXPECT_CALL(mock_time, clock_gettime(_, _, _, _, _))
        .WillOnce(
            [](const char *, const int, const char *, clockid_t clk_id, struct timespec *ts)
            {
                EXPECT_EQ(CLOCK_REALTIME, clk_id);
                ts->tv_sec = 100;
                ts->tv_nsec = 800000000L;
                return 0;
            });
#elif defined(PLATFORM_WINDOWS)
    Mock_windows mock_windows;

    EXPECT_CALL(mock_windows, GetSystemTimeAsFileTime(_, _, _, _))
        .WillOnce([](const char *, const int, const char *, LPFILETIME file_time)
                  { *file_time = to_filetime(100, 800000000); });
#endif

    // Act
    com_util_get_realtime_deadline_ms(
        timeout_ms, &abs_timeout); // [手順] - com_util_get_realtime_deadline_ms(timeout_ms, &abs_timeout) を呼び出す。

    // Assert
    EXPECT_EQ(expected_sec, abs_timeout.tv_sec); // [確認_正常系] - ナノ秒 overflow により秒部が 101 に繰り上がること。
    EXPECT_EQ(expected_nsec, abs_timeout.tv_nsec); // [確認_正常系] - ナノ秒部が 50000000 に正規化されること。
}
