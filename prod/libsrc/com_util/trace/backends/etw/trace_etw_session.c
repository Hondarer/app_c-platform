#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <com_util/crt/string.h>
    #include <com_util/sync/sync.h>
    #include <evntcons.h>
    #include <evntrace.h>
    #pragma comment(lib, "Advapi32.lib")
    #include <com_util/trace/etw.h>
    #include <stdio.h>
    #include <stdlib.h>

    #ifndef INVALID_PROCESSTRACE_HANDLE
        #define INVALID_PROCESSTRACE_HANDLE ((TRACEHANDLE)INVALID_HANDLE_VALUE)
    #endif /* INVALID_PROCESSTRACE_HANDLE */

/**
 *  @brief  ETW セッション構造体 (内部定義)。
 */
struct com_util_etw_session
{
    /** トレースセッションハンドル。 */
    TRACEHANDLE session_handle;
    /** トレース処理ハンドル。 */
    TRACEHANDLE trace_handle;
    /** ProcessTrace ワーカースレッド。 */
    com_util_thread_t thread_handle;
    /** イベント受信コールバック。 */
    com_util_etw_event_callback_t callback;
    /** コールバックに渡すユーザーデータ。 */
    void *context;
    /** セッションプロパティ (可変長)。 */
    EVENT_TRACE_PROPERTIES *properties;
    /** セッション名 (ワイド文字列)。 */
    wchar_t *session_name_w;
    /** プロバイダ GUID (イベントフィルタリング用)。 */
    GUID provider_guid;
};

/**
 *  @brief  メモリ領域をゼロクリアする (memset 代替)。
 *  @note   testfw が memset をモックするため、直接ゼロ代入で初期化する。
 */
static void zero_bytes(void *ptr, size_t size)
{
    unsigned char *p = (unsigned char *)ptr;
    size_t i;
    for (i = 0; i < size; i++)
    {
        p[i] = 0;
    }
}

/**
 *  @brief  GUID 文字列 "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" をパースする。
 *  @return 成功 0 / 失敗 -1。
 */
static int parse_guid(const char *str, GUID *guid)
{
    unsigned int d[11];
    int n;

    if (str == NULL || guid == NULL)
    {
        return -1;
    }

    n = com_util_sscanf(str, "%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x", &d[0], &d[1], &d[2], &d[3], &d[4], &d[5], &d[6],
                        &d[7], &d[8], &d[9], &d[10]);

    if (n != 11)
    {
        return -1;
    }

    guid->Data1 = (ULONG)d[0];
    guid->Data2 = (USHORT)d[1];
    guid->Data3 = (USHORT)d[2];
    guid->Data4[0] = (UCHAR)d[3];
    guid->Data4[1] = (UCHAR)d[4];
    guid->Data4[2] = (UCHAR)d[5];
    guid->Data4[3] = (UCHAR)d[6];
    guid->Data4[4] = (UCHAR)d[7];
    guid->Data4[5] = (UCHAR)d[8];
    guid->Data4[6] = (UCHAR)d[9];
    guid->Data4[7] = (UCHAR)d[10];
    return 0;
}

/**
 *  @brief  GUID 一致判定。
 */
static int guid_equal(const GUID *a, const GUID *b)
{
    return a->Data1 == b->Data1 && a->Data2 == b->Data2 && a->Data3 == b->Data3 && a->Data4[0] == b->Data4[0] &&
           a->Data4[1] == b->Data4[1] && a->Data4[2] == b->Data4[2] && a->Data4[3] == b->Data4[3] &&
           a->Data4[4] == b->Data4[4] && a->Data4[5] == b->Data4[5] && a->Data4[6] == b->Data4[6] &&
           a->Data4[7] == b->Data4[7];
}

/**
 *  @brief  ETW イベントレコードコールバック (ProcessTrace から呼ばれる)。
 *
 *  プロバイダ GUID でフィルタリングし、UserData を null 終端文字列として読み取る。
 *  TraceLoggingString は UserData に null 終端 ANSI 文字列を直接格納する。
 */
static VOID WINAPI event_record_callback(PEVENT_RECORD pEvent)
{
    com_util_etw_session_t *session;
    const char *message;
    int level;

    if (pEvent == NULL)
    {
        return;
    }

    session = (com_util_etw_session_t *)pEvent->UserContext;
    if (session == NULL || session->callback == NULL)
    {
        return;
    }

    /* プロバイダ GUID でフィルタリング */
    if (!guid_equal(&pEvent->EventHeader.ProviderId, &session->provider_guid))
    {
        return;
    }

    level = pEvent->EventHeader.EventDescriptor.Level;

    message = NULL;
    if (pEvent->UserData != NULL && pEvent->UserDataLength > 0)
    {
        message = (const char *)pEvent->UserData;
    }

    session->callback(level, message, session->context);
}

/**
 *  @brief  ProcessTrace ワーカースレッド関数。
 */
static void trace_thread_proc(void *param)
{
    com_util_etw_session_t *session = (com_util_etw_session_t *)param;

    ProcessTrace(&session->trace_handle, 1, NULL, NULL);
    return;
}

/**
 *  @brief  out_status が非 NULL なら値を設定する。
 */
static void set_status(int *out_status, int value)
{
    if (out_status != NULL)
    {
        *out_status = value;
    }
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_etw_session_check_access(void)
{
    static const wchar_t probe_name[] = L"EtwUtil_AccessProbe";
    size_t props_size;
    EVENT_TRACE_PROPERTIES *props;
    TRACEHANDLE handle = 0;
    ULONG status;
    int result;

    props_size = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(probe_name);
    props = (EVENT_TRACE_PROPERTIES *)malloc(props_size);
    if (props == NULL)
    {
        return COM_UTIL_ETW_SESSION_ERR_SYSTEM;
    }

    zero_bytes(props, props_size);
    props->Wnode.BufferSize = (ULONG)props_size;
    props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    props->Wnode.ClientContext = 1;
    props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    status = StartTraceW(&handle, probe_name, props);

    if (status == ERROR_SUCCESS)
    {
        ControlTraceW(handle, NULL, props, EVENT_TRACE_CONTROL_STOP);
        result = COM_UTIL_ETW_SESSION_OK;
    }
    else if (status == ERROR_ACCESS_DENIED)
    {
        result = COM_UTIL_ETW_SESSION_ERR_ACCESS;
    }
    else
    {
        result = COM_UTIL_ETW_SESSION_ERR_SYSTEM;
    }

    free(props);
    return result;
}

COM_UTIL_EXPORT com_util_etw_session_t *COM_UTIL_API com_util_etw_session_start(const char *session_name,
                                                                          const char *provider_guid_str,
                                                                          com_util_etw_event_callback_t callback,
                                                                          void *context, int *out_status)
{
    com_util_etw_session_t *session = NULL;
    GUID provider_guid;
    ULONG status;
    int name_len_w;
    size_t props_size;
    ENABLE_TRACE_PARAMETERS etp = {0};

    if (session_name == NULL || provider_guid_str == NULL || callback == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_PARAM);
        return NULL;
    }

    if (parse_guid(provider_guid_str, &provider_guid) != 0)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_PARAM);
        return NULL;
    }

    /* セッション名をワイド文字列に変換 */
    name_len_w = MultiByteToWideChar(CP_UTF8, 0, session_name, -1, NULL, 0);
    if (name_len_w <= 0)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_PARAM);
        return NULL;
    }

    session = (com_util_etw_session_t *)malloc(sizeof(com_util_etw_session_t));
    if (session == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        return NULL;
    }

    zero_bytes(session, sizeof(com_util_etw_session_t));
    session->callback = callback;
    session->context = context;
    session->session_handle = 0;
    session->trace_handle = INVALID_PROCESSTRACE_HANDLE;
    session->thread_handle = NULL;
    session->properties = NULL;
    session->session_name_w = NULL;
    session->provider_guid = provider_guid;

    /* ワイド文字列セッション名を確保 */
    session->session_name_w = (wchar_t *)malloc((size_t)name_len_w * sizeof(wchar_t));
    if (session->session_name_w == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        goto cleanup;
    }
    MultiByteToWideChar(CP_UTF8, 0, session_name, -1, session->session_name_w, name_len_w);

    /* EVENT_TRACE_PROPERTIES を確保 (セッション名領域を含む) */
    props_size = sizeof(EVENT_TRACE_PROPERTIES) + ((size_t)name_len_w * sizeof(wchar_t));
    session->properties = (EVENT_TRACE_PROPERTIES *)malloc(props_size);
    if (session->properties == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        goto cleanup;
    }

    /* リアルタイムセッションを開始 */
    zero_bytes(session->properties, props_size);
    session->properties->Wnode.BufferSize = (ULONG)props_size;
    session->properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    session->properties->Wnode.ClientContext = 1; /* QPC */
    session->properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    session->properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    session->properties->FlushTimer = 1;

    status = StartTraceW(&session->session_handle, session->session_name_w, session->properties);

    if (status == ERROR_ACCESS_DENIED)
    {
        session->session_handle = 0;
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_ACCESS);
        goto cleanup;
    }

    if (status != ERROR_SUCCESS)
    {
        session->session_handle = 0;
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        goto cleanup;
    }

    /* プロバイダを有効化 */
    etp.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    status = EnableTraceEx2(session->session_handle, &provider_guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, 5,
                            0xFFFFFFFFFFFFFFFF, 0, 0, &etp);
    if (status != ERROR_SUCCESS)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        goto cleanup;
    }

    /* トレースをオープンしワーカースレッドを起動 */
    {
        EVENT_TRACE_LOGFILEW trace_logfile = {0};
        trace_logfile.LoggerName = session->session_name_w;
        trace_logfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
        trace_logfile.EventRecordCallback = event_record_callback;
        trace_logfile.Context = session;

        session->trace_handle = OpenTraceW(&trace_logfile);
    }
    if (session->trace_handle == INVALID_PROCESSTRACE_HANDLE)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        goto cleanup;
    }

    if (com_util_thread_create(&session->thread_handle, trace_thread_proc, session) != 0)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        goto cleanup;
    }

    set_status(out_status, COM_UTIL_ETW_SESSION_OK);
    return session;

cleanup:
    if (session != NULL)
    {
        if (session->trace_handle != INVALID_PROCESSTRACE_HANDLE)
        {
            CloseTrace(session->trace_handle);
        }
        if (session->session_handle != 0)
        {
            ControlTraceW(session->session_handle, NULL, session->properties, EVENT_TRACE_CONTROL_STOP);
        }
        free(session->session_name_w);
        free(session->properties);
        free(session);
    }
    return NULL;
}

COM_UTIL_EXPORT void COM_UTIL_API com_util_etw_session_stop(com_util_etw_session_t *session)
{
    if (session == NULL)
    {
        return;
    }

    /* セッション停止 (バッファフラッシュ → ProcessTrace が残イベントを処理して戻る) */
    if (session->session_handle != 0 && session->properties != NULL)
    {
        ControlTraceW(session->session_handle, NULL, session->properties, EVENT_TRACE_CONTROL_STOP);
    }

    /* ワーカースレッド join */
    if (session->thread_handle != NULL)
    {
        com_util_thread_join(&session->thread_handle);
    }

    /* トレースハンドルクローズ */
    if (session->trace_handle != INVALID_PROCESSTRACE_HANDLE)
    {
        CloseTrace(session->trace_handle);
    }

    free(session->session_name_w);
    free(session->properties);
    free(session);
}

#endif /* PLATFORM_WINDOWS */
