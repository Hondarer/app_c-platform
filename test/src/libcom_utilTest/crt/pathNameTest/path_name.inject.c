/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 関数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef _IN_TEST_SRC
    #include "path_name.c"
#endif /* _IN_TEST_SRC */

#include "path_name.inject.h"

int test_copy_path_name_text(char *path_out, const size_t path_size, com_util_error *detail_out, const char *text)
{
    return com_util_copy_path_name_text(path_out, path_size, detail_out, text);
}
