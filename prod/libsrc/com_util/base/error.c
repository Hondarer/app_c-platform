/**
 *******************************************************************************
 *  @file           error.c
 *  @brief          OS エラーの取り込み、TLS 記録、要因判定を提供します。
 *
 *  TLS 実体を本ファイルに限定し、共有ライブラリ境界を越える参照は公開 API
 *  による値のコピーだけに制限します。
 *
 *******************************************************************************
 */

#include <com_util/base/error_internal.h>
#include <com_util/base/compiler.h>
#include <com_util/base/result_internal.h>

#include <errno.h>
#include <stddef.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif

/* TLS 実体はヘッダーへ公開せず、エクスポート関数経由でのみ参照する。 */
static THREAD_LOCAL com_util_error com_util_error_last;

/**
 *  @brief          詳細エラーへ各フィールドを格納します。
 *  @param[out]     error  格納先。NULL 可。
 *  @param[in]      domain エラー値のドメイン。
 *  @param[in]      result 共通結果コード。
 *  @param[in]      code   ドメイン固有のエラー値。
 */
static void com_util_error_store(com_util_error *error, const com_util_error_domain_t domain, const int result,
                                 const unsigned long code)
{
    if (error != NULL)
    {
        error->domain = domain;
        error->result = result;
        error->code = code;
    }
}

/**
 *  @brief          errno をプラットフォーム共通の要因へ変換します。
 *  @param[in]      errno_value errno の値。
 *  @return         対応する要因を返します。
 */
static com_util_error_cause_t com_util_error_cause_from_errno(const int errno_value)
{
    com_util_error_cause_t cause;

    switch (errno_value)
    {
    case ENOENT:
#if defined(ENODEV)
    case ENODEV:
#endif
#if defined(ENXIO)
    case ENXIO:
#endif
        cause = COM_UTIL_CAUSE_NOT_FOUND;
        break;
    case EEXIST:
        cause = COM_UTIL_CAUSE_ALREADY_EXISTS;
        break;
    case EACCES:
    case EPERM:
        cause = COM_UTIL_CAUSE_ACCESS_DENIED;
        break;
    case ENOTDIR:
        cause = COM_UTIL_CAUSE_NOT_A_DIRECTORY;
        break;
    case EISDIR:
        cause = COM_UTIL_CAUSE_IS_A_DIRECTORY;
        break;
    case ENOTEMPTY:
        cause = COM_UTIL_CAUSE_DIRECTORY_NOT_EMPTY;
        break;
    case ENAMETOOLONG:
        cause = COM_UTIL_CAUSE_NAME_TOO_LONG;
        break;
    case EINVAL:
        cause = COM_UTIL_CAUSE_INVALID_ARGUMENT;
        break;
    case ENOMEM:
        cause = COM_UTIL_CAUSE_OUT_OF_MEMORY;
        break;
    case ENOSPC:
#if defined(EDQUOT)
    case EDQUOT:
#endif
        cause = COM_UTIL_CAUSE_DISK_FULL;
        break;
    case EBUSY:
    case EAGAIN:
#if defined(ETXTBSY)
    case ETXTBSY:
#endif
        cause = COM_UTIL_CAUSE_BUSY;
        break;
    case ETIMEDOUT:
        cause = COM_UTIL_CAUSE_TIMEOUT;
        break;
    case EINTR:
        cause = COM_UTIL_CAUSE_INTERRUPTED;
        break;
    case EPIPE:
        cause = COM_UTIL_CAUSE_BROKEN_PIPE;
        break;
    case EMFILE:
    case ENFILE:
        cause = COM_UTIL_CAUSE_TOO_MANY_OPEN_FILES;
        break;
    case EROFS:
        cause = COM_UTIL_CAUSE_READ_ONLY;
        break;
    case ERANGE:
        cause = COM_UTIL_CAUSE_BUFFER_TOO_SMALL;
        break;
#if defined(ENOTSUP)
    case ENOTSUP:
        cause = COM_UTIL_CAUSE_UNSUPPORTED;
        break;
#endif
#if defined(EOPNOTSUPP) && (!defined(ENOTSUP) || (EOPNOTSUPP != ENOTSUP))
    case EOPNOTSUPP:
        cause = COM_UTIL_CAUSE_UNSUPPORTED;
        break;
#endif
#if defined(ENOSYS)
    case ENOSYS:
        cause = COM_UTIL_CAUSE_UNSUPPORTED;
        break;
#endif
    case EIO:
        cause = COM_UTIL_CAUSE_IO_ERROR;
        break;
    default:
        cause = COM_UTIL_CAUSE_OTHER;
        break;
    }

    return cause;
}

#if defined(PLATFORM_WINDOWS)
/**
 *  @brief          Win32 エラーをプラットフォーム共通の要因へ変換します。
 *  @param[in]      error_code Win32 エラー コード。
 *  @return         対応する要因を返します。
 */
static com_util_error_cause_t com_util_error_cause_from_windows_error(const unsigned long error_code)
{
    com_util_error_cause_t cause;

    switch (error_code)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_BAD_NETPATH:
    case ERROR_INVALID_DRIVE:
    case ERROR_SERVICE_DOES_NOT_EXIST:
        cause = COM_UTIL_CAUSE_NOT_FOUND;
        break;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
    case ERROR_SERVICE_EXISTS:
        cause = COM_UTIL_CAUSE_ALREADY_EXISTS;
        break;
    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
        cause = COM_UTIL_CAUSE_ACCESS_DENIED;
        break;
    case ERROR_SHARING_VIOLATION:
        cause = COM_UTIL_CAUSE_SHARING_VIOLATION;
        break;
    case ERROR_DIRECTORY:
        cause = COM_UTIL_CAUSE_NOT_A_DIRECTORY;
        break;
    case ERROR_DIR_NOT_EMPTY:
        cause = COM_UTIL_CAUSE_DIRECTORY_NOT_EMPTY;
        break;
    case ERROR_FILENAME_EXCED_RANGE:
    case ERROR_BUFFER_OVERFLOW:
        cause = COM_UTIL_CAUSE_NAME_TOO_LONG;
        break;
    case ERROR_INVALID_PARAMETER:
        cause = COM_UTIL_CAUSE_INVALID_ARGUMENT;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
        cause = COM_UTIL_CAUSE_OUT_OF_MEMORY;
        break;
    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL:
        cause = COM_UTIL_CAUSE_DISK_FULL;
        break;
    case ERROR_BUSY:
    case ERROR_SERVICE_ALREADY_RUNNING:
    case ERROR_SERVICE_MARKED_FOR_DELETE:
    case ERROR_DEPENDENT_SERVICES_RUNNING:
    case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
        cause = COM_UTIL_CAUSE_BUSY;
        break;
    case WAIT_TIMEOUT:
    case ERROR_TIMEOUT:
    case ERROR_SERVICE_REQUEST_TIMEOUT:
        cause = COM_UTIL_CAUSE_TIMEOUT;
        break;
    case ERROR_OPERATION_ABORTED:
        cause = COM_UTIL_CAUSE_INTERRUPTED;
        break;
    case ERROR_BROKEN_PIPE:
        cause = COM_UTIL_CAUSE_BROKEN_PIPE;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
        cause = COM_UTIL_CAUSE_TOO_MANY_OPEN_FILES;
        break;
    case ERROR_WRITE_PROTECT:
        cause = COM_UTIL_CAUSE_READ_ONLY;
        break;
    case ERROR_INSUFFICIENT_BUFFER:
        cause = COM_UTIL_CAUSE_BUFFER_TOO_SMALL;
        break;
    case ERROR_NOT_SUPPORTED:
    case ERROR_CALL_NOT_IMPLEMENTED:
        cause = COM_UTIL_CAUSE_UNSUPPORTED;
        break;
    case ERROR_IO_DEVICE:
        cause = COM_UTIL_CAUSE_IO_ERROR;
        break;
    default:
        cause = COM_UTIL_CAUSE_OTHER;
        break;
    }

    return cause;
}
#endif

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_clear(com_util_error *error)
{
    com_util_error_store(error, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_capture_errno(com_util_error *error, const int errno_value)
{
    if (errno_value == 0)
    {
        com_util_error_store(error, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    }
    else
    {
        com_util_error_store(error, COM_UTIL_ERROR_DOMAIN_ERRNO, com_util_result_from_errno(errno_value),
                             (unsigned long)errno_value);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_capture_current_errno(com_util_error *error)
{
    const int errno_value = errno;

    com_util_error_capture_errno(error, errno_value);
}

#if defined(PLATFORM_WINDOWS)
/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_capture_windows_error(com_util_error *error, const unsigned long error_code)
{
    if (error_code == ERROR_SUCCESS)
    {
        com_util_error_store(error, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    }
    else
    {
        com_util_error_store(error, COM_UTIL_ERROR_DOMAIN_WINDOWS, com_util_result_from_windows_error(error_code),
                             error_code);
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_capture_current_windows_error(com_util_error *error)
{
    const DWORD error_code = GetLastError();

    com_util_error_capture_windows_error(error, error_code);
}
#endif

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_get_last(com_util_error *error_out)
{
    if (error_out != NULL)
    {
        *error_out = com_util_error_last;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_clear_last(void)
{
    com_util_error_store(&com_util_error_last, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_is_set(const com_util_error *error)
{
    int is_set = 0;

    if (error != NULL)
    {
        if ((error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO) || (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS))
        {
            is_set = 1;
        }
    }

    return is_set;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_error_domain_t com_util_error_get_domain(const com_util_error *error)
{
    com_util_error_domain_t domain = COM_UTIL_ERROR_DOMAIN_NONE;

    if (error != NULL)
    {
        if ((error->domain == COM_UTIL_ERROR_DOMAIN_NONE) || (error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO) ||
            (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS))
        {
            domain = error->domain;
        }
    }

    return domain;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_get_errno(const com_util_error *error)
{
    int errno_value = 0;

    if ((error != NULL) && (error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO))
    {
        errno_value = (int)error->code;
    }

    return errno_value;
}

#if defined(PLATFORM_WINDOWS)
/* Doxygen コメントは、ヘッダーに記載 */

unsigned long com_util_error_get_windows_error(const com_util_error *error)
{
    unsigned long error_code = ERROR_SUCCESS;

    if ((error != NULL) && (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS))
    {
        error_code = error->code;
    }

    return error_code;
}
#endif

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_to_result(const com_util_error *error)
{
    int result = COM_UTIL_ERR_INVALID_ARGUMENT;

    if (error != NULL)
    {
        if (error->domain == COM_UTIL_ERROR_DOMAIN_NONE)
        {
            result = COM_UTIL_OK;
        }
        else if ((error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO) || (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS))
        {
            result = error->result;
        }
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_error_cause_t com_util_error_get_cause(const com_util_error *error)
{
    com_util_error_cause_t cause = COM_UTIL_CAUSE_NONE;

    if (error != NULL)
    {
        if (error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO)
        {
            cause = com_util_error_cause_from_errno((int)error->code);
        }
        else if (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS)
        {
#if defined(PLATFORM_WINDOWS)
            cause = com_util_error_cause_from_windows_error(error->code);
#else
            cause = COM_UTIL_CAUSE_OTHER;
#endif
        }
        else if (error->domain != COM_UTIL_ERROR_DOMAIN_NONE)
        {
            cause = COM_UTIL_CAUSE_OTHER;
        }
    }

    return cause;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_is(const com_util_error *error, const com_util_error_cause_t cause)
{
    int matches = 0;

    if ((error != NULL) && (cause >= COM_UTIL_CAUSE_NONE) && (cause <= COM_UTIL_CAUSE_IO_ERROR))
    {
        if (com_util_error_get_cause(error) == cause)
        {
            matches = 1;
        }
    }

    return matches;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_errno(com_util_error *detail_out, const int errno_value)
{
    int result;

    if (errno_value == 0)
    {
        result = COM_UTIL_OK;
    }
    else
    {
        result = com_util_result_from_errno(errno_value);
    }

    return com_util_error_report_errno_as(detail_out, errno_value, result);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_errno_as(com_util_error *detail_out, const int errno_value, const int result)
{
    com_util_error_domain_t domain = COM_UTIL_ERROR_DOMAIN_ERRNO;

    if ((errno_value == 0) && (result == COM_UTIL_OK))
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
    }

    com_util_error_store(detail_out, domain, result, (unsigned long)errno_value);
    com_util_error_store(&com_util_error_last, domain, result, (unsigned long)errno_value);

    return result;
}

#if defined(PLATFORM_WINDOWS)
/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_windows_error(com_util_error *detail_out, const unsigned long error_code)
{
    int result;

    if (error_code == ERROR_SUCCESS)
    {
        result = COM_UTIL_OK;
    }
    else
    {
        result = com_util_result_from_windows_error(error_code);
    }

    return com_util_error_report_windows_error_as(detail_out, error_code, result);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_windows_error_as(com_util_error *detail_out, const unsigned long error_code, const int result)
{
    com_util_error_domain_t domain = COM_UTIL_ERROR_DOMAIN_WINDOWS;

    if ((error_code == ERROR_SUCCESS) && (result == COM_UTIL_OK))
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
    }

    com_util_error_store(detail_out, domain, result, error_code);
    com_util_error_store(&com_util_error_last, domain, result, error_code);

    return result;
}
#endif

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_success(com_util_error *detail_out)
{
    com_util_error_store(detail_out, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    com_util_error_store(&com_util_error_last, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);

    return COM_UTIL_OK;
}
