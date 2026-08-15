/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 変数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 変数にアクセスできます
 */
#ifndef _IN_TEST_SRC
    #include "tracer.c"
#endif /* _IN_TEST_SRC */

#include "tracer.inject.h"

#include <stdlib.h>
#include <string.h>

void test_trace_registry_reset_shutdown_state(void)
{
    com_util_once_flag reset_once = {0};

    s_trace_registry.shutdown_started = 0;
    s_trace_shutdown_once = reset_once;
    s_registry_lock_once = reset_once;
    s_registry_lock = NULL;
    s_registry_lock_init_result = COM_UTIL_ERR_UNKNOWN;
    s_shutdown_registration_result = COM_UTIL_ERR_UNKNOWN;
}

void test_trace_registry_set_shutdown_started(const size_t shutdown_started)
{
    s_trace_registry.shutdown_started = shutdown_started;
}

void test_trace_registry_set_counts(const size_t count, const size_t capacity)
{
    s_trace_registry.count = count;
    s_trace_registry.capacity = capacity;
}

void test_trace_registry_reinit_lock(void)
{
    com_util_once_flag reset_once = {0};

    s_registry_lock_once = reset_once;
    s_registry_lock = NULL;
    s_registry_lock_init_result = COM_UTIL_ERR_UNKNOWN;
}

void test_trace_registry_null_lock(void)
{
    s_registry_lock = NULL;
}

int test_trace_registry_append_null(void)
{
    if (s_trace_registry.count >= s_trace_registry.capacity)
    {
        return COM_UTIL_ERR_OUT_OF_MEMORY;
    }

    s_trace_registry.items[s_trace_registry.count] = NULL;
    s_trace_registry.count++;
    return COM_UTIL_OK;
}

int test_tracer_handle_is_active(const com_util_tracer *handle)
{
    return handle_is_active(handle);
}

int test_tracer_begin_dispose(com_util_tracer *handle)
{
    return begin_dispose(handle);
}

#if defined(PLATFORM_LINUX)
int test_tracer_to_syslog_level(const com_util_trace_level level)
{
    return to_syslog_level(level);
}
#endif /* PLATFORM_LINUX */

size_t test_tracer_utf8_safe_truncate(const char *text, const size_t position)
{
    return utf8_safe_truncate(text, position);
}

int test_tracer_build_default_file_path(const com_util_tracer *handle, char *out, const size_t out_size)
{
    return build_default_file_path(handle, out, out_size);
}

int test_tracer_hex_write_impl(com_util_tracer *handle, const com_util_trace_level level,
                               const com_util_timespec *timestamp, const void *data, const size_t size,
                               const char *label)
{
    return hex_write_impl(handle, level, timestamp, data, size, label);
}

void test_tracer_set_lifecycle_state(com_util_tracer *handle, const int lifecycle_state)
{
    handle->lifecycle_state = lifecycle_state;
}

void test_tracer_set_running(com_util_tracer *handle, const int running)
{
    handle->running = running;
}

void test_tracer_set_file_handle(com_util_tracer *handle, com_util_trace_file_sink *file_handle)
{
    handle->file_handle = file_handle;
}

void test_tracer_set_hook_fn(com_util_tracer_hook_entry *entry, com_util_tracer_hook_fn fn)
{
    entry->fn = fn;
}

void test_tracer_unregister(com_util_tracer *handle)
{
    (void)registry_unregister_handle(handle);
}

void test_tracer_install_null_fn_hook(com_util_tracer *handle)
{
    com_util_tracer_hook_entry *entry = (com_util_tracer_hook_entry *)calloc(1, sizeof(*entry));
    handle->hook_head = entry;
}

void test_tracer_clear_hook_head(com_util_tracer *handle)
{
    free(handle->hook_head);
    handle->hook_head = NULL;
}

void test_tracer_call_next_null(com_util_tracer *handle)
{
    com_util_tracer_call_next_hook(NULL, handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "x");
}

void test_tracer_call_next_null_fn(com_util_tracer *handle)
{
    com_util_tracer_hook_entry entry = {0};

    com_util_tracer_call_next_hook(&entry, handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "x");
}

void test_tracer_call_next_with_fn(com_util_tracer *handle, com_util_tracer_hook_fn fn)
{
    com_util_tracer_hook_entry entry = {0};

    entry.fn = fn;
    com_util_tracer_call_next_hook(&entry, handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "x");
}

void test_tracer_clear_file_path(com_util_tracer *handle)
{
    free(handle->file_path);
    handle->file_path = NULL;
}
