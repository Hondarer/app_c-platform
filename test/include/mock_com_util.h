#ifndef MOCK_UTIL_H
#define MOCK_UTIL_H

#include <com_util/base/platform.h>
#include <testfw.h>
#include <stdint.h>
#include <time.h>

#if defined(COMPILER_MSVC)
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfprintf")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfopen_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vaccess_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vopen_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vremove_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vmkdir_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vstat_fmt")
#pragma comment(linker, "/INCLUDE:_mock_impl_com_util_console_dispose_on_shutdown")
#endif /* COMPILER_MSVC */

#include <com_util/compress/compress.h>
#include <com_util/crypto/crypto.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/time.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/stdlib.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/string.h>
#include <com_util/crt/unistd.h>
#include <com_util/crt/file.h>
#include <com_util/crt/path.h>
#include <com_util/trace/tracer.h>
#include <com_util/clock/clock.h>
#include <com_util/console/console.h>
#include <com_util/sync/sync.h>
#include <com_util/runtime/module.h>
#include <com_util/runtime/sym_loader.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/trace/trace_file.h>
#include <com_util/trace/syslog.h>
#include <com_util/trace/etw.h>
#include <com_util/prompt/prompt.h>
#include <com_util/prompt/pinned_prompt.h>

inline constexpr char kLibComUtilName[] = "libcom_util" TESTFW_SHARED_LIBRARY_EXTENSION;

// compress
extern int delegate_real_com_util_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);
extern int delegate_real_com_util_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);

// crypto
extern int delegate_real_com_util_encrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_com_util_decrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_com_util_passphrase_to_key(uint8_t *key, const uint8_t *passphrase, size_t passphrase_len);

// crt
extern FILE *delegate_real_com_util_fopen(const char *path, const char *modes, int *errno_out);
extern int delegate_real_com_util_stat(com_util_file_stat_t *buf, const char *path);
extern int delegate_real_com_util_open(const char *path, int flags, int mode);
extern int delegate_real_com_util_access(const char *path, int mode);
extern int delegate_real_com_util_mkdir(const char *path);
extern int delegate_real_com_util_remove(const char *path);
extern int delegate_real_com_util_sscanf(const char *buffer, const char *format, va_list args);
extern int delegate_real_com_util_vsscanf(const char *buffer, const char *format, va_list args);
extern int delegate_real_com_util_gmtime(struct tm *utc_tm, const time_t *timep);
extern int delegate_real_com_util_localtime(struct tm *local_tm, const time_t *timep);
extern int delegate_real_com_util_getenv(const char *name, char *buf, size_t buf_size);
extern int delegate_real_com_util_path_get_full(char *path_out, size_t path_size, int *errno_out, const char *path);
extern int delegate_real_com_util_paths_equal(const char *lhs, const char *rhs, int *errno_out);

// crt - stdio
extern int delegate_real_com_util_rename(const char *oldpath, const char *newpath);
extern int delegate_real_com_util_fclose(FILE *stream);
extern size_t delegate_real_com_util_fread(void *ptr, size_t size, size_t count, FILE *stream);
extern size_t delegate_real_com_util_fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
extern char *delegate_real_com_util_fgets(char *buf, int size, FILE *stream);
extern int delegate_real_com_util_fputs(const char *str, FILE *stream);
extern int delegate_real_com_util_fprintf(FILE *stream, const char *format, ...);
extern int delegate_real_com_util_vfprintf(FILE *stream, const char *format, va_list args);
extern int delegate_real_com_util_fflush(FILE *stream);
extern int delegate_real_com_util_feof(FILE *stream);
extern int delegate_real_com_util_ferror(FILE *stream);
extern void delegate_real_com_util_clearerr(FILE *stream);
extern void delegate_real_com_util_rewind(FILE *stream);
extern int delegate_real_com_util_fseek(FILE *stream, int64_t offset, int whence);
extern int64_t delegate_real_com_util_ftell(FILE *stream);
extern FILE *delegate_real_com_util_fopen_fmt(const char *modes, int *errno_out, const char *format, ...);
extern FILE *delegate_real_com_util_vfopen_fmt(const char *modes, int *errno_out, const char *format, va_list args);
extern int delegate_real_com_util_remove_fmt(const char *format, ...);
extern int delegate_real_com_util_vremove_fmt(const char *format, va_list args);
extern FILE *delegate_real_com_util_fopen_temp(const char *prefix, const char *modes, char *path_out, size_t path_size, int *errno_out);

// crt - unistd
extern int delegate_real_com_util_access_fmt(int mode, const char *format, ...);
extern int delegate_real_com_util_vaccess_fmt(int mode, const char *format, va_list args);

// crt - fcntl
extern int delegate_real_com_util_open_fmt(int flags, int mode, const char *format, ...);
extern int delegate_real_com_util_vopen_fmt(int flags, int mode, const char *format, va_list args);

// crt - string
extern int delegate_real_com_util_strcpy(char *dest, size_t dest_size, const char *src);
extern int delegate_real_com_util_strncpy(char *dest, size_t dest_size, const char *src, size_t count);
extern int delegate_real_com_util_strcat(char *dest, size_t dest_size, const char *src);
extern int delegate_real_com_util_wcscpy(wchar_t *dest, size_t dest_size, const wchar_t *src);

// crt - sys/stat
extern int delegate_real_com_util_stat_fmt(com_util_file_stat_t *buf, const char *format, ...);
extern int delegate_real_com_util_vstat_fmt(com_util_file_stat_t *buf, const char *format, va_list args);
extern int delegate_real_com_util_mkdir_fmt(const char *format, ...);
extern int delegate_real_com_util_vmkdir_fmt(const char *format, va_list args);

// crt - file
extern void delegate_real_com_util_file_init(com_util_file_t *file);
extern int delegate_real_com_util_file_open(com_util_file_t *file, const char *path, uint32_t flags);
extern int delegate_real_com_util_file_write(com_util_file_t *file, const void *buf, size_t len);
extern int delegate_real_com_util_file_get_size(com_util_file_t *file, size_t *size_out);
extern void delegate_real_com_util_file_close(com_util_file_t *file);

// trace - tracer
extern com_util_tracer_t *delegate_real_com_util_tracer_create(void);
extern void delegate_real_com_util_tracer_dispose(com_util_tracer_t *handle);
extern int delegate_real_com_util_tracer_start(com_util_tracer_t *handle);
extern int delegate_real_com_util_tracer_stop(com_util_tracer_t *handle);
extern int delegate_real__com_util_tracer_write(com_util_tracer_t *handle, com_util_trace_level_t level, const com_util_realtime_timestamp_t *timestamp, const char *message);
extern int delegate_real__com_util_tracer_write_hex(com_util_tracer_t *handle, com_util_trace_level_t level, const com_util_realtime_timestamp_t *timestamp, const void *data, size_t size, const char *message);
extern int delegate_real__com_util_tracer_writef(com_util_tracer_t *handle, com_util_trace_level_t level, const com_util_realtime_timestamp_t *timestamp, const char *format, ...);
extern int delegate_real__com_util_tracer_write_hexf(com_util_tracer_t *handle, com_util_trace_level_t level, const com_util_realtime_timestamp_t *timestamp, const void *data, size_t size, const char *format, ...);
extern int delegate_real_com_util_tracer_set_name(com_util_tracer_t *handle, const char *name, int64_t identifier);
extern int delegate_real_com_util_tracer_set_os_level(com_util_tracer_t *handle, com_util_trace_level_t level);
extern int delegate_real_com_util_tracer_set_file_level(com_util_tracer_t *handle, const char *path, com_util_trace_level_t level, size_t max_bytes, int generations);
extern int delegate_real_com_util_tracer_set_stderr_level(com_util_tracer_t *handle, com_util_trace_level_t level);
extern com_util_tracer_hook_entry_t *delegate_real_com_util_tracer_set_hook(com_util_tracer_t *handle, com_util_tracer_hook_fn_t fn, void *context);
extern void delegate_real_com_util_tracer_remove_hook(com_util_tracer_t *handle, com_util_tracer_hook_entry_t *hook_entry);
extern void delegate_real_com_util_tracer_call_next_hook(com_util_tracer_hook_entry_t *prev, com_util_tracer_t *handle, com_util_trace_level_t level, const com_util_realtime_timestamp_t *timestamp, const char *message);
extern com_util_tracer_state_t delegate_real_com_util_tracer_get_state(com_util_tracer_t *handle);
extern com_util_trace_level_t delegate_real_com_util_tracer_get_os_level(com_util_tracer_t *handle);
extern com_util_trace_level_t delegate_real_com_util_tracer_get_file_level(com_util_tracer_t *handle);
extern com_util_trace_level_t delegate_real_com_util_tracer_get_stderr_level(com_util_tracer_t *handle);

// clock
extern uint64_t delegate_real_com_util_get_monotonic_ms(void);
extern void delegate_real_com_util_get_monotonic(int64_t *tv_sec, int32_t *tv_nsec);
extern void delegate_real_com_util_get_realtime(int64_t *tv_sec, int32_t *tv_nsec);
extern void delegate_real_com_util_get_realtime_utc(struct tm *utc_tm, int32_t *tv_nsec);
extern int delegate_real_com_util_format_realtime_iso8601_local(char *buf, size_t buf_size, int64_t tv_sec, int32_t tv_nsec);
extern int delegate_real_com_util_format_realtime_iso8601_utc(char *buf, size_t buf_size, int64_t tv_sec, int32_t tv_nsec);
extern void delegate_real_com_util_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout);

// console
extern void delegate_real_com_util_console_init(void);
extern void delegate_real_com_util_console_dispose(void);
extern void delegate_real_com_util_console_dispose_on_shutdown(const com_util_shutdown_event_t *event, void *context);

// sync
extern com_util_sync_result_t delegate_real_com_util_local_lock_create(com_util_local_lock_t **mtx);
extern com_util_sync_result_t delegate_real_com_util_local_lock_lock(com_util_local_lock_t *mtx, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_local_lock_try_lock(com_util_local_lock_t *mtx);
extern com_util_sync_result_t delegate_real_com_util_local_lock_unlock(com_util_local_lock_t *mtx);
extern void delegate_real_com_util_local_lock_destroy(com_util_local_lock_t *mtx);
extern com_util_sync_result_t delegate_real_com_util_condvar_create(com_util_condvar_t **cv);
extern com_util_sync_result_t delegate_real_com_util_condvar_wait(com_util_condvar_t *cv, com_util_local_lock_t *mtx, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_condvar_signal(com_util_condvar_t *cv);
extern com_util_sync_result_t delegate_real_com_util_condvar_broadcast(com_util_condvar_t *cv);
extern void delegate_real_com_util_condvar_destroy(com_util_condvar_t *cv);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_create(com_util_local_rwlock_t **rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_lock_shared(com_util_local_rwlock_t *rwlock, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_try_lock_shared(com_util_local_rwlock_t *rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_lock_exclusive(com_util_local_rwlock_t *rwlock, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock_t *rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_unlock_shared(com_util_local_rwlock_t *rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock_t *rwlock);
extern void delegate_real_com_util_local_rwlock_destroy(com_util_local_rwlock_t *rwlock);
extern com_util_sync_result_t delegate_real_com_util_thread_create(com_util_thread_t **thread, com_util_thread_func_t func, void *arg);
extern com_util_sync_result_t delegate_real_com_util_thread_join(com_util_thread_t *thread, int timeout_ms);
extern void delegate_real_com_util_thread_detach(com_util_thread_t *thread);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_open(const char *identity, com_util_interprocess_lock_t **lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size, com_util_interprocess_lock_t **lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_export_descriptor(const com_util_interprocess_lock_t *lock, void *descriptor, size_t *descriptor_size);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_lock(com_util_interprocess_lock_t *lock, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_try_lock(com_util_interprocess_lock_t *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_unlock(com_util_interprocess_lock_t *lock);
extern void delegate_real_com_util_interprocess_lock_destroy(com_util_interprocess_lock_t *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_open(const char *identity, com_util_interprocess_rwlock_t **lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size, com_util_interprocess_rwlock_t **lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_export_descriptor(const com_util_interprocess_rwlock_t *lock, void *descriptor, size_t *descriptor_size);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock_t *lock, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_try_lock_shared(com_util_interprocess_rwlock_t *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_lock_exclusive(com_util_interprocess_rwlock_t *lock, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock_t *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock_t *lock);
extern void delegate_real_com_util_interprocess_rwlock_destroy(com_util_interprocess_rwlock_t *lock);
extern void delegate_real_com_util_call_once(com_util_once_flag_t *flag, com_util_once_func_t func);
extern void delegate_real_com_util_sleep_ms(int ms);

// runtime - module_info
extern int delegate_real_com_util_module_get_path(char *out_path, size_t out_path_sz, const void *func_addr);
extern int delegate_real_com_util_module_get_basename(char *out_basename, size_t out_basename_sz, const void *func_addr);

// runtime - sym_loader
extern void *delegate_real_com_util_sym_loader_resolve(com_util_sym_loader_entry_t *fobj);
extern int delegate_real_com_util_sym_loader_is_default(com_util_sym_loader_entry_t *fobj);
extern void delegate_real_com_util_sym_loader_init(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length, const char *configpath);
extern void delegate_real_com_util_sym_loader_dispose(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length);
extern int delegate_real_com_util_sym_loader_info(com_util_sym_loader_entry_t *const *fobj_array, size_t fobj_length);

// runtime - shutdown
extern int delegate_real_com_util_shutdown_register(com_util_shutdown_callback_t callback, void *context);
extern int delegate_real_com_util_shutdown_request_register(com_util_shutdown_callback_t callback, void *context);

// trace - trace_file_sink
extern com_util_trace_file_sink_t *delegate_real_com_util_trace_file_sink_create(const char *path, size_t max_bytes, int generations);
extern int delegate_real_com_util_trace_file_sink_write(com_util_trace_file_sink_t *handle, int level, const com_util_realtime_timestamp_t *timestamp, const char *message);
extern void delegate_real_com_util_trace_file_sink_dispose(com_util_trace_file_sink_t *handle);

#if defined(PLATFORM_LINUX)
// trace - syslog_sink (Linux only)
extern com_util_syslog_sink_t *delegate_real_com_util_syslog_sink_create(const char *ident, int facility);
extern int delegate_real_com_util_syslog_sink_write(com_util_syslog_sink_t *handle, int level, const com_util_realtime_timestamp_t *timestamp, const char *message);
extern int delegate_real_com_util_syslog_sink_rename(com_util_syslog_sink_t *handle, const char *new_ident);
extern void delegate_real_com_util_syslog_sink_dispose(com_util_syslog_sink_t *handle);
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
// trace - etw (Windows only)
extern com_util_etw_provider_t *delegate_real_com_util_etw_provider_create(com_util_etw_provider_ref_t provider_ref);
extern void delegate_real_com_util_etw_provider_dispose(com_util_etw_provider_t *handle);
extern int delegate_real_com_util_etw_provider_write(com_util_etw_provider_t *handle, int level, const char *service, const char *message);
extern int delegate_real_com_util_etw_session_check_access(void);
extern com_util_etw_session_t *delegate_real_com_util_etw_session_start(const char *session_name, const char *provider_guid_str, com_util_etw_event_callback_t callback, void *context, int *out_status);
extern void delegate_real_com_util_etw_session_stop(com_util_etw_session_t *session);
#endif /* PLATFORM_WINDOWS */

// prompt
extern com_util_prompt_t *delegate_real_com_util_prompt_create(const com_util_prompt_options_t *options);
extern void delegate_real_com_util_prompt_dispose(com_util_prompt_t *prompt);
extern int delegate_real_com_util_prompt_readline_at(com_util_prompt_t *p, char *buf, size_t buf_size, const char *prompt_str, const char *file, int line);
extern int delegate_real_com_util_prompt_readline_fmt_at(com_util_prompt_t *p, char *buf, size_t buf_size, const char *file, int line, const char *fmt, va_list args);
extern com_util_pinned_prompt_t *delegate_real_com_util_pinned_prompt_create(const com_util_pinned_prompt_options_t *options);
extern void delegate_real_com_util_pinned_prompt_dispose(com_util_pinned_prompt_t *screen);
extern int delegate_real__com_util_pinned_prompt_readline(com_util_pinned_prompt_t *screen, char *buf, size_t buf_size, const char *prompt_str, const char *file, int line);
extern int delegate_real__com_util_pinned_prompt_readline_fmt(com_util_pinned_prompt_t *screen, char *buf, size_t buf_size, const char *file, int line, const char *fmt, va_list args);
extern size_t delegate_real_com_util_pinned_prompt_write(com_util_pinned_prompt_t *screen, com_util_pinned_prompt_channel_t channel, const void *data, size_t size);
extern int delegate_real_com_util_pinned_prompt_printf(com_util_pinned_prompt_t *screen, com_util_pinned_prompt_channel_t channel, const char *fmt, ...);
extern int delegate_real_com_util_pinned_prompt_status_enable(com_util_pinned_prompt_t *screen, com_util_pinned_prompt_status_position_t position, int enable);
extern int delegate_real_com_util_pinned_prompt_status_set(com_util_pinned_prompt_t *screen, com_util_pinned_prompt_status_position_t position, com_util_pinned_prompt_status_align_t align, const char *content);

class Mock_com_util
{
  public:
    // compress
    MOCK_METHOD(int, com_util_compress, (uint8_t *, size_t *, const uint8_t *, size_t));
    MOCK_METHOD(int, com_util_decompress, (uint8_t *, size_t *, const uint8_t *, size_t));

    // crypto
    MOCK_METHOD(int, com_util_encrypt,
                (uint8_t *, size_t *, const uint8_t *, size_t, const uint8_t *, const uint8_t *, const uint8_t *,
                 size_t));
    MOCK_METHOD(int, com_util_decrypt,
                (uint8_t *, size_t *, const uint8_t *, size_t, const uint8_t *, const uint8_t *, const uint8_t *,
                 size_t));
    MOCK_METHOD(int, com_util_passphrase_to_key, (uint8_t *, const uint8_t *, size_t));

    // crt
    MOCK_METHOD(FILE *, com_util_fopen, (const char *, const char *, int *));
    MOCK_METHOD(int, com_util_stat, (com_util_file_stat_t *, const char *));
    MOCK_METHOD(int, com_util_open, (const char *, int, int));
    MOCK_METHOD(int, com_util_access, (const char *, int));
    MOCK_METHOD(int, com_util_mkdir, (const char *));
    MOCK_METHOD(int, com_util_remove, (const char *));
    MOCK_METHOD(int, com_util_sscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int, com_util_vsscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int, com_util_gmtime, (struct tm *, const time_t *));
    MOCK_METHOD(int, com_util_localtime, (struct tm *, const time_t *));
    MOCK_METHOD(int, com_util_getenv, (const char *, char *, size_t));
    MOCK_METHOD(int, com_util_path_get_full, (char *, size_t, int *, const char *));
    MOCK_METHOD(int, com_util_paths_equal, (const char *, const char *, int *));

    // crt - stdio
    MOCK_METHOD(int, com_util_rename, (const char *, const char *));
    MOCK_METHOD(int, com_util_fclose, (FILE *));
    MOCK_METHOD(size_t, com_util_fread, (void *, size_t, size_t, FILE *));
    MOCK_METHOD(size_t, com_util_fwrite, (const void *, size_t, size_t, FILE *));
    MOCK_METHOD(char *, com_util_fgets, (char *, int, FILE *));
    MOCK_METHOD(int, com_util_fputs, (const char *, FILE *));
    MOCK_METHOD(int, com_util_fprintf, (FILE *, const char *));
    MOCK_METHOD(int, com_util_vfprintf, (FILE *, const char *));
    MOCK_METHOD(int, com_util_fflush, (FILE *));
    MOCK_METHOD(int, com_util_feof, (FILE *));
    MOCK_METHOD(int, com_util_ferror, (FILE *));
    MOCK_METHOD(void, com_util_clearerr, (FILE *));
    MOCK_METHOD(void, com_util_rewind, (FILE *));
    MOCK_METHOD(int, com_util_fseek, (FILE *, int64_t, int));
    MOCK_METHOD(int64_t, com_util_ftell, (FILE *));
    MOCK_METHOD(FILE *, com_util_fopen_fmt, (const char *, int *, const char *));
    MOCK_METHOD(FILE *, com_util_vfopen_fmt, (const char *, int *, const char *));
    MOCK_METHOD(int, com_util_remove_fmt, (const char *));
    MOCK_METHOD(int, com_util_vremove_fmt, (const char *));
    MOCK_METHOD(FILE *, com_util_fopen_temp, (const char *, const char *, char *, size_t, int *));

    // crt - unistd
    MOCK_METHOD(int, com_util_access_fmt, (int, const char *));
    MOCK_METHOD(int, com_util_vaccess_fmt, (int, const char *));

    // crt - fcntl
    MOCK_METHOD(int, com_util_open_fmt, (int, int, const char *));
    MOCK_METHOD(int, com_util_vopen_fmt, (int, int, const char *));

    // crt - string
    MOCK_METHOD(int, com_util_strcpy, (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_strncpy, (char *, size_t, const char *, size_t));
    MOCK_METHOD(int, com_util_strcat, (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_wcscpy, (wchar_t *, size_t, const wchar_t *));

    // crt - sys/stat
    MOCK_METHOD(int, com_util_stat_fmt, (com_util_file_stat_t *, const char *));
    MOCK_METHOD(int, com_util_vstat_fmt, (com_util_file_stat_t *, const char *));
    MOCK_METHOD(int, com_util_mkdir_fmt, (const char *));
    MOCK_METHOD(int, com_util_vmkdir_fmt, (const char *));

    // crt - file
    MOCK_METHOD(void, com_util_file_init, (com_util_file_t *));
    MOCK_METHOD(int, com_util_file_open, (com_util_file_t *, const char *, uint32_t));
    MOCK_METHOD(int, com_util_file_write, (com_util_file_t *, const void *, size_t));
    MOCK_METHOD(int, com_util_file_get_size, (com_util_file_t *, size_t *));
    MOCK_METHOD(void, com_util_file_close, (com_util_file_t *));

    // trace - tracer
    MOCK_METHOD(com_util_tracer_t *, com_util_tracer_create, ());
    MOCK_METHOD(void, com_util_tracer_dispose, (com_util_tracer_t *));
    MOCK_METHOD(int, com_util_tracer_start, (com_util_tracer_t *));
    MOCK_METHOD(int, com_util_tracer_stop, (com_util_tracer_t *));
    MOCK_METHOD(int, _com_util_tracer_write,
                (com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *, const char *));
    MOCK_METHOD(int, _com_util_tracer_write_hex,
                (com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *, const void *,
                 size_t, const char *));
    MOCK_METHOD(int, _com_util_tracer_writef,
                (com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *, const char *));
    MOCK_METHOD(int, _com_util_tracer_write_hexf,
                (com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *, const void *,
                 size_t, const char *));
    MOCK_METHOD(int, com_util_tracer_set_name, (com_util_tracer_t *, const char *, int64_t));
    MOCK_METHOD(int, com_util_tracer_set_os_level, (com_util_tracer_t *, com_util_trace_level_t));
    MOCK_METHOD(int, com_util_tracer_set_file_level,
                (com_util_tracer_t *, const char *, com_util_trace_level_t, size_t, int));
    MOCK_METHOD(int, com_util_tracer_set_stderr_level, (com_util_tracer_t *, com_util_trace_level_t));
    MOCK_METHOD(com_util_tracer_hook_entry_t *, com_util_tracer_set_hook,
                (com_util_tracer_t *, com_util_tracer_hook_fn_t, void *));
    MOCK_METHOD(void, com_util_tracer_remove_hook, (com_util_tracer_t *, com_util_tracer_hook_entry_t *));
    MOCK_METHOD(void, com_util_tracer_call_next_hook,
                (com_util_tracer_hook_entry_t *, com_util_tracer_t *, com_util_trace_level_t,
                 const com_util_realtime_timestamp_t *, const char *));
    MOCK_METHOD(com_util_tracer_state_t, com_util_tracer_get_state, (com_util_tracer_t *));
    MOCK_METHOD(com_util_trace_level_t, com_util_tracer_get_os_level, (com_util_tracer_t *));
    MOCK_METHOD(com_util_trace_level_t, com_util_tracer_get_file_level, (com_util_tracer_t *));
    MOCK_METHOD(com_util_trace_level_t, com_util_tracer_get_stderr_level, (com_util_tracer_t *));

    // clock
    MOCK_METHOD(uint64_t, com_util_get_monotonic_ms, ());
    MOCK_METHOD(void, com_util_get_monotonic, (int64_t *, int32_t *));
    MOCK_METHOD(void, com_util_get_realtime, (int64_t *, int32_t *));
    MOCK_METHOD(void, com_util_get_realtime_utc, (struct tm *, int32_t *));
    MOCK_METHOD(int, com_util_format_realtime_iso8601_local, (char *, size_t, int64_t, int32_t));
    MOCK_METHOD(int, com_util_format_realtime_iso8601_utc, (char *, size_t, int64_t, int32_t));
    MOCK_METHOD(void, com_util_get_realtime_deadline_ms, (uint64_t, struct timespec *));

    // console
    MOCK_METHOD(void, com_util_console_init, ());
    MOCK_METHOD(void, com_util_console_dispose, ());
    MOCK_METHOD(void, com_util_console_dispose_on_shutdown, (const com_util_shutdown_event_t *, void *));

    // sync
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_create, (com_util_local_lock_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_lock, (com_util_local_lock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_try_lock, (com_util_local_lock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_unlock, (com_util_local_lock_t *));
    MOCK_METHOD(void, com_util_local_lock_destroy, (com_util_local_lock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_create, (com_util_condvar_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_wait, (com_util_condvar_t *, com_util_local_lock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_signal, (com_util_condvar_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_broadcast, (com_util_condvar_t *));
    MOCK_METHOD(void, com_util_condvar_destroy, (com_util_condvar_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_create, (com_util_local_rwlock_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_lock_shared, (com_util_local_rwlock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_try_lock_shared, (com_util_local_rwlock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_lock_exclusive, (com_util_local_rwlock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_try_lock_exclusive, (com_util_local_rwlock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_unlock_shared, (com_util_local_rwlock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_unlock_exclusive, (com_util_local_rwlock_t *));
    MOCK_METHOD(void, com_util_local_rwlock_destroy, (com_util_local_rwlock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_thread_create, (com_util_thread_t **, com_util_thread_func_t, void *));
    MOCK_METHOD(com_util_sync_result_t, com_util_thread_join, (com_util_thread_t *, int));
    MOCK_METHOD(void, com_util_thread_detach, (com_util_thread_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_open, (const char *, com_util_interprocess_lock_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_import_descriptor, (const void *, size_t, com_util_interprocess_lock_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_export_descriptor, (const com_util_interprocess_lock_t *, void *, size_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_lock, (com_util_interprocess_lock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_try_lock, (com_util_interprocess_lock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_unlock, (com_util_interprocess_lock_t *));
    MOCK_METHOD(void, com_util_interprocess_lock_destroy, (com_util_interprocess_lock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_open, (const char *, com_util_interprocess_rwlock_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_import_descriptor, (const void *, size_t, com_util_interprocess_rwlock_t **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_export_descriptor, (const com_util_interprocess_rwlock_t *, void *, size_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_lock_shared, (com_util_interprocess_rwlock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_try_lock_shared, (com_util_interprocess_rwlock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_lock_exclusive, (com_util_interprocess_rwlock_t *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_try_lock_exclusive, (com_util_interprocess_rwlock_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_unlock, (com_util_interprocess_rwlock_t *));
    MOCK_METHOD(void, com_util_interprocess_rwlock_destroy, (com_util_interprocess_rwlock_t *));
    MOCK_METHOD(void, com_util_call_once, (com_util_once_flag_t *, com_util_once_func_t));
    MOCK_METHOD(void, com_util_sleep_ms, (int));

    // runtime - module_info
    MOCK_METHOD(int, com_util_module_get_path, (char *, size_t, const void *));
    MOCK_METHOD(int, com_util_module_get_basename, (char *, size_t, const void *));

    // runtime - sym_loader
    MOCK_METHOD(void *, com_util_sym_loader_resolve, (com_util_sym_loader_entry_t *));
    MOCK_METHOD(int, com_util_sym_loader_is_default, (com_util_sym_loader_entry_t *));
    MOCK_METHOD(void, com_util_sym_loader_init, (com_util_sym_loader_entry_t *const *, size_t, const char *));
    MOCK_METHOD(void, com_util_sym_loader_dispose, (com_util_sym_loader_entry_t *const *, size_t));
    MOCK_METHOD(int, com_util_sym_loader_info, (com_util_sym_loader_entry_t *const *, size_t));

    // runtime - shutdown
    MOCK_METHOD(int, com_util_shutdown_register, (com_util_shutdown_callback_t, void *));
    MOCK_METHOD(int, com_util_shutdown_request_register, (com_util_shutdown_callback_t, void *));

    // trace - trace_file_sink
    MOCK_METHOD(com_util_trace_file_sink_t *, com_util_trace_file_sink_create, (const char *, size_t, int));
    MOCK_METHOD(int, com_util_trace_file_sink_write,
                (com_util_trace_file_sink_t *, int, const com_util_realtime_timestamp_t *, const char *));
    MOCK_METHOD(void, com_util_trace_file_sink_dispose, (com_util_trace_file_sink_t *));

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    MOCK_METHOD(com_util_syslog_sink_t *, com_util_syslog_sink_create, (const char *, int));
    MOCK_METHOD(int, com_util_syslog_sink_write,
                (com_util_syslog_sink_t *, int, const com_util_realtime_timestamp_t *, const char *));
    MOCK_METHOD(int, com_util_syslog_sink_rename, (com_util_syslog_sink_t *, const char *));
    MOCK_METHOD(void, com_util_syslog_sink_dispose, (com_util_syslog_sink_t *));
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
    // trace - trace_etw (Windows only)
    MOCK_METHOD(com_util_etw_provider_t *, com_util_etw_provider_create, (com_util_etw_provider_ref_t));
    MOCK_METHOD(int, com_util_etw_provider_write, (com_util_etw_provider_t *, int, const char *, const char *));
    MOCK_METHOD(void, com_util_etw_provider_dispose, (com_util_etw_provider_t *));
    MOCK_METHOD(int, com_util_etw_session_check_access, ());
    MOCK_METHOD(com_util_etw_session_t *, com_util_etw_session_start,
                (const char *, const char *, com_util_etw_event_callback_t, void *, int *));
    MOCK_METHOD(void, com_util_etw_session_stop, (com_util_etw_session_t *));
#endif /* PLATFORM_WINDOWS */

    // prompt
    MOCK_METHOD(com_util_prompt_t *, com_util_prompt_create, (const com_util_prompt_options_t *));
    MOCK_METHOD(void, com_util_prompt_dispose, (com_util_prompt_t *));
    MOCK_METHOD(int, com_util_prompt_readline_at, (com_util_prompt_t *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, com_util_prompt_readline_fmt_at,
                (com_util_prompt_t *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(com_util_pinned_prompt_t *, com_util_pinned_prompt_create, (const com_util_pinned_prompt_options_t *));
    MOCK_METHOD(void, com_util_pinned_prompt_dispose, (com_util_pinned_prompt_t *));
    MOCK_METHOD(int, _com_util_pinned_prompt_readline,
                (com_util_pinned_prompt_t *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, _com_util_pinned_prompt_readline_fmt,
                (com_util_pinned_prompt_t *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(size_t, com_util_pinned_prompt_write,
                (com_util_pinned_prompt_t *, com_util_pinned_prompt_channel_t, const void *, size_t));
    MOCK_METHOD(int, com_util_pinned_prompt_printf,
                (com_util_pinned_prompt_t *, com_util_pinned_prompt_channel_t, const char *));
    MOCK_METHOD(int, com_util_pinned_prompt_status_enable,
                (com_util_pinned_prompt_t *, com_util_pinned_prompt_status_position_t, int));
    MOCK_METHOD(int, com_util_pinned_prompt_status_set,
                (com_util_pinned_prompt_t *, com_util_pinned_prompt_status_position_t,
                 com_util_pinned_prompt_status_align_t, const char *));

    Mock_com_util();
    ~Mock_com_util();
};

extern Mock_com_util *_mock_com_util;

#endif /* MOCK_UTIL_H */
