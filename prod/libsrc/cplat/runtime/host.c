/**
 *******************************************************************************
 *  @file           host.c
 *  @brief          ホスト識別情報を取得する API を実装します。
 *
 *  現在のホストの DNS ホスト名を UTF-8 で取得します。
 *
 *******************************************************************************
 */

#include <cplat/runtime/host.h>

#include <cplat/base/platform.h>
#include <cplat/base/result_internal.h>
#include <cplat/crt/string.h>

#include <errno.h>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <cplat/base/windows_sdk.h>
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_get_hostname(char *name_out, const size_t name_size)
{
    if (name_out == NULL || name_size == 0)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

#if defined(PLATFORM_LINUX)
    {
        char tmp[CPLAT_HOST_NAME_MAX];

        /* gethostname はバッファー不足時に NUL 終端しない場合があり、POSIX.1-2008 では
           ENAMETOOLONG で失敗する。内部バッファーへ取得してから呼び出し側へ写す。
           see: https://man7.org/linux/man-pages/man2/gethostname.2.html */
        if (gethostname(tmp, sizeof(tmp)) != 0)
        {
            name_out[0] = '\0';
            return cplat_result_from_errno(errno);
        }
        tmp[sizeof(tmp) - 1] = '\0';
        return cplat_strcpy(name_out, name_size, tmp);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wname[CPLAT_HOST_NAME_MAX];
        DWORD wname_count = (DWORD)(sizeof(wname) / sizeof(wname[0]));
        int converted;

        /* Winsock の gethostname は WSAStartup が必要で、NetBIOS 名の 15 文字制限にも
           当たり得る。DNS ホスト名を UTF-16 で取り、UTF-8 へ変換する。
           see: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getcomputernameexw */
        if (GetComputerNameExW(ComputerNameDnsHostname, wname, &wname_count) == 0)
        {
            DWORD err = GetLastError();

            name_out[0] = '\0';
            if (err == ERROR_MORE_DATA)
            {
                return CPLAT_ERR_BUFFER_TOO_SMALL;
            }
            return cplat_result_from_windows_error(err);
        }

        converted = WideCharToMultiByte(CP_UTF8, 0, wname, -1, name_out, (int)name_size, NULL, NULL);
        if (converted <= 0)
        {
            int result = cplat_result_from_windows_error(GetLastError());

            name_out[0] = '\0';
            return result;
        }
        return CPLAT_OK;
    }
#else
    name_out[0] = '\0';
    return CPLAT_ERR_UNSUPPORTED;
#endif /* PLATFORM_ */
}
