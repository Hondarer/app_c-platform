/**
 *******************************************************************************
 *  @file           trace_etw_session.c
 *  @brief          リアルタイム ETW セッション API の実装です。
 *
 *  ETW セッションの開始、イベント データの復元、ワーカー スレッドの停止を管理します。
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <com_util/crt/wchar_conv.h>
    #include <com_util/crt/string.h>
    #include <com_util/sync/sync.h>
    #include <evntcons.h>
    #include <evntrace.h>
    #include <tdh.h>
    #pragma comment(lib, "Tdh.lib")
    #include <com_util/trace/etw.h>
    #include <stdio.h>
    #include <stdlib.h>

    #ifndef INVALID_PROCESSTRACE_HANDLE
        #define INVALID_PROCESSTRACE_HANDLE ((TRACEHANDLE)INVALID_HANDLE_VALUE)
    #endif /* INVALID_PROCESSTRACE_HANDLE */

/**
 *  @brief  ETW セッション構造体 (内部定義) です。
 */
struct com_util_etw_session
{
    /** トレース セッション ハンドル。 */
    TRACEHANDLE session_handle;
    /** トレース処理ハンドル。 */
    TRACEHANDLE trace_handle;
    /** ProcessTrace ワーカー スレッド。 */
    com_util_thread *thread_handle;
    /** イベント受信コールバック。 */
    com_util_etw_event_callback_t callback;
    /** コールバックに渡すユーザーデータ。 */
    void *context;
    /** セッション プロパティ (可変長)。 */
    EVENT_TRACE_PROPERTIES *properties;
    /** セッション名 (ワイド文字列)。 */
    wchar_t *session_name_w;
    /** プロバイダー GUID (イベント フィルタリング用)。 */
    GUID provider_guid;
};

/**
 *  @brief  メモリ領域をゼロ クリアする (memset 代替) です。
 *  @note   testfw が memset をモックするため、直接ゼロ代入で初期化します。
 */
static void zero_bytes(void *ptr, const size_t size)
{
    unsigned char *p = (unsigned char *)ptr;
    size_t i;
    for (i = 0; i < size; i++)
    {
        p[i] = 0;
    }
}

static com_util_etw_session *dispose_session_and_return_null(com_util_etw_session *session)
{
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

/**
 *  @brief  GUID 文字列 "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" をパースします。
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
 *  @brief  GUID 一致判定です。
 */
static int guid_equal(const GUID *a, const GUID *b)
{
    return a->Data1 == b->Data1 && a->Data2 == b->Data2 && a->Data3 == b->Data3 && a->Data4[0] == b->Data4[0] &&
           a->Data4[1] == b->Data4[1] && a->Data4[2] == b->Data4[2] && a->Data4[3] == b->Data4[3] &&
           a->Data4[4] == b->Data4[4] && a->Data4[5] == b->Data4[5] && a->Data4[6] == b->Data4[6] &&
           a->Data4[7] == b->Data4[7];
}

/**
 *  @brief  TRACE_EVENT_INFO を取得します。
 *  @return 成功時は確保済みポインター。失敗時は NULL。
 */
static TRACE_EVENT_INFO *get_trace_event_info(PEVENT_RECORD pEvent)
{
    TRACE_EVENT_INFO *info = NULL;
    ULONG size = 0;
    ULONG status;

    status = TdhGetEventInformation(pEvent, 0, NULL, NULL, &size);
    if (status != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        return NULL;
    }

    info = (TRACE_EVENT_INFO *)malloc(size);
    if (info == NULL)
    {
        return NULL;
    }

    status = TdhGetEventInformation(pEvent, 0, NULL, info, &size);
    if (status != ERROR_SUCCESS)
    {
        free(info);
        return NULL;
    }

    return info;
}

/**
 *  @brief  TRACE_EVENT_INFO からイベント名を取得します。
 *  @return イベント名。取得できない場合は NULL。
 */
static const wchar_t *get_event_name(const TRACE_EVENT_INFO *info)
{
    if (info == NULL || info->EventNameOffset == 0)
    {
        return NULL;
    }

    return (const wchar_t *)((const unsigned char *)info + info->EventNameOffset);
}

/**
 *  @brief  UserData 上の null 終端 ANSI 文字列を 1 つ読み取ります。
 *  @return 成功 0 / 失敗 -1。
 */
static int read_ansi_string_field(const unsigned char *cursor, const USHORT remaining, const char **out_text,
                                  USHORT *out_consumed)
{
    USHORT i;

    if (cursor == NULL || out_text == NULL || out_consumed == NULL)
    {
        return -1;
    }

    for (i = 0; i < remaining; i++)
    {
        if (cursor[i] == '\0')
        {
            *out_text = (const char *)cursor;
            *out_consumed = (USHORT)(i + 1U);
            return 0;
        }
    }

    return -1;
}

/**
 *  @brief  UserData 上の uint32 値を 1 つ読み取ります。
 *  @return 成功 0 / 失敗 -1。
 */
static int read_uint32_field(const unsigned char *cursor, const USHORT remaining, uint32_t *out_value,
                             USHORT *out_consumed)
{
    if (cursor == NULL || out_value == NULL || out_consumed == NULL || remaining < 4U)
    {
        return -1;
    }

    *out_value =
        (uint32_t)cursor[0] | ((uint32_t)cursor[1] << 8) | ((uint32_t)cursor[2] << 16) | ((uint32_t)cursor[3] << 24);
    *out_consumed = 4U;
    return 0;
}

/**
 *  @brief  TraceLogging payload から Service / Message を復元します。
 *
 *  TdhGetEventInformation で得たプロパティ順に ANSI 文字列を読み進める。
 *  Service / Message が存在しないイベントは out_* を NULL のまま返します。
 */
static void extract_event_fields(PEVENT_RECORD pEvent, const TRACE_EVENT_INFO *info, const char **out_service,
                                 const char **out_message, uint32_t *out_process_id)
{
    const unsigned char *cursor;
    USHORT remaining;
    ULONG count;
    ULONG i;

    if (out_service != NULL)
    {
        *out_service = NULL;
    }
    if (out_message != NULL)
    {
        *out_message = NULL;
    }
    if (out_process_id != NULL)
    {
        *out_process_id = 0U;
    }
    if (pEvent == NULL || out_message == NULL)
    {
        return;
    }

    if (pEvent->UserData == NULL || pEvent->UserDataLength == 0)
    {
        return;
    }

    if (info == NULL || info->TopLevelPropertyCount == 0)
    {
        return;
    }

    cursor = (const unsigned char *)pEvent->UserData;
    remaining = pEvent->UserDataLength;
    count = info->TopLevelPropertyCount;

    for (i = 0; i < count && remaining > 0; i++)
    {
        const EVENT_PROPERTY_INFO *prop;
        const wchar_t *name;
        const char *text;
        uint32_t process_id_value;
        USHORT consumed;

        prop = &info->EventPropertyInfoArray[i];
        if ((prop->Flags & PropertyStruct) != 0)
        {
            continue;
        }
        if (prop->NameOffset == 0)
        {
            continue;
        }
        name = (const wchar_t *)((const unsigned char *)info + prop->NameOffset);
        if (name == NULL)
        {
            continue;
        }

        if (prop->nonStructType.InType == TDH_INTYPE_ANSISTRING)
        {
            if (read_ansi_string_field(cursor, remaining, &text, &consumed) != 0)
            {
                break;
            }

            if (wcscmp(name, L"Service") == 0)
            {
                if (out_service != NULL)
                {
                    *out_service = text;
                }
            }
            else if (wcscmp(name, L"Message") == 0)
            {
                *out_message = text;
            }
        }
        else if (prop->nonStructType.InType == TDH_INTYPE_UINT32)
        {
            if (read_uint32_field(cursor, remaining, &process_id_value, &consumed) != 0)
            {
                break;
            }

            if (wcscmp(name, L"ProcessId") == 0 && out_process_id != NULL)
            {
                *out_process_id = process_id_value;
            }
        }
        else
        {
            break;
        }

        cursor += consumed;
        remaining = (USHORT)(remaining - consumed);
    }
}

/**
 *  @brief  ETW イベント レコード コールバック (ProcessTrace から呼ばれる) です。
 *
 *  プロバイダー GUID でフィルタリングし、UserData を null 終端文字列として読み取ります。
 *  TraceLoggingString は UserData に null 終端 ANSI 文字列を直接格納します。
 */
static VOID WINAPI event_record_callback(PEVENT_RECORD pEvent)
{
    com_util_etw_session *session;
    com_util_etw_event event;
    TRACE_EVENT_INFO *info;
    const wchar_t *event_name_w;
    char *event_name_utf8;
    const char *service;
    const char *message;
    uint32_t payload_process_id;

    if (pEvent == NULL)
    {
        return;
    }

    session = (com_util_etw_session *)pEvent->UserContext;
    if (session == NULL || session->callback == NULL)
    {
        return;
    }

    /* プロバイダー GUID でフィルタリング */
    if (!guid_equal(&pEvent->EventHeader.ProviderId, &session->provider_guid))
    {
        return;
    }

    info = get_trace_event_info(pEvent);
    event_name_w = get_event_name(info);
    if (event_name_w == NULL || wcscmp(event_name_w, L"Trace") != 0)
    {
        free(info);
        return;
    }
    event_name_utf8 = com_util_wstr_to_utf8_alloc(event_name_w);

    service = NULL;
    message = NULL;
    payload_process_id = 0U;
    extract_event_fields(pEvent, info, &service, &message, &payload_process_id);
    free(info);

    event.level = pEvent->EventHeader.EventDescriptor.Level;
    event.process_id = payload_process_id != 0U ? payload_process_id : pEvent->EventHeader.ProcessId;
    event.event_name = event_name_utf8;
    event.service = service;
    event.message = message;
    event.timestamp_100ns = pEvent->EventHeader.TimeStamp.QuadPart;

    session->callback(&event, session->context);
    free(event_name_utf8);
}

/**
 *  @brief  ProcessTrace ワーカー スレッド関数です。
 */
static void trace_thread_proc(void *param)
{
    com_util_etw_session *session = (com_util_etw_session *)param;

    ProcessTrace(&session->trace_handle, 1, NULL, NULL);
    return;
}

/**
 *  @brief  out_status が非 NULL なら値を設定します。
 */
static void set_status(int *out_status, int value)
{
    if (out_status != NULL)
    {
        *out_status = value;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

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

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_etw_session *COM_UTIL_API com_util_etw_session_start(const char *session_name,
                                                                              const char *provider_guid_str,
                                                                              com_util_etw_event_callback_t callback,
                                                                              void *context, int *out_status)
{
    com_util_etw_session *session = NULL;
    GUID provider_guid;
    ULONG status;
    size_t name_len_w;
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

    session = (com_util_etw_session *)malloc(sizeof(com_util_etw_session));
    if (session == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        return NULL;
    }

    zero_bytes(session, sizeof(com_util_etw_session));
    session->callback = callback;
    session->context = context;
    session->session_handle = 0;
    session->trace_handle = INVALID_PROCESSTRACE_HANDLE;
    session->thread_handle = NULL;
    session->properties = NULL;
    session->session_name_w = NULL;
    session->provider_guid = provider_guid;

    /* セッション名をワイド文字列に変換して確保 */
    session->session_name_w = com_util_utf8_to_wstr_alloc(session_name);
    if (session->session_name_w == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_PARAM);
        return dispose_session_and_return_null(session);
    }
    name_len_w = wcslen(session->session_name_w) + 1;

    /* EVENT_TRACE_PROPERTIES を確保 (セッション名領域を含む) */
    props_size = sizeof(EVENT_TRACE_PROPERTIES) + (name_len_w * sizeof(wchar_t));
    session->properties = (EVENT_TRACE_PROPERTIES *)malloc(props_size);
    if (session->properties == NULL)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        return dispose_session_and_return_null(session);
    }

    /* リアルタイム セッションを開始 */
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
        return dispose_session_and_return_null(session);
    }

    if (status != ERROR_SUCCESS)
    {
        session->session_handle = 0;
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        return dispose_session_and_return_null(session);
    }

    /* プロバイダーを有効化 */
    etp.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
    status = EnableTraceEx2(session->session_handle, &provider_guid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, 5,
                            0xFFFFFFFFFFFFFFFF, 0, 0, &etp);
    if (status != ERROR_SUCCESS)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        return dispose_session_and_return_null(session);
    }

    /* トレースをオープンしワーカー スレッドを起動 */
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
        return dispose_session_and_return_null(session);
    }

    if (com_util_thread_create(&session->thread_handle, trace_thread_proc, session) != 0)
    {
        set_status(out_status, COM_UTIL_ETW_SESSION_ERR_SYSTEM);
        return dispose_session_and_return_null(session);
    }

    set_status(out_status, COM_UTIL_ETW_SESSION_OK);
    return session;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API com_util_etw_session_stop(com_util_etw_session *session)
{
    if (session == NULL)
    {
        return;
    }

    /* セッション停止 (バッファー フラッシュ → ProcessTrace が残イベントを処理して戻る) */
    if (session->session_handle != 0 && session->properties != NULL)
    {
        ControlTraceW(session->session_handle, NULL, session->properties, EVENT_TRACE_CONTROL_STOP);
    }

    /* ワーカー スレッド join */
    if (session->thread_handle != NULL)
    {
        com_util_thread_join(session->thread_handle, COM_UTIL_SYNC_WAIT_FOREVER);
    }

    /* トレース ハンドル クローズ */
    if (session->trace_handle != INVALID_PROCESSTRACE_HANDLE)
    {
        CloseTrace(session->trace_handle);
    }

    free(session->session_name_w);
    free(session->properties);
    free(session);
}

#endif /* PLATFORM_WINDOWS */
