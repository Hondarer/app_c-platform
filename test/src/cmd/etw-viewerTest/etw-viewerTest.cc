#include <testfw.h>
#include <mock_stdio.h>
#include <mock_com_util.h>
#include "etw-viewer.h"

#include <cstdint>
#include <cstring>

using testing::_;
using testing::HasSubstr;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;

namespace
{

static int emulate_com_util_strncpy(char *dest, size_t dest_size, const char *src, size_t count)
{
    size_t i;

    if (dest == nullptr || dest_size == 0U || src == nullptr)
    {
        return -1;
    }

    for (i = 0; i < count && i + 1U < dest_size && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return 0;
}

static int emulate_com_util_format_realtime_iso8601_local(char *dest, size_t dest_size, int64_t tv_sec, int32_t tv_nsec)
{
    const char *text;

    if (dest == nullptr || dest_size == 0U)
    {
        return -1;
    }
    if (tv_sec == 0 && tv_nsec == 0)
    {
        text = "1970-01-01T09:00:00.000+09:00";
    }
    else
    {
        text = "2000-01-01T00:00:00.000+09:00";
    }

    return emulate_com_util_strncpy(dest, dest_size, text, strlen(text));
}

} // namespace

class etw_viewerTest : public Test
{
  protected:
    NiceMock<Mock_stdio> mock_stdio_;
    NiceMock<Mock_com_util> mock_com_util_;

    void SetUp() override
    {
        ON_CALL(mock_com_util_, com_util_strncpy(_, _, _, _)).WillByDefault(emulate_com_util_strncpy);
        ON_CALL(mock_com_util_, com_util_format_realtime_iso8601_local(_, _, _, _))
            .WillByDefault(emulate_com_util_format_realtime_iso8601_local);
    }
};

#if defined(PLATFORM_WINDOWS)

TEST_F(etw_viewerTest, parse_args_accepts_pid_filter)
{
    // Arrange
    etw_viewer_options options{};
    char arg0[] = "etw-viewer";
    char arg1[] = "--pid";
    char arg2[] = "4321";
    char *argv[] = {arg0, arg1, arg2};

    // Pre-Assert

    // Act
    int rc = etw_viewer_parse_args(3, argv, &options); // [手順] - pid 引数を解析する。

    // Assert
    EXPECT_EQ(0, rc);                                     // [確認_正常系] - 引数解析が成功すること。
    EXPECT_EQ(1, options.has_process_id_filter);          // [確認_正常系] - pid フィルター有効が反映されること。
    EXPECT_EQ((uint32_t)4321, options.process_id_filter); // [確認_正常系] - pid フィルター値が反映されること。
}

TEST_F(etw_viewerTest, parse_args_rejects_unknown_option)
{
    // Arrange
    etw_viewer_options options{};
    char arg0[] = "etw-viewer";
    char arg1[] = "--unknown";
    char *argv[] = {arg0, arg1};

    // Pre-Assert

    // Act
    int rc = etw_viewer_parse_args(2, argv, &options); // [手順] - 未対応オプションを解析する。

    // Assert
    EXPECT_NE(0, rc); // [確認_異常系] - 未対応オプションで失敗すること。
}

TEST_F(etw_viewerTest, parse_args_rejects_invalid_pid)
{
    // Arrange
    etw_viewer_options options{};
    char arg0[] = "etw-viewer";
    char arg1[] = "--pid";
    char arg2[] = "abc";
    char *argv[] = {arg0, arg1, arg2};

    // Act
    int rc = etw_viewer_parse_args(3, argv, &options); // [手順] - 不正な pid 引数を解析する。

    // Assert
    EXPECT_NE(0, rc); // [確認_異常系] - 数値でない pid は拒否されること。
}

TEST_F(etw_viewerTest, build_default_session_name_formats_process_id)
{
    // Arrange
    char buffer[ETW_VIEWER_SESSION_NAME_MAX];

    // Pre-Assert

    // Act
    int rc =
        etw_viewer_build_default_session_name(4321UL, buffer, sizeof(buffer)); // [手順] - 既定 session 名を生成する。

    // Assert
    EXPECT_EQ(0, rc);                        // [確認_正常系] - session 名生成が成功すること。
    EXPECT_STREQ("etw-viewer_4321", buffer); // [確認_正常系] - process id を含む名前になること。
}

TEST_F(etw_viewerTest, format_timestamp_utc_formats_filetime)
{
    // Arrange
    char buffer[64];

    // Pre-Assert

    // Act
    int rc = etw_viewer_format_timestamp_utc(116444736000000000LL, buffer,
                                             sizeof(buffer)); // [手順] - FILETIME epoch を UTC 文字列へ変換する。

    // Assert
    EXPECT_EQ(0, rc);                                      // [確認_正常系] - タイムスタンプ変換が成功すること。
    EXPECT_STREQ("1970-01-01T09:00:00.000+09:00", buffer); // [確認_正常系] - ローカル ISO8601 形式へ変換されること。
}

TEST_F(etw_viewerTest, handle_event_prints_service_and_message)
{
    // Arrange
    com_util_etw_event event = {4, 1234U, "Trace", "worker-7", "started", 116444736000000000LL};

    // Pre-Assert
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("1970-01-01T09:00:00.000+09:00 <I>worker-7[1234]: started\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - syslog 互換形式で出力されること。
    EXPECT_CALL(mock_stdio_, fflush(_, _, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 出力後に flush されること。

    // Act
    etw_viewer_handle_event(&event, NULL); // [手順] - Service ありイベントを表示する。

    // Assert
}

TEST_F(etw_viewerTest, handle_event_prints_message_without_service)
{
    // Arrange
    com_util_etw_event event = {3, 5678U, "Trace", NULL, "degraded", 116444736000000000LL};

    // Pre-Assert
    EXPECT_CALL(mock_stdio_,
                printf(_, _, _, StrEq("1970-01-01T09:00:00.000+09:00 <W>Company.Product[5678]: degraded\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - Service なしでは既定 tag を使って出力されること。
    EXPECT_CALL(mock_stdio_, fflush(_, _, _, _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 出力後に flush されること。

    // Act
    etw_viewer_handle_event(&event, NULL); // [手順] - Service なしイベントを表示する。

    // Assert
}

TEST_F(etw_viewerTest, handle_event_skips_non_trace_event)
{
    // Arrange
    com_util_etw_event event = {5, 9012U, "EventMetadata", NULL, NULL, 116444736000000000LL};

    // Pre-Assert
    EXPECT_CALL(mock_stdio_, printf(_, _, _, _)).Times(0); // [Pre-Assert確認_正常系] - Trace 以外は表示しないこと。
    EXPECT_CALL(mock_stdio_, fflush(_, _, _, _)).Times(0); // [Pre-Assert確認_正常系] - Trace 以外は flush しないこと。

    // Act
    etw_viewer_handle_event(&event, NULL); // [手順] - Trace 以外のイベントを表示処理へ渡す。

    // Assert
}

TEST_F(etw_viewerTest, handle_event_skips_non_matching_pid_filter)
{
    // Arrange
    com_util_etw_event event = {4, 2222U, "Trace", NULL, "filtered", 116444736000000000LL};
    etw_viewer_context context = {1111U, 1};

    // Pre-Assert
    EXPECT_CALL(mock_stdio_, printf(_, _, _, _)).Times(0); // [Pre-Assert確認_正常系] - 不一致 pid は出力されないこと。
    EXPECT_CALL(mock_stdio_, fflush(_, _, _, _))
        .Times(0); // [Pre-Assert確認_正常系] - 不一致 pid は flush もしないこと。

    // Act
    etw_viewer_handle_event(&event, &context); // [手順] - pid フィルター不一致イベントを表示処理へ渡す。

    // Assert
}

TEST_F(etw_viewerTest, main_rejects_invalid_arguments)
{
    // Arrange
    int argc = 2;
    const char *argv[] = {"etw-viewer", "--unknown"}; // [状態] - 未対応オプション付きで起動する。
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - main 起動時に console 初期化が呼ばれること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("使用方法: etw-viewer")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - 使用方法が表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 不正引数で main を呼び出す。

    // Assert
    EXPECT_NE(0, rc); // [確認_異常系] - 不正引数で失敗終了すること。
}

TEST_F(etw_viewerTest, main_stops_when_access_is_denied)
{
    // Arrange
    int argc = 1;
    const char *argv[] = {"etw-viewer"}; // [状態] - 引数なしで起動し、権限不足を模擬する。
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - main 起動時に console 初期化が呼ばれること。
    EXPECT_CALL(mock_com_util_, com_util_etw_session_check_access())
        .WillOnce(Return(COM_UTIL_ETW_SESSION_ERR_ACCESS)); // [Pre-Assert確認_異常系] - 権限不足が返ること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("Performance Log Users")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - 権限不足メッセージが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 権限不足の状態で main を呼び出す。

    // Assert
    EXPECT_NE(0, rc); // [確認_異常系] - 権限不足で失敗終了すること。
}

#endif
