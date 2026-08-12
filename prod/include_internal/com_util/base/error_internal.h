/**
 *******************************************************************************
 *  @file           error_internal.h
 *  @brief          詳細エラーの出力と TLS 記録を一体化する内部 API を提供します。
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_BASE_ERROR_INTERNAL_H
#define COM_UTIL_BASE_ERROR_INTERNAL_H

#include <com_util/base/error.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          errno と対応する共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      errno_value errno の値。
     *  @return         errno に対応する共通結果コードを返します。
     */
    int com_util_error_report_errno(com_util_error *detail_out, int errno_value);

    /**
     *  @brief          errno と指定された共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      errno_value errno の値。
     *  @param[in]      result 記録して返す共通結果コード。
     *  @return         @p result を返します。
     */
    int com_util_error_report_errno_as(com_util_error *detail_out, int errno_value, int result);

    /**
     *  @brief          ソケット操作の errno と対応する共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      errno_value errno の値。
     *  @return         errno に対応する共通結果コードを返します。
     *
     *  @ref COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO として記録するため、EAGAIN の要因が
     *  @ref COM_UTIL_CAUSE_WOULD_BLOCK と解釈されます。
     */
    int com_util_error_report_socket_errno(com_util_error *detail_out, int errno_value);

    /**
     *  @brief          ソケット操作の errno と指定された共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      errno_value errno の値。
     *  @param[in]      result 記録して返す共通結果コード。
     *  @return         @p result を返します。
     *
     *  ソケット操作が持つ生の errno を保持したまま、操作固有の結果コードを返す場合に使用します。
     *  非ブロッキング connect の EINPROGRESS は @ref COM_UTIL_ERR_IN_PROGRESS として記録します。
     */
    int com_util_error_report_socket_errno_as(com_util_error *detail_out, int errno_value, int result);

    /**
     *  @brief          getaddrinfo のエラー コードと対応する共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      error_code getaddrinfo が返した EAI_* の値。
     *  @return         EAI_* に対応する共通結果コードを返します。
     */
    int com_util_error_report_gai_error(com_util_error *detail_out, int error_code);

#if defined(PLATFORM_WINDOWS)
    /**
     *  @brief          Win32 エラーと対応する共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      error_code GetLastError() が返した値。
     *  @return         Win32 エラーに対応する共通結果コードを返します。
     */
    int com_util_error_report_windows_error(com_util_error *detail_out, unsigned long error_code);

    /**
     *  @brief          Win32 エラーと指定された共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      error_code GetLastError() が返した値。
     *  @param[in]      result 記録して返す共通結果コード。
     *  @return         @p result を返します。
     */
    int com_util_error_report_windows_error_as(com_util_error *detail_out, unsigned long error_code, int result);

    /**
     *  @brief          Winsock エラーと対応する共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      error_code WSAGetLastError() が返した値。
     *  @return         Winsock エラーに対応する共通結果コードを返します。
     *
     *  Winsock のエラー番号空間は Win32 の GetLastError() と異なるため、
     *  com_util_error_report_windows_error() では分類できません。
     */
    int com_util_error_report_winsock_error(com_util_error *detail_out, unsigned long error_code);

    /**
     *  @brief          Winsock エラーと指定された共通結果コードを記録します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @param[in]      error_code WSAGetLastError() が返した値。
     *  @param[in]      result 記録して返す共通結果コード。
     *  @return         @p result を返します。
     *
     *  Winsock の生のエラー番号を保持したまま、操作固有の結果コードを返す場合に使用します。
     *  非ブロッキング connect の WSAEWOULDBLOCK は @ref COM_UTIL_ERR_IN_PROGRESS として記録します。
     */
    int com_util_error_report_winsock_error_as(com_util_error *detail_out, unsigned long error_code, int result);
#endif

    /**
     *  @brief          成功を記録し、詳細エラーをクリアします。
     *  @param[out]     detail_out エラー詳細の格納先。NULL を指定した場合、本引数へは
     *                  エラー詳細を設定せず、返却しません。
     *  @return         COM_UTIL_OK を返します。
     */
    int com_util_error_report_success(com_util_error *detail_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_BASE_ERROR_INTERNAL_H */
