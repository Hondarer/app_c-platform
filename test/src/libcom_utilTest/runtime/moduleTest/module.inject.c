// テスト対象ソース ファイルの注入用追加ソース
// このソースはテスト対象ソースの末尾に結合されます
// この static 関数へのアクセサーによって
// テスト プログラムからテスト対象ソースの static 関数にアクセスできます
#ifndef _IN_TEST_SRC
    #include "module.c"
#endif /* _IN_TEST_SRC */

#include "module.inject.h"

const char *test_find_shared_lib_extension_cut(const char *s)
{
    return find_shared_lib_extension_cut(s);
}
