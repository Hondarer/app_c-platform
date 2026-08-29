#ifndef MMAP_TEST_COMMON_H
#define MMAP_TEST_COMMON_H

#include <testfw.h>
#include <mock_cplat.h>

#include <cplat/base/platform.h>
#include <cplat/base/result.h>
#include <cplat/crt/file.h>
#include <cplat/mmap/mmap.h>
#include <cplat/sync/sync.h>

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

cplat_local_lock *const kFakeGuard = reinterpret_cast<cplat_local_lock *>(static_cast<uintptr_t>(0x100));
cplat_interprocess_rwlock *const kFakeRwlock =
    reinterpret_cast<cplat_interprocess_rwlock *>(static_cast<uintptr_t>(0x200));

int flags_create_new(void)
{
    return CPLAT_FILE_OPEN_READ | CPLAT_FILE_OPEN_WRITE | CPLAT_FILE_OPEN_CREATE |
           CPLAT_FILE_OPEN_CREATE_NEW;
}

int flags_existing_rw(void)
{
    return CPLAT_FILE_OPEN_READ | CPLAT_FILE_OPEN_WRITE;
}

int flags_read_only(void)
{
    return CPLAT_FILE_OPEN_READ;
}

void fill_open_file(cplat_file *file, int flags)
{
    file->handle = kFakeFileHandle;
    if ((flags & CPLAT_FILE_OPEN_WRITE) != 0)
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
    NiceMock<Mock_cplat> mock_cplat_;
#if defined(PLATFORM_LINUX)
    NiceMock<Mock_sys_mman> mock_sys_mman_;
#elif defined(PLATFORM_WINDOWS)
    NiceMock<Mock_windows> mock_windows_;
#endif /* PLATFORM_ */
    char mapped_buf_[64];

    void SetUp() override
    {
        ON_CALL(mock_cplat_, cplat_file_init(_))
            .WillByDefault(
                [](cplat_file *file)
                {
                    file->handle = kFakeFileHandle;
                    file->writable = 0;
                });
        ON_CALL(mock_cplat_, cplat_file_open(_, _, _, _))
            .WillByDefault(
                [](cplat_file *file, const char *, int flags, cplat_error *)
                {
                    fill_open_file(file, flags);
                    return CPLAT_OK;
                });
        ON_CALL(mock_cplat_, cplat_file_get_size(_, _, _))
            .WillByDefault(
                [](const cplat_file *, size_t *size_out, cplat_error *)
                {
                    *size_out = kMapSize;
                    return CPLAT_OK;
                });
        ON_CALL(mock_cplat_, cplat_file_set_size(_, _, _)).WillByDefault(Return(CPLAT_OK));
        ON_CALL(mock_cplat_, cplat_file_close(_, _)).WillByDefault(Return(CPLAT_OK));
        ON_CALL(mock_cplat_, cplat_remove(_, _)).WillByDefault(Return(CPLAT_OK));
        ON_CALL(mock_cplat_, cplat_local_lock_create(_))
            .WillByDefault(
                [](cplat_local_lock **lock)
                {
                    *lock = kFakeGuard;
                    return CPLAT_OK;
                });
        ON_CALL(mock_cplat_, cplat_local_lock_lock(_, _)).WillByDefault(Return(CPLAT_OK));
        ON_CALL(mock_cplat_, cplat_local_lock_unlock(_)).WillByDefault(Return(CPLAT_OK));
        ON_CALL(mock_cplat_, cplat_local_lock_dispose(_)).WillByDefault(Return());
        ON_CALL(mock_cplat_, cplat_interprocess_rwlock_open(_, _))
            .WillByDefault(
                [](const char *, cplat_interprocess_rwlock **lock)
                {
                    *lock = kFakeRwlock;
                    return CPLAT_OK;
                });
        ON_CALL(mock_cplat_, cplat_interprocess_rwlock_dispose(_)).WillByDefault(Return());

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

    void attachNewFile(cplat_mmap **map)
    {
        ASSERT_EQ(CPLAT_OK, cplat_mmap_attach(kPath, CPLAT_MMAP_ACCESS_READ_WRITE, kMapSize, map, NULL));
        ASSERT_NE(static_cast<cplat_mmap *>(NULL), *map);
    }
};

#endif /* MMAP_TEST_COMMON_H */
