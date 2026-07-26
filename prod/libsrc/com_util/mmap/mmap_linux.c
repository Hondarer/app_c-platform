/**
 *******************************************************************************
 *  @file           mmap_linux.c
 *  @brief          Linux 向けのメモリマップド ファイルを実装します。
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <stdlib.h>
    #include <sys/mman.h>

    #include <com_util/crt/file.h>
    #include <com_util/crt/stdio.h>
    #include <com_util/mmap/mmap.h>
    #include <com_util/sync/sync.h>

struct com_util_mmap
{
    void *address;
    size_t size;
    com_util_interprocess_rwlock *rwlock;
    com_util_file file;
    int _pad_struct_end;
};

static com_util_mmap_result_t open_backing_file(const char *path, com_util_mmap_access_t access, size_t create_size,
                                                com_util_file *file, size_t *size_out)
{
    int open_result;

    com_util_file_init(file);

    if (access == COM_UTIL_MMAP_ACCESS_READ_ONLY)
    {
        open_result = com_util_file_open(file, path, COM_UTIL_FILE_OPEN_READ);
        if (open_result != 0)
        {
            return COM_UTIL_MMAP_SYSTEM_ERROR;
        }
        if (com_util_file_get_size(file, size_out) != 0)
        {
            com_util_file_close(file);
            return COM_UTIL_MMAP_SYSTEM_ERROR;
        }
        return COM_UTIL_MMAP_OK;
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
            return COM_UTIL_MMAP_INVALID_ARGUMENT;
        }
        if (com_util_file_set_size(file, create_size) != 0)
        {
            com_util_file_close(file);
            (void)com_util_remove(path);
            return COM_UTIL_MMAP_SYSTEM_ERROR;
        }
        *size_out = create_size;
        return COM_UTIL_MMAP_OK;
    }

    /* 新規作成に失敗した場合は既存ファイルとみなし、CREATE_NEW を外して再オープンする。 */
    open_result = com_util_file_open(file, path, COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE);
    if (open_result != 0)
    {
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }
    if (com_util_file_get_size(file, size_out) != 0)
    {
        com_util_file_close(file);
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }
    return COM_UTIL_MMAP_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_mmap_result_t com_util_mmap_attach(const char *path, com_util_mmap_access_t access, size_t create_size,
                                            com_util_mmap **map)
{
    com_util_mmap *new_map;
    com_util_mmap_result_t result;
    size_t size = 0;
    void *address;
    int prot;

    if (path == NULL || path[0] == '\0' || map == NULL ||
        (access != COM_UTIL_MMAP_ACCESS_READ_ONLY && access != COM_UTIL_MMAP_ACCESS_READ_WRITE))
    {
        return COM_UTIL_MMAP_INVALID_ARGUMENT;
    }

    new_map = (com_util_mmap *)calloc(1, sizeof(*new_map));
    if (new_map == NULL)
    {
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }

    result = open_backing_file(path, access, create_size, &new_map->file, &size);
    if (result != COM_UTIL_MMAP_OK)
    {
        free(new_map);
        return result;
    }

    if (size == 0)
    {
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }

    prot = (access == COM_UTIL_MMAP_ACCESS_READ_WRITE) ? (PROT_READ | PROT_WRITE) : PROT_READ;
    address = mmap(NULL, size, prot, MAP_SHARED, new_map->file.handle, 0);
    if (address == MAP_FAILED)
    {
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }

    if (com_util_interprocess_rwlock_open(path, &new_map->rwlock) != COM_UTIL_SYNC_OK)
    {
        munmap(address, size);
        com_util_file_close(&new_map->file);
        free(new_map);
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }

    new_map->address = address;
    new_map->size = size;
    *map = new_map;
    return COM_UTIL_MMAP_OK;
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
    if (map == NULL)
    {
        return NULL;
    }
    return map->rwlock;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_mmap_result_t com_util_mmap_flush(com_util_mmap *map, void *address, size_t length)
{
    void *target;
    size_t target_len;

    if (map == NULL)
    {
        return COM_UTIL_MMAP_INVALID_ARGUMENT;
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
        return COM_UTIL_MMAP_SYSTEM_ERROR;
    }
    return COM_UTIL_MMAP_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_mmap_detach(com_util_mmap *map)
{
    if (map == NULL)
    {
        return;
    }
    com_util_interprocess_rwlock_destroy(map->rwlock);
    munmap(map->address, map->size);
    com_util_file_close(&map->file);
    free(map);
}

#elif defined(PLATFORM_WINDOWS) && defined(COMPILER_MSVC)
    #pragma warning(disable : 4206)
#endif /* PLATFORM_ */
