/**
 *******************************************************************************
 *  @file           result.c
 *  @brief          OS エラー値を共通結果コードへ変換する内部 API を実装します。
 *
 *  errno および Windows の GetLastError() の値のうち、汎用的に対応が
 *  決まるものだけを共通結果コードへ変換します。\n
 *  モジュールに固有の解釈は、本 API を呼び出す前段で各モジュールが判定します。
 *
 *******************************************************************************
 */

#include <com_util/base/result_internal.h>
#include <com_util/base/result.h>

#include <errno.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_result_from_errno(const int errno_value)
{
    int result;

    if (errno_value == EINVAL)
    {
        result = COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    else if ((errno_value == EACCES) || (errno_value == EPERM))
    {
        result = COM_UTIL_ERR_PERMISSION_DENIED;
    }
    else if (errno_value == ETIMEDOUT)
    {
        result = COM_UTIL_ERR_TIMEOUT;
    }
    else if ((errno_value == EBUSY) || (errno_value == EAGAIN))
    {
        result = COM_UTIL_ERR_BUSY;
    }
    else if (errno_value == ENOMEM)
    {
        result = COM_UTIL_ERR_OUT_OF_MEMORY;
    }
    else
    {
        result = COM_UTIL_ERR_UNKNOWN;
    }

    return result;
}

#if defined(PLATFORM_WINDOWS)
/* Doxygen コメントは、ヘッダーに記載 */

int com_util_result_from_windows_error(const unsigned long error_code)
{
    int result;

    if (error_code == ERROR_INVALID_PARAMETER)
    {
        result = COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    else if ((error_code == ERROR_ACCESS_DENIED) || (error_code == ERROR_PRIVILEGE_NOT_HELD))
    {
        result = COM_UTIL_ERR_PERMISSION_DENIED;
    }
    else if ((error_code == WAIT_TIMEOUT) || (error_code == ERROR_TIMEOUT))
    {
        result = COM_UTIL_ERR_TIMEOUT;
    }
    else if (error_code == ERROR_BUSY)
    {
        result = COM_UTIL_ERR_BUSY;
    }
    else if ((error_code == ERROR_NOT_ENOUGH_MEMORY) || (error_code == ERROR_OUTOFMEMORY))
    {
        result = COM_UTIL_ERR_OUT_OF_MEMORY;
    }
    else if (error_code == ERROR_INSUFFICIENT_BUFFER)
    {
        result = COM_UTIL_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        result = COM_UTIL_ERR_UNKNOWN;
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_errno_from_windows_error(const unsigned long error_code)
{
    int errno_value;

    if (error_code == ERROR_INVALID_PARAMETER)
    {
        errno_value = EINVAL;
    }
    else if ((error_code == ERROR_ACCESS_DENIED) || (error_code == ERROR_PRIVILEGE_NOT_HELD))
    {
        errno_value = EACCES;
    }
    else if ((error_code == ERROR_FILE_NOT_FOUND) || (error_code == ERROR_PATH_NOT_FOUND))
    {
        errno_value = ENOENT;
    }
    else if (error_code == ERROR_FILE_EXISTS || error_code == ERROR_ALREADY_EXISTS)
    {
        errno_value = EEXIST;
    }
    else if ((error_code == ERROR_NOT_ENOUGH_MEMORY) || (error_code == ERROR_OUTOFMEMORY))
    {
        errno_value = ENOMEM;
    }
    else if ((error_code == ERROR_INSUFFICIENT_BUFFER) || (error_code == ERROR_BUFFER_OVERFLOW))
    {
        errno_value = ENAMETOOLONG;
    }
    else if (error_code == ERROR_BUSY)
    {
        errno_value = EBUSY;
    }
    else if ((error_code == WAIT_TIMEOUT) || (error_code == ERROR_TIMEOUT))
    {
        errno_value = ETIMEDOUT;
    }
    else
    {
        errno_value = EIO;
    }

    return errno_value;
}
#endif
