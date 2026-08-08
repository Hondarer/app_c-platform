#ifndef MOCK_UTIL_H
#define MOCK_UTIL_H

#include <com_util/base/platform.h>
#include <testfw.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <vector>

#if defined(COMPILER_MSVC)
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vscanf")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfscanf")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vsnprintf")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfprintf")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vfopen_fmt")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vaccess_fmt")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vopen_fmt")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vremove_fmt")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vmkdir_fmt")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_vstat_fmt")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_console_dispose_on_shutdown")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_path_basename")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_shutdown_register")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_call_once")
    #pragma comment(linker, "/INCLUDE:_mock_impl_com_util_local_lock_create")
#endif /* COMPILER_MSVC */

#include <com_util/compress/compress.h>
#include <com_util/crypto/crypto.h>
#include <com_util/crypto/random.h>
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
#include <com_util/runtime/memory_lock.h>
#include <com_util/runtime/elevated_process.h>
#include <com_util/runtime/process.h>
#include <com_util/runtime/sym_loader.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/crt/wchar_conv.h>
#include <com_util/trace/trace_file.h>
#include <com_util/trace/syslog.h>
#include <com_util/trace/etw.h>
#include <com_util/trace/eventlog.h>
#include <com_util/prompt/prompt.h>
#include <com_util/prompt/pinned_prompt.h>
#include <com_util/argparser/argparser.h>

inline constexpr char kLibComUtilName[] = "libcom_util" TESTFW_SHARED_LIBRARY_EXTENSION;

// 書式を展開した NUL 終端文字列を返す。
// 期待値の照合と実関数への委譲の双方で使用する。固定長バッファーで展開すると長い出力が
// 切り詰められ、被テスト側の切り詰め判定が実関数と食い違うため、testfw の allocvprintf で
// 必要な長さを確保する。戻り値は解放不要で、.data() は常に有効な NUL 終端文字列を指す。
extern std::vector<char> mock_com_util_expand_format(const char *format, va_list args);

// compress
extern int delegate_real_com_util_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);
extern int delegate_real_com_util_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);

// crypto
extern int delegate_real_com_util_encrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                          const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_com_util_decrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                          const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_com_util_passphrase_to_key(uint8_t *key, const uint8_t *passphrase, size_t passphrase_len);
extern int delegate_real_com_util_random_bytes(void *buf, size_t size);

// crt
extern FILE *delegate_real_com_util_fopen(const char *path, const char *modes, com_util_error *detail_out);
extern FILE *delegate_real_com_util_freopen(const char *path, const char *modes, FILE *stream,
                                            com_util_error *detail_out);
extern int delegate_real_com_util_fclose(FILE *stream, com_util_error *detail_out);
extern int delegate_real_com_util_fflush(FILE *stream, com_util_error *detail_out);
extern size_t delegate_real_com_util_fread(void *buffer, size_t size, size_t count, FILE *stream,
                                           com_util_error *detail_out);
extern size_t delegate_real_com_util_fwrite(const void *buffer, size_t size, size_t count, FILE *stream,
                                            com_util_error *detail_out);
extern int delegate_real_com_util_stat(com_util_file_stat_t *buf, com_util_error *detail_out, const char *path);
extern int delegate_real_com_util_open(const char *path, int flags, int mode, com_util_error *detail_out);
extern int delegate_real_com_util_access(const char *path, int mode, com_util_error *detail_out);
extern int delegate_real_com_util_mkdir(const char *path, com_util_error *detail_out);
extern int delegate_real_com_util_makedirs(const char *path, com_util_error *detail_out);
extern int delegate_real_com_util_rmdir(const char *path, com_util_error *detail_out);
extern int delegate_real_com_util_remove(const char *path, com_util_error *detail_out);
extern int delegate_real_com_util_sscanf(const char *buffer, const char *format, va_list args);
extern int delegate_real_com_util_vsscanf(const char *buffer, const char *format, va_list args);
extern int delegate_real_com_util_gmtime(struct tm *utc_tm, const time_t *timep);
extern int delegate_real_com_util_localtime(struct tm *local_tm, const time_t *timep);
extern int delegate_real_com_util_ctime(char *buf, size_t buf_size, const time_t *timep);
extern int delegate_real_com_util_getenv(const char *name, char *buf, size_t buf_size, int *exists_out,
                                         com_util_error *detail_out);
extern int delegate_real_com_util_setenv(const char *name, const char *value, int overwrite,
                                         com_util_error *detail_out);
extern int delegate_real_com_util_unsetenv(const char *name, com_util_error *detail_out);
extern int delegate_real_com_util_parse_int64(int64_t *value_out, const char *text, int base);
extern int delegate_real_com_util_parse_uint64(uint64_t *value_out, const char *text, int base);
extern int delegate_real_com_util_parse_int(int *value_out, const char *text, int base);
extern int delegate_real_com_util_parse_double(double *value_out, const char *text);
extern int delegate_real_com_util_path_get_full(char *path_out, size_t path_size, com_util_error *detail_out,
                                                const char *path);
extern int delegate_real_com_util_paths_equal(const char *lhs, const char *rhs, int *equal_out,
                                              com_util_error *detail_out);
extern const char *delegate_real_com_util_path_basename(const char *path);

// crt - stdio
extern int delegate_real_com_util_scanf(const char *format, va_list args);
extern int delegate_real_com_util_vscanf(const char *format, va_list args);
extern int delegate_real_com_util_fscanf(FILE *stream, const char *format, va_list args);
extern int delegate_real_com_util_vfscanf(FILE *stream, const char *format, va_list args);
extern int delegate_real_com_util_snprintf(char *dest, size_t dest_size, const char *format, ...);
extern int delegate_real_com_util_vsnprintf(char *dest, size_t dest_size, const char *format, va_list args);
extern int delegate_real_com_util_fgets(char *dest, size_t dest_size, FILE *stream, com_util_error *detail_out);
extern int delegate_real_com_util_rename(const char *oldpath, const char *newpath, com_util_error *detail_out);
extern int delegate_real_com_util_fprintf(FILE *stream, const char *format, ...);
extern int delegate_real_com_util_vfprintf(FILE *stream, const char *format, va_list args);
extern int delegate_real_com_util_fseek(FILE *stream, int64_t offset, int whence);
extern int64_t delegate_real_com_util_ftell(FILE *stream);
extern FILE *delegate_real_com_util_fopen_fmt(const char *modes, com_util_error *detail_out, const char *format, ...);
extern FILE *delegate_real_com_util_vfopen_fmt(const char *modes, com_util_error *detail_out, const char *format,
                                               va_list args);
extern int delegate_real_com_util_remove_fmt(com_util_error *detail_out, const char *format, ...);
extern int delegate_real_com_util_vremove_fmt(com_util_error *detail_out, const char *format, va_list args);
extern FILE *delegate_real_com_util_fopen_temp(const char *prefix, const char *modes, char *path_out, size_t path_size,
                                               com_util_error *detail_out);

// crt - unistd
extern int delegate_real_com_util_isatty(com_util_stream stream);
extern int delegate_real_com_util_access_fmt(int mode, com_util_error *detail_out, const char *format, ...);
extern int delegate_real_com_util_vaccess_fmt(int mode, com_util_error *detail_out, const char *format, va_list args);
extern int64_t delegate_real_com_util_lseek(int fd, int64_t offset, int whence, com_util_error *detail_out);
extern int delegate_real_com_util_close(int fd, com_util_error *detail_out);
extern int delegate_real_com_util_dup(int fd, com_util_error *detail_out);
extern int delegate_real_com_util_dup2(int oldfd, int newfd, com_util_error *detail_out);
extern int64_t delegate_real_com_util_read(int fd, void *buf, size_t count, com_util_error *detail_out);
extern int64_t delegate_real_com_util_write(int fd, const void *buf, size_t count, com_util_error *detail_out);

// crt - fcntl
extern int delegate_real_com_util_open_fmt(int flags, int mode, com_util_error *detail_out, const char *format, ...);
extern int delegate_real_com_util_vopen_fmt(int flags, int mode, com_util_error *detail_out, const char *format,
                                            va_list args);

// crt - string
extern int delegate_real_com_util_strcpy(char *dest, size_t dest_size, const char *src);
extern int delegate_real_com_util_strncpy(char *dest, size_t dest_size, const char *src, size_t count);
extern int delegate_real_com_util_strcat(char *dest, size_t dest_size, const char *src);
extern int delegate_real_com_util_strncat(char *dest, size_t dest_size, const char *src, size_t count);
extern char *delegate_real_com_util_strtok_r(char *str, const char *delim, char **saveptr);
extern char *delegate_real_com_util_strdup(const char *src);
extern int delegate_real_com_util_wcscpy(wchar_t *dest, size_t dest_size, const wchar_t *src);

// crt - sys/stat
extern int delegate_real_com_util_stat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out, const char *format,
                                           ...);
extern int delegate_real_com_util_vstat_fmt(com_util_file_stat_t *buf, com_util_error *detail_out, const char *format,
                                            va_list args);
extern int delegate_real_com_util_mkdir_fmt(com_util_error *detail_out, const char *format, ...);
extern int delegate_real_com_util_vmkdir_fmt(com_util_error *detail_out, const char *format, va_list args);

// crt - file
extern void delegate_real_com_util_file_init(com_util_file *file);
extern int delegate_real_com_util_file_open(com_util_file *file, const char *path, int flags,
                                            com_util_error *detail_out);
extern int delegate_real_com_util_file_write(com_util_file *file, const void *buf, size_t len,
                                             com_util_error *detail_out);
extern int delegate_real_com_util_file_read(com_util_file *file, void *buf, size_t len, size_t *read_out,
                                            com_util_error *detail_out);
extern int delegate_real_com_util_file_get_size(const com_util_file *file, size_t *size_out,
                                                com_util_error *detail_out);
extern int delegate_real_com_util_file_set_size(com_util_file *file, size_t size, com_util_error *detail_out);
extern int delegate_real_com_util_file_get_id(const com_util_file *file, com_util_file_id *id_out,
                                              com_util_error *detail_out);
extern int delegate_real_com_util_file_get_path_id(const char *path, com_util_file_id *id_out,
                                                   com_util_error *detail_out);
extern int delegate_real_com_util_file_flush(com_util_file *file, com_util_error *detail_out);
extern int delegate_real_com_util_file_close(com_util_file *file, com_util_error *detail_out);

// trace - tracer
extern com_util_tracer *delegate_real_com_util_tracer_create(void);
extern void delegate_real_com_util_tracer_dispose(com_util_tracer *handle);
extern int delegate_real_com_util_tracer_start(com_util_tracer *handle);
extern int delegate_real_com_util_tracer_stop(com_util_tracer *handle);
extern int delegate_real__com_util_tracer_write(com_util_tracer *handle, com_util_trace_level level,
                                                const com_util_timespec *timestamp, const char *message);
extern int delegate_real__com_util_tracer_write_hex(com_util_tracer *handle, com_util_trace_level level,
                                                    const com_util_timespec *timestamp, const void *data, size_t size,
                                                    const char *message);
extern int delegate_real__com_util_tracer_writef(com_util_tracer *handle, com_util_trace_level level,
                                                 const com_util_timespec *timestamp, const char *format, ...);
extern int delegate_real__com_util_tracer_write_hexf(com_util_tracer *handle, com_util_trace_level level,
                                                     const com_util_timespec *timestamp, const void *data, size_t size,
                                                     const char *format, ...);
extern int delegate_real_com_util_tracer_set_name(com_util_tracer *handle, const char *name, int64_t identifier);
extern int delegate_real_com_util_tracer_set_os_level(com_util_tracer *handle, com_util_trace_level level);
extern int delegate_real_com_util_tracer_set_etw_level(com_util_tracer *handle, com_util_trace_level level);
extern int delegate_real_com_util_tracer_set_file_level(com_util_tracer *handle, const char *path,
                                                        com_util_trace_level level, size_t max_bytes, int generations,
                                                        int flags);
extern int delegate_real_com_util_tracer_set_stderr_level(com_util_tracer *handle, com_util_trace_level level);
extern com_util_tracer_hook_entry *delegate_real_com_util_tracer_set_hook(com_util_tracer *handle,
                                                                          com_util_tracer_hook_fn fn, void *context);
extern void delegate_real_com_util_tracer_remove_hook(com_util_tracer *handle, com_util_tracer_hook_entry *hook_entry);
extern void delegate_real_com_util_tracer_call_next_hook(com_util_tracer_hook_entry *prev, com_util_tracer *handle,
                                                         com_util_trace_level level,
                                                         const com_util_timespec *timestamp, const char *message);
extern com_util_tracer_state delegate_real_com_util_tracer_get_state(com_util_tracer *handle);
extern com_util_trace_level delegate_real_com_util_tracer_get_os_level(com_util_tracer *handle);
extern com_util_trace_level delegate_real_com_util_tracer_get_etw_level(com_util_tracer *handle);
extern com_util_trace_level delegate_real_com_util_tracer_get_file_level(com_util_tracer *handle);
extern com_util_trace_level delegate_real_com_util_tracer_get_stderr_level(com_util_tracer *handle);

// clock
extern uint64_t delegate_real_com_util_get_monotonic_ms(void);
extern void delegate_real_com_util_get_monotonic(com_util_timespec *ts);
extern void delegate_real_com_util_get_realtime(com_util_timespec *ts);
extern void delegate_real_com_util_get_realtime_utc(struct tm *utc_tm, int32_t *tv_nsec);
extern int delegate_real_com_util_format_realtime_iso8601_local(char *buf, size_t buf_size,
                                                                const com_util_timespec *timestamp);
extern int delegate_real_com_util_format_realtime_iso8601_utc(char *buf, size_t buf_size,
                                                              const com_util_timespec *timestamp);
extern void delegate_real_com_util_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout);

// console
extern void delegate_real_com_util_console_init(void);
extern void delegate_real_com_util_console_dispose(void);
extern int delegate_real_com_util_console_attach_parent(int *argc, char **argv, int *attached_out);
extern void delegate_real_com_util_console_dispose_on_shutdown(const com_util_shutdown_event *event, void *context);

// sync
extern int delegate_real_com_util_local_lock_create(com_util_local_lock **mtx);
extern int delegate_real_com_util_local_lock_lock(com_util_local_lock *mtx, int timeout_ms);
extern int delegate_real_com_util_local_lock_try_lock(com_util_local_lock *mtx);
extern int delegate_real_com_util_local_lock_unlock(com_util_local_lock *mtx);
extern void delegate_real_com_util_local_lock_destroy(com_util_local_lock *mtx);
extern int delegate_real_com_util_condvar_create(com_util_condvar **cv);
extern int delegate_real_com_util_condvar_wait(com_util_condvar *cv, com_util_local_lock *mtx, int timeout_ms);
extern int delegate_real_com_util_condvar_signal(com_util_condvar *cv);
extern int delegate_real_com_util_condvar_broadcast(com_util_condvar *cv);
extern void delegate_real_com_util_condvar_destroy(com_util_condvar *cv);
extern int delegate_real_com_util_local_rwlock_create(com_util_local_rwlock **rwlock);
extern int delegate_real_com_util_local_rwlock_lock_shared(com_util_local_rwlock *rwlock, int timeout_ms);
extern int delegate_real_com_util_local_rwlock_try_lock_shared(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_local_rwlock_lock_exclusive(com_util_local_rwlock *rwlock, int timeout_ms);
extern int delegate_real_com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_local_rwlock_unlock_shared(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock *rwlock);
extern void delegate_real_com_util_local_rwlock_destroy(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_thread_create(com_util_thread **thread, com_util_thread_fn func, void *arg);
extern int delegate_real_com_util_thread_join(com_util_thread *thread, int timeout_ms);
extern void delegate_real_com_util_thread_detach(com_util_thread *thread);
extern int delegate_real_com_util_interprocess_lock_open(const char *identity, com_util_interprocess_lock **lock);
extern int delegate_real_com_util_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                                      com_util_interprocess_lock **lock);
extern int delegate_real_com_util_interprocess_lock_export_descriptor(const com_util_interprocess_lock *lock,
                                                                      void *descriptor, size_t *descriptor_size);
extern int delegate_real_com_util_interprocess_lock_lock(com_util_interprocess_lock *lock, int timeout_ms);
extern int delegate_real_com_util_interprocess_lock_try_lock(com_util_interprocess_lock *lock);
extern int delegate_real_com_util_interprocess_lock_unlock(com_util_interprocess_lock *lock);
extern void delegate_real_com_util_interprocess_lock_destroy(com_util_interprocess_lock *lock);
extern int delegate_real_com_util_interprocess_rwlock_open(const char *identity, com_util_interprocess_rwlock **lock);
extern int delegate_real_com_util_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                                        com_util_interprocess_rwlock **lock);
extern int delegate_real_com_util_interprocess_rwlock_export_descriptor(const com_util_interprocess_rwlock *lock,
                                                                        void *descriptor, size_t *descriptor_size);
extern int delegate_real_com_util_interprocess_rwlock_lock_shared(com_util_interprocess_rwlock *lock, int timeout_ms);
extern int delegate_real_com_util_interprocess_rwlock_try_lock_shared(com_util_interprocess_rwlock *lock);
extern int delegate_real_com_util_interprocess_rwlock_lock_exclusive(com_util_interprocess_rwlock *lock,
                                                                     int timeout_ms);
extern int delegate_real_com_util_interprocess_rwlock_try_lock_exclusive(com_util_interprocess_rwlock *lock);
extern int delegate_real_com_util_interprocess_rwlock_unlock(com_util_interprocess_rwlock *lock);
extern void delegate_real_com_util_interprocess_rwlock_destroy(com_util_interprocess_rwlock *lock);
extern void delegate_real_com_util_call_once(com_util_once_flag *flag, com_util_once_fn func);
extern void delegate_real_com_util_sleep_ms(int ms);

// runtime - module_info
extern int delegate_real_com_util_module_get_path(char *out_path, size_t out_path_sz, const void *func_addr);
extern int delegate_real_com_util_module_get_basename(char *out_basename, size_t out_basename_sz,
                                                      const void *func_addr);

// runtime - memory_lock
extern int delegate_real_com_util_memory_lock_range(const void *address, size_t size);
extern int delegate_real_com_util_memory_unlock_range(const void *address, size_t size);
extern int delegate_real_com_util_memory_lock_self(const com_util_memory_lock_self_options *options,
                                                   com_util_memory_lock_scope **scope);
extern int delegate_real_com_util_memory_lock_scope_release(com_util_memory_lock_scope *scope);
extern void delegate_real_com_util_secure_zero(void *buf, size_t size);

// runtime - process_info
extern int delegate_real_com_util_process_get_executable_path(char *out_path, size_t out_path_sz);
extern int delegate_real_com_util_elevated_process_is_elevated(int *elevated);
extern int delegate_real_com_util_elevated_process_run_if_needed(const char *arguments, int *exit_code, int *handled);
extern int delegate_real_com_util_elevated_process_run_with_result(const char *arguments, int *exit_code, int *handled,
                                                                   char *result_message, size_t result_message_size);
extern int delegate_real_com_util_elevated_process_extract_result_target(int *argc, char **argv, int *detected_out);
extern int delegate_real_com_util_elevated_process_report_result(const char *message);
extern int delegate_real_com_util_process_start(const com_util_process_options *options, com_util_process **process);
extern int delegate_real_com_util_process_wait(com_util_process *process, int timeout_ms);
extern int delegate_real_com_util_process_get_exit_code(com_util_process *process, int *exit_code);
extern int delegate_real_com_util_process_terminate(com_util_process *process);
extern void delegate_real_com_util_process_destroy(com_util_process *process);
extern int delegate_real_com_util_process_run_sync(const com_util_process_options *options, int timeout_ms,
                                                   int *exit_code);

// runtime - sym_loader
extern void *delegate_real_com_util_sym_loader_resolve(com_util_sym_loader_entry *fobj);
extern int delegate_real_com_util_sym_loader_is_default(com_util_sym_loader_entry *fobj);
extern void delegate_real_com_util_sym_loader_init(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length,
                                                   const char *configpath);
extern void delegate_real_com_util_sym_loader_dispose(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length);
extern int delegate_real_com_util_sym_loader_info(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length);

// runtime - shutdown
extern int delegate_real_com_util_shutdown_register(com_util_shutdown_fn callback, void *context);
extern int delegate_real_com_util_shutdown_request_register(com_util_shutdown_fn callback, void *context);

// trace - trace_file_sink
extern com_util_trace_file_sink *delegate_real_com_util_trace_file_sink_create(const char *path, size_t max_bytes,
                                                                               int generations, int flags);
extern int delegate_real_com_util_trace_file_sink_write(com_util_trace_file_sink *handle, int level,
                                                        const com_util_timespec *timestamp, const char *message);
extern void delegate_real_com_util_trace_file_sink_dispose(com_util_trace_file_sink *handle);

#if defined(PLATFORM_LINUX)
// trace - syslog_sink (Linux only)
extern com_util_syslog_sink *delegate_real_com_util_syslog_sink_create(const char *ident, int facility);
extern int delegate_real_com_util_syslog_sink_write(com_util_syslog_sink *handle, int level,
                                                    const com_util_timespec *timestamp, const char *message);
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
extern int delegate_real_com_util_etw_session_start(const char *session_name, const char *provider_guid_str,
                                                    com_util_etw_event_fn callback, void *context,
                                                    com_util_etw_session **session_out);
extern void delegate_real_com_util_etw_session_stop(com_util_etw_session *session);

// trace - eventlog (Windows only)
extern com_util_eventlog_sink *delegate_real_com_util_eventlog_sink_create(const char *source_name);
extern int delegate_real_com_util_eventlog_sink_write(com_util_eventlog_sink *handle, int level,
                                                      int64_t file_identifier, const char *instance_name,
                                                      int64_t instance_identifier, const char *message);
extern void delegate_real_com_util_eventlog_sink_dispose(com_util_eventlog_sink *handle);
extern int delegate_real_com_util_eventlog_register_source(const char *source_name, const char *message_file_path);
extern int delegate_real_com_util_eventlog_unregister_source(const char *source_name);
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
extern int delegate_real_com_util_pinned_prompt_write(com_util_pinned_prompt *screen,
                                                      com_util_pinned_prompt_channel channel, const void *data,
                                                      size_t size, size_t *written_out);
extern int delegate_real_com_util_pinned_prompt_printf(com_util_pinned_prompt *screen,
                                                       com_util_pinned_prompt_channel channel, const char *fmt, ...);
extern int delegate_real_com_util_pinned_prompt_status_enable(com_util_pinned_prompt *screen,
                                                              com_util_pinned_prompt_status_position position,
                                                              int enable);
extern int delegate_real_com_util_pinned_prompt_status_set(com_util_pinned_prompt *screen,
                                                           com_util_pinned_prompt_status_position position,
                                                           com_util_pinned_prompt_status_align align,
                                                           const char *content);

// argparser
extern com_util_argparser *delegate_real__com_util_argparser_create(const com_util_argparser_options *options);
extern com_util_argparser *delegate_real__com_util_argparser_default(const com_util_argparser_options *options);
extern void delegate_real__com_util_argparser_dispose(com_util_argparser *parser);
extern int delegate_real__com_util_argparser_register_flag(com_util_argparser *parser, const char *short_name,
                                                           const char *long_name, const char *description,
                                                           int *storage);
extern int delegate_real__com_util_argparser_register_option_int(com_util_argparser *parser, const char *short_name,
                                                                 const char *long_name, const char *value_name,
                                                                 const char *description, unsigned int flags,
                                                                 int *storage);
extern int delegate_real__com_util_argparser_register_option_string(com_util_argparser *parser, const char *short_name,
                                                                    const char *long_name, const char *value_name,
                                                                    const char *description, unsigned int flags,
                                                                    const char **storage);
extern int delegate_real__com_util_argparser_register_option_int_array(com_util_argparser *parser,
                                                                       const char *short_name, const char *long_name,
                                                                       const char *value_name, const char *description,
                                                                       unsigned int flags, int *storage,
                                                                       size_t capacity, size_t *count);
extern int delegate_real__com_util_argparser_register_option_string_array(
    com_util_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
    const char *description, unsigned int flags, const char **storage, size_t capacity, size_t *count);
extern int delegate_real__com_util_argparser_register_positional_int(com_util_argparser *parser, const char *name,
                                                                     const char *description, unsigned int flags,
                                                                     int *storage);
extern int delegate_real__com_util_argparser_register_positional_string(com_util_argparser *parser, const char *name,
                                                                        const char *description, unsigned int flags,
                                                                        const char **storage);
extern int delegate_real__com_util_argparser_register_positional_int_array(com_util_argparser *parser, const char *name,
                                                                           const char *description, unsigned int flags,
                                                                           int *storage, size_t capacity,
                                                                           size_t *count);
extern int delegate_real__com_util_argparser_register_positional_string_array(com_util_argparser *parser,
                                                                              const char *name, const char *description,
                                                                              unsigned int flags, const char **storage,
                                                                              size_t capacity, size_t *count);
extern int delegate_real__com_util_argparser_parse(com_util_argparser *parser, int argc, char *const *argv);
extern int delegate_real__com_util_argparser_get_error(const com_util_argparser *parser);
extern const char *delegate_real__com_util_argparser_get_error_target(const com_util_argparser *parser);
extern int delegate_real__com_util_argparser_get_error_index(const com_util_argparser *parser);
extern int delegate_real__com_util_argparser_get_error_message(const com_util_argparser *parser, char *buffer,
                                                               size_t buffer_size);
extern int delegate_real__com_util_argparser_get_usage(const com_util_argparser *parser, char *buffer,
                                                       size_t buffer_size, size_t *required_size);
extern int delegate_real__com_util_argparser_print_usage(const com_util_argparser *parser, FILE *stream);
extern int delegate_real__com_util_argparser_print_error_messages(const com_util_argparser *parser, FILE *stream);
extern int delegate_real__com_util_argparser_get_register_error(const com_util_argparser *parser, size_t index);
extern size_t delegate_real__com_util_argparser_get_register_error_count(const com_util_argparser *parser);
extern const char *delegate_real__com_util_argparser_get_register_error_target(const com_util_argparser *parser,
                                                                               size_t index);
extern int delegate_real__com_util_argparser_get_register_error_message(const com_util_argparser *parser, size_t index,
                                                                        char *buffer, size_t buffer_size);
extern int delegate_real__com_util_argparser_print_register_error_messages(const com_util_argparser *parser,
                                                                           FILE *stream);

// argparser (省略可能な単一インスタンス API)
extern void delegate_real_com_util_argparser_init(const char *description);
extern int delegate_real_com_util_argparser_register_flag(const char *short_name, const char *long_name,
                                                          const char *description, int *storage);
extern int delegate_real_com_util_argparser_register_option_int(const char *short_name, const char *long_name,
                                                                const char *value_name, const char *description,
                                                                unsigned int flags, int *storage);
extern int delegate_real_com_util_argparser_register_option_string(const char *short_name, const char *long_name,
                                                                   const char *value_name, const char *description,
                                                                   unsigned int flags, const char **storage);
extern int delegate_real_com_util_argparser_register_option_int_array(const char *short_name, const char *long_name,
                                                                      const char *value_name, const char *description,
                                                                      unsigned int flags, int *storage, size_t capacity,
                                                                      size_t *count);
extern int delegate_real_com_util_argparser_register_option_string_array(const char *short_name, const char *long_name,
                                                                         const char *value_name,
                                                                         const char *description, unsigned int flags,
                                                                         const char **storage, size_t capacity,
                                                                         size_t *count);
extern int delegate_real_com_util_argparser_register_positional_int(const char *name, const char *description,
                                                                    unsigned int flags, int *storage);
extern int delegate_real_com_util_argparser_register_positional_string(const char *name, const char *description,
                                                                       unsigned int flags, const char **storage);
extern int delegate_real_com_util_argparser_register_positional_int_array(const char *name, const char *description,
                                                                          unsigned int flags, int *storage,
                                                                          size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_register_positional_string_array(const char *name, const char *description,
                                                                             unsigned int flags, const char **storage,
                                                                             size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_parse(int argc, char *const *argv);
extern int delegate_real_com_util_argparser_get_error(void);
extern const char *delegate_real_com_util_argparser_get_error_target(void);
extern int delegate_real_com_util_argparser_get_error_index(void);
extern int delegate_real_com_util_argparser_get_error_message(char *buffer, size_t buffer_size);
extern int delegate_real_com_util_argparser_get_usage(char *buffer, size_t buffer_size, size_t *required_size);
extern int delegate_real_com_util_argparser_print_usage(FILE *stream);
extern int delegate_real_com_util_argparser_print_error_messages(FILE *stream);
extern int delegate_real_com_util_argparser_get_register_error(size_t index);
extern size_t delegate_real_com_util_argparser_get_register_error_count(void);
extern const char *delegate_real_com_util_argparser_get_register_error_target(size_t index);
extern int delegate_real_com_util_argparser_get_register_error_message(size_t index, char *buffer, size_t buffer_size);
extern int delegate_real_com_util_argparser_print_register_error_messages(FILE *stream);

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
    MOCK_METHOD(int, com_util_random_bytes, (void *, size_t));

    // crt
    MOCK_METHOD(FILE *, com_util_fopen, (const char *, const char *, com_util_error *));
    MOCK_METHOD(FILE *, com_util_freopen, (const char *, const char *, FILE *, com_util_error *));
    MOCK_METHOD(int, com_util_fclose, (FILE *, com_util_error *));
    MOCK_METHOD(int, com_util_fflush, (FILE *, com_util_error *));
    MOCK_METHOD(size_t, com_util_fread, (void *, size_t, size_t, FILE *, com_util_error *));
    MOCK_METHOD(size_t, com_util_fwrite, (const void *, size_t, size_t, FILE *, com_util_error *));
    MOCK_METHOD(int, com_util_stat, (com_util_file_stat_t *, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_open, (const char *, int, int, com_util_error *));
    MOCK_METHOD(int, com_util_access, (const char *, int, com_util_error *));
    MOCK_METHOD(int, com_util_mkdir, (const char *, com_util_error *));
    MOCK_METHOD(int, com_util_makedirs, (const char *, com_util_error *));
    MOCK_METHOD(int, com_util_rmdir, (const char *, com_util_error *));
    MOCK_METHOD(int, com_util_remove, (const char *, com_util_error *));
    MOCK_METHOD(int, com_util_sscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int, com_util_vsscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int, com_util_gmtime, (struct tm *, const time_t *));
    MOCK_METHOD(int, com_util_localtime, (struct tm *, const time_t *));
    MOCK_METHOD(int, com_util_ctime, (char *, size_t, const time_t *));
    MOCK_METHOD(int, com_util_getenv, (const char *, char *, size_t, int *, com_util_error *));
    MOCK_METHOD(int, com_util_setenv, (const char *, const char *, int, com_util_error *));
    MOCK_METHOD(int, com_util_parse_int64, (int64_t *, const char *, int));
    MOCK_METHOD(int, com_util_parse_uint64, (uint64_t *, const char *, int));
    MOCK_METHOD(int, com_util_parse_int, (int *, const char *, int));
    MOCK_METHOD(int, com_util_parse_double, (double *, const char *));
    MOCK_METHOD(int, com_util_unsetenv, (const char *, com_util_error *));
    MOCK_METHOD(int, com_util_path_get_full, (char *, size_t, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_paths_equal, (const char *, const char *, int *, com_util_error *));
    MOCK_METHOD(const char *, com_util_path_basename, (const char *));

    // crt - stdio
    MOCK_METHOD(int, com_util_scanf, (const char *, va_list));
    MOCK_METHOD(int, com_util_vscanf, (const char *, va_list));
    MOCK_METHOD(int, com_util_fscanf, (FILE *, const char *, va_list));
    MOCK_METHOD(int, com_util_vfscanf, (FILE *, const char *, va_list));
    MOCK_METHOD(int, com_util_snprintf, (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_vsnprintf, (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_fgets, (char *, size_t, FILE *, com_util_error *));
    MOCK_METHOD(int, com_util_rename, (const char *, const char *, com_util_error *));
    MOCK_METHOD(int, com_util_fprintf, (FILE *, const char *));
    MOCK_METHOD(int, com_util_vfprintf, (FILE *, const char *));
    MOCK_METHOD(int, com_util_fseek, (FILE *, int64_t, int));
    MOCK_METHOD(int64_t, com_util_ftell, (FILE *));
    MOCK_METHOD(FILE *, com_util_fopen_fmt, (const char *, com_util_error *, const char *));
    MOCK_METHOD(FILE *, com_util_vfopen_fmt, (const char *, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_remove_fmt, (com_util_error *, const char *));
    MOCK_METHOD(int, com_util_vremove_fmt, (com_util_error *, const char *));
    MOCK_METHOD(FILE *, com_util_fopen_temp, (const char *, const char *, char *, size_t, com_util_error *));

    // crt - unistd
    MOCK_METHOD(int, com_util_isatty, (com_util_stream));
    MOCK_METHOD(int, com_util_access_fmt, (int, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_vaccess_fmt, (int, com_util_error *, const char *));
    MOCK_METHOD(int64_t, com_util_lseek, (int, int64_t, int, com_util_error *));
    MOCK_METHOD(int, com_util_close, (int, com_util_error *));
    MOCK_METHOD(int, com_util_dup, (int, com_util_error *));
    MOCK_METHOD(int, com_util_dup2, (int, int, com_util_error *));
    MOCK_METHOD(int64_t, com_util_read, (int, void *, size_t, com_util_error *));
    MOCK_METHOD(int64_t, com_util_write, (int, const void *, size_t, com_util_error *));

    // crt - fcntl
    MOCK_METHOD(int, com_util_open_fmt, (int, int, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_vopen_fmt, (int, int, com_util_error *, const char *));

    // crt - string
    MOCK_METHOD(int, com_util_strcpy, (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_strncpy, (char *, size_t, const char *, size_t));
    MOCK_METHOD(int, com_util_strcat, (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_strncat, (char *, size_t, const char *, size_t));
    MOCK_METHOD(char *, com_util_strtok_r, (char *, const char *, char **));
    MOCK_METHOD(char *, com_util_strdup, (const char *));
    MOCK_METHOD(int, com_util_wcscpy, (wchar_t *, size_t, const wchar_t *));

    // crt - sys/stat
    MOCK_METHOD(int, com_util_stat_fmt, (com_util_file_stat_t *, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_vstat_fmt, (com_util_file_stat_t *, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_mkdir_fmt, (com_util_error *, const char *));
    MOCK_METHOD(int, com_util_vmkdir_fmt, (com_util_error *, const char *));

    // crt - file
    MOCK_METHOD(void, com_util_file_init, (com_util_file *));
    MOCK_METHOD(int, com_util_file_open, (com_util_file *, const char *, int, com_util_error *));
    MOCK_METHOD(int, com_util_file_write, (com_util_file *, const void *, size_t, com_util_error *));
    MOCK_METHOD(int, com_util_file_read, (com_util_file *, void *, size_t, size_t *, com_util_error *));
    MOCK_METHOD(int, com_util_file_get_size, (const com_util_file *, size_t *, com_util_error *));
    MOCK_METHOD(int, com_util_file_set_size, (com_util_file *, size_t, com_util_error *));
    MOCK_METHOD(int, com_util_file_get_id, (const com_util_file *, com_util_file_id *, com_util_error *));
    MOCK_METHOD(int, com_util_file_get_path_id, (const char *, com_util_file_id *, com_util_error *));
    MOCK_METHOD(int, com_util_file_flush, (com_util_file *, com_util_error *));
    MOCK_METHOD(int, com_util_file_close, (com_util_file *, com_util_error *));

    // trace - tracer
    MOCK_METHOD(com_util_tracer *, com_util_tracer_create, ());
    MOCK_METHOD(void, com_util_tracer_dispose, (com_util_tracer *));
    MOCK_METHOD(int, com_util_tracer_start, (com_util_tracer *));
    MOCK_METHOD(int, com_util_tracer_stop, (com_util_tracer *));
    MOCK_METHOD(int, _com_util_tracer_write,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const char *));
    MOCK_METHOD(int, _com_util_tracer_write_hex,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const void *, size_t,
                 const char *));
    MOCK_METHOD(int, _com_util_tracer_writef,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const char *));
    MOCK_METHOD(int, _com_util_tracer_write_hexf,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const void *, size_t,
                 const char *));
    MOCK_METHOD(int, com_util_tracer_set_name, (com_util_tracer *, const char *, int64_t));
    MOCK_METHOD(int, com_util_tracer_set_os_level, (com_util_tracer *, com_util_trace_level));
    MOCK_METHOD(int, com_util_tracer_set_etw_level, (com_util_tracer *, com_util_trace_level));
    MOCK_METHOD(int, com_util_tracer_set_file_level,
                (com_util_tracer *, const char *, com_util_trace_level, size_t, int, int));
    MOCK_METHOD(int, com_util_tracer_set_stderr_level, (com_util_tracer *, com_util_trace_level));
    MOCK_METHOD(com_util_tracer_hook_entry *, com_util_tracer_set_hook,
                (com_util_tracer *, com_util_tracer_hook_fn, void *));
    MOCK_METHOD(void, com_util_tracer_remove_hook, (com_util_tracer *, com_util_tracer_hook_entry *));
    MOCK_METHOD(void, com_util_tracer_call_next_hook,
                (com_util_tracer_hook_entry *, com_util_tracer *, com_util_trace_level, const com_util_timespec *,
                 const char *));
    MOCK_METHOD(com_util_tracer_state, com_util_tracer_get_state, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level, com_util_tracer_get_os_level, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level, com_util_tracer_get_etw_level, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level, com_util_tracer_get_file_level, (com_util_tracer *));
    MOCK_METHOD(com_util_trace_level, com_util_tracer_get_stderr_level, (com_util_tracer *));

    // clock
    MOCK_METHOD(uint64_t, com_util_get_monotonic_ms, ());
    MOCK_METHOD(void, com_util_get_monotonic, (com_util_timespec *));
    MOCK_METHOD(void, com_util_get_realtime, (com_util_timespec *));
    MOCK_METHOD(void, com_util_get_realtime_utc, (struct tm *, int32_t *));
    MOCK_METHOD(int, com_util_format_realtime_iso8601_local, (char *, size_t, const com_util_timespec *));
    MOCK_METHOD(int, com_util_format_realtime_iso8601_utc, (char *, size_t, const com_util_timespec *));
    MOCK_METHOD(void, com_util_get_realtime_deadline_ms, (uint64_t, struct timespec *));

    // console
    MOCK_METHOD(void, com_util_console_init, ());
    MOCK_METHOD(void, com_util_console_dispose, ());
    MOCK_METHOD(int, com_util_console_attach_parent, (int *, char **, int *));
    MOCK_METHOD(void, com_util_console_dispose_on_shutdown, (const com_util_shutdown_event *, void *));

    // sync
    MOCK_METHOD(int, com_util_local_lock_create, (com_util_local_lock **));
    MOCK_METHOD(int, com_util_local_lock_lock, (com_util_local_lock *, int));
    MOCK_METHOD(int, com_util_local_lock_try_lock, (com_util_local_lock *));
    MOCK_METHOD(int, com_util_local_lock_unlock, (com_util_local_lock *));
    MOCK_METHOD(void, com_util_local_lock_destroy, (com_util_local_lock *));
    MOCK_METHOD(int, com_util_condvar_create, (com_util_condvar **));
    MOCK_METHOD(int, com_util_condvar_wait, (com_util_condvar *, com_util_local_lock *, int));
    MOCK_METHOD(int, com_util_condvar_signal, (com_util_condvar *));
    MOCK_METHOD(int, com_util_condvar_broadcast, (com_util_condvar *));
    MOCK_METHOD(void, com_util_condvar_destroy, (com_util_condvar *));
    MOCK_METHOD(int, com_util_local_rwlock_create, (com_util_local_rwlock **));
    MOCK_METHOD(int, com_util_local_rwlock_lock_shared, (com_util_local_rwlock *, int));
    MOCK_METHOD(int, com_util_local_rwlock_try_lock_shared, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_local_rwlock_lock_exclusive, (com_util_local_rwlock *, int));
    MOCK_METHOD(int, com_util_local_rwlock_try_lock_exclusive, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_local_rwlock_unlock_shared, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_local_rwlock_unlock_exclusive, (com_util_local_rwlock *));
    MOCK_METHOD(void, com_util_local_rwlock_destroy, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_thread_create, (com_util_thread **, com_util_thread_fn, void *));
    MOCK_METHOD(int, com_util_thread_join, (com_util_thread *, int));
    MOCK_METHOD(void, com_util_thread_detach, (com_util_thread *));
    MOCK_METHOD(int, com_util_interprocess_lock_open, (const char *, com_util_interprocess_lock **));
    MOCK_METHOD(int, com_util_interprocess_lock_import_descriptor,
                (const void *, size_t, com_util_interprocess_lock **));
    MOCK_METHOD(int, com_util_interprocess_lock_export_descriptor,
                (const com_util_interprocess_lock *, void *, size_t *));
    MOCK_METHOD(int, com_util_interprocess_lock_lock, (com_util_interprocess_lock *, int));
    MOCK_METHOD(int, com_util_interprocess_lock_try_lock, (com_util_interprocess_lock *));
    MOCK_METHOD(int, com_util_interprocess_lock_unlock, (com_util_interprocess_lock *));
    MOCK_METHOD(void, com_util_interprocess_lock_destroy, (com_util_interprocess_lock *));
    MOCK_METHOD(int, com_util_interprocess_rwlock_open, (const char *, com_util_interprocess_rwlock **));
    MOCK_METHOD(int, com_util_interprocess_rwlock_import_descriptor,
                (const void *, size_t, com_util_interprocess_rwlock **));
    MOCK_METHOD(int, com_util_interprocess_rwlock_export_descriptor,
                (const com_util_interprocess_rwlock *, void *, size_t *));
    MOCK_METHOD(int, com_util_interprocess_rwlock_lock_shared, (com_util_interprocess_rwlock *, int));
    MOCK_METHOD(int, com_util_interprocess_rwlock_try_lock_shared, (com_util_interprocess_rwlock *));
    MOCK_METHOD(int, com_util_interprocess_rwlock_lock_exclusive, (com_util_interprocess_rwlock *, int));
    MOCK_METHOD(int, com_util_interprocess_rwlock_try_lock_exclusive, (com_util_interprocess_rwlock *));
    MOCK_METHOD(int, com_util_interprocess_rwlock_unlock, (com_util_interprocess_rwlock *));
    MOCK_METHOD(void, com_util_interprocess_rwlock_destroy, (com_util_interprocess_rwlock *));
    MOCK_METHOD(void, com_util_call_once, (com_util_once_flag *, com_util_once_fn));
    MOCK_METHOD(void, com_util_sleep_ms, (int));

    // runtime - module_info
    MOCK_METHOD(int, com_util_module_get_path, (char *, size_t, const void *));
    MOCK_METHOD(int, com_util_module_get_basename, (char *, size_t, const void *));

    // runtime - memory_lock
    MOCK_METHOD(int, com_util_memory_lock_range, (const void *, size_t));
    MOCK_METHOD(int, com_util_memory_unlock_range, (const void *, size_t));
    MOCK_METHOD(int, com_util_memory_lock_self,
                (const com_util_memory_lock_self_options *, com_util_memory_lock_scope **));
    MOCK_METHOD(int, com_util_memory_lock_scope_release, (com_util_memory_lock_scope *));
    MOCK_METHOD(void, com_util_secure_zero, (void *, size_t));

    // runtime - process_info
    MOCK_METHOD(int, com_util_process_get_executable_path, (char *, size_t));
    MOCK_METHOD(int, com_util_elevated_process_is_elevated, (int *));
    MOCK_METHOD(int, com_util_elevated_process_run_if_needed, (const char *, int *, int *));
    MOCK_METHOD(int, com_util_elevated_process_run_with_result, (const char *, int *, int *, char *, size_t));
    MOCK_METHOD(int, com_util_elevated_process_extract_result_target, (int *, char **, int *));
    MOCK_METHOD(int, com_util_elevated_process_report_result, (const char *));
    MOCK_METHOD(int, com_util_process_start, (const com_util_process_options *, com_util_process **));
    MOCK_METHOD(int, com_util_process_wait, (com_util_process *, int));
    MOCK_METHOD(int, com_util_process_get_exit_code, (com_util_process *, int *));
    MOCK_METHOD(int, com_util_process_terminate, (com_util_process *));
    MOCK_METHOD(void, com_util_process_destroy, (com_util_process *));
    MOCK_METHOD(int, com_util_process_run_sync, (const com_util_process_options *, int, int *));

    // runtime - sym_loader
    MOCK_METHOD(void *, com_util_sym_loader_resolve, (com_util_sym_loader_entry *));
    MOCK_METHOD(int, com_util_sym_loader_is_default, (com_util_sym_loader_entry *));
    MOCK_METHOD(void, com_util_sym_loader_init, (com_util_sym_loader_entry *const *, size_t, const char *));
    MOCK_METHOD(void, com_util_sym_loader_dispose, (com_util_sym_loader_entry *const *, size_t));
    MOCK_METHOD(int, com_util_sym_loader_info, (com_util_sym_loader_entry *const *, size_t));

    // runtime - shutdown
    MOCK_METHOD(int, com_util_shutdown_register, (com_util_shutdown_fn, void *));
    MOCK_METHOD(int, com_util_shutdown_request_register, (com_util_shutdown_fn, void *));

    // trace - trace_file_sink
    MOCK_METHOD(com_util_trace_file_sink *, com_util_trace_file_sink_create, (const char *, size_t, int, int));
    MOCK_METHOD(int, com_util_trace_file_sink_write,
                (com_util_trace_file_sink *, int, const com_util_timespec *, const char *));
    MOCK_METHOD(void, com_util_trace_file_sink_dispose, (com_util_trace_file_sink *));

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    MOCK_METHOD(com_util_syslog_sink *, com_util_syslog_sink_create, (const char *, int));
    MOCK_METHOD(int, com_util_syslog_sink_write,
                (com_util_syslog_sink *, int, const com_util_timespec *, const char *));
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
    MOCK_METHOD(int, com_util_etw_session_start,
                (const char *, const char *, com_util_etw_event_fn, void *, com_util_etw_session **));
    MOCK_METHOD(void, com_util_etw_session_stop, (com_util_etw_session *));

    // trace - trace_eventlog (Windows only)
    MOCK_METHOD(com_util_eventlog_sink *, com_util_eventlog_sink_create, (const char *));
    MOCK_METHOD(int, com_util_eventlog_sink_write,
                (com_util_eventlog_sink *, int, int64_t, const char *, int64_t, const char *));
    MOCK_METHOD(void, com_util_eventlog_sink_dispose, (com_util_eventlog_sink *));
    MOCK_METHOD(int, com_util_eventlog_register_source, (const char *, const char *));
    MOCK_METHOD(int, com_util_eventlog_unregister_source, (const char *));
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
    MOCK_METHOD(int, com_util_pinned_prompt_write,
                (com_util_pinned_prompt *, com_util_pinned_prompt_channel, const void *, size_t, size_t *));
    MOCK_METHOD(int, com_util_pinned_prompt_printf,
                (com_util_pinned_prompt *, com_util_pinned_prompt_channel, const char *));
    MOCK_METHOD(int, com_util_pinned_prompt_status_enable,
                (com_util_pinned_prompt *, com_util_pinned_prompt_status_position, int));
    MOCK_METHOD(int, com_util_pinned_prompt_status_set,
                (com_util_pinned_prompt *, com_util_pinned_prompt_status_position,
                 com_util_pinned_prompt_status_align, const char *));

    // argparser
    MOCK_METHOD(com_util_argparser *, _com_util_argparser_create, (const com_util_argparser_options *));
    MOCK_METHOD(com_util_argparser *, _com_util_argparser_default, (const com_util_argparser_options *));
    MOCK_METHOD(void, _com_util_argparser_dispose, (com_util_argparser *));
    MOCK_METHOD(int, _com_util_argparser_register_flag,
                (com_util_argparser *, const char *, const char *, const char *, int *));
    MOCK_METHOD(int, _com_util_argparser_register_option_int,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, _com_util_argparser_register_option_string,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int,
                 const char **));
    MOCK_METHOD(int, _com_util_argparser_register_option_int_array,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int, int *,
                 size_t, size_t *));
    MOCK_METHOD(int, _com_util_argparser_register_option_string_array,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int,
                 const char **, size_t, size_t *));
    MOCK_METHOD(int, _com_util_argparser_register_positional_int,
                (com_util_argparser *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, _com_util_argparser_register_positional_string,
                (com_util_argparser *, const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, _com_util_argparser_register_positional_int_array,
                (com_util_argparser *, const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, _com_util_argparser_register_positional_string_array,
                (com_util_argparser *, const char *, const char *, unsigned int, const char **, size_t, size_t *));
    MOCK_METHOD(int, _com_util_argparser_parse, (com_util_argparser *, int, char *const *));
    MOCK_METHOD(int, _com_util_argparser_get_error, (const com_util_argparser *));
    MOCK_METHOD(const char *, _com_util_argparser_get_error_target, (const com_util_argparser *));
    MOCK_METHOD(int, _com_util_argparser_get_error_index, (const com_util_argparser *));
    MOCK_METHOD(int, _com_util_argparser_get_error_message, (const com_util_argparser *, char *, size_t));
    MOCK_METHOD(int, _com_util_argparser_get_usage, (const com_util_argparser *, char *, size_t, size_t *));
    MOCK_METHOD(int, _com_util_argparser_print_usage, (const com_util_argparser *, FILE *));
    MOCK_METHOD(int, _com_util_argparser_print_error_messages, (const com_util_argparser *, FILE *));
    MOCK_METHOD(int, _com_util_argparser_get_register_error, (const com_util_argparser *, size_t));
    MOCK_METHOD(size_t, _com_util_argparser_get_register_error_count, (const com_util_argparser *));
    MOCK_METHOD(const char *, _com_util_argparser_get_register_error_target, (const com_util_argparser *, size_t));
    MOCK_METHOD(int, _com_util_argparser_get_register_error_message,
                (const com_util_argparser *, size_t, char *, size_t));
    MOCK_METHOD(int, _com_util_argparser_print_register_error_messages, (const com_util_argparser *, FILE *));

    // argparser (省略可能な単一インスタンス API)
    MOCK_METHOD(void, com_util_argparser_init, (const char *));
    MOCK_METHOD(int, com_util_argparser_register_flag, (const char *, const char *, const char *, int *));
    MOCK_METHOD(int, com_util_argparser_register_option_int,
                (const char *, const char *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, com_util_argparser_register_option_string,
                (const char *, const char *, const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, com_util_argparser_register_option_int_array,
                (const char *, const char *, const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_register_option_string_array,
                (const char *, const char *, const char *, const char *, unsigned int, const char **, size_t,
                 size_t *));
    MOCK_METHOD(int, com_util_argparser_register_positional_int, (const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, com_util_argparser_register_positional_string,
                (const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, com_util_argparser_register_positional_int_array,
                (const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_register_positional_string_array,
                (const char *, const char *, unsigned int, const char **, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_parse, (int, char *const *));
    MOCK_METHOD(int, com_util_argparser_get_error, ());
    MOCK_METHOD(const char *, com_util_argparser_get_error_target, ());
    MOCK_METHOD(int, com_util_argparser_get_error_index, ());
    MOCK_METHOD(int, com_util_argparser_get_error_message, (char *, size_t));
    MOCK_METHOD(int, com_util_argparser_get_usage, (char *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_print_usage, (FILE *));
    MOCK_METHOD(int, com_util_argparser_print_error_messages, (FILE *));
    MOCK_METHOD(int, com_util_argparser_get_register_error, (size_t));
    MOCK_METHOD(size_t, com_util_argparser_get_register_error_count, ());
    MOCK_METHOD(const char *, com_util_argparser_get_register_error_target, (size_t));
    MOCK_METHOD(int, com_util_argparser_get_register_error_message, (size_t, char *, size_t));
    MOCK_METHOD(int, com_util_argparser_print_register_error_messages, (FILE *));

    Mock_com_util();
    ~Mock_com_util();
};

extern Mock_com_util *_mock_com_util;

#endif /* MOCK_UTIL_H */
