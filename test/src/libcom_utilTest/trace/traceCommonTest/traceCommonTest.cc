#include <testfw.h>
#include <mock_com_util.h>

#include <com_util/trace/trace_common.h>

using namespace testing;

// 有効な明示タイムスタンプを変更せず採用することの確認
TEST(traceCommonTest, resolves_valid_explicit_timestamp)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec timestamp = {123, 456};
    com_util_timespec resolved = {0, 0};
    int fallback_used = -1;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(_))
        .Times(0); // [Pre-Assert確認_正常系] - 有効な明示タイムスタンプでは現在時刻を取得しないこと。

    // Act
    int rtc = trace_resolve_timestamp(
        &timestamp, &resolved,
        &fallback_used); // [手順] - 有効な明示タイムスタンプを指定して trace_resolve_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);                            // [確認_正常系] - trace_resolve_timestamp の戻り値が 0 であること。
    EXPECT_EQ(timestamp.tv_sec, resolved.tv_sec); // [確認_正常系] - resolved の秒値が明示タイムスタンプと一致すること。
    EXPECT_EQ(timestamp.tv_nsec,
              resolved.tv_nsec); // [確認_正常系] - resolved のナノ秒値が明示タイムスタンプと一致すること。
    EXPECT_EQ(0, fallback_used); // [確認_正常系] - fallback_used が 0 であること。
}

// NULL タイムスタンプでは現在時刻を採用し代替扱いにしないことの確認
TEST(traceCommonTest, resolves_current_time_for_null_timestamp)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec resolved = {0, 0};
    int fallback_used = -1;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(&resolved))
        .WillOnce(
            [](com_util_timespec *timestamp)
            {
                timestamp->tv_sec = 789;
                timestamp->tv_nsec = 123;
            }); // [Pre-Assert確認_正常系] - NULL タイムスタンプでは現在時刻を 1 回取得すること。
                // [Pre-Assert手順] - 現在時刻として 789 秒 123 ナノ秒を返却する。

    // Act
    int rtc = trace_resolve_timestamp(
        NULL, &resolved, &fallback_used); // [手順] - timestamp に NULL を指定して trace_resolve_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);                // [確認_正常系] - trace_resolve_timestamp の戻り値が 0 であること。
    EXPECT_EQ(789, resolved.tv_sec);  // [確認_正常系] - resolved の秒値が取得した現在時刻と一致すること。
    EXPECT_EQ(123, resolved.tv_nsec); // [確認_正常系] - resolved のナノ秒値が取得した現在時刻と一致すること。
    EXPECT_EQ(0, fallback_used);      // [確認_正常系] - NULL 指定は代替と見なさず fallback_used が 0 であること。
}

// 無効な明示タイムスタンプでは現在時刻へ代替することの確認
TEST(traceCommonTest, falls_back_from_invalid_explicit_timestamp)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec timestamp = {123, 1000000000};
    com_util_timespec resolved = {0, 0};
    int fallback_used = 0;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(&resolved))
        .WillOnce(
            [](com_util_timespec *current)
            {
                current->tv_sec = 456;
                current->tv_nsec = 789;
            }); // [Pre-Assert確認_正常系] - 無効な明示タイムスタンプでは現在時刻を 1 回取得すること。
                // [Pre-Assert手順] - 現在時刻として 456 秒 789 ナノ秒を返却する。

    // Act
    int rtc = trace_resolve_timestamp(
        &timestamp, &resolved,
        &fallback_used); // [手順] - 無効な明示タイムスタンプを指定して trace_resolve_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);                // [確認_正常系] - trace_resolve_timestamp の戻り値が 0 であること。
    EXPECT_EQ(456, resolved.tv_sec);  // [確認_正常系] - resolved の秒値が代替の現在時刻と一致すること。
    EXPECT_EQ(789, resolved.tv_nsec); // [確認_正常系] - resolved のナノ秒値が代替の現在時刻と一致すること。
    EXPECT_EQ(1, fallback_used);      // [確認_正常系] - fallback_used が 1 であること。
}

// 取得した現在時刻も無効な場合に失敗することの確認
TEST(traceCommonTest, rejects_invalid_current_time)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec resolved = {0, 0};
    int fallback_used = -1;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(&resolved))
        .WillOnce(
            [](com_util_timespec *timestamp)
            {
                timestamp->tv_sec = 0;
                timestamp->tv_nsec = -1;
            }); // [Pre-Assert確認_異常系] - 現在時刻を 1 回取得すること。
                // [Pre-Assert手順] - 無効なナノ秒値 -1 を返却する。

    // Act
    int rtc = trace_resolve_timestamp(
        NULL, &resolved, &fallback_used); // [手順] - 取得時刻が無効になる条件で trace_resolve_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);          // [確認_異常系] - trace_resolve_timestamp の戻り値が -1 であること。
    EXPECT_EQ(0, fallback_used); // [確認_異常系] - NULL 指定のため fallback_used が 0 であること。
}

// resolved が NULL の場合に出力へ触れず失敗することの確認
TEST(traceCommonTest, rejects_null_resolved)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec timestamp = {123, 456};
    int fallback_used = 77;

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(_))
        .Times(0); // [Pre-Assert確認_異常系] - resolved が NULL の場合は現在時刻を取得しないこと。

    // Act
    int rtc = trace_resolve_timestamp(
        &timestamp, NULL, &fallback_used); // [手順] - resolved に NULL を指定して trace_resolve_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc);           // [確認_異常系] - trace_resolve_timestamp の戻り値が -1 であること。
    EXPECT_EQ(77, fallback_used); // [確認_異常系] - fallback_used が変更されないこと。
}

// fallback_used が NULL でも現在時刻へ代替できることの確認
TEST(traceCommonTest, allows_null_fallback_used)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec timestamp = {123, -1};
    com_util_timespec resolved = {0, 0};

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_get_realtime(&resolved))
        .WillOnce(
            [](com_util_timespec *current)
            {
                current->tv_sec = 456;
                current->tv_nsec = 789;
            }); // [Pre-Assert確認_正常系] - 無効な明示タイムスタンプでは現在時刻を 1 回取得すること。
                // [Pre-Assert手順] - 有効な現在時刻を返却する。

    // Act
    int rtc = trace_resolve_timestamp(
        &timestamp, &resolved, NULL); // [手順] - fallback_used に NULL を指定して trace_resolve_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);                // [確認_正常系] - trace_resolve_timestamp の戻り値が 0 であること。
    EXPECT_EQ(456, resolved.tv_sec);  // [確認_正常系] - resolved の秒値が代替の現在時刻と一致すること。
    EXPECT_EQ(789, resolved.tv_nsec); // [確認_正常系] - resolved のナノ秒値が代替の現在時刻と一致すること。
}

// 有効なタイムスタンプを書式化して結果を返すことの確認
TEST(traceCommonTest, formats_valid_timestamp)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    char buf[32] = "";
    com_util_timespec timestamp = {123, 456};

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_format_realtime_iso8601_local(buf, sizeof(buf), &timestamp))
        .WillOnce(
            [](char *output, size_t output_size, const com_util_timespec *)
            {
                (void)com_util_strcpy(output, output_size, "formatted");
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_正常系] - 有効なタイムスタンプの書式化を 1 回呼び出すこと。
                // [Pre-Assert手順] - "formatted" を格納して COM_UTIL_OK を返却する。

    // Act
    int rtc = trace_format_local_timestamp(
        buf, sizeof(buf),
        &timestamp); // [手順] - 有効なタイムスタンプを指定して trace_format_local_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc);    // [確認_正常系] - trace_format_local_timestamp の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("formatted", buf); // [確認_正常系] - buf に書式化した文字列が格納されること。
}

// 無効なタイムスタンプを書式化せず拒否することの確認
TEST(traceCommonTest, rejects_invalid_timestamp_for_formatting)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    char buf[32] = "before";
    com_util_timespec timestamp = {123, 1000000000};

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_format_realtime_iso8601_local(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 無効なタイムスタンプを書式化しないこと。

    // Act
    int rtc = trace_format_local_timestamp(
        buf, sizeof(buf),
        &timestamp); // [手順] - 無効なタイムスタンプを指定して trace_format_local_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - trace_format_local_timestamp の戻り値が -1 であること。
}

// buf が NULL の場合に書式化関数の結果を伝搬することの確認
TEST(traceCommonTest, propagates_error_for_null_buffer)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    com_util_timespec timestamp = {123, 456};

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_format_realtime_iso8601_local(nullptr, 32, &timestamp))
        .WillOnce(Return(
            COM_UTIL_ERR_INVALID_ARGUMENT)); // [Pre-Assert確認_異常系] - NULL バッファーの書式化を 1 回呼び出すこと。
                                             // [Pre-Assert手順] - COM_UTIL_ERR_INVALID_ARGUMENT を返却する。

    // Act
    int rtc = trace_format_local_timestamp(
        NULL, 32, &timestamp); // [手順] - buf に NULL を指定して trace_format_local_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - trace_format_local_timestamp の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// buf_size が 0 の場合に書式化関数の結果を伝搬することの確認
TEST(traceCommonTest, propagates_error_for_zero_buffer_size)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    char buf[1] = "";
    com_util_timespec timestamp = {123, 456};

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_format_realtime_iso8601_local(buf, 0, &timestamp))
        .WillOnce(Return(
            COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - サイズ 0 のバッファーの書式化を 1 回呼び出すこと。
                                    // [Pre-Assert手順] - COM_UTIL_ERR_UNKNOWN を返却する。

    // Act
    int rtc = trace_format_local_timestamp(
        buf, 0, &timestamp); // [手順] - buf_size に 0 を指定して trace_format_local_timestamp を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - trace_format_local_timestamp の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 全トレース レベルと範囲外の値が対応する文字へ変換されることの確認
TEST(traceCommonTest, maps_all_trace_levels)
{
    // Arrange
    /* 列挙範囲外の不正値を意図的に渡す (定数キャストは -Wconversion になるため変数経由) */
    int negative_level_value = -1;
    int too_large_level_value = 999;
    const com_util_trace_level negative_level =
        (com_util_trace_level)negative_level_value; // [状態] - 範囲外の負値 -1 とする。
    const com_util_trace_level too_large_level =
        (com_util_trace_level)too_large_level_value; // [状態] - 範囲外の大きな値 999 とする。
    const com_util_trace_level levels[] = {COM_UTIL_TRACE_LEVEL_NONE,
                                           COM_UTIL_TRACE_LEVEL_CRITICAL,
                                           COM_UTIL_TRACE_LEVEL_ERROR,
                                           COM_UTIL_TRACE_LEVEL_WARNING,
                                           COM_UTIL_TRACE_LEVEL_INFO,
                                           COM_UTIL_TRACE_LEVEL_VERBOSE,
                                           COM_UTIL_TRACE_LEVEL_DEBUG,
                                           negative_level,
                                           too_large_level};
    const char expected[] = {'D', 'C', 'E', 'W', 'I', 'V', 'D', 'D', 'D'};
    char actual[sizeof(levels) / sizeof(levels[0])] = {0};

    // Pre-Assert

    // Act
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
    {
        actual[i] = trace_level_char(levels[i]); // [手順] - 各トレース レベルを trace_level_char で文字へ変換する。
    }

    // Assert
    EXPECT_EQ(
        0, memcmp(expected, actual,
                  sizeof(expected))); // [確認_正常系] - 全トレース レベルと範囲外の値が期待する文字へ変換されること。
}
