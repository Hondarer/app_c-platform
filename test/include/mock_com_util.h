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
#include <com_util/runtime/process.h>
#include <com_util/runtime/sym_loader.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/crt/wchar_conv.h>
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
extern int delegate_real_com_util_encrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                          const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_com_util_decrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                          const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_com_util_passphrase_to_key(uint8_t *key, const uint8_t *passphrase, size_t passphrase_len);

// crt
extern FILE *delegate_real_com_util_fopen(const char *path, const char *modes, int *errno_out);
extern int delegate_real_com_util_stat(com_util_file_stat_t *buf, const char *path);
extern int delegate_real_com_util_open(const char *path, int flags, int mode);
extern int delegate_real_com_util_access(const char *path, int mode);
extern int delegate_real_com_util_mkdir(const char *path);
extern int delegate_real_com_util_makedirs(const char *path);
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
extern FILE *delegate_real_com_util_fopen_temp(const char *prefix, const char *modes, char *path_out, size_t path_size,
                                               int *errno_out);

// crt - unistd
extern int delegate_real_com_util_isatty(com_util_stream_t stream);
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
extern void delegate_real_com_util_file_init(com_util_file *file);
extern int delegate_real_com_util_file_open(com_util_file *file, const char *path, int flags);
extern int delegate_real_com_util_file_write(com_util_file *file, const void *buf, size_t len);
extern int delegate_real_com_util_file_get_size(const com_util_file *file, size_t *size_out);
extern int delegate_real_com_util_file_get_id(const com_util_file *file, com_util_file_id *id_out);
extern int delegate_real_com_util_file_get_path_id(const char *path, com_util_file_id *id_out);
extern void delegate_real_com_util_file_close(com_util_file *file);

// trace - tracer
extern com_util_tracer *delegate_real_com_util_tracer_create(void);
extern void delegate_real_com_util_tracer_dispose(com_util_tracer *handle);
extern int delegate_real_com_util_tracer_start(com_util_tracer *handle);
extern int delegate_real_com_util_tracer_stop(com_util_tracer *handle);
extern int delegate_real__com_util_tracer_write(com_util_tracer *handle, com_util_trace_level_t level,
                                                const com_util_realtime_timestamp *timestamp, const char *message);
extern int delegate_real__com_util_tracer_write_hex(com_util_tracer *handle, com_util_trace_level_t level,
                                                    const com_util_realtime_timestamp *timestamp, const void *data,
                                                    size_t size, const char *message);
extern int delegate_real__com_util_tracer_writef(com_util_tracer *handle, com_util_trace_level_t level,
                                                 const com_util_realtime_timestamp *timestamp, const char *format, ...);
extern int delegate_real__com_util_tracer_write_hexf(com_util_tracer *handle, com_util_trace_level_t level,
                                                     const com_util_realtime_timestamp *timestamp, const void *data,
                                                     size_t size, const char *format, ...);
extern int delegate_real_com_util_tracer_set_name(com_util_tracer *handle, const char *name, int64_t identifier);
extern int delegate_real_com_util_tracer_set_os_level(com_util_tracer *handle, com_util_trace_level_t level);
extern int delegate_real_com_util_tracer_set_file_level(com_util_tracer *handle, const char *path,
                                                        com_util_trace_level_t level, size_t max_bytes, int generations,
                                                        int flags);
extern int delegate_real_com_util_tracer_set_stderr_level(com_util_tracer *handle, com_util_trace_level_t level);
extern com_util_tracer_hook_entry *delegate_real_com_util_tracer_set_hook(com_util_tracer *handle,
                                                                          com_util_tracer_hook_fn_t fn, void *context);
extern void delegate_real_com_util_tracer_remove_hook(com_util_tracer *handle, com_util_tracer_hook_entry *hook_entry);
extern void delegate_real_com_util_tracer_call_next_hook(com_util_tracer_hook_entry *prev, com_util_tracer *handle,
                                                         com_util_trace_level_t level,
                                                         const com_util_realtime_timestamp *timestamp,
                                                         const char *message);
extern com_util_tracer_state_t delegate_real_com_util_tracer_get_state(com_util_tracer *handle);
extern com_util_trace_level_t delegate_real_com_util_tracer_get_os_level(com_util_tracer *handle);
extern com_util_trace_level_t delegate_real_com_util_tracer_get_file_level(com_util_tracer *handle);
extern com_util_trace_level_t delegate_real_com_util_tracer_get_stderr_level(com_util_tracer *handle);

// clock
extern uint64_t delegate_real_com_util_get_monotonic_ms(void);
extern void delegate_real_com_util_get_monotonic(int64_t *tv_sec, int32_t *tv_nsec);
extern void delegate_real_com_util_get_realtime(int64_t *tv_sec, int32_t *tv_nsec);
extern void delegate_real_com_util_get_realtime_utc(struct tm *utc_tm, int32_t *tv_nsec);
extern int delegate_real_com_util_format_realtime_iso8601_local(char *buf, size_t buf_size, int64_t tv_sec,
                                                                int32_t tv_nsec);
extern int delegate_real_com_util_format_realtime_iso8601_utc(char *buf, size_t buf_size, int64_t tv_sec,
                                                              int32_t tv_nsec);
extern void delegate_real_com_util_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout);

// console
extern void delegate_real_com_util_console_init(void);
extern void delegate_real_com_util_console_dispose(void);
extern void delegate_real_com_util_console_dispose_on_shutdown(const com_util_shutdown_event *event, void *context);

// sync
extern com_util_sync_result_t delegate_real_com_util_local_lock_create(com_util_local_lock **mtx);
extern com_util_sync_result_t delegate_real_com_util_local_lock_lock(com_util_local_lock *mtx, int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_local_lock_try_lock(com_util_local_lock *mtx);
extern com_util_sync_result_t delegate_real_com_util_local_lock_unlock(com_util_local_lock *mtx);
extern void delegate_real_com_util_local_lock_destroy(com_util_local_lock *mtx);
extern com_util_sync_result_t delegate_real_com_util_condvar_create(com_util_condvar **cv);
extern com_util_sync_result_t delegate_real_com_util_condvar_wait(com_util_condvar *cv, com_util_local_lock *mtx,
                                                                  int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_condvar_signal(com_util_condvar *cv);
extern com_util_sync_result_t delegate_real_com_util_condvar_broadcast(com_util_condvar *cv);
extern void delegate_real_com_util_condvar_destroy(com_util_condvar *cv);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_create(com_util_local_rwlock **rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_lock_shared(com_util_local_rwlock *rwlock,
                                                                              int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_try_lock_shared(com_util_local_rwlock *rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_lock_exclusive(com_util_local_rwlock *rwlock,
                                                                                 int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock *rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_unlock_shared(com_util_local_rwlock *rwlock);
extern com_util_sync_result_t delegate_real_com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock *rwlock);
extern void delegate_real_com_util_local_rwlock_destroy(com_util_local_rwlock *rwlock);
extern com_util_sync_result_t delegate_real_com_util_thread_create(com_util_thread **thread,
                                                                   com_util_thread_func_t func, void *arg);
extern com_util_sync_result_t delegate_real_com_util_thread_join(com_util_thread *thread, int timeout_ms);
extern void delegate_real_com_util_thread_detach(com_util_thread *thread);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_open(const char *identity,
                                                                            com_util_interprocess_lock **lock);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                           com_util_interprocess_lock **lock);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_lock_export_descriptor(const com_util_interprocess_lock *lock, void *descriptor,
                                                           size_t *descriptor_size);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_lock(com_util_interprocess_lock *lock,
                                                                            int timeout_ms);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_try_lock(com_util_interprocess_lock *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_lock_unlock(com_util_interprocess_lock *lock);
extern void delegate_real_com_util_interprocess_lock_destroy(com_util_interprocess_lock *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_open(const char *identity,
                                                                              com_util_interprocess_rwlock **lock);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                             com_util_interprocess_rwlock **lock);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_rwlock_export_descriptor(const com_util_interprocess_rwlock *lock, void *descriptor,
                                                             size_t *descriptor_size);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock *lock,
                                                                                     int timeout_ms);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_rwlock_try_lock_shared(com_util_interprocess_rwlock *lock);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_rwlock_lock_exclusive(com_util_interprocess_rwlock *lock, int timeout_ms);
extern com_util_sync_result_t
delegate_real_com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock *lock);
extern com_util_sync_result_t delegate_real_com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock *lock);
extern void delegate_real_com_util_interprocess_rwlock_destroy(com_util_interprocess_rwlock *lock);
extern void delegate_real_com_util_call_once(com_util_once_flag *flag, com_util_once_func_t func);
extern void delegate_real_com_util_sleep_ms(int ms);

// runtime - module_info
extern int delegate_real_com_util_module_get_path(char *out_path, size_t out_path_sz, const void *func_addr);
extern int delegate_real_com_util_module_get_basename(char *out_basename, size_t out_basename_sz,
                                                      const void *func_addr);

// runtime - process_info
extern int delegate_real_com_util_process_run_elevated_if_needed(const char *arguments, int *exit_code, int *handled);
extern com_util_process_result_t delegate_real_com_util_process_start(const com_util_process_options_t *options,
                                                                      com_util_process **process);
extern com_util_process_result_t delegate_real_com_util_process_wait(com_util_process *process, int timeout_ms);
extern com_util_process_result_t delegate_real_com_util_process_get_exit_code(com_util_process *process,
                                                                              int *exit_code);
extern com_util_process_result_t delegate_real_com_util_process_terminate(com_util_process *process);
extern void delegate_real_com_util_process_destroy(com_util_process *process);
extern com_util_process_result_t delegate_real_com_util_process_run_sync(const com_util_process_options_t *options,
                                                                         int timeout_ms, int *exit_code);

// runtime - sym_loader
extern void *delegate_real_com_util_sym_loader_resolve(com_util_sym_loader_entry *fobj);
extern int delegate_real_com_util_sym_loader_is_default(com_util_sym_loader_entry *fobj);
extern void delegate_real_com_util_sym_loader_init(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length,
                                                   const char *configpath);
extern void delegate_real_com_util_sym_loader_dispose(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length);
extern int delegate_real_com_util_sym_loader_info(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length);

// runtime - shutdown
extern int delegate_real_com_util_shutdown_register(com_util_shutdown_callback_t callback, void *context);
extern int delegate_real_com_util_shutdown_request_register(com_util_shutdown_callback_t callback, void *context);

// trace - trace_file_sink
extern com_util_trace_file_sink *delegate_real_com_util_trace_file_sink_create(const char *path, size_t max_bytes,
                                                                               int generations, int flags);
extern int delegate_real_com_util_trace_file_sink_write(com_util_trace_file_sink *handle, int level,
                                                        const com_util_realtime_timestamp *timestamp,
                                                        const char *message);
extern void delegate_real_com_util_trace_file_sink_dispose(com_util_trace_file_sink *handle);

#if defined(PLATFORM_LINUX)
// trace - syslog_sink (Linux only)
extern com_util_syslog_sink *delegate_real_com_util_syslog_sink_create(const char *ident, int facility);
extern int delegate_real_com_util_syslog_sink_write(com_util_syslog_sink *handle, int level,
                                                    const com_util_realtime_timestamp *timestamp, const char *message);
extern int delegate_real_com_util_syslog_sink_rename(com_util_syslog_sink *handle, const char *new_ident);
extern void delegate_real_com_util_syslog_sink_dispose(com_util_syslog_sink *handle);
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
// crt - wchar_conv (Windows only)
extern int delegate_real_com_util_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path);
extern int delegate_real_com_util_wpath_to_utf8(char *out, size_t out_size, const wchar_t *wpath);
extern wchar_t *delegate_real_com_util_utf8_to_wstr_alloc(const char *utf8_text);
extern char *delegate_real_com_util_wstr_to_utf8_alloc(const wchar_t *wtext);
#endif /* PLATFORM_WINDOWS */

#if defined(PLATFORM_WINDOWS)
// trace - etw (Windows only)
extern com_util_etw_provider *delegate_real_com_util_etw_provider_create(com_util_etw_provider_ref_t provider_ref);
extern void delegate_real_com_util_etw_provider_dispose(com_util_etw_provider *handle);
extern int delegate_real_com_util_etw_provider_write(com_util_etw_provider *handle, int level, const char *service,
                                                     const char *message);
extern int delegate_real_com_util_etw_session_check_access(void);
extern com_util_etw_session *delegate_real_com_util_etw_session_start(const char *session_name,
                                                                      const char *provider_guid_str,
                                                                      com_util_etw_event_callback_t callback,
                                                                      void *context, int *out_status);
extern void delegate_real_com_util_etw_session_stop(com_util_etw_session *session);
#endif /* PLATFORM_WINDOWS */

// prompt
extern com_util_prompt *delegate_real_com_util_prompt_create(const com_util_prompt_options *options);
extern void delegate_real_com_util_prompt_dispose(com_util_prompt *prompt);
extern int delegate_real_com_util_prompt_readline_at(com_util_prompt *p, char *buf, size_t buf_size,
                                                     const char *prompt_str, const char *file, int line);
extern int delegate_real_com_util_prompt_readline_fmt_at(com_util_prompt *p, char *buf, size_t buf_size,
                                                         const char *file, int line, const char *fmt, va_list args);
extern com_util_pinned_prompt *
delegate_real_com_util_pinned_prompt_create(const com_util_pinned_prompt_options *options);
extern void delegate_real_com_util_pinned_prompt_dispose(com_util_pinned_prompt *screen);
extern int delegate_real__com_util_pinned_prompt_readline(com_util_pinned_prompt *screen, char *buf, size_t buf_size,
                                                          const char *prompt_str, const char *file, int line);
extern int delegate_real__com_util_pinned_prompt_readline_fmt(com_util_pinned_prompt *screen, char *buf,
                                                              size_t buf_size, const char *file, int line,
                                                              const char *fmt, va_list args);
extern size_t delegate_real_com_util_pinned_prompt_write(com_util_pinned_prompt *screen,
                                                         com_util_pinned_prompt_channel_t channel, const void *data,
                                                         size_t size);
extern int delegate_real_com_util_pinned_prompt_printf(com_util_pinned_prompt *screen,
                                                       com_util_pinned_prompt_channel_t channel, const char *fmt, ...);
extern int delegate_real_com_util_pinned_prompt_status_enable(com_util_pinned_prompt *screen,
                                                              com_util_pinned_prompt_status_position_t position,
                                                              int enable);
extern int delegate_real_com_util_pinned_prompt_status_set(com_util_pinned_prompt *screen,
                                                           com_util_pinned_prompt_status_position_t position,
                                                           com_util_pinned_prompt_status_align_t align,
                                                           const char *content);

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
    MOCK_METHOD(int, com_util_makedirs, (const char *));
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
    MOCK_METHOD(int, com_util_isatty, (com_util_stream_t));
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
    MOCK_METHOD(void, com_util_file_init, (com_util_file *));
    MOCK_METHOD(int, com_util_file_open, (com_util_file *, const char *, int));
    MOCK_METHOD(int, com_util_file_write, (com_util_file *, const void *, size_t));
    MOCK_METHOD(int, com_util_file_get_size, (const com_util_file *, size_t *));
    MOCK_METHOD(int, com_util_file_get_id, (const com_util_file *, com_util_file_id *));
    MOCK_METHOD(int, com_util_file_get_path_id, (const char *, com_util_file_id *));
    MOCK_METHOD(void, com_util_file_close, (com_util_file *));

    // trace - tracer
    MOCK_METHOD(com_util_tracer *, com_util_tracer_create, ());
    MOCK_METHOD(void, com_util_tracer_dispose, (com_util_tracer *));
    MOCK_METHOD(int, com_util_tracer_start, (com_util_tracer *));
    MOCK_METHOD(int, com_util_tracer_stop, (com_util_tracer *));
    MOCK_METHOD(int, _com_util_tracer_write,
                (com_util_tracer *, com_util_trace_level_t, const com_util_realtime_timestamp *, const char *));
    MOCK_METHOD(int, _com_util_tracer_write_hex,
                (com_util_tracer *, com_util_trace_level_t, const com_util_realtime_timestamp *, const void *, size_t,
                 const char *));
    MOCK_METHOD(int, _com_util_tracer_writef,
                (com_util_tracer *, com_util_trace_level_t, const com_util_realtime_timestamp *, const char *));
    MOCK_METHOD(int, _com_util_tracer_write_hexf,
                (com_util_tracer *, com_util_trace_level_t, const com_util_realtime_timestamp *, const void *, size_t,
                 const char *));
    MOCK_METHOD(int, com_util_tracer_set_name, (com_util_tracer *, const char *, int64_t));
    MOCK_METHOD(int, com_util_tracer_set_os_level, (com_util_tracer *, com_util_trace_level_t));
    MOCK_METHOD(int, com_util_tracer_set_file_level,
                (com_util_tracer *, const char *, com_util_trace_level_t, size_t, int, int));
    MOCK_METHOD(int, com_util_tracer_set_stderr_level, (com_util_tracer *, com_util_trace_level_t));
    MOCK_METHOD(com_util_tracer_hook_entry *, com_util_tracer_set_hook,
                (com_util_tracer *, com_util_tracer_hook_fn_t, void *));
    MOCK_METHOD(void, com_util_tracer_remove_hook, (com_util_tracer *, com_util_tracer_hook_entry *));
    MOCK_METHOD(void, com_util_tracer_call_next_hook,
                (com_util_tracer_hook_entry *, com_util_tracer *, com_util_trace_level_t,
                 const com_util_realtime_timestamp *, const char *));
    MOCK_METHOD(com_util_tracer_state_t, com_util_tracer_get_state, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level_t, com_util_tracer_get_os_level, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level_t, com_util_tracer_get_file_level, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level_t, com_util_tracer_get_stderr_level, (com_util_tracer *));

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
    MOCK_METHOD(void, com_util_console_dispose_on_shutdown, (const com_util_shutdown_event *, void *));

    // sync
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_create, (com_util_local_lock **));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_lock, (com_util_local_lock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_try_lock, (com_util_local_lock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_lock_unlock, (com_util_local_lock *));
    MOCK_METHOD(void, com_util_local_lock_destroy, (com_util_local_lock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_create, (com_util_condvar **));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_wait, (com_util_condvar *, com_util_local_lock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_signal, (com_util_condvar *));
    MOCK_METHOD(com_util_sync_result_t, com_util_condvar_broadcast, (com_util_condvar *));
    MOCK_METHOD(void, com_util_condvar_destroy, (com_util_condvar *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_create, (com_util_local_rwlock **));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_lock_shared, (com_util_local_rwlock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_try_lock_shared, (com_util_local_rwlock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_lock_exclusive, (com_util_local_rwlock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_try_lock_exclusive, (com_util_local_rwlock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_unlock_shared, (com_util_local_rwlock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_local_rwlock_unlock_exclusive, (com_util_local_rwlock *));
    MOCK_METHOD(void, com_util_local_rwlock_destroy, (com_util_local_rwlock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_thread_create, (com_util_thread **, com_util_thread_func_t, void *));
    MOCK_METHOD(com_util_sync_result_t, com_util_thread_join, (com_util_thread *, int));
    MOCK_METHOD(void, com_util_thread_detach, (com_util_thread *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_open, (const char *, com_util_interprocess_lock **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_import_descriptor,
                (const void *, size_t, com_util_interprocess_lock **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_export_descriptor,
                (const com_util_interprocess_lock *, void *, size_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_lock, (com_util_interprocess_lock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_try_lock, (com_util_interprocess_lock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_lock_unlock, (com_util_interprocess_lock *));
    MOCK_METHOD(void, com_util_interprocess_lock_destroy, (com_util_interprocess_lock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_open,
                (const char *, com_util_interprocess_rwlock **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_import_descriptor,
                (const void *, size_t, com_util_interprocess_rwlock **));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_export_descriptor,
                (const com_util_interprocess_rwlock *, void *, size_t *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_lock_shared,
                (com_util_interprocess_rwlock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_try_lock_shared, (com_util_interprocess_rwlock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_lock_exclusive,
                (com_util_interprocess_rwlock *, int));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_try_lock_exclusive,
                (com_util_interprocess_rwlock *));
    MOCK_METHOD(com_util_sync_result_t, com_util_interprocess_rwlock_unlock, (com_util_interprocess_rwlock *));
    MOCK_METHOD(void, com_util_interprocess_rwlock_destroy, (com_util_interprocess_rwlock *));
    MOCK_METHOD(void, com_util_call_once, (com_util_once_flag *, com_util_once_func_t));
    MOCK_METHOD(void, com_util_sleep_ms, (int));

    // runtime - module_info
    MOCK_METHOD(int, com_util_module_get_path, (char *, size_t, const void *));
    MOCK_METHOD(int, com_util_module_get_basename, (char *, size_t, const void *));

    // runtime - process_info
    MOCK_METHOD(int, com_util_process_run_elevated_if_needed, (const char *, int *, int *));
    MOCK_METHOD(com_util_process_result_t, com_util_process_start,
                (const com_util_process_options_t *, com_util_process **));
    MOCK_METHOD(com_util_process_result_t, com_util_process_wait, (com_util_process *, int));
    MOCK_METHOD(com_util_process_result_t, com_util_process_get_exit_code, (com_util_process *, int *));
    MOCK_METHOD(com_util_process_result_t, com_util_process_terminate, (com_util_process *));
    MOCK_METHOD(void, com_util_process_destroy, (com_util_process *));
    MOCK_METHOD(com_util_process_result_t, com_util_process_run_sync, (const com_util_process_options_t *, int, int *));

    // runtime - sym_loader
    MOCK_METHOD(void *, com_util_sym_loader_resolve, (com_util_sym_loader_entry *));
    MOCK_METHOD(int, com_util_sym_loader_is_default, (com_util_sym_loader_entry *));
    MOCK_METHOD(void, com_util_sym_loader_init, (com_util_sym_loader_entry *const *, size_t, const char *));
    MOCK_METHOD(void, com_util_sym_loader_dispose, (com_util_sym_loader_entry *const *, size_t));
    MOCK_METHOD(int, com_util_sym_loader_info, (com_util_sym_loader_entry *const *, size_t));

    // runtime - shutdown
    MOCK_METHOD(int, com_util_shutdown_register, (com_util_shutdown_callback_t, void *));
    MOCK_METHOD(int, com_util_shutdown_request_register, (com_util_shutdown_callback_t, void *));

    // trace - trace_file_sink
    MOCK_METHOD(com_util_trace_file_sink *, com_util_trace_file_sink_create, (const char *, size_t, int, int));
    MOCK_METHOD(int, com_util_trace_file_sink_write,
                (com_util_trace_file_sink *, int, const com_util_realtime_timestamp *, const char *));
    MOCK_METHOD(void, com_util_trace_file_sink_dispose, (com_util_trace_file_sink *));

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    MOCK_METHOD(com_util_syslog_sink *, com_util_syslog_sink_create, (const char *, int));
    MOCK_METHOD(int, com_util_syslog_sink_write,
                (com_util_syslog_sink *, int, const com_util_realtime_timestamp *, const char *));
    MOCK_METHOD(int, com_util_syslog_sink_rename, (com_util_syslog_sink *, const char *));
    MOCK_METHOD(void, com_util_syslog_sink_dispose, (com_util_syslog_sink *));
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
    // crt - wchar_conv (Windows only)
    MOCK_METHOD(int, com_util_utf8_to_wpath, (wchar_t *, size_t, const char *));
    MOCK_METHOD(int, com_util_wpath_to_utf8, (char *, size_t, const wchar_t *));
    MOCK_METHOD(wchar_t *, com_util_utf8_to_wstr_alloc, (const char *));
    MOCK_METHOD(char *, com_util_wstr_to_utf8_alloc, (const wchar_t *));
#endif /* PLATFORM_WINDOWS */

#if defined(PLATFORM_WINDOWS)
    // trace - trace_etw (Windows only)
    MOCK_METHOD(com_util_etw_provider *, com_util_etw_provider_create, (com_util_etw_provider_ref_t));
    MOCK_METHOD(int, com_util_etw_provider_write, (com_util_etw_provider *, int, const char *, const char *));
    MOCK_METHOD(void, com_util_etw_provider_dispose, (com_util_etw_provider *));
    MOCK_METHOD(int, com_util_etw_session_check_access, ());
    MOCK_METHOD(com_util_etw_session *, com_util_etw_session_start,
                (const char *, const char *, com_util_etw_event_callback_t, void *, int *));
    MOCK_METHOD(void, com_util_etw_session_stop, (com_util_etw_session *));
#endif /* PLATFORM_WINDOWS */

    // prompt
    MOCK_METHOD(com_util_prompt *, com_util_prompt_create, (const com_util_prompt_options *));
    MOCK_METHOD(void, com_util_prompt_dispose, (com_util_prompt *));
    MOCK_METHOD(int, com_util_prompt_readline_at, (com_util_prompt *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, com_util_prompt_readline_fmt_at,
                (com_util_prompt *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(com_util_pinned_prompt *, com_util_pinned_prompt_create, (const com_util_pinned_prompt_options *));
    MOCK_METHOD(void, com_util_pinned_prompt_dispose, (com_util_pinned_prompt *));
    MOCK_METHOD(int, _com_util_pinned_prompt_readline,
                (com_util_pinned_prompt *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, _com_util_pinned_prompt_readline_fmt,
                (com_util_pinned_prompt *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(size_t, com_util_pinned_prompt_write,
                (com_util_pinned_prompt *, com_util_pinned_prompt_channel_t, const void *, size_t));
    MOCK_METHOD(int, com_util_pinned_prompt_printf,
                (com_util_pinned_prompt *, com_util_pinned_prompt_channel_t, const char *));
    MOCK_METHOD(int, com_util_pinned_prompt_status_enable,
                (com_util_pinned_prompt *, com_util_pinned_prompt_status_position_t, int));
    MOCK_METHOD(int, com_util_pinned_prompt_status_set,
                (com_util_pinned_prompt *, com_util_pinned_prompt_status_position_t,
                 com_util_pinned_prompt_status_align_t, const char *));

    Mock_com_util();
    ~Mock_com_util();
};

extern Mock_com_util *_mock_com_util;

#endif /* MOCK_UTIL_H */
