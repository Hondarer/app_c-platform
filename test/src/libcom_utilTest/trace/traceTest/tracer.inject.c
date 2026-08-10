// テスト対象ソース ファイルの注入用追加ソース
// このソースはテスト対象ソースの末尾に結合されます
// この static 変数へのアクセサーによって
// テスト プログラムからテスト対象ソースの static 変数にアクセスできます
#ifndef _IN_TEST_SRC
    #include "tracer.c"
#endif /* _IN_TEST_SRC */

#include "tracer.inject.h"

void test_trace_registry_reset_shutdown_state(void)
{
    com_util_once_flag reset_once = {0};

    s_trace_registry.shutdown_started = 0;
    s_trace_shutdown_once = reset_once;
}
