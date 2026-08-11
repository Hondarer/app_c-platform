// sync_linux.c の static 関数へテストからアクセスするための宣言
#ifndef SYNC_LINUX_INJECT_H
#define SYNC_LINUX_INJECT_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

    #include <time.h>

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    extern int test_sync_map_wait_rc(int rc);
    extern void test_sync_monotonic_deadline(struct timespec *abs_ts, int timeout_ms);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_LINUX */

#endif /* SYNC_LINUX_INJECT_H */
