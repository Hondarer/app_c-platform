/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 関数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef _IN_TEST_SRC
    #include "module.c"
#endif /* _IN_TEST_SRC */

#include "module.inject.h"

const char *test_find_shared_lib_extension_cut(const char *s)
{
    return find_shared_lib_extension_cut(s);
}

int test_get_basename_from_path(char *basename_out, size_t basename_size, const char *path)
{
    return get_basename_from_path(basename_out, basename_size, path);
}
