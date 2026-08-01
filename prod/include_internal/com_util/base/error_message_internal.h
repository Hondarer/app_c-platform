/**
 *******************************************************************************
 *  @file           error_message_internal.h
 *  @brief          ドメイン固有の OS エラー値を文字列化する内部 API を提供します。
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_BASE_ERROR_MESSAGE_INTERNAL_H
#define COM_UTIL_BASE_ERROR_MESSAGE_INTERNAL_H

#include <com_util/base/error_message.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          errno の値に対応するメッセージを取得します。
     *  @param[out]     buf         メッセージの格納先。
     *  @param[in]      buf_size    @p buf のバイト数。
     *  @param[in]      errno_value errno の値。
     *  @return         COM_UTIL_OK、COM_UTIL_ERR_INVALID_ARGUMENT、COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     */
    int com_util_errno_message(char *buf, size_t buf_size, int errno_value);

#if defined(PLATFORM_WINDOWS)
    /**
     *  @brief          Win32 エラー コードに対応するメッセージを取得します。
     *  @param[out]     buf        メッセージの格納先。
     *  @param[in]      buf_size   @p buf のバイト数。
     *  @param[in]      error_code GetLastError() が返した値。
     *  @return         COM_UTIL_OK、COM_UTIL_ERR_INVALID_ARGUMENT、COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     */
    int com_util_win32_error_message(char *buf, size_t buf_size, unsigned long error_code);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_BASE_ERROR_MESSAGE_INTERNAL_H */
