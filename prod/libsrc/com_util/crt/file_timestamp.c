/*
 *  glibc は struct stat の st_mtim と、utimensat および AT_FDCWD を __USE_XOPEN2K8 の
 *  内側に置く。makefw の既定である -std=c17 (厳密 ISO) では __STRICT_ANSI__ が定義され、
 *  _DEFAULT_SOURCE が自動定義されないため、これらが宣言されない。
 *  サブ秒の最終更新日時を扱うために本ファイルでのみ機能テスト マクロを定義する。
 *  see: prod/libsrc/com_util/runtime/memory_lock.c の同種の定義
 */
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif /* _GNU_SOURCE */

/**
 *******************************************************************************
 *  @file           file_timestamp.c
 *  @brief          ファイルの最終更新日時を取得および設定する API を実装します。
 *
 *  ハンドルを対象とする版とパスを対象とする版を、OS 非依存のインターフェースで提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/file.h>
#include <com_util/crt/path.h>

#include <com_util/base/error_internal.h>
#include <com_util/base/result.h>
#include <com_util/clock/timespec.h>

#include <errno.h>
#include <stddef.h>

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <time.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/clock/filetime_conv.h>
    #include <com_util/crt/wchar_conv.h>
#endif /* PLATFORM_ */

static int file_is_open(const com_util_file *file)
{
    if (file == NULL)
    {
        return 0;
    }

#if defined(PLATFORM_LINUX)
    return file->handle != -1;
#elif defined(PLATFORM_WINDOWS)
    return file->handle != INVALID_HANDLE_VALUE;
#endif /* PLATFORM_ */
}

#if defined(PLATFORM_LINUX)

/*
 *  utimensat と futimens へ渡す時刻の組を組み立てる。
 *  最終アクセス日時は UTIME_OMIT で据え置き、最終更新日時だけを設定する。
 */
static void build_times(const com_util_timespec *timestamp, struct timespec *times)
{
    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_OMIT;
    com_util_timespec_to_native(timestamp, &times[1]);
}

/* struct stat の最終更新日時を com_util_timespec へ写す。 */
static void stat_to_timestamp(const struct stat *file_stat, com_util_timespec *timestamp_out)
{
    com_util_timespec_from_native(&file_stat->st_mtim, timestamp_out);
}

#elif defined(PLATFORM_WINDOWS)

/*
 *  属性アクセスだけを要求してパスを一時的に開く。
 *  他プロセスの読み書きと削除を妨げない共有モードを指定する。
 */
static HANDLE open_for_attributes(const char *path, DWORD desired_access, com_util_error *detail_out, int *failed_out)
{
    wchar_t wpath[PLATFORM_PATH_MAX];
    HANDLE handle;

    *failed_out = 0;

    if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
    {
        *failed_out = com_util_error_report_errno(detail_out, ENAMETOOLONG);
        return INVALID_HANDLE_VALUE;
    }

    /* wpath は本関数内ですでに UTF-8 から変換済みのため、CreateFileU を経由しない */
    handle = CreateFileW(wpath, desired_access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        *failed_out = com_util_error_report_windows_error(detail_out, GetLastError());
    }

    return handle;
}

#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_get_modified_timestamp(const com_util_file *file, com_util_timespec *timestamp_out,
                                         com_util_error *detail_out)
{
    if (!file_is_open(file) || timestamp_out == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

#if defined(PLATFORM_LINUX)
    {
        struct stat st;

        if (fstat(file->handle, &st) != 0)
        {
            return com_util_error_report_errno(detail_out, errno);
        }

        stat_to_timestamp(&st, timestamp_out);
        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        FILETIME write_time;

        if (!GetFileTime(file->handle, NULL, NULL, &write_time))
        {
            return com_util_error_report_windows_error(detail_out, GetLastError());
        }

        com_util_internal_filetime_to_timespec(&write_time, timestamp_out);
        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_set_modified_timestamp(com_util_file *file, const com_util_timespec *timestamp,
                                         com_util_error *detail_out)
{
    if (!file_is_open(file) || timestamp == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

    /*
     *  両プラットフォームで挙動をそろえるため、書き込みアクセスを要求する。
     *  Linux の futimens は読み取り専用の記述子でも所有者なら成功するため、
     *  厳しい側の Windows に合わせてここで失敗させる。
     */
    if (file->writable == 0)
    {
        return com_util_error_report_errno_as(detail_out, EACCES, COM_UTIL_ERR_PERMISSION_DENIED);
    }

#if defined(PLATFORM_LINUX)
    {
        struct timespec times[2];

        build_times(timestamp, times);

        if (futimens(file->handle, times) != 0)
        {
            return com_util_error_report_errno(detail_out, errno);
        }

        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        FILETIME write_time;

        com_util_internal_timespec_to_filetime(timestamp, &write_time);

        if (!SetFileTime(file->handle, NULL, NULL, &write_time))
        {
            return com_util_error_report_windows_error(detail_out, GetLastError());
        }

        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_get_path_modified_timestamp(const char *path, com_util_timespec *timestamp_out,
                                              com_util_error *detail_out)
{
    if (path == NULL || timestamp_out == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

#if defined(PLATFORM_LINUX)
    {
        struct stat st;

        if (stat(path, &st) != 0)
        {
            return com_util_error_report_errno(detail_out, errno);
        }

        stat_to_timestamp(&st, timestamp_out);
        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        HANDLE handle;
        FILETIME write_time;
        BOOL got_time;
        int failed = 0;

        handle = open_for_attributes(path, FILE_READ_ATTRIBUTES, detail_out, &failed);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return failed;
        }

        got_time = GetFileTime(handle, NULL, NULL, &write_time);
        if (!got_time)
        {
            const DWORD error_code = GetLastError();

            (void)CloseHandle(handle);
            return com_util_error_report_windows_error(detail_out, error_code);
        }
        if (!CloseHandle(handle))
        {
            return com_util_error_report_windows_error(detail_out, GetLastError());
        }

        com_util_internal_filetime_to_timespec(&write_time, timestamp_out);
        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_set_path_modified_timestamp(const char *path, const com_util_timespec *timestamp,
                                              com_util_error *detail_out)
{
    if (path == NULL || timestamp == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

#if defined(PLATFORM_LINUX)
    {
        struct timespec times[2];

        build_times(timestamp, times);

        if (utimensat(AT_FDCWD, path, times, 0) != 0)
        {
            return com_util_error_report_errno(detail_out, errno);
        }

        return com_util_error_report_success(detail_out);
    }
#elif defined(PLATFORM_WINDOWS)
    {
        HANDLE handle;
        FILETIME write_time;
        BOOL set_time;
        int failed = 0;

        handle = open_for_attributes(path, FILE_WRITE_ATTRIBUTES, detail_out, &failed);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return failed;
        }

        com_util_internal_timespec_to_filetime(timestamp, &write_time);

        set_time = SetFileTime(handle, NULL, NULL, &write_time);
        if (!set_time)
        {
            const DWORD error_code = GetLastError();

            (void)CloseHandle(handle);
            return com_util_error_report_windows_error(detail_out, error_code);
        }
        if (!CloseHandle(handle))
        {
            return com_util_error_report_windows_error(detail_out, GetLastError());
        }

        return com_util_error_report_success(detail_out);
    }
#endif /* PLATFORM_ */
}
