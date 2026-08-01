/**
 *******************************************************************************
 *  @file           mmap_linux.c
 *  @brief          Linux 向けのメモリ マップド ファイルを実装します。
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <errno.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/mman.h>

    #include <com_util/crt/file.h>
    #include <com_util/crt/stdio.h>
    #include <com_util/base/error_internal.h>
    #include <com_util/mmap/mmap.h>
    #include <com_util/sync/sync.h>

struct com_util_mmap
{
    void *address;
    size_t size;
    com_util_interprocess_rwlock *rwlock; /* 初回参照時に生成する。未生成の間は NULL。 */
    com_util_local_lock *rwlock_guard;    /* rwlock の初回生成を直列化する。 */
    char *identity;                       /* rwlock の識別子として使うパスの複製。 */
    com_util_file file;
    int _pad_struct_end;
};

static char *duplicate_path(const char *path)
{
    size_t size = strlen(path) + 1;
    char *copy = (char *)malloc(size);

    if (copy == NULL)
    {
        return NULL;
    }
    memcpy(copy, path, size);
    return copy;
}

static int open_backing_file(const char *path, com_util_mmap_access_t access, size_t create_size, com_util_file *file,
                             size_t *size_out, com_util_error *detail_out)
{
    int open_result;

    com_util_file_init(file);

    if (access == COM_UTIL_MMAP_ACCESS_READ_ONLY)
    {
        open_result = com_util_file_open(file, path, COM_UTIL_FILE_OPEN_READ, detail_out);
        if (open_result != COM_UTIL_OK)
        {
            return open_result;
        }
        open_result = com_util_file_get_size(file, size_out, detail_out);
        if (open_result != COM_UTIL_OK)
        {
            (void)com_util_file_close(file, NULL);
            return open_result;
        }
        return com_util_error_report_success(detail_out);
    }

    /* 新規作成のみ許可するオープンをまず試みる。成功すれば新規作成と判定できる。 */
    open_result = com_util_file_open(file, path,
                                     COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE | COM_UTIL_FILE_OPEN_CREATE |
                                         COM_UTIL_FILE_OPEN_CREATE_NEW,
                                     detail_out);
    if (open_result == 0)
    {
        if (create_size == 0)
        {
            (void)com_util_file_close(file, NULL);
            (void)com_util_remove(path, NULL);
            return com_util_error_report_errno(detail_out, EINVAL);
        }
        open_result = com_util_file_set_size(file, create_size, detail_out);
        if (open_result != COM_UTIL_OK)
        {
            (void)com_util_file_close(file, NULL);
            (void)com_util_remove(path, NULL);
            return open_result;
        }
        *size_out = create_size;
        return com_util_error_report_success(detail_out);
    }

    /* 新規作成に失敗した場合は既存ファイルとみなし、CREATE_NEW を外して再オープンする。 */
    open_result = com_util_file_open(file, path, COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE, detail_out);
    if (open_result != COM_UTIL_OK)
    {
        return open_result;
    }
    open_result = com_util_file_get_size(file, size_out, detail_out);
    if (open_result != COM_UTIL_OK)
    {
        (void)com_util_file_close(file, NULL);
        return open_result;
    }
    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mmap_attach(const char *path, com_util_mmap_access_t access, size_t create_size, com_util_mmap **map,
                         com_util_error *detail_out)
{
    com_util_mmap *new_map;
    int result;
    size_t size = 0;
    void *address;
    int prot;

    if (path == NULL || path[0] == '\0' || map == NULL ||
        (access != COM_UTIL_MMAP_ACCESS_READ_ONLY && access != COM_UTIL_MMAP_ACCESS_READ_WRITE))
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

    new_map = (com_util_mmap *)calloc(1, sizeof(*new_map));
    if (new_map == NULL)
    {
        return com_util_error_report_errno(detail_out, ENOMEM);
    }

    result = open_backing_file(path, access, create_size, &new_map->file, &size, detail_out);
    if (result != COM_UTIL_OK)
    {
        free(new_map);
        return result;
    }

    if (size == 0)
    {
        (void)com_util_file_close(&new_map->file, NULL);
        free(new_map);
        return com_util_error_report_errno(detail_out, EINVAL);
    }

    if (access == COM_UTIL_MMAP_ACCESS_READ_WRITE)
    {
        prot = PROT_READ | PROT_WRITE;
    }
    else
    {
        prot = PROT_READ;
    }
    address = mmap(NULL, size, prot, MAP_SHARED, new_map->file.handle, 0);
    if (address == MAP_FAILED)
    {
        const int errno_value = errno;

        (void)com_util_file_close(&new_map->file, NULL);
        free(new_map);
        return com_util_error_report_errno(detail_out, errno_value);
    }

    /* プロセス横断ロックは実際に参照されるまで開かない。                          */
    /* ここでは識別子の保持と、初回生成を直列化するミューテックスの用意に留める。 */
    new_map->identity = duplicate_path(path);
    if (new_map->identity == NULL)
    {
        munmap(address, size);
        (void)com_util_file_close(&new_map->file, NULL);
        free(new_map);
        return com_util_error_report_errno(detail_out, ENOMEM);
    }

    if (com_util_local_lock_create(&new_map->rwlock_guard) != COM_UTIL_OK)
    {
        free(new_map->identity);
        munmap(address, size);
        (void)com_util_file_close(&new_map->file, NULL);
        free(new_map);
        return com_util_error_report_errno(detail_out, EIO);
    }

    new_map->address = address;
    new_map->size = size;
    *map = new_map;
    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

void *com_util_mmap_get_address(const com_util_mmap *map)
{
    if (map == NULL)
    {
        return NULL;
    }
    return map->address;
}

/* Doxygen コメントは、ヘッダーに記載 */

size_t com_util_mmap_get_size(const com_util_mmap *map)
{
    if (map == NULL)
    {
        return 0;
    }
    return map->size;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mmap_get_rwlock(const com_util_mmap *map, com_util_interprocess_rwlock **lock_out,
                             com_util_error *detail_out)
{
    com_util_mmap *target;
    if (lock_out == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }
    *lock_out = NULL;
    if (map == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

    /* ロックは本関数の初回呼び出しで生成するため、ハンドルの内容を更新する。      */
    /* 更新対象はロックのキャッシュだけであり、マップ済みアドレスやサイズなど      */
    /* 呼び出し側から見える状態は変化しないため、他のアクセサーと同様に引数の      */
    /* const を維持する。ハンドルの実体は com_util_mmap_attach() が非 const で     */
    /* 確保しているため、const を外す操作は定義動作である。                       */
    #if defined(COMPILER_GCC)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wcast-qual"
    #endif /* COMPILER_GCC */
    target = (com_util_mmap *)map;
    #if defined(COMPILER_GCC)
        #pragma GCC diagnostic pop
    #endif /* COMPILER_GCC */

    if (com_util_local_lock_lock(target->rwlock_guard, COM_UTIL_SYNC_WAIT_FOREVER) != COM_UTIL_OK)
    {
        return com_util_error_report_errno(detail_out, EIO);
    }

    if (target->rwlock == NULL)
    {
        if (com_util_interprocess_rwlock_open(target->identity, &target->rwlock) != COM_UTIL_OK)
        {
            /* 失敗時に中途半端な値が残らないよう明示的に戻す。 */
            target->rwlock = NULL;
        }
    }
    (void)com_util_local_lock_unlock(target->rwlock_guard);
    if (target->rwlock == NULL)
    {
        return com_util_error_report_errno(detail_out, EIO);
    }
    *lock_out = target->rwlock;
    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mmap_flush(com_util_mmap *map, void *address, size_t length, com_util_error *detail_out)
{
    void *target;
    size_t target_len;

    if (map == NULL)
    {
        return com_util_error_report_errno(detail_out, EINVAL);
    }

    if (address == NULL)
    {
        target = map->address;
        target_len = map->size;
    }
    else
    {
        target = address;
        target_len = length;
    }

    if (msync(target, target_len, MS_SYNC) != 0)
    {
        return com_util_error_report_errno(detail_out, errno);
    }
    return com_util_error_report_success(detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mmap_detach(com_util_mmap *map, com_util_error *detail_out)
{
    int unmap_result;
    int unmap_errno = 0;
    int close_result;

    if (map == NULL)
    {
        return com_util_error_report_success(detail_out);
    }
    if (map->rwlock != NULL)
    {
        com_util_interprocess_rwlock_destroy(map->rwlock);
    }
    com_util_local_lock_destroy(map->rwlock_guard);
    free(map->identity);
    unmap_result = munmap(map->address, map->size);
    if (unmap_result != 0)
    {
        unmap_errno = errno;
    }
    close_result = com_util_file_close(&map->file, detail_out);
    free(map);
    if (unmap_result != 0)
    {
        return com_util_error_report_errno(detail_out, unmap_errno);
    }
    if (close_result != COM_UTIL_OK)
    {
        return close_result;
    }
    return com_util_error_report_success(detail_out);
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif
