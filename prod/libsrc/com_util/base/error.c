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

/* EAI_* の定数を参照するために取り込む。Linux の EAI_* は errno とも Winsock とも
   異なる番号体系であり、COM_UTIL_ERROR_DOMAIN_GAI の解釈に必要となる。
   Windows の EAI_* は Winsock エラー コードと同一値であるため windows_sdk.h で足りる。 */
#if defined(PLATFORM_LINUX)
    #include <netdb.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif /* PLATFORM_ */

/* TLS 実体はヘッダーへ公開せず、エクスポート関数経由でのみ参照する。 */
static THREAD_LOCAL com_util_error com_util_error_last;

/**
 *  @brief          詳細エラーへ各フィールドを格納します。
 *  @param[out]     error  格納先。NULL 可。
 *  @param[in]      domain エラー値のドメイン。
 *  @param[in]      result 共通結果コード。
 *  @param[in]      code   ドメイン固有のエラー値。
 */
static void error_store(com_util_error *error, const com_util_error_domain domain, const int result,
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
static com_util_error_cause error_cause_from_errno(const int errno_value)
{
    com_util_error_cause cause;

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
#if defined(EINPROGRESS)
    case EINPROGRESS:
        cause = COM_UTIL_CAUSE_IN_PROGRESS;
        break;
#endif
#if defined(ECONNREFUSED)
    case ECONNREFUSED:
        cause = COM_UTIL_CAUSE_CONNECTION_REFUSED;
        break;
#endif
#if defined(ECONNRESET)
    case ECONNRESET:
        cause = COM_UTIL_CAUSE_CONNECTION_RESET;
        break;
#endif
#if defined(ECONNABORTED)
    case ECONNABORTED:
        cause = COM_UTIL_CAUSE_CONNECTION_ABORTED;
        break;
#endif
#if defined(ENOTCONN)
    case ENOTCONN:
        cause = COM_UTIL_CAUSE_NOT_CONNECTED;
        break;
#endif
#if defined(EISCONN)
    case EISCONN:
        cause = COM_UTIL_CAUSE_ALREADY_CONNECTED;
        break;
#endif
#if defined(EADDRINUSE)
    case EADDRINUSE:
        cause = COM_UTIL_CAUSE_ADDRESS_IN_USE;
        break;
#endif
#if defined(EADDRNOTAVAIL)
    case EADDRNOTAVAIL:
        cause = COM_UTIL_CAUSE_ADDRESS_NOT_AVAILABLE;
        break;
#endif
#if defined(ENETDOWN)
    case ENETDOWN:
        cause = COM_UTIL_CAUSE_NETWORK_DOWN;
        break;
#endif
#if defined(ENETUNREACH)
    case ENETUNREACH:
        cause = COM_UTIL_CAUSE_NETWORK_UNREACHABLE;
        break;
#endif
#if defined(EHOSTUNREACH)
    case EHOSTUNREACH:
        cause = COM_UTIL_CAUSE_HOST_UNREACHABLE;
        break;
#endif
#if defined(EMSGSIZE)
    case EMSGSIZE:
        cause = COM_UTIL_CAUSE_MESSAGE_SIZE;
        break;
#endif
#if defined(ESHUTDOWN)
    case ESHUTDOWN:
        cause = COM_UTIL_CAUSE_SHUTDOWN;
        break;
#endif
    default:
        cause = COM_UTIL_CAUSE_OTHER;
        break;
    }

    return cause;
}

/**
 *  @brief          ソケット操作の errno をプラットフォーム共通の要因へ変換します。
 *  @param[in]      errno_value errno の値。
 *  @return         対応する要因を返します。
 *
 *  ソケット操作の EAGAIN は非ブロッキング操作の待機を意味するため、共通の
 *  errno マッピングとは別に @ref COM_UTIL_CAUSE_WOULD_BLOCK として扱います。\n
 *  Linux では EWOULDBLOCK が EAGAIN と同値です。\n
 *  それ以外の errno は error_cause_from_errno() の分類に委譲します。
 *
 *  fork() や pthread_create() が返す EAGAIN は資源の上限超過を意味し、待機とは
 *  異なるため、共通の errno マッピングでは @ref COM_UTIL_CAUSE_BUSY のままとします。
 */
static com_util_error_cause error_cause_from_socket_errno(const int errno_value)
{
    com_util_error_cause cause;

    if (errno_value == EAGAIN)
    {
        cause = COM_UTIL_CAUSE_WOULD_BLOCK;
    }
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
    else if (errno_value == EWOULDBLOCK)
    {
        cause = COM_UTIL_CAUSE_WOULD_BLOCK;
    }
#endif
    else
    {
        cause = error_cause_from_errno(errno_value);
    }

    return cause;
}

/**
 *  @brief          getaddrinfo のエラー コードをプラットフォーム共通の要因へ変換します。
 *  @param[in]      error_code getaddrinfo が返した EAI_* の値。
 *  @return         対応する要因を返します。
 *
 *  EAI_* は errno とも Winsock エラーとも異なる番号体系のため、専用の変換を行います。
 *  see: https://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html
 */
static com_util_error_cause error_cause_from_gai_error(const int error_code)
{
    com_util_error_cause cause;

    switch (error_code)
    {
#if defined(EAI_NONAME)
    case EAI_NONAME:
#endif
#if defined(EAI_NODATA) && (!defined(EAI_NONAME) || (EAI_NODATA != EAI_NONAME))
    case EAI_NODATA:
#endif
        cause = COM_UTIL_CAUSE_NOT_FOUND;
        break;
#if defined(EAI_AGAIN)
    case EAI_AGAIN:
        cause = COM_UTIL_CAUSE_BUSY;
        break;
#endif
#if defined(EAI_MEMORY)
    case EAI_MEMORY:
        cause = COM_UTIL_CAUSE_OUT_OF_MEMORY;
        break;
#endif
#if defined(EAI_FAMILY)
    case EAI_FAMILY:
#endif
#if defined(EAI_SOCKTYPE)
    case EAI_SOCKTYPE:
#endif
#if defined(EAI_SERVICE)
    case EAI_SERVICE:
#endif
        cause = COM_UTIL_CAUSE_UNSUPPORTED;
        break;
#if defined(EAI_BADFLAGS)
    case EAI_BADFLAGS:
        cause = COM_UTIL_CAUSE_INVALID_ARGUMENT;
        break;
#endif
#if defined(EAI_SYSTEM)
    case EAI_SYSTEM:
        /* 要因は EAI_* ではなく errno 側にある。名前解決はソケット操作ではないため、
           一般の errno として分類する。 */
        cause = error_cause_from_errno(errno);
        break;
#endif
    default:
        cause = COM_UTIL_CAUSE_OTHER;
        break;
    }

    return cause;
}

#if defined(PLATFORM_WINDOWS)
/**
 *  @brief          Winsock エラーをプラットフォーム共通の要因へ変換します。
 *  @param[in]      error_code WSAGetLastError() が返した値。
 *  @return         対応する要因を返します。
 *
 *  Winsock のエラー番号空間は Win32 の GetLastError() と異なるため、
 *  error_cause_from_windows_error() では分類できません。
 *  see: https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
 */
static com_util_error_cause error_cause_from_winsock_error(const unsigned long error_code)
{
    com_util_error_cause cause;

    switch (error_code)
    {
    case WSAEWOULDBLOCK:
        cause = COM_UTIL_CAUSE_WOULD_BLOCK;
        break;
    case WSAEINPROGRESS:
    case WSAEALREADY:
        cause = COM_UTIL_CAUSE_IN_PROGRESS;
        break;
    case WSAEINTR:
        cause = COM_UTIL_CAUSE_INTERRUPTED;
        break;
    case WSAECONNREFUSED:
        cause = COM_UTIL_CAUSE_CONNECTION_REFUSED;
        break;
    case WSAECONNRESET:
        cause = COM_UTIL_CAUSE_CONNECTION_RESET;
        break;
    case WSAECONNABORTED:
        cause = COM_UTIL_CAUSE_CONNECTION_ABORTED;
        break;
    case WSAENOTCONN:
        cause = COM_UTIL_CAUSE_NOT_CONNECTED;
        break;
    case WSAEISCONN:
        cause = COM_UTIL_CAUSE_ALREADY_CONNECTED;
        break;
    case WSAEADDRINUSE:
        cause = COM_UTIL_CAUSE_ADDRESS_IN_USE;
        break;
    case WSAEADDRNOTAVAIL:
        cause = COM_UTIL_CAUSE_ADDRESS_NOT_AVAILABLE;
        break;
    case WSAENETDOWN:
        cause = COM_UTIL_CAUSE_NETWORK_DOWN;
        break;
    case WSAENETUNREACH:
        cause = COM_UTIL_CAUSE_NETWORK_UNREACHABLE;
        break;
    case WSAEHOSTUNREACH:
        cause = COM_UTIL_CAUSE_HOST_UNREACHABLE;
        break;
    case WSAEMSGSIZE:
        cause = COM_UTIL_CAUSE_MESSAGE_SIZE;
        break;
    case WSAESHUTDOWN:
        cause = COM_UTIL_CAUSE_SHUTDOWN;
        break;
    case WSANOTINITIALISED:
        cause = COM_UTIL_CAUSE_NOT_INITIALIZED;
        break;
    case WSAETIMEDOUT:
        cause = COM_UTIL_CAUSE_TIMEOUT;
        break;
    case WSAEINVAL:
        cause = COM_UTIL_CAUSE_INVALID_ARGUMENT;
        break;
    case WSAEACCES:
        cause = COM_UTIL_CAUSE_ACCESS_DENIED;
        break;
    case WSAEMFILE:
        cause = COM_UTIL_CAUSE_TOO_MANY_OPEN_FILES;
        break;
    case WSAENOBUFS:
        cause = COM_UTIL_CAUSE_OUT_OF_MEMORY;
        break;
    case WSAEOPNOTSUPP:
    case WSAEAFNOSUPPORT:
    case WSAEPROTONOSUPPORT:
        cause = COM_UTIL_CAUSE_UNSUPPORTED;
        break;
    default:
        cause = COM_UTIL_CAUSE_OTHER;
        break;
    }

    return cause;
}

/**
 *  @brief          Win32 エラーをプラットフォーム共通の要因へ変換します。
 *  @param[in]      error_code Win32 エラー コード。
 *  @return         対応する要因を返します。
 */
static com_util_error_cause error_cause_from_windows_error(const unsigned long error_code)
{
    com_util_error_cause cause;

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
    error_store(error, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_capture_errno(com_util_error *error, const int errno_value)
{
    if (errno_value == 0)
    {
        error_store(error, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    }
    else
    {
        error_store(error, COM_UTIL_ERROR_DOMAIN_ERRNO, com_util_result_from_errno(errno_value),
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
        error_store(error, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    }
    else
    {
        error_store(error, COM_UTIL_ERROR_DOMAIN_WINDOWS, com_util_result_from_windows_error(error_code),
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

void com_util_error_set_last(const com_util_error *error)
{
    if (error == NULL)
    {
        error_store(&com_util_error_last, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    }
    else
    {
        com_util_error_last = *error;
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_error_clear_last(void)
{
    error_store(&com_util_error_last, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_is_set(const com_util_error *error)
{
    int is_set = 0;

    if (error != NULL)
    {
        if (error->domain != COM_UTIL_ERROR_DOMAIN_NONE)
        {
            is_set = 1;
        }
    }

    return is_set;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_error_domain com_util_error_get_domain(const com_util_error *error)
{
    com_util_error_domain domain = COM_UTIL_ERROR_DOMAIN_NONE;

    if (error != NULL)
    {
        if ((error->domain == COM_UTIL_ERROR_DOMAIN_NONE) || (error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO) ||
            (error->domain == COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO) ||
            (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS) || (error->domain == COM_UTIL_ERROR_DOMAIN_WINSOCK) ||
            (error->domain == COM_UTIL_ERROR_DOMAIN_GAI))
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
        else
        {
            result = error->result;
        }
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_error_cause com_util_error_get_cause(const com_util_error *error)
{
    com_util_error_cause cause = COM_UTIL_CAUSE_NONE;

    if (error != NULL)
    {
        if (error->domain == COM_UTIL_ERROR_DOMAIN_ERRNO)
        {
            cause = error_cause_from_errno((int)error->code);
        }
        else if (error->domain == COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO)
        {
            cause = error_cause_from_socket_errno((int)error->code);
        }
        else if (error->domain == COM_UTIL_ERROR_DOMAIN_GAI)
        {
            cause = error_cause_from_gai_error((int)error->code);
        }
        else if (error->domain == COM_UTIL_ERROR_DOMAIN_WINDOWS)
        {
#if defined(PLATFORM_WINDOWS)
            cause = error_cause_from_windows_error(error->code);
#else
            cause = COM_UTIL_CAUSE_OTHER;
#endif
        }
        else if (error->domain == COM_UTIL_ERROR_DOMAIN_WINSOCK)
        {
#if defined(PLATFORM_WINDOWS)
            cause = error_cause_from_winsock_error(error->code);
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

int com_util_error_is(const com_util_error *error, const com_util_error_cause cause)
{
    int matches = 0;

    /* 引数の要因が有効な列挙値であるかは検査しない。com_util_error_get_cause() は
       定義済みの要因しか返さないため、未定義の値を指定しても一致しない。
       上限を列挙の末尾と比較する形にすると、要因を追加するたびに更新が必要になり、
       更新漏れがあると追加した要因の判定が常に不一致になる。 */
    if ((error != NULL) && (com_util_error_get_cause(error) == cause))
    {
        matches = 1;
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
    com_util_error_domain domain = COM_UTIL_ERROR_DOMAIN_ERRNO;

    if ((errno_value == 0) && (result == COM_UTIL_OK))
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
    }

    error_store(detail_out, domain, result, (unsigned long)errno_value);
    error_store(&com_util_error_last, domain, result, (unsigned long)errno_value);

    return result;
}

#if defined(PLATFORM_WINDOWS)
/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_winsock_error(com_util_error *detail_out, const unsigned long error_code)
{
    int result;

    if (error_code == 0UL)
    {
        result = COM_UTIL_OK;
    }
    else
    {
        result = com_util_result_from_winsock_error(error_code);
    }

    return com_util_error_report_winsock_error_as(detail_out, error_code, result);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_winsock_error_as(com_util_error *detail_out, const unsigned long error_code, const int result)
{
    com_util_error_domain domain = COM_UTIL_ERROR_DOMAIN_WINSOCK;

    if ((error_code == 0UL) && (result == COM_UTIL_OK))
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
    }

    error_store(detail_out, domain, result, error_code);
    error_store(&com_util_error_last, domain, result, error_code);

    return result;
}
#endif /* PLATFORM_WINDOWS */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_socket_errno(com_util_error *detail_out, const int errno_value)
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

    return com_util_error_report_socket_errno_as(detail_out, errno_value, result);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_socket_errno_as(com_util_error *detail_out, const int errno_value, const int result)
{
    com_util_error_domain domain = COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO;

    if ((errno_value == 0) && (result == COM_UTIL_OK))
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
    }

    error_store(detail_out, domain, result, (unsigned long)errno_value);
    error_store(&com_util_error_last, domain, result, (unsigned long)errno_value);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_gai_error(com_util_error *detail_out, const int error_code)
{
    com_util_error_domain domain = COM_UTIL_ERROR_DOMAIN_GAI;
    int result;

    if (error_code == 0)
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
        result = COM_UTIL_OK;
    }
    else if (error_cause_from_gai_error(error_code) == COM_UTIL_CAUSE_NOT_FOUND)
    {
        result = COM_UTIL_ERR_NOT_FOUND;
    }
    else
    {
        result = COM_UTIL_ERR_UNKNOWN;
    }

    error_store(detail_out, domain, result, (unsigned long)error_code);
    error_store(&com_util_error_last, domain, result, (unsigned long)error_code);

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
    com_util_error_domain domain = COM_UTIL_ERROR_DOMAIN_WINDOWS;

    if ((error_code == ERROR_SUCCESS) && (result == COM_UTIL_OK))
    {
        domain = COM_UTIL_ERROR_DOMAIN_NONE;
    }

    error_store(detail_out, domain, result, error_code);
    error_store(&com_util_error_last, domain, result, error_code);

    return result;
}
#endif

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_error_report_success(com_util_error *detail_out)
{
    error_store(detail_out, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);
    error_store(&com_util_error_last, COM_UTIL_ERROR_DOMAIN_NONE, COM_UTIL_OK, 0UL);

    return COM_UTIL_OK;
}
