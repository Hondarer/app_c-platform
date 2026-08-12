/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 変数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 変数にアクセスできます
 */
#ifndef _IN_TEST_SRC
    #include "prompt_linux.c"
#endif /* _IN_TEST_SRC */

#include "prompt_linux.inject.h"

#if defined(PLATFORM_LINUX)

void test_prompt_set_resize_pending(int value)
{
    s_resize_pending = (sig_atomic_t)value;
}

int test_prompt_resize_pending(void)
{
    return (int)s_resize_pending;
}

int test_prompt_sigwinch_installed(void)
{
    return s_sigwinch_installed;
}

void test_prompt_set_sigwinch_installed(int value)
{
    s_sigwinch_installed = value;
}

#endif /* PLATFORM_LINUX */
