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
