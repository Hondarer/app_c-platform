/**
 *******************************************************************************
 *  @file           mmap_linux.c
 *  @brief          Linux 向けのメモリ マップド ファイルを実装します。
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <stdlib.h>
    #include <string.h>
    #include <sys/mman.h>

    #include <com_util/crt/file.h>
    #include <com_util/crt/stdio.h>
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
                             size_t *size_out)
{
    int open_result;

    com_util_file_init(file);

    if (access == COM_UTIL_MMAP_ACCESS_READ_ONLY)
    {
        open_result = com_util_file_open(file, path, COM_UTIL_FILE_OPEN_READ);
        if (open_result != 0)
        {
            return COM_UTIL_ERR_UNKNOWN;
        }
        if (com_util_file_get_size(file, size_out) != COM_UTIL_OK)
        {
            com_util_file_close(file);
            return COM_UTIL_ERR_UNKNOWN;
        }
        return COM_UTIL_OK;
    }

    /* 新規作成のみ許可するオープンをまず試みる。成功すれば新規作成と判定できる。 */
    open_result = com_util_file_open(file, path,
                                     COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE | COM_UTIL_FILE_OPEN_CREATE |
                                         COM_UTIL_FILE_OPEN_CREATE_NEW);
    if (open_result == 0)
    {
        if (create_size == 0)
        {
            com_util_file_close(file);
            (void)com_util_remove(path);
            return COM_UTIL_ERR_INVALID_ARGUMENT;
        }
        if (com_util_file_set_size(file, create_size) != COM_UTIL_OK)
        {
            com_util_file_close(file);
            (void)com_util_remove(path);
            return COM_UTIL_ERR_UNKNOWN;
        }
        *size_out = create_size;
        return COM_UTIL_OK;
    }

    /* 新規作成に失敗した場合は既存ファイルとみなし、CREATE_NEW を外して再オープンする。 */
    open_result = com_util_file_open(file, path, COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE);
    if (open_result != 0)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }
    if (com_util_file_get_size(file, size_out) != COM_UTIL_OK)
    {
        com_util_file_close(file);
        return COM_UTIL_ERR_UNKNOWN;
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mmap_attach(const char *path, com_util_mmap_access_t access, size_t create_size, com_util_mmap **map)
{
    com_util_mmap *new_map;
    int result;
    size_t size = 0;
    void *address;
    int prot;

    if (path == NULL || path[0] == '\0' || map == NULL ||
        (access != COM_UTIL_MMAP_ACCESS_READ_ONLY && access != COM_UTIL_MMAP_ACCESS_READ_WRITE))
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    new_map = (com_util_mmap *)calloc(1, sizeof(*new_map));
    if (new_map == NULL)
    {
        return COM_UTIL_ERR_UNKNOWN;
    }

    result = open_backing_file(path, access, create_size, &new_map->file, &size);
    if (result != COM_UTIL_OK)
    {
        free(new_map);
        return result;
    }

    if (size == 0)
    {
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_ERR_UNKNOWN;
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
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_ERR_UNKNOWN;
    }

    /* プロセス横断ロックは実際に参照されるまで開かない。                          */
    /* ここでは識別子の保持と、初回生成を直列化するミューテックスの用意に留める。 */
    new_map->identity = duplicate_path(path);
    if (new_map->identity == NULL)
    {
        munmap(address, size);
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_ERR_UNKNOWN;
    }

    if (com_util_local_lock_create(&new_map->rwlock_guard) != COM_UTIL_OK)
    {
        free(new_map->identity);
        munmap(address, size);
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_ERR_UNKNOWN;
    }

    new_map->address = address;
    new_map->size = size;
    *map = new_map;
    return COM_UTIL_OK;
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

com_util_interprocess_rwlock *com_util_mmap_get_rwlock(const com_util_mmap *map)
{
    com_util_mmap *target;
    com_util_interprocess_rwlock *result;

    if (map == NULL)
    {
        return NULL;
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
        return NULL;
    }

    if (target->rwlock == NULL)
    {
        if (com_util_interprocess_rwlock_open(target->identity, &target->rwlock) != COM_UTIL_OK)
        {
            /* 失敗時に中途半端な値が残らないよう明示的に戻す。 */
            target->rwlock = NULL;
        }
    }
    result = target->rwlock;

    (void)com_util_local_lock_unlock(target->rwlock_guard);

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mmap_flush(com_util_mmap *map, void *address, size_t length)
{
    void *target;
    size_t target_len;

    if (map == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
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
        return COM_UTIL_ERR_UNKNOWN;
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_mmap_detach(com_util_mmap *map)
{
    if (map == NULL)
    {
        return;
    }
    if (map->rwlock != NULL)
    {
        com_util_interprocess_rwlock_destroy(map->rwlock);
    }
    com_util_local_lock_destroy(map->rwlock_guard);
    free(map->identity);
    munmap(map->address, map->size);
    com_util_file_close(&map->file);
    free(map);
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif
