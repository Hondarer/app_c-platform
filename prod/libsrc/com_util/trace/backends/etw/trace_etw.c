/**
 *******************************************************************************
 *  @file           trace_etw.c
 *  @brief          ETW プロバイダ実装ファイル。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  Windows TraceLogging ベースの ETW プロバイダを提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

#include <com_util/base/windows_sdk.h>
#include <TraceLoggingProvider.h>
#include <com_util/trace/etw.h>
#include <stdlib.h>

#include <com_util/trace/backends/etw/etw_internal.h>

/**
 *  @brief  ETW プロバイダハンドル構造体 (内部定義)。
 */
struct com_util_etw_provider
{
    /** TraceLogging プロバイダ参照。 */
    com_util_etw_provider_ref_t provider_ref;
};

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT com_util_etw_provider_t *COM_UTIL_API
    com_util_etw_provider_create(com_util_etw_provider_ref_t provider_ref)
{
    com_util_etw_provider_t *handle;
    TLG_STATUS status;

    if (provider_ref == NULL)
    {
        return NULL;
    }

    handle = (com_util_etw_provider_t *)malloc(sizeof(com_util_etw_provider_t));
    if (handle == NULL)
    {
        return NULL;
    }

    handle->provider_ref = provider_ref;

    status = TraceLoggingRegister(provider_ref);
    if (status != S_OK)
    {
        free(handle);
        return NULL;
    }

    return handle;
}

/**
 *  @brief          ETW イベントを書き込む内部関数。
 *
 *  service が NULL の場合は Service フィールドを含めない。
 *
 *  @param[in]      ref     TraceLogging プロバイダ参照。
 *  @param[in]      level   トレースレベル (1=Critical 〜 5=Verbose)。
 *  @param[in]      service サービス名 (NULL 可)。NULL の場合 Service フィールドを省略。
 *  @param[in]      message メッセージ文字列。
 */
static void write_trace_event(com_util_etw_provider_ref_t ref, const int level,
                               const char *service, const char *message)
{
    uint32_t process_id = (uint32_t)GetCurrentProcessId();

    if (service != NULL)
    {
        switch (level)
        {
        case 1:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(1),
                TraceLoggingString(service, "Service"),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        case 2:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(2),
                TraceLoggingString(service, "Service"),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        case 3:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(3),
                TraceLoggingString(service, "Service"),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        case 4:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(4),
                TraceLoggingString(service, "Service"),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        default:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(5),
                TraceLoggingString(service, "Service"),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        }
    }
    else
    {
        switch (level)
        {
        case 1:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(1),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        case 2:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(2),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        case 3:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(3),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        case 4:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(4),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        default:
            TraceLoggingWrite(ref, "Trace",
                TraceLoggingLevel(5),
                TraceLoggingString(message, "Message"),
                TraceLoggingUInt32(process_id, "ProcessId"));
            break;
        }
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API
    com_util_etw_provider_write(com_util_etw_provider_t *handle, const int level,
                       const char *service, const char *message)
{
    if (handle == NULL || message == NULL)
    {
        return 0;
    }

    write_trace_event(handle->provider_ref, level, service, message);

    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API
    com_util_etw_provider_dispose(com_util_etw_provider_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    TraceLoggingUnregister(handle->provider_ref);
    free(handle);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_etw_provider_dispose_on_shutdown(com_util_etw_provider_t *handle,
                                               const com_util_shutdown_event_t *event)
{
    if (handle == NULL)
    {
        return;
    }

    if (event == NULL || event->reason == COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT)
    {
        TraceLoggingUnregister(handle->provider_ref);
    }
    free(handle);
}

#endif /* PLATFORM_WINDOWS */
