// syncTest の各スイートで共有するテスト ヘルパー
#ifndef SYNC_TEST_HELPER_H
#define SYNC_TEST_HELPER_H

#include <com_util/base/platform.h>
#include <com_util/sync/sync.h>
#include <mock_com_util.h>

#include <stdio.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <time.h>
    #include <unistd.h>
    #include <sys/wait.h>
static inline void make_test_interprocess_path(char *buf, size_t size, const char *tag)
{
    snprintf(buf, size, "/tmp/com_util_%s_%ld.lock", tag, (long)getpid());
}
    #define TEST_INTERPROCESS_UNLINK(path) unlink(path)
static inline uint64_t test_monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}
#elif defined(PLATFORM_WINDOWS)
    #include <process.h>
    #include <io.h>
    #include <com_util/base/windows_sdk.h>
static inline uint64_t test_monotonic_ms(void)
{
    return (uint64_t)GetTickCount64();
}
static inline void make_test_interprocess_path(char *buf, size_t size, const char *tag)
{
    char tmp_dir[256];
    DWORD len = GetTempPathA((DWORD)sizeof(tmp_dir), tmp_dir);
    if (len == 0)
    {
        tmp_dir[0] = '.';
        tmp_dir[1] = '\0';
        len = 1;
    }
    while (len > 0 && (tmp_dir[len - 1] == '\\' || tmp_dir[len - 1] == '/'))
    {
        tmp_dir[--len] = '\0';
    }
    snprintf(buf, size, "%s/com_util_%s_%ld.lock", tmp_dir, tag, (long)_getpid());
}
    #define TEST_INTERPROCESS_UNLINK(path) _unlink(path)
#endif

#endif /* SYNC_TEST_HELPER_H */
