/**
 *******************************************************************************
 *  @file           file.c
 *  @brief          低レベルのファイル I/O を抽象化する API を実装します。
 *
 *  ファイルのオープン、書き込み、同一性取得、クローズを OS 非依存のインターフェースで提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/file.h>
#include <com_util/crt/path.h>

#include <com_util/base/result.h>
#include <com_util/crt/wchar_conv.h>

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif /* PLATFORM_LINUX */

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

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_file_init(com_util_file *file)
{
    if (file == NULL)
    {
        return;
    }

#if defined(PLATFORM_LINUX)
    file->handle = -1;
#elif defined(PLATFORM_WINDOWS)
    file->handle = INVALID_HANDLE_VALUE;
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_open(com_util_file *file, const char *path, int flags)
{
    if (file == NULL || path == NULL || flags < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if ((flags & COM_UTIL_FILE_OPEN_CREATE_NEW) != 0 && (flags & COM_UTIL_FILE_OPEN_CREATE) == 0)
    {
        /* CREATE_NEW は CREATE との併用が必須。単独指定は Linux では O_EXCL が黙って無視され、
           Windows では OPEN_EXISTING 相当に落ちるため、意図と乖離しないよう明示的に失敗させる。 */
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    com_util_file_close(file);

#if defined(PLATFORM_LINUX)
    {
        int open_flags;
        int has_read = (flags & COM_UTIL_FILE_OPEN_READ) != 0;
        int has_write = (flags & COM_UTIL_FILE_OPEN_WRITE) != 0;

        if (!has_read && !has_write)
        {
            has_write = 1; /* 既定は書き込み専用として扱い、COM_UTIL_FILE_OPEN_WRITE が指定されているものとして扱う。 */
        }

        if (has_read && has_write)
        {
            open_flags = O_RDWR;
        }
        else if (has_read)
        {
            open_flags = O_RDONLY;
        }
        else
        {
            open_flags = O_WRONLY;
        }

        if ((flags & COM_UTIL_FILE_OPEN_CREATE) != 0)
        {
            open_flags |= O_CREAT;
        }
        if ((flags & COM_UTIL_FILE_OPEN_CREATE_NEW) != 0)
        {
            open_flags |= O_EXCL;
        }
        if ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) != 0)
        {
            open_flags |= O_TRUNC;
        }
        if ((flags & COM_UTIL_FILE_OPEN_APPEND) != 0)
        {
            open_flags |= O_APPEND;
        }
        if ((flags & COM_UTIL_FILE_OPEN_WRITE_THROUGH) != 0)
        {
    #if defined(O_DSYNC)
            open_flags |= O_DSYNC;
    #elif defined(O_SYNC)
            open_flags |= O_SYNC;
    #endif /* sync flag */
        }

        file->handle = open(path, open_flags, 0644);
        if (file_is_open(file))
        {
            return COM_UTIL_OK;
        }
        else
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        /* Linux の open() には他プロセスへの共有可否を制御する概念がなく常に共有可能であるため、
           Windows も同じ既定動作に揃えて常にフル共有でオープンする (file.h の com_util_file_open 参照)。 */
        DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        DWORD desired_access;
        DWORD creation_disposition;
        DWORD file_flags = FILE_ATTRIBUTE_NORMAL;
        int use_append_access;
        int has_read = (flags & COM_UTIL_FILE_OPEN_READ) != 0;
        int has_write = (flags & COM_UTIL_FILE_OPEN_WRITE) != 0;
        LARGE_INTEGER pos;

        if (!has_read && !has_write)
        {
            has_write = 1; /* 既定は書き込み専用として扱い、COM_UTIL_FILE_OPEN_WRITE が指定されているものとして扱う。 */
        }

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        if ((flags & COM_UTIL_FILE_OPEN_WRITE_THROUGH) != 0)
        {
            file_flags |= FILE_FLAG_WRITE_THROUGH;
        }

        if ((flags & COM_UTIL_FILE_OPEN_CREATE) != 0 && (flags & COM_UTIL_FILE_OPEN_CREATE_NEW) != 0)
        {
            /* 新規作成のみ許可する。既存ファイルがある場合は失敗させる。 */
            creation_disposition = CREATE_NEW;
        }
        else if ((flags & COM_UTIL_FILE_OPEN_CREATE) != 0)
        {
            if ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) != 0)
            {
                creation_disposition = CREATE_ALWAYS;
            }
            else
            {
                creation_disposition = OPEN_ALWAYS;
            }
        }
        else if ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) != 0)
        {
            creation_disposition = TRUNCATE_EXISTING;
        }
        else
        {
            creation_disposition = OPEN_EXISTING;
        }

        if (has_write)
        {
            /* APPEND かつ TRUNCATE なしのとき FILE_APPEND_DATA で開くと書き込みが EOF へ原子的に向かう。 */
            /* FILE_READ_ATTRIBUTES は後続の GetFileSizeEx (com_util_file_get_size) に必要。           */
            /* TRUNCATE ありの場合は既存ファイルをゼロ化して先頭から書くため GENERIC_WRITE を使う。    */
            use_append_access =
                ((flags & COM_UTIL_FILE_OPEN_APPEND) != 0) && ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) == 0);
            if (use_append_access != 0)
            {
                desired_access = FILE_APPEND_DATA | FILE_READ_ATTRIBUTES;
            }
            else
            {
                desired_access = GENERIC_WRITE;
            }
            if (has_read)
            {
                desired_access |= GENERIC_READ;
            }
        }
        else
        {
            use_append_access = 0;
            desired_access = GENERIC_READ;
        }

        /* wpath は本関数内ですでに UTF-8 から変換済みのため、CreateFileU を経由しない */
        file->handle = CreateFileW(wpath, desired_access, share_mode, NULL, creation_disposition, file_flags, NULL);
        if (!file_is_open(file))
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        /* GENERIC_WRITE 経路の APPEND: 手動で末尾へシークする。 */
        /* FILE_APPEND_DATA 経路は OS が書き込みを EOF へ原子的に向けるためシーク不要。 */
        if (use_append_access == 0 && (flags & COM_UTIL_FILE_OPEN_APPEND) != 0)
        {
            pos.QuadPart = 0;
            if (!SetFilePointerEx(file->handle, pos, NULL, FILE_END))
            {
                com_util_file_close(file);
                return COM_UTIL_ERR_UNKNOWN;
            }
        }

        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_write(com_util_file *file, const void *buf, size_t len)
{
    if (!file_is_open(file) || (buf == NULL && len > 0u))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    if (len == 0u)
    {
        return COM_UTIL_OK;
    }

#if defined(PLATFORM_LINUX)
    {
        const char *cursor = (const char *)buf;
        size_t remaining = len;

        while (remaining > 0u)
        {
            ssize_t written = write(file->handle, cursor, remaining);
            if (written <= 0)
            {
                return COM_UTIL_ERR_UNKNOWN;
            }

            cursor += (size_t)written;
            remaining -= (size_t)written;
        }

        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        const char *cursor = (const char *)buf;
        size_t remaining = len;

        while (remaining > 0u)
        {
            DWORD chunk;
            /* WriteFile の DWORD 境界に合わせるため、コーディング規範の例外として UINT32_MAX を維持する。 */
            if (remaining > (size_t)UINT32_MAX)
            {
                chunk = UINT32_MAX;
            }
            else
            {
                chunk = (DWORD)remaining;
            }
            DWORD written = 0;

            if (!WriteFile(file->handle, cursor, chunk, &written, NULL) || written == 0u)
            {
                return COM_UTIL_ERR_UNKNOWN;
            }

            cursor += (size_t)written;
            remaining -= (size_t)written;
        }

        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_set_size(com_util_file *file, size_t size)
{
    if (!file_is_open(file))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

#if defined(PLATFORM_LINUX)
    {
        if (ftruncate(file->handle, (off_t)size) != 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        LARGE_INTEGER pos;

        pos.QuadPart = (LONGLONG)size;
        if (!SetFilePointerEx(file->handle, pos, NULL, FILE_BEGIN))
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (!SetEndOfFile(file->handle))
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_read(com_util_file *file, void *buf, const size_t len, size_t *read_out)
{
    if (!file_is_open(file) || buf == NULL || read_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    *read_out = 0u;

    if (len == 0u)
    {
        return COM_UTIL_OK;
    }

#if defined(PLATFORM_LINUX)
    {
        ssize_t bytes = read(file->handle, buf, len);

        if (bytes < 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        *read_out = (size_t)bytes;
        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        DWORD chunk;
        DWORD bytes = 0;

        /* ReadFile の DWORD 境界に合わせるため、コーディング規範の例外として UINT32_MAX を維持する。 */
        if (len > (size_t)UINT32_MAX)
        {
            chunk = UINT32_MAX;
        }
        else
        {
            chunk = (DWORD)len;
        }

        if (!ReadFile(file->handle, buf, chunk, &bytes, NULL))
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        *read_out = (size_t)bytes;
        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_get_size(const com_util_file *file, size_t *size_out)
{
    if (!file_is_open(file) || size_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

#if defined(PLATFORM_LINUX)
    {
        struct stat st;

        if (fstat(file->handle, &st) != 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        *size_out = (size_t)st.st_size;
        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        LARGE_INTEGER size;

        if (!GetFileSizeEx(file->handle, &size))
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        *size_out = (size_t)size.QuadPart;
        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_get_id(const com_util_file *file, com_util_file_id *id_out)
{
    if (!file_is_open(file) || id_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

#if defined(PLATFORM_LINUX)
    {
        struct stat st;

        if (fstat(file->handle, &st) != 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        id_out->volume = (uint64_t)st.st_dev;
        id_out->index = (uint64_t)st.st_ino;
        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        BY_HANDLE_FILE_INFORMATION info;

        if (!GetFileInformationByHandle(file->handle, &info))
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        id_out->volume = (uint64_t)info.dwVolumeSerialNumber;
        id_out->index = ((uint64_t)info.nFileIndexHigh << 32) | (uint64_t)info.nFileIndexLow;
        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_file_get_path_id(const char *path, com_util_file_id *id_out)
{
    if (path == NULL || id_out == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

#if defined(PLATFORM_LINUX)
    {
        struct stat st;

        if (stat(path, &st) != 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        id_out->volume = (uint64_t)st.st_dev;
        id_out->index = (uint64_t)st.st_ino;
        return COM_UTIL_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        HANDLE handle;
        BY_HANDLE_FILE_INFORMATION info;
        BOOL got_info;

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        /* 属性読み取り専用で開くため、他プロセスの共有モードの影響を受けない。 */
        /* wpath は本関数内ですでに UTF-8 から変換済みのため、CreateFileU を経由しない */
        handle = CreateFileW(wpath, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        got_info = GetFileInformationByHandle(handle, &info);
        CloseHandle(handle);
        if (!got_info)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }

        id_out->volume = (uint64_t)info.dwVolumeSerialNumber;
        id_out->index = ((uint64_t)info.nFileIndexHigh << 32) | (uint64_t)info.nFileIndexLow;
        return COM_UTIL_OK;
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_file_close(com_util_file *file)
{
    if (file == NULL)
    {
        return;
    }

#if defined(PLATFORM_LINUX)
    if (file->handle != -1)
    {
        close(file->handle);
        file->handle = -1;
    }
#elif defined(PLATFORM_WINDOWS)
    if (file->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file->handle);
        file->handle = INVALID_HANDLE_VALUE;
    }
#endif /* PLATFORM_ */
}
