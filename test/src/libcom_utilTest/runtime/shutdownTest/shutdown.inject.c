// shutdown.c の static 関数へテスト用アクセサーを追加する
#ifndef _IN_TEST_SRC
    #include "shutdown.c"
#endif /* _IN_TEST_SRC */

#include "shutdown.inject.h"

#if defined(PLATFORM_LINUX)
void test_shutdown_signal_handler(const int signal_number)
{
    shutdown_signal_handler(signal_number);
}
#endif /* PLATFORM_LINUX */
