/**
 *******************************************************************************
 *  @file           result_internal.h
 *  @brief          OS エラー値を共通結果コードへ変換する内部 API を提供します。
 *
 *  errno および Windows の GetLastError() の値を、
 *  `com_util/base/result.h` の共通結果コードへ変換します。\n
 *  各モジュールに固有のエラー値の解釈 (例: 特定の errno を別コードへ割り当てる) は、
 *  本 API を呼び出す前段でモジュール側が判定してください。
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_RESULT_INTERNAL_H
#define COM_UTIL_RESULT_INTERNAL_H

#include <com_util/base/platform.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          errno の値を共通結果コードへ変換します。
     *  @param[in]      errno_value errno の値。
     *  @return         対応する共通結果コード。対応がない値は @ref COM_UTIL_ERR_UNKNOWN を返します。
     */
    int com_util_result_from_errno(int errno_value);

#if defined(PLATFORM_WINDOWS)
    /**
     *  @brief          GetLastError() の値を共通結果コードへ変換します。
     *  @param[in]      error_code GetLastError() で取得したエラー コード。
     *  @return         対応する共通結果コード。対応がない値は @ref COM_UTIL_ERR_UNKNOWN を返します。
     */
    int com_util_result_from_windows_error(unsigned long error_code);

    /**
     *  @brief          GetLastError() の値を errno 相当の値へ変換します。
     *  @param[in]      error_code GetLastError() で取得したエラー コード。
     *  @return         対応する errno の値。対応がない値は `EIO` を返します。
     *
     *  errno を返す API の `errno_out` へ Win32 エラー コードをそのまま格納すると、
     *  errno として文字列化した際に無関係なメッセージになります。\n
     *  Windows 固有の実装から errno ドメインの出力引数へ値を渡す場合は本 API を使用してください。
     */
    int com_util_errno_from_windows_error(unsigned long error_code);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_RESULT_INTERNAL_H */
