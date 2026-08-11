// sync_linux.c の static 関数へテスト用アクセサーを追加する
#ifndef _IN_TEST_SRC
    #include "sync_linux.c"
#endif /* _IN_TEST_SRC */

#include "sync_linux.inject.h"

#if defined(PLATFORM_LINUX)

int test_sync_map_wait_rc(const int rc)
{
    return map_wait_rc(rc);
}

void test_sync_monotonic_deadline(struct timespec *abs_ts, const int timeout_ms)
{
    monotonic_deadline(abs_ts, timeout_ms);
}

#endif /* PLATFORM_LINUX */
