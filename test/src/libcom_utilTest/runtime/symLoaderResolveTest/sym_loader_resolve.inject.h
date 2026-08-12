/* テスト対象ソース ファイルの注入用追加ヘッダー
 * このヘッダーをテスト プログラムが参照することで
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef SYM_LOADER_RESOLVE_INJECT_H
#define SYM_LOADER_RESOLVE_INJECT_H

#include <com_util/runtime/sym_loader.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern int test_sym_loader_ensure_entry_lock_initialized(com_util_sym_loader_entry *fobj);
#if defined(PLATFORM_LINUX)
    extern void test_sym_loader_set_entry_lock_wait_hook(void (*hook)(com_util_sym_loader_entry *fobj));
    extern void test_sym_loader_default_entry_lock_wait(com_util_sym_loader_entry *fobj);
#endif /* PLATFORM_LINUX */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SYM_LOADER_RESOLVE_INJECT_H */
