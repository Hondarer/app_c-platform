/**
 *******************************************************************************
 *  @file           trace_eventlog.c
 *  @brief          Windows イベント ログ (EventLog) シンク実装ファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/06/14
 *  @version        1.0.0
 *
 *  Windows のアプリケーション イベント ログへの書き込みと、共通イベント
 *  ソースのレジストリ登録/削除を提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <com_util/crt/wchar_conv.h>
    #include <com_util/trace/eventlog.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <wchar.h>

    #include <com_util/trace/backends/eventlog/eventlog_internal.h>

/** イベント ソース登録キーのレジストリ パス接頭辞 (HKLM 配下)。 */
    #define EVENTLOG_KEY_PREFIX L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"

/**
 *  @brief  EventLog シンク ハンドル構造体 (内部定義)。
 */
struct com_util_eventlog_sink
{
    /** RegisterEventSourceW が返すイベント ソース ハンドル。 */
    HANDLE source;
};

/**
 *  @brief          トレース レベルをイベント タイプ・カテゴリー・イベント ID に写像する。
 *  @param[in]      level     トレース レベル (0=CRITICAL 〜 5=DEBUG)。範囲外は Information 扱い。
 *  @param[out]     type      EventLog イベント タイプ (EVENTLOG_*_TYPE)。
 *  @param[out]     category  イベント カテゴリー (レベル毎に分離)。
 *  @param[out]     event_id  イベント ID (レベル毎に分離)。
 *
 *  分析性を高めるため、同一ソースでもレベル毎にカテゴリーとイベント ID を
 *  分けて割り当てます。
 */
static void map_level(const int level, WORD *type, WORD *category, DWORD *event_id)
{
    switch (level)
    {
    case 0: /* CRITICAL */
        *type = EVENTLOG_ERROR_TYPE;
        break;
    case 1: /* ERROR */
        *type = EVENTLOG_ERROR_TYPE;
        break;
    case 2: /* WARNING */
        *type = EVENTLOG_WARNING_TYPE;
        break;
    case 3: /* INFO */
        *type = EVENTLOG_INFORMATION_TYPE;
        break;
    case 4: /* VERBOSE */
        *type = EVENTLOG_INFORMATION_TYPE;
        break;
    case 5: /* DEBUG */
        *type = EVENTLOG_INFORMATION_TYPE;
        break;
    default:
        *type = EVENTLOG_INFORMATION_TYPE;
        break;
    }

    if (level >= 0 && level < COM_UTIL_EVENTLOG_LEVEL_COUNT)
    {
        *category = (WORD)(level + 1);
        *event_id = (DWORD)(level + 1);
    }
    else
    {
        *category = 0;
        *event_id = 0;
    }
}

/**
 *  @brief          イベント ソース名からレジストリ キー パスを組み立てる。
 *  @param[in]      source_name  イベント ソース名 (UTF-8)。
 *  @param[out]     key_path     書き込み先のワイド文字列バッファー。
 *  @param[in]      key_count    key_path の要素数。
 *  @return         成功 0 / 失敗 -1。
 */
static int build_source_key_path(const char *source_name, wchar_t *key_path, const size_t key_count)
{
    wchar_t *wsource;
    int written;

    wsource = com_util_utf8_to_wstr_alloc(source_name);
    if (wsource == NULL)
    {
        return -1;
    }

    written = swprintf(key_path, key_count, L"%ls%ls", EVENTLOG_KEY_PREFIX, wsource);
    free(wsource);

    if (written < 0)
    {
        return -1;
    }
    return 0;
}

/**
 *  @brief          RegCreateKeyExW / RegDeleteKeyW の戻り値をステータス コードに変換する。
 *  @param[in]      rc  Win32 レジストリ API の戻り値。
 *  @return         COM_UTIL_EVENTLOG_OK / COM_UTIL_EVENTLOG_ERR_ACCESS / COM_UTIL_EVENTLOG_ERR_SYSTEM。
 */
static int registry_status(const LONG rc)
{
    if (rc == ERROR_SUCCESS)
    {
        return COM_UTIL_EVENTLOG_OK;
    }
    if (rc == ERROR_ACCESS_DENIED)
    {
        return COM_UTIL_EVENTLOG_ERR_ACCESS;
    }
    return COM_UTIL_EVENTLOG_ERR_SYSTEM;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_eventlog_sink *COM_UTIL_API com_util_eventlog_sink_create(const char *source_name)
{
    com_util_eventlog_sink *handle;
    wchar_t *wsource;
    HANDLE source;

    if (source_name == NULL)
    {
        return NULL;
    }

    wsource = com_util_utf8_to_wstr_alloc(source_name);
    if (wsource == NULL)
    {
        return NULL;
    }

    source = RegisterEventSourceW(NULL, wsource);
    free(wsource);
    if (source == NULL)
    {
        return NULL;
    }

    handle = (com_util_eventlog_sink *)malloc(sizeof(com_util_eventlog_sink));
    if (handle == NULL)
    {
        DeregisterEventSource(source);
        return NULL;
    }

    handle->source = source;
    return handle;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_eventlog_sink_write(com_util_eventlog_sink *handle, const int level,
                                                              const char *instance_name, const char *message)
{
    WORD type;
    WORD category;
    DWORD event_id;
    wchar_t *wmsg;
    LPCWSTR strings[1];
    BOOL ok;

    if (handle == NULL || handle->source == NULL || message == NULL)
    {
        return 0;
    }

    map_level(level, &type, &category, &event_id);

    if (instance_name != NULL)
    {
        char *body;
        int need;

        need = snprintf(NULL, 0, "[%s] %s", instance_name, message);
        if (need < 0)
        {
            return -1;
        }

        body = (char *)malloc((size_t)need + 1);
        if (body == NULL)
        {
            return -1;
        }

        snprintf(body, (size_t)need + 1, "[%s] %s", instance_name, message);
        wmsg = com_util_utf8_to_wstr_alloc(body);
        free(body);
    }
    else
    {
        wmsg = com_util_utf8_to_wstr_alloc(message);
    }

    if (wmsg == NULL)
    {
        return -1;
    }

    strings[0] = wmsg;
    ok = ReportEventW(handle->source, type, category, event_id, NULL, 1, 0, strings, NULL);
    free(wmsg);

    if (!ok)
    {
        return -1;
    }
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_eventlog_sink_dispose(com_util_eventlog_sink *handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->source != NULL)
    {
        DeregisterEventSource(handle->source);
    }
    free(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_eventlog_sink_dispose_on_shutdown(com_util_eventlog_sink *handle, const com_util_shutdown_event *event)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->source != NULL && (event == NULL || event->reason == COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT))
    {
        DeregisterEventSource(handle->source);
    }
    free(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_eventlog_register_source(const char *source_name)
{
    wchar_t key_path[512];
    HKEY hkey;
    LONG rc;
    DWORD disposition;
    DWORD types_supported;
    DWORD category_count;

    if (source_name == NULL)
    {
        return COM_UTIL_EVENTLOG_ERR_PARAM;
    }

    if (build_source_key_path(source_name, key_path, sizeof(key_path) / sizeof(key_path[0])) != 0)
    {
        return COM_UTIL_EVENTLOG_ERR_SYSTEM;
    }

    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, key_path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkey,
                         &disposition);
    if (rc != ERROR_SUCCESS)
    {
        return registry_status(rc);
    }

    types_supported = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    rc = RegSetValueExW(hkey, L"TypesSupported", 0, REG_DWORD, (const BYTE *)&types_supported,
                        (DWORD)sizeof(types_supported));
    if (rc == ERROR_SUCCESS)
    {
        category_count = COM_UTIL_EVENTLOG_LEVEL_COUNT;
        rc = RegSetValueExW(hkey, L"CategoryCount", 0, REG_DWORD, (const BYTE *)&category_count,
                            (DWORD)sizeof(category_count));
    }

    RegCloseKey(hkey);
    return registry_status(rc);
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_eventlog_unregister_source(const char *source_name)
{
    wchar_t key_path[512];
    LONG rc;

    if (source_name == NULL)
    {
        return COM_UTIL_EVENTLOG_ERR_PARAM;
    }

    if (build_source_key_path(source_name, key_path, sizeof(key_path) / sizeof(key_path[0])) != 0)
    {
        return COM_UTIL_EVENTLOG_ERR_SYSTEM;
    }

    rc = RegDeleteKeyW(HKEY_LOCAL_MACHINE, key_path);
    if (rc == ERROR_FILE_NOT_FOUND)
    {
        return COM_UTIL_EVENTLOG_OK;
    }
    return registry_status(rc);
}

#endif /* PLATFORM_WINDOWS */
