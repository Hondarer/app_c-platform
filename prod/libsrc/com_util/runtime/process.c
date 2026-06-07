/**
 *******************************************************************************
 *  @file           process.c
 *  @brief          プロセス情報取得 実装。
 *  @author         Tetsuo Honda
 *  @date           2026/06/07
 *  @version        1.0.0
 *
 *  現在のプロセスの実行ファイル本体の絶対パスを取得します。
 *  - Linux  : readlink("/proc/self/exe") を使用します。
 *  - Windows: GetModuleFileNameW(NULL, ...) で取得し、UTF-8 に変換します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/runtime/process.h>
#include <com_util/crt/crt_internal.h>
#include <com_util/crt/path.h>

#if defined(PLATFORM_LINUX)
    #include <sys/types.h>
    #include <unistd.h>
#endif /* PLATFORM_LINUX */

/* Doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API com_util_process_get_executable_path(char *out_path, const size_t out_path_sz)
{
    if (out_path == NULL || out_path_sz == 0)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        ssize_t len;

        len = readlink("/proc/self/exe", out_path, out_path_sz - 1);
        if (len < 0 || (size_t)len >= out_path_sz)
        {
            out_path[0] = '\0';
            return -1;
        }
        out_path[len] = '\0';
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wbuf[PLATFORM_PATH_MAX];
        DWORD n;

        n = GetModuleFileNameW(NULL, wbuf, (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])));
        if (n == 0 || n >= (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])))
        {
            out_path[0] = '\0';
            return -1;
        }
        wbuf[n] = L'\0';
        if (com_util_wpath_to_utf8(out_path, out_path_sz, wbuf) < 0)
        {
            out_path[0] = '\0';
            return -1;
        }
        return 0;
    }
#else
    out_path[0] = '\0';
    return -1;
#endif /* PLATFORM_ */
}
