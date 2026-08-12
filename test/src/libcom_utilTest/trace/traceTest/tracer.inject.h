// テスト対象ソース ファイルの注入用追加ヘッダー
// このヘッダーをテスト プログラムが参照することで
// テスト プログラムからテスト対象ソースの static 変数にアクセスできます
#ifndef TRACER_INJECT_H
#define TRACER_INJECT_H

#include <stddef.h>

#include <com_util/trace/tracer.h>
#include <com_util/trace/trace_file.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /* tracer.c のファイル内 static 変数 s_trace_registry の shutdown 状態を戻すアクセサー。
       shutdown_started は本来一度立つと戻らないため、shutdown を検証したテストの後に
       同一プロセスで実行される他のテストが tracer を生成できるようにする。 */
    extern void test_trace_registry_reset_shutdown_state(void);
    extern void test_trace_registry_set_shutdown_started(size_t shutdown_started);
    extern int test_trace_registry_append_null(void);

    extern int test_tracer_handle_is_active(const com_util_tracer *handle);
    extern int test_tracer_begin_dispose(com_util_tracer *handle);
#if defined(PLATFORM_LINUX)
    extern int test_tracer_to_syslog_level(com_util_trace_level level);
#endif /* PLATFORM_LINUX */
    extern size_t test_tracer_utf8_safe_truncate(const char *text, size_t position);
    extern int test_tracer_build_default_file_path(const com_util_tracer *handle, char *out, size_t out_size);
    extern int test_tracer_hex_write_impl(com_util_tracer *handle, com_util_trace_level level,
                                          const com_util_timespec *timestamp, const void *data, size_t size,
                                          const char *label);

    extern void test_tracer_set_lifecycle_state(com_util_tracer *handle, int lifecycle_state);
    extern void test_tracer_set_running(com_util_tracer *handle, int running);
    extern void test_tracer_set_file_handle(com_util_tracer *handle, com_util_trace_file_sink *file_handle);
    extern void test_tracer_set_config_rwlock_initialized(com_util_tracer *handle, int initialized);
    extern com_util_local_rwlock *test_tracer_get_config_rwlock(com_util_tracer *handle);
    extern void test_tracer_set_hook_fn(com_util_tracer_hook_entry *entry, com_util_tracer_hook_fn fn);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* TRACER_INJECT_H */
