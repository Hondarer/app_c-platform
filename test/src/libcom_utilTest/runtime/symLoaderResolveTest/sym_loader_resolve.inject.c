/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 関数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef _IN_TEST_SRC
    #include "sym_loader_resolve.c"
#endif /* _IN_TEST_SRC */

#include "sym_loader_resolve.inject.h"

int test_sym_loader_ensure_entry_lock_initialized(com_util_sym_loader_entry *fobj)
{
    return ensure_entry_lock_initialized(fobj);
}

#if defined(PLATFORM_LINUX)
void test_sym_loader_set_entry_lock_wait_hook(void (*hook)(com_util_sym_loader_entry *fobj))
{
    if (hook == NULL)
    {
        s_entry_lock_wait_hook = wait_for_entry_lock_initialization;
    }
    else
    {
        s_entry_lock_wait_hook = hook;
    }
}

void test_sym_loader_default_entry_lock_wait(com_util_sym_loader_entry *fobj)
{
    wait_for_entry_lock_initialization(fobj);
}
#endif /* PLATFORM_LINUX */
