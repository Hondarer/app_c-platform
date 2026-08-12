#ifndef MMAP_TEST_COMMON_H
#define MMAP_TEST_COMMON_H

#include <testfw.h>
#include <mock_com_util.h>

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/crt/file.h>
#include <com_util/mmap/mmap.h>
#include <com_util/sync/sync.h>

#if defined(PLATFORM_LINUX)
    #include <sys/mman.h>
    #include <sys/mock_mman.h>
#elif defined(PLATFORM_WINDOWS)
    #include <mock_windows.h>
#endif /* PLATFORM_ */

#include <cstddef>
#include <cstdint>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

namespace
{

const char kPath[] = "mmap.dat";
const size_t kMapSize = 64u;

#if defined(PLATFORM_LINUX)
const int kFakeFileHandle = 7;
#elif defined(PLATFORM_WINDOWS)
const HANDLE kFakeFileHandle = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(0x71));
const HANDLE kFakeMappingHandle = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(0x72));
#endif /* PLATFORM_ */

com_util_local_lock *const kFakeGuard = reinterpret_cast<com_util_local_lock *>(static_cast<uintptr_t>(0x100));
com_util_interprocess_rwlock *const kFakeRwlock =
    reinterpret_cast<com_util_interprocess_rwlock *>(static_cast<uintptr_t>(0x200));

int flags_create_new(void)
{
    return COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE | COM_UTIL_FILE_OPEN_CREATE |
           COM_UTIL_FILE_OPEN_CREATE_NEW;
}

int flags_existing_rw(void)
{
    return COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE;
}

int flags_read_only(void)
{
    return COM_UTIL_FILE_OPEN_READ;
}

void fill_open_file(com_util_file *file, int flags)
{
    file->handle = kFakeFileHandle;
    if ((flags & COM_UTIL_FILE_OPEN_WRITE) != 0)
    {
        file->writable = 1;
    }
    else
    {
        file->writable = 0;
    }
}

} // namespace

class mmapTestFixture : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
#if defined(PLATFORM_LINUX)
    NiceMock<Mock_sys_mman> mock_sys_mman_;
#elif defined(PLATFORM_WINDOWS)
    NiceMock<Mock_windows> mock_windows_;
#endif /* PLATFORM_ */
    char mapped_buf_[64];

    void SetUp() override
    {
        ON_CALL(mock_com_util_, com_util_file_init(_))
            .WillByDefault(
                [](com_util_file *file)
                {
                    file->handle = kFakeFileHandle;
                    file->writable = 0;
                });
        ON_CALL(mock_com_util_, com_util_file_open(_, _, _, _))
            .WillByDefault(
                [](com_util_file *file, const char *, int flags, com_util_error *)
                {
                    fill_open_file(file, flags);
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_com_util_, com_util_file_get_size(_, _, _))
            .WillByDefault(
                [](const com_util_file *, size_t *size_out, com_util_error *)
                {
                    *size_out = kMapSize;
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_com_util_, com_util_file_set_size(_, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util_, com_util_file_close(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util_, com_util_remove(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util_, com_util_local_lock_create(_))
            .WillByDefault(
                [](com_util_local_lock **lock)
                {
                    *lock = kFakeGuard;
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_com_util_, com_util_local_lock_lock(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util_, com_util_local_lock_unlock(_)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_com_util_, com_util_local_lock_destroy(_)).WillByDefault(Return());
        ON_CALL(mock_com_util_, com_util_interprocess_rwlock_open(_, _))
            .WillByDefault(
                [](const char *, com_util_interprocess_rwlock **lock)
                {
                    *lock = kFakeRwlock;
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_com_util_, com_util_interprocess_rwlock_destroy(_)).WillByDefault(Return());

#if defined(PLATFORM_LINUX)
        ON_CALL(mock_sys_mman_, mmap(_, _, _, _, _, _, _, _, _)).WillByDefault(Return(mapped_buf_));
        ON_CALL(mock_sys_mman_, munmap(_, _, _, _, _)).WillByDefault(Return(0));
        ON_CALL(mock_sys_mman_, msync(_, _, _, _, _, _)).WillByDefault(Return(0));
#elif defined(PLATFORM_WINDOWS)
        ON_CALL(mock_windows_, CreateFileMappingA(_, _, _, _, _, _, _, _, _)).WillByDefault(Return(kFakeMappingHandle));
        ON_CALL(mock_windows_, MapViewOfFile(_, _, _, _, _, _, _, _)).WillByDefault(Return(mapped_buf_));
        ON_CALL(mock_windows_, UnmapViewOfFile(_, _, _, _)).WillByDefault(Return(TRUE));
        ON_CALL(mock_windows_, FlushViewOfFile(_, _, _, _, _)).WillByDefault(Return(TRUE));
        ON_CALL(mock_windows_, FlushFileBuffers(_, _, _, _)).WillByDefault(Return(TRUE));
        ON_CALL(mock_windows_, CloseHandle(_, _, _, _)).WillByDefault(Return(TRUE));
#endif /* PLATFORM_ */
    }

    void attachNewFile(com_util_mmap **map)
    {
        ASSERT_EQ(COM_UTIL_OK, com_util_mmap_attach(kPath, COM_UTIL_MMAP_ACCESS_READ_WRITE, kMapSize, map, NULL));
        ASSERT_NE(static_cast<com_util_mmap *>(NULL), *map);
    }
};

#endif /* MMAP_TEST_COMMON_H */
