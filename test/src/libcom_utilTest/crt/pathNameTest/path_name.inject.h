// テスト対象ソース ファイルの注入用追加ヘッダー
// このヘッダーをテスト プログラムが参照することで
// テスト プログラムからテスト対象ソースの static 関数にアクセスできます
#ifndef PATH_NAME_INJECT_H
#define PATH_NAME_INJECT_H

#include <com_util/base/error.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* path_name.c のファイル内 static 関数 com_util_copy_path_name_text へのアクセサー。
       公開 API 経由では呼び出し元がすべて事前に引数を検証するため、
       この関数が持つ引数検証の分岐へは到達できない。 */
    extern int test_copy_path_name_text(char *path_out, const size_t path_size, com_util_error *detail_out,
                                        const char *text);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PATH_NAME_INJECT_H */
