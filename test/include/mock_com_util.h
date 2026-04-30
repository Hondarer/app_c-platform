#ifndef MOCK_UTIL_H
#define MOCK_UTIL_H

#include <testfw.h>
#include <stdint.h>
#include <time.h>
#include <com_util/compress/compress.h>
#include <com_util/crypto/crypto.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/time.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/string.h>
#include <com_util/crt/unistd.h>
#include <com_util/crt/file.h>
#include <com_util/trace/tracer.h>
#include <com_util/clock/clock.h>
#include <com_util/console/console.h>
#include <com_util/sync/sync.h>
#include <com_util/runtime/module.h>
#include <com_util/runtime/sym_loader.h>
#include <com_util/trace/trace_file.h>
#include <com_util/trace/syslog.h>
#include <com_util/trace/etw.h>

class Mock_com_util
{
public:
    // compress
    MOCK_METHOD(int, com_util_compress,   (uint8_t *, size_t *, const uint8_t *, size_t));
    MOCK_METHOD(int, com_util_decompress, (uint8_t *, size_t *, const uint8_t *, size_t));

    // crypto
    MOCK_METHOD(int, com_util_encrypt,
                (uint8_t *, size_t *, const uint8_t *, size_t,
                 const uint8_t *, const uint8_t *, const uint8_t *, size_t));
    MOCK_METHOD(int, com_util_decrypt,
                (uint8_t *, size_t *, const uint8_t *, size_t,
                 const uint8_t *, const uint8_t *, const uint8_t *, size_t));
    MOCK_METHOD(int, com_util_passphrase_to_key, (uint8_t *, const uint8_t *, size_t));

    // crt
    MOCK_METHOD(FILE *, com_util_fopen,  (const char *, const char *, int *));
    MOCK_METHOD(int,    com_util_stat,   (com_util_file_stat_t *, const char *));
    MOCK_METHOD(int,    com_util_open,   (const char *, int, int));
    MOCK_METHOD(int,    com_util_access, (const char *, int));
    MOCK_METHOD(int,    com_util_mkdir,  (const char *));
    MOCK_METHOD(int,    com_util_remove, (const char *));
    MOCK_METHOD(int,    com_util_sscanf,  (const char *, const char *, va_list));
    MOCK_METHOD(int,    com_util_vsscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int,    com_util_gmtime, (struct tm *, const time_t *));
    MOCK_METHOD(int,    com_util_localtime, (struct tm *, const time_t *));

    // crt - stdio
    MOCK_METHOD(int,    com_util_rename,  (const char *, const char *));
    MOCK_METHOD(int,    com_util_fclose,  (FILE *));
    MOCK_METHOD(size_t, com_util_fread,   (void *, size_t, size_t, FILE *));
    MOCK_METHOD(size_t, com_util_fwrite,  (const void *, size_t, size_t, FILE *));
    MOCK_METHOD(char *, com_util_fgets,   (char *, int, FILE *));
    MOCK_METHOD(int,    com_util_fputs,   (const char *, FILE *));
    MOCK_METHOD(int,    com_util_fprintf, (FILE *, const char *));
    MOCK_METHOD(int,    com_util_vfprintf,(FILE *, const char *));
    MOCK_METHOD(int,    com_util_fflush,  (FILE *));
    MOCK_METHOD(int,    com_util_feof,    (FILE *));
    MOCK_METHOD(int,    com_util_ferror,  (FILE *));
    MOCK_METHOD(void,   com_util_clearerr,(FILE *));
    MOCK_METHOD(void,   com_util_rewind,  (FILE *));
    MOCK_METHOD(int,    com_util_fseek,   (FILE *, int64_t, int));
    MOCK_METHOD(int64_t,com_util_ftell,   (FILE *));
    MOCK_METHOD(FILE *, com_util_fopen_fmt,  (const char *, int *, const char *));
    MOCK_METHOD(FILE *, com_util_vfopen_fmt, (const char *, int *, const char *));
    MOCK_METHOD(int,    com_util_remove_fmt,  (const char *));
    MOCK_METHOD(int,    com_util_vremove_fmt, (const char *));
    MOCK_METHOD(FILE *, com_util_fopen_temp,  (const char *, const char *, char *, size_t, int *));
    // crt - unistd
    MOCK_METHOD(int, com_util_access_fmt,  (int, const char *));
    MOCK_METHOD(int, com_util_vaccess_fmt, (int, const char *));
    // crt - fcntl
    MOCK_METHOD(int, com_util_open_fmt,  (int, int, const char *));
    MOCK_METHOD(int, com_util_vopen_fmt, (int, int, const char *));
    // crt - string
    MOCK_METHOD(int, com_util_strcpy,  (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_strncpy, (char *, size_t, const char *, size_t));
    MOCK_METHOD(int, com_util_strcat,  (char *, size_t, const char *));
    MOCK_METHOD(int, com_util_wcscpy,  (wchar_t *, size_t, const wchar_t *));
    // crt - sys/stat
    MOCK_METHOD(int, com_util_stat_fmt,   (com_util_file_stat_t *, const char *));
    MOCK_METHOD(int, com_util_vstat_fmt,  (com_util_file_stat_t *, const char *));
    MOCK_METHOD(int, com_util_mkdir_fmt,  (const char *));
    MOCK_METHOD(int, com_util_vmkdir_fmt, (const char *));
    // crt - file
    MOCK_METHOD(void, com_util_file_init,     (com_util_file_t *));
    MOCK_METHOD(int,  com_util_file_open,     (com_util_file_t *, const char *, uint32_t));
    MOCK_METHOD(int,  com_util_file_write,    (com_util_file_t *, const void *, size_t));
    MOCK_METHOD(int,  com_util_file_get_size, (com_util_file_t *, size_t *));
    MOCK_METHOD(void, com_util_file_close,    (com_util_file_t *));

    // 初期化・終了
    MOCK_METHOD(com_util_tracer_t *, com_util_tracer_create, ());
    MOCK_METHOD(void, com_util_tracer_dispose, (com_util_tracer_t *));

    // 制御
    MOCK_METHOD(int, com_util_tracer_start, (com_util_tracer_t *));
    MOCK_METHOD(int, com_util_tracer_stop, (com_util_tracer_t *));

    // 書き込み (固定引数版)
    MOCK_METHOD(int, com_util_tracer_write,
                (com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *,
                 const char *));
    MOCK_METHOD(int, com_util_tracer_write_hex, (com_util_tracer_t *, com_util_trace_level_t,
                                      const com_util_realtime_timestamp_t *,
                                      const void *, size_t, const char *));

    // 書き込み (可変引数対応: vsnprintf 展開後の文字列を受け取る代替メソッド)
    MOCK_METHOD(int, com_util_tracer_writef,
                (com_util_tracer_t *, com_util_trace_level_t,
                 const com_util_realtime_timestamp_t *, const char *));
    MOCK_METHOD(int, com_util_tracer_write_hexf,
                (com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *,
                 const void *, size_t, const char *));

    // 設定
    MOCK_METHOD(int, com_util_tracer_set_name, (com_util_tracer_t *, const char *, int64_t));
    MOCK_METHOD(int, com_util_tracer_set_os_level, (com_util_tracer_t *, com_util_trace_level_t));
    MOCK_METHOD(int, com_util_tracer_set_file_level,
                (com_util_tracer_t *, const char *, com_util_trace_level_t, size_t, int));
    MOCK_METHOD(int, com_util_tracer_set_stderr_level, (com_util_tracer_t *, com_util_trace_level_t));

    // 取得
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

    // sync
    MOCK_METHOD(int,  com_util_mutex_init, (com_util_mutex_t *));
    MOCK_METHOD(int,  com_util_mutex_lock, (com_util_mutex_t *));
    MOCK_METHOD(int,  com_util_mutex_timedlock, (com_util_mutex_t *, uint32_t));
    MOCK_METHOD(int,  com_util_mutex_unlock, (com_util_mutex_t *));
    MOCK_METHOD(int,  com_util_mutex_destroy, (com_util_mutex_t *));
    MOCK_METHOD(int,  com_util_condvar_init, (com_util_condvar_t *));
    MOCK_METHOD(int,  com_util_condvar_wait, (com_util_condvar_t *, com_util_mutex_t *));
    MOCK_METHOD(int,  com_util_condvar_timedwait, (com_util_condvar_t *, com_util_mutex_t *, uint32_t));
    MOCK_METHOD(int,  com_util_condvar_signal, (com_util_condvar_t *));
    MOCK_METHOD(int,  com_util_condvar_broadcast, (com_util_condvar_t *));
    MOCK_METHOD(int,  com_util_condvar_destroy, (com_util_condvar_t *));
    MOCK_METHOD(int,  com_util_rwlock_init, (com_util_rwlock_t *));
    MOCK_METHOD(int,  com_util_rwlock_lock_shared, (com_util_rwlock_t *));
    MOCK_METHOD(int,  com_util_rwlock_timedlock_shared, (com_util_rwlock_t *, uint32_t));
    MOCK_METHOD(int,  com_util_rwlock_lock_exclusive, (com_util_rwlock_t *));
    MOCK_METHOD(int,  com_util_rwlock_unlock_shared, (com_util_rwlock_t *));
    MOCK_METHOD(int,  com_util_rwlock_unlock_exclusive, (com_util_rwlock_t *));
    MOCK_METHOD(int,  com_util_rwlock_destroy, (com_util_rwlock_t *));
    MOCK_METHOD(int,  com_util_thread_create, (com_util_thread_t *, com_util_thread_func_t, void *));
    MOCK_METHOD(void, com_util_thread_join, (com_util_thread_t *));
    MOCK_METHOD(int,  com_util_thread_join_timed, (com_util_thread_t *, uint32_t));
    MOCK_METHOD(int,  com_util_thread_detach, (com_util_thread_t *));
    MOCK_METHOD(void, com_util_call_once, (com_util_once_flag_t *, com_util_once_func_t));

    // runtime - module_info
    MOCK_METHOD(int, com_util_module_get_path,     (char *, size_t, const void *));
    MOCK_METHOD(int, com_util_module_get_basename, (char *, size_t, const void *));

    // runtime - sym_loader
    MOCK_METHOD(void *, com_util_sym_loader_resolve,    (com_util_sym_loader_entry_t *));
    MOCK_METHOD(int,    com_util_sym_loader_is_default, (com_util_sym_loader_entry_t *));
    MOCK_METHOD(void,   com_util_sym_loader_init,    (com_util_sym_loader_entry_t *const *, size_t, const char *));
    MOCK_METHOD(void,   com_util_sym_loader_dispose, (com_util_sym_loader_entry_t *const *, size_t));
    MOCK_METHOD(int,    com_util_sym_loader_info,    (com_util_sym_loader_entry_t *const *, size_t));

    // trace - log_file_sink
    MOCK_METHOD(com_util_trace_file_sink_t *, com_util_trace_file_sink_create, (const char *, size_t, int));
    MOCK_METHOD(int,  com_util_trace_file_sink_write,
                (com_util_trace_file_sink_t *, int, const com_util_realtime_timestamp_t *,
                 const char *));
    MOCK_METHOD(void, com_util_trace_file_sink_dispose, (com_util_trace_file_sink_t *));

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    MOCK_METHOD(com_util_syslog_sink_t *, com_util_syslog_sink_create, (const char *, int));
    MOCK_METHOD(int,  com_util_syslog_sink_write,   (com_util_syslog_sink_t *, int, const char *));
    MOCK_METHOD(int,  com_util_syslog_sink_rename,  (com_util_syslog_sink_t *, const char *));
    MOCK_METHOD(void, com_util_syslog_sink_dispose, (com_util_syslog_sink_t *));
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
    // trace - trace_etw (Windows only)
    MOCK_METHOD(com_util_etw_provider_t *, com_util_etw_provider_create, (com_util_etw_provider_ref_t));
    MOCK_METHOD(int,  com_util_etw_provider_write,   (com_util_etw_provider_t *, int, const char *, const char *));
    MOCK_METHOD(void, com_util_etw_provider_dispose, (com_util_etw_provider_t *));
    MOCK_METHOD(int,  com_util_etw_session_check_access, ());
    MOCK_METHOD(com_util_etw_session_t *, com_util_etw_session_start,
                (const char *, const char *, com_util_etw_event_callback_t, void *, int *));
    MOCK_METHOD(void, com_util_etw_session_stop, (com_util_etw_session_t *));
#endif /* PLATFORM_WINDOWS */

    Mock_com_util();
    ~Mock_com_util();
};

extern Mock_com_util *_mock_com_util;

#endif /* MOCK_UTIL_H */
