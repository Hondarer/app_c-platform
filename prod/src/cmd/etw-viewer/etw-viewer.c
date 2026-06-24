/**
 *******************************************************************************
 *  @file           etw-viewer.c
 *  @brief          com_util ETW provider のイベントをリアルタイムに表示するコマンドを実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/05/01
 *  @version        1.0.0
 *
 *  Windows 上で com_util の ETW provider を購読し、受信イベントを stdout に表示します。
 *  既定では com_util tracer の標準 provider GUID を購読し、Ctrl+C で終了します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "etw-viewer.h"

#include <com_util/base/platform.h>
#include <com_util/clock/clock.h>
#include <com_util/console/console.h>
#include <com_util/crt/string.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/sync/sync.h>
#include <signal.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <com_util/trace/etw.h>
    #include <com_util/trace/tracer.h>

    #define ETW_VIEWER_WAIT_MS     200
    #define ETW_VIEWER_DEFAULT_TAG COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME

    #define EXIT_ACCESS_DENIED 2

static volatile sig_atomic_t g_stop_requested = 0;

static void etw_viewer_shutdown_request_callback(const com_util_shutdown_event *event, void *context)
{
    (void)event;
    (void)context;
    g_stop_requested = 1;
}

static int parse_process_id_arg(const char *text, uint32_t *out_process_id)
{
    char *endptr;
    unsigned long value;

    if (text == NULL || out_process_id == NULL || text[0] == '\0')
    {
        return -1;
    }

    errno = 0;
    value = strtoul(text, &endptr, 10);
    /* プロセス ID は Windows DWORD のため、コーディング規範の例外として UINT32_MAX 上限を維持する。 */
    if (errno != 0 || endptr == text || *endptr != '\0' || value > UINT32_MAX)
    {
        return -1;
    }

    *out_process_id = (uint32_t)value;
    return 0;
}

void etw_viewer_options_init(etw_viewer_options *options)
{
    if (options == NULL)
    {
        return;
    }

    options->process_id_filter = 0U;
    options->has_process_id_filter = 0;
}

int etw_viewer_parse_args(int argc, char *argv[], etw_viewer_options *options)
{
    int i;

    if (argc <= 0 || argv == NULL || options == NULL)
    {
        return -1;
    }

    etw_viewer_options_init(options);

    for (i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (strcmp(arg, "--pid") == 0)
        {
            if ((i + 1) >= argc || parse_process_id_arg(argv[i + 1], &options->process_id_filter) != 0)
            {
                return -1;
            }
            options->has_process_id_filter = 1;
            i++;
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

int etw_viewer_build_default_session_name(unsigned long process_id, char *buffer, size_t buffer_size)
{
    int written;

    if (buffer == NULL || buffer_size == 0U)
    {
        return -1;
    }

    written = snprintf(buffer, buffer_size, "etw-viewer_%lu", process_id);
    if (written < 0 || (size_t)written >= buffer_size)
    {
        return -1;
    }

    return 0;
}

const char *etw_viewer_level_name(int level)
{
    switch (level)
    {
    case 1:
        return "CRITICAL";
    case 2:
        return "ERROR";
    case 3:
        return "WARNING";
    case 4:
        return "INFO";
    case 5:
        return "VERBOSE";
    default:
        return "UNKNOWN";
    }
}

static char etw_viewer_priority_letter(int level)
{
    switch (level)
    {
    case 1:
        return 'C';
    case 2:
        return 'E';
    case 3:
        return 'W';
    case 4:
        return 'I';
    case 5:
        return 'V';
    default:
        return 'U';
    }
}

void etw_viewer_print_usage(const char *argv0, int is_windows)
{
    const char *name;
    if (argv0 != NULL)
    {
        name = argv0;
    }
    else
    {
        name = "etw-viewer";
    }

    (void)is_windows;
    fprintf(stderr, "使用方法: %s [--pid <process-id>]\n", name);
}

int etw_viewer_format_timestamp_utc(int64_t timestamp_100ns, char *buffer, size_t buffer_size)
{
    static const int64_t filetime_unix_epoch_100ns = 116444736000000000LL;
    int64_t unix_100ns;
    int64_t tv_sec;
    int32_t tv_nsec;

    if (buffer == NULL || buffer_size == 0U)
    {
        return -1;
    }
    if (timestamp_100ns < filetime_unix_epoch_100ns)
    {
        return -1;
    }

    unix_100ns = timestamp_100ns - filetime_unix_epoch_100ns;
    tv_sec = unix_100ns / 10000000LL;
    tv_nsec = (int32_t)((unix_100ns % 10000000LL) * 100LL);
    return com_util_format_realtime_iso8601_local(buffer, buffer_size, tv_sec, tv_nsec);
}

void etw_viewer_handle_event(const com_util_etw_event *event, void *context)
{
    char timestamp_text[64];
    const char *timestamp_value;
    const char *tag;
    const char *message;
    const etw_viewer_context *viewer_context = (const etw_viewer_context *)context;

    if (event == NULL)
    {
        return;
    }
    if (event->event_name == NULL || strcmp(event->event_name, "Trace") != 0)
    {
        return;
    }
    if (event->message == NULL)
    {
        return;
    }
    if (viewer_context != NULL && viewer_context->has_process_id_filter &&
        event->process_id != viewer_context->process_id_filter)
    {
        return;
    }

    if (event->service != NULL && event->service[0] != '\0')
    {
        tag = event->service;
    }
    else
    {
        tag = ETW_VIEWER_DEFAULT_TAG;
    }
    message = event->message;
    if (etw_viewer_format_timestamp_utc(event->timestamp_100ns, timestamp_text, sizeof(timestamp_text)) != 0)
    {
        timestamp_value = "(invalid-timestamp)";
    }
    else
    {
        timestamp_value = timestamp_text;
    }

    printf("%s <%c>%s[%" PRIu32 "]: %s\n", timestamp_value, etw_viewer_priority_letter(event->level), tag,
           event->process_id, message);
    fflush(stdout);
}

static void print_access_error(void)
{
    fprintf(stderr, "ETW session の開始権限がありません。Administrators または "
                    "\"Performance Log Users\" が必要です。\n"
                    "対処方法: net localgroup \"Performance Log Users\" %%USERNAME%% /add\n"
                    "          この操作後にサインアウト/サインインが必要です。\n");
    fflush(stderr);
}

static void print_start_error(int status, const char *session_name)
{
    if (status == COM_UTIL_ETW_SESSION_ERR_ACCESS)
    {
        print_access_error();
    }
    else if (status == COM_UTIL_ETW_SESSION_ERR_PARAM)
    {
        fprintf(stderr, "ETW session の開始に失敗しました。内部パラメータが不正です。\n");
    }
    else
    {
        fprintf(stderr, "ETW session の開始に失敗しました。session=\"%s\" provider=\"%s\"\n", session_name,
                COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID_STR);
    }
}

int main(int argc, char *argv[])
{
    etw_viewer_options options;
    etw_viewer_context viewer_context;
    com_util_etw_session *session;
    int status;
    int exit_code = EXIT_FAILURE;
    char session_name[ETW_VIEWER_SESSION_NAME_MAX];

    com_util_console_init();
    g_stop_requested = 0;

    if (etw_viewer_parse_args(argc, argv, &options) != 0)
    {
        const char *print_usage_arg;
        if (argc > 0)
        {
            print_usage_arg = argv[0];
        }
        else
        {
            print_usage_arg = "etw-viewer";
        }
        etw_viewer_print_usage(print_usage_arg, 1);
        return exit_code;
    }

    if (etw_viewer_build_default_session_name((unsigned long)GetCurrentProcessId(), session_name,
                                              sizeof(session_name)) != 0)
    {
        fprintf(stderr, "既定 session 名の生成に失敗しました。\n");
        return exit_code;
    }

    viewer_context.process_id_filter = options.process_id_filter;
    viewer_context.has_process_id_filter = options.has_process_id_filter;

    status = com_util_etw_session_check_access();
    if (status == COM_UTIL_ETW_SESSION_ERR_ACCESS)
    {
        print_access_error();
        exit_code = EXIT_ACCESS_DENIED;
        return exit_code;
    }
    if (status != COM_UTIL_ETW_SESSION_OK)
    {
        fprintf(stderr, "ETW session の権限確認に失敗しました。\n");
        return exit_code;
    }

    if (com_util_shutdown_request_register(etw_viewer_shutdown_request_callback, NULL) != 0)
    {
        fprintf(stderr, "終了要求 callback の登録に失敗しました。\n");
        return exit_code;
    }

    session = com_util_etw_session_start(session_name, COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID_STR,
                                         etw_viewer_handle_event, &viewer_context, &status);
    if (session == NULL)
    {
        if (status == COM_UTIL_ETW_SESSION_ERR_ACCESS)
        {
            exit_code = EXIT_ACCESS_DENIED;
        }
        print_start_error(status, session_name);
        return exit_code;
    }

    printf("session=%s provider=%s\n", session_name, COM_UTIL_TRACER_DEFAULT_PROVIDER_GUID_STR);
    if (options.has_process_id_filter)
    {
        printf("filter.pid=%" PRIu32 "\n", options.process_id_filter);
    }
    printf("Ctrl+C で終了します。\n");
    fflush(stdout);

    while (!g_stop_requested)
    {
        com_util_sleep_ms(ETW_VIEWER_WAIT_MS);
    }

    com_util_etw_session_stop(session);
    exit_code = EXIT_SUCCESS;

    return exit_code;
}

#elif defined(PLATFORM_LINUX)

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    com_util_console_init();
    fprintf(stderr, "ETW viewer is supported only on Windows.\n");
    return EXIT_FAILURE;
}

#endif /* PLATFORM_ */
