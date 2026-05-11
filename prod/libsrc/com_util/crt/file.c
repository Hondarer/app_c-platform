#include <com_util/crt/file.h>
#include <com_util/crt/path.h>

#include <com_util/crt/crt_internal.h>

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif /* PLATFORM_LINUX */

static int file_is_open(const com_util_file_t *file)
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

COM_UTIL_EXPORT void COM_UTIL_API com_util_file_init(com_util_file_t *file)
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_file_open(com_util_file_t *file,
                                                            const char             *path,
                                                            uint32_t                flags)
{
    if (file == NULL || path == NULL)
    {
        return -1;
    }

    com_util_file_close(file);

#if defined(PLATFORM_LINUX)
    {
        int open_flags = O_WRONLY;

        if ((flags & COM_UTIL_FILE_OPEN_CREATE) != 0u)
        {
            open_flags |= O_CREAT;
        }
        if ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) != 0u)
        {
            open_flags |= O_TRUNC;
        }
        if ((flags & COM_UTIL_FILE_OPEN_APPEND) != 0u)
        {
            open_flags |= O_APPEND;
        }
        if ((flags & COM_UTIL_FILE_OPEN_WRITE_THROUGH) != 0u)
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
            return 0;
        }
        else
        {
            return -1;
        }
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];
        DWORD   share_mode = 0;
        DWORD   creation_disposition;
        DWORD   file_flags = FILE_ATTRIBUTE_NORMAL;
        LARGE_INTEGER pos;

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        if ((flags & COM_UTIL_FILE_OPEN_SHARE_READ) != 0u)
        {
            share_mode |= FILE_SHARE_READ;
        }
        if ((flags & COM_UTIL_FILE_OPEN_SHARE_DELETE) != 0u)
        {
            share_mode |= FILE_SHARE_DELETE;
        }
        if ((flags & COM_UTIL_FILE_OPEN_WRITE_THROUGH) != 0u)
        {
            file_flags |= FILE_FLAG_WRITE_THROUGH;
        }

        if ((flags & COM_UTIL_FILE_OPEN_CREATE) != 0u)
        {
            if ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) != 0u)
            {
                creation_disposition = CREATE_ALWAYS;
            }
            else
            {
                creation_disposition = OPEN_ALWAYS;
            }
        }
        else if ((flags & COM_UTIL_FILE_OPEN_TRUNCATE) != 0u)
        {
            creation_disposition = TRUNCATE_EXISTING;
        }
        else
        {
            creation_disposition = OPEN_EXISTING;
        }

        file->handle = CreateFileW(wpath,
                                   GENERIC_WRITE,
                                   share_mode,
                                   NULL,
                                   creation_disposition,
                                   file_flags,
                                   NULL);
        if (!file_is_open(file))
        {
            return -1;
        }

        if ((flags & COM_UTIL_FILE_OPEN_APPEND) != 0u)
        {
            pos.QuadPart = 0;
            if (!SetFilePointerEx(file->handle, pos, NULL, FILE_END))
            {
                com_util_file_close(file);
                return -1;
            }
        }

        return 0;
    }
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_file_write(com_util_file_t *file,
                                                             const void             *buf,
                                                             size_t                  len)
{
    if (!file_is_open(file) || (buf == NULL && len > 0u))
    {
        return -1;
    }

    if (len == 0u)
    {
        return 0;
    }

#if defined(PLATFORM_LINUX)
    {
        const char *cursor = (const char *)buf;
        size_t      remaining = len;

        while (remaining > 0u)
        {
            ssize_t written = write(file->handle, cursor, remaining);
            if (written <= 0)
            {
                return -1;
            }

            cursor += (size_t)written;
            remaining -= (size_t)written;
        }

        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        const char *cursor = (const char *)buf;
        size_t      remaining = len;

        while (remaining > 0u)
        {
            DWORD chunk;
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
                return -1;
            }

            cursor += (size_t)written;
            remaining -= (size_t)written;
        }

        return 0;
    }
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_file_get_size(com_util_file_t *file,
                                                                size_t                 *size_out)
{
    if (!file_is_open(file) || size_out == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        struct stat st;

        if (fstat(file->handle, &st) != 0)
        {
            return -1;
        }

        *size_out = (size_t)st.st_size;
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        LARGE_INTEGER size;

        if (!GetFileSizeEx(file->handle, &size))
        {
            return -1;
        }

        *size_out = (size_t)size.QuadPart;
        return 0;
    }
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT void COM_UTIL_API com_util_file_close(com_util_file_t *file)
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
