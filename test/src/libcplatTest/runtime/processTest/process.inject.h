/* テスト対象ソース ファイルの注入用追加ヘッダー
 * このヘッダーをテスト プログラムが参照することで
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef CPLAT_PROCESS_TEST_INJECT_H
#define CPLAT_PROCESS_TEST_INJECT_H

#include <stddef.h>
#include <stdint.h>

#include <cplat/runtime/process.h>

#ifdef __cplusplus
extern "C"
{
#endif

    extern uint64_t test_process_monotonic_ms(void);
    extern char *test_process_string_duplicate(const char *text);
    extern size_t test_process_env_key_len(const char *entry);
    extern int test_process_env_key_matches(const char *entry, const char *key, size_t key_len);
    extern int test_process_set_env_entry(char **envp, size_t capacity, const char *entry);
    extern char **test_process_build_environment(char *const *overrides);
    extern void test_process_free_envp(char **envp);
    extern const char *test_process_find_env_value(char *const *envp, const char *key);
    extern int test_process_setup_child_stdio_one(const cplat_process_stdio *spec, int target_fd, int null_flags);
    extern int test_process_setup_child_stdio(const cplat_process_options *options);
    extern void test_process_exec_with_path(char *const *argv, char *const *envp);
    extern int test_process_run_child(const cplat_process_options *options, char *const *envp);

#ifdef __cplusplus
}
#endif

#endif /* CPLAT_PROCESS_TEST_INJECT_H */
