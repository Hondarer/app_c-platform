#include <com_util/base/platform.h>
#include <testfw.h>
#include <mock_com_util.h>

Mock_com_util *_mock_com_util = nullptr;

Mock_com_util::Mock_com_util()
{
    // compress
    ON_CALL(*this, com_util_compress(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_compress));
    ON_CALL(*this, com_util_decompress(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_decompress));

    // crypto
    ON_CALL(*this, com_util_encrypt(_, _, _, _, _, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_encrypt));
    ON_CALL(*this, com_util_decrypt(_, _, _, _, _, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_decrypt));
    ON_CALL(*this, com_util_passphrase_to_key(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_passphrase_to_key));

    // crt
    ON_CALL(*this, com_util_fopen(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_fopen));
    ON_CALL(*this, com_util_freopen(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_freopen));
    ON_CALL(*this, com_util_stat(_, _)).WillByDefault(Invoke(delegate_real_com_util_stat));
    ON_CALL(*this, com_util_open(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_open));
    ON_CALL(*this, com_util_access(_, _)).WillByDefault(Invoke(delegate_real_com_util_access));
    ON_CALL(*this, com_util_mkdir(_)).WillByDefault(Invoke(delegate_real_com_util_mkdir));
    ON_CALL(*this, com_util_makedirs(_)).WillByDefault(Invoke(delegate_real_com_util_makedirs));
    ON_CALL(*this, com_util_remove(_)).WillByDefault(Invoke(delegate_real_com_util_remove));
    ON_CALL(*this, com_util_sscanf(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_sscanf));
    ON_CALL(*this, com_util_vsscanf(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_vsscanf));
    ON_CALL(*this, com_util_gmtime(_, _)).WillByDefault(Invoke(delegate_real_com_util_gmtime));
    ON_CALL(*this, com_util_localtime(_, _)).WillByDefault(Invoke(delegate_real_com_util_localtime));
    ON_CALL(*this, com_util_ctime(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_ctime));
    ON_CALL(*this, com_util_getenv(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_getenv));
    ON_CALL(*this, com_util_path_get_full(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_path_get_full));
    ON_CALL(*this, com_util_paths_equal(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_paths_equal));
    ON_CALL(*this, com_util_path_basename(_)).WillByDefault(Invoke(delegate_real_com_util_path_basename));

    // crt - stdio
    ON_CALL(*this, com_util_rename(_, _)).WillByDefault(Invoke(delegate_real_com_util_rename));
    ON_CALL(*this, com_util_fclose(_)).WillByDefault(Invoke(delegate_real_com_util_fclose));
    ON_CALL(*this, com_util_fread(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_fread));
    ON_CALL(*this, com_util_fwrite(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_fwrite));
    ON_CALL(*this, com_util_fgets(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_fgets));
    ON_CALL(*this, com_util_fputs(_, _)).WillByDefault(Invoke(delegate_real_com_util_fputs));
    ON_CALL(*this, com_util_fprintf(_, _)).WillByDefault(Invoke(delegate_real_com_util_fprintf));
    ON_CALL(*this, com_util_vfprintf(_, _)).WillByDefault(Invoke(delegate_real_com_util_fprintf));
    ON_CALL(*this, com_util_fflush(_)).WillByDefault(Invoke(delegate_real_com_util_fflush));
    ON_CALL(*this, com_util_feof(_)).WillByDefault(Invoke(delegate_real_com_util_feof));
    ON_CALL(*this, com_util_ferror(_)).WillByDefault(Invoke(delegate_real_com_util_ferror));
    ON_CALL(*this, com_util_clearerr(_)).WillByDefault(Invoke(delegate_real_com_util_clearerr));
    ON_CALL(*this, com_util_rewind(_)).WillByDefault(Invoke(delegate_real_com_util_rewind));
    ON_CALL(*this, com_util_fseek(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_fseek));
    ON_CALL(*this, com_util_ftell(_)).WillByDefault(Invoke(delegate_real_com_util_ftell));
    ON_CALL(*this, com_util_fopen_fmt(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_fopen_fmt));
    ON_CALL(*this, com_util_vfopen_fmt(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_fopen_fmt));
    ON_CALL(*this, com_util_remove_fmt(_)).WillByDefault(Invoke(delegate_real_com_util_remove_fmt));
    ON_CALL(*this, com_util_vremove_fmt(_)).WillByDefault(Invoke(delegate_real_com_util_remove_fmt));
    ON_CALL(*this, com_util_fopen_temp(_, _, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_fopen_temp));

    // crt - unistd
    ON_CALL(*this, com_util_isatty(_)).WillByDefault(Invoke(delegate_real_com_util_isatty));
    ON_CALL(*this, com_util_access_fmt(_, _)).WillByDefault(Invoke(delegate_real_com_util_access_fmt));
    ON_CALL(*this, com_util_vaccess_fmt(_, _)).WillByDefault(Invoke(delegate_real_com_util_access_fmt));
    ON_CALL(*this, com_util_lseek(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_lseek));
    ON_CALL(*this, com_util_close(_)).WillByDefault(Invoke(delegate_real_com_util_close));
    ON_CALL(*this, com_util_dup(_)).WillByDefault(Invoke(delegate_real_com_util_dup));
    ON_CALL(*this, com_util_dup2(_, _)).WillByDefault(Invoke(delegate_real_com_util_dup2));
    ON_CALL(*this, com_util_read(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_read));
    ON_CALL(*this, com_util_write(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_write));

    // crt - fcntl
    ON_CALL(*this, com_util_open_fmt(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_open_fmt));
    ON_CALL(*this, com_util_vopen_fmt(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_open_fmt));

    // crt - string
    ON_CALL(*this, com_util_strcpy(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_strcpy));
    ON_CALL(*this, com_util_strncpy(_, _, _, _)).WillByDefault(Invoke(delegate_real_com_util_strncpy));
    ON_CALL(*this, com_util_strcat(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_strcat));
    ON_CALL(*this, com_util_wcscpy(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_wcscpy));

    // crt - sys/stat
    ON_CALL(*this, com_util_stat_fmt(_, _)).WillByDefault(Invoke(delegate_real_com_util_stat_fmt));
    ON_CALL(*this, com_util_vstat_fmt(_, _)).WillByDefault(Invoke(delegate_real_com_util_stat_fmt));
    ON_CALL(*this, com_util_mkdir_fmt(_)).WillByDefault(Invoke(delegate_real_com_util_mkdir_fmt));
    ON_CALL(*this, com_util_vmkdir_fmt(_)).WillByDefault(Invoke(delegate_real_com_util_mkdir_fmt));

    // crt - file
    ON_CALL(*this, com_util_file_init(_)).WillByDefault(Invoke(delegate_real_com_util_file_init));
    ON_CALL(*this, com_util_file_open(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_file_open));
    ON_CALL(*this, com_util_file_write(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_file_write));
    ON_CALL(*this, com_util_file_get_size(_, _)).WillByDefault(Invoke(delegate_real_com_util_file_get_size));
    ON_CALL(*this, com_util_file_get_id(_, _)).WillByDefault(Invoke(delegate_real_com_util_file_get_id));
    ON_CALL(*this, com_util_file_get_path_id(_, _)).WillByDefault(Invoke(delegate_real_com_util_file_get_path_id));
    ON_CALL(*this, com_util_file_close(_)).WillByDefault(Invoke(delegate_real_com_util_file_close));

    // trace - tracer
    ON_CALL(*this, com_util_tracer_create()).WillByDefault(Invoke(delegate_real_com_util_tracer_create));
    ON_CALL(*this, com_util_tracer_dispose(_)).WillByDefault(Invoke(delegate_real_com_util_tracer_dispose));
    ON_CALL(*this, com_util_tracer_start(_)).WillByDefault(Invoke(delegate_real_com_util_tracer_start));
    ON_CALL(*this, com_util_tracer_stop(_)).WillByDefault(Invoke(delegate_real_com_util_tracer_stop));
    ON_CALL(*this, _com_util_tracer_write(_, _, _, _)).WillByDefault(Invoke(delegate_real__com_util_tracer_write));
    ON_CALL(*this, _com_util_tracer_write_hex(_, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real__com_util_tracer_write_hex));
    ON_CALL(*this, _com_util_tracer_writef(_, _, _, _)).WillByDefault(Invoke(delegate_real__com_util_tracer_writef));
    ON_CALL(*this, _com_util_tracer_write_hexf(_, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real__com_util_tracer_write_hexf));
    ON_CALL(*this, com_util_tracer_set_name(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_tracer_set_name));
    ON_CALL(*this, com_util_tracer_set_os_level(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_set_os_level));
    ON_CALL(*this, com_util_tracer_set_etw_level(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_set_etw_level));
    ON_CALL(*this, com_util_tracer_set_file_level(_, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_set_file_level));
    ON_CALL(*this, com_util_tracer_set_stderr_level(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_set_stderr_level));
    ON_CALL(*this, com_util_tracer_set_hook(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_tracer_set_hook));
    ON_CALL(*this, com_util_tracer_remove_hook(_, _)).WillByDefault(Invoke(delegate_real_com_util_tracer_remove_hook));
    ON_CALL(*this, com_util_tracer_call_next_hook(_, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_call_next_hook));
    ON_CALL(*this, com_util_tracer_get_state(_)).WillByDefault(Invoke(delegate_real_com_util_tracer_get_state));
    ON_CALL(*this, com_util_tracer_get_os_level(_)).WillByDefault(Invoke(delegate_real_com_util_tracer_get_os_level));
    ON_CALL(*this, com_util_tracer_get_etw_level(_)).WillByDefault(Invoke(delegate_real_com_util_tracer_get_etw_level));
    ON_CALL(*this, com_util_tracer_get_file_level(_))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_get_file_level));
    ON_CALL(*this, com_util_tracer_get_stderr_level(_))
        .WillByDefault(Invoke(delegate_real_com_util_tracer_get_stderr_level));

    // clock
    ON_CALL(*this, com_util_get_monotonic_ms()).WillByDefault(Invoke(delegate_real_com_util_get_monotonic_ms));
    ON_CALL(*this, com_util_get_monotonic(_)).WillByDefault(Invoke(delegate_real_com_util_get_monotonic));
    ON_CALL(*this, com_util_get_realtime(_)).WillByDefault(Invoke(delegate_real_com_util_get_realtime));
    ON_CALL(*this, com_util_get_realtime_utc(_, _)).WillByDefault(Invoke(delegate_real_com_util_get_realtime_utc));
    ON_CALL(*this, com_util_format_realtime_iso8601_local(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_format_realtime_iso8601_local));
    ON_CALL(*this, com_util_format_realtime_iso8601_utc(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_format_realtime_iso8601_utc));
    ON_CALL(*this, com_util_get_realtime_deadline_ms(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_get_realtime_deadline_ms));

    // console
    ON_CALL(*this, com_util_console_init()).WillByDefault(Invoke(delegate_real_com_util_console_init));
    ON_CALL(*this, com_util_console_dispose()).WillByDefault(Invoke(delegate_real_com_util_console_dispose));
    ON_CALL(*this, com_util_console_attach_parent(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_console_attach_parent));

    // sync
    ON_CALL(*this, com_util_local_lock_create(_)).WillByDefault(Invoke(delegate_real_com_util_local_lock_create));
    ON_CALL(*this, com_util_local_lock_lock(_, _)).WillByDefault(Invoke(delegate_real_com_util_local_lock_lock));
    ON_CALL(*this, com_util_local_lock_try_lock(_)).WillByDefault(Invoke(delegate_real_com_util_local_lock_try_lock));
    ON_CALL(*this, com_util_local_lock_unlock(_)).WillByDefault(Invoke(delegate_real_com_util_local_lock_unlock));
    ON_CALL(*this, com_util_local_lock_destroy(_)).WillByDefault(Invoke(delegate_real_com_util_local_lock_destroy));
    ON_CALL(*this, com_util_condvar_create(_)).WillByDefault(Invoke(delegate_real_com_util_condvar_create));
    ON_CALL(*this, com_util_condvar_wait(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_condvar_wait));
    ON_CALL(*this, com_util_condvar_signal(_)).WillByDefault(Invoke(delegate_real_com_util_condvar_signal));
    ON_CALL(*this, com_util_condvar_broadcast(_)).WillByDefault(Invoke(delegate_real_com_util_condvar_broadcast));
    ON_CALL(*this, com_util_condvar_destroy(_)).WillByDefault(Invoke(delegate_real_com_util_condvar_destroy));
    ON_CALL(*this, com_util_local_rwlock_create(_)).WillByDefault(Invoke(delegate_real_com_util_local_rwlock_create));
    ON_CALL(*this, com_util_local_rwlock_lock_shared(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_local_rwlock_lock_shared));
    ON_CALL(*this, com_util_local_rwlock_try_lock_shared(_))
        .WillByDefault(Invoke(delegate_real_com_util_local_rwlock_try_lock_shared));
    ON_CALL(*this, com_util_local_rwlock_lock_exclusive(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_local_rwlock_lock_exclusive));
    ON_CALL(*this, com_util_local_rwlock_try_lock_exclusive(_))
        .WillByDefault(Invoke(delegate_real_com_util_local_rwlock_try_lock_exclusive));
    ON_CALL(*this, com_util_local_rwlock_unlock_shared(_))
        .WillByDefault(Invoke(delegate_real_com_util_local_rwlock_unlock_shared));
    ON_CALL(*this, com_util_local_rwlock_unlock_exclusive(_))
        .WillByDefault(Invoke(delegate_real_com_util_local_rwlock_unlock_exclusive));
    ON_CALL(*this, com_util_local_rwlock_destroy(_)).WillByDefault(Invoke(delegate_real_com_util_local_rwlock_destroy));
    ON_CALL(*this, com_util_thread_create(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_thread_create));
    ON_CALL(*this, com_util_thread_join(_, _)).WillByDefault(Invoke(delegate_real_com_util_thread_join));
    ON_CALL(*this, com_util_thread_detach(_)).WillByDefault(Invoke(delegate_real_com_util_thread_detach));
    ON_CALL(*this, com_util_interprocess_lock_open(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_open));
    ON_CALL(*this, com_util_interprocess_lock_import_descriptor(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_import_descriptor));
    ON_CALL(*this, com_util_interprocess_lock_export_descriptor(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_export_descriptor));
    ON_CALL(*this, com_util_interprocess_lock_lock(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_lock));
    ON_CALL(*this, com_util_interprocess_lock_try_lock(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_try_lock));
    ON_CALL(*this, com_util_interprocess_lock_unlock(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_unlock));
    ON_CALL(*this, com_util_interprocess_lock_destroy(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_lock_destroy));
    ON_CALL(*this, com_util_interprocess_rwlock_open(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_open));
    ON_CALL(*this, com_util_interprocess_rwlock_import_descriptor(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_import_descriptor));
    ON_CALL(*this, com_util_interprocess_rwlock_export_descriptor(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_export_descriptor));
    ON_CALL(*this, com_util_interprocess_rwlock_lock_shared(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_lock_shared));
    ON_CALL(*this, com_util_interprocess_rwlock_try_lock_shared(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_try_lock_shared));
    ON_CALL(*this, com_util_interprocess_rwlock_lock_exclusive(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_lock_exclusive));
    ON_CALL(*this, com_util_interprocess_rwlock_try_lock_exclusive(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_try_lock_exclusive));
    ON_CALL(*this, com_util_interprocess_rwlock_unlock(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_unlock));
    ON_CALL(*this, com_util_interprocess_rwlock_destroy(_))
        .WillByDefault(Invoke(delegate_real_com_util_interprocess_rwlock_destroy));
    ON_CALL(*this, com_util_call_once(_, _)).WillByDefault(Invoke(delegate_real_com_util_call_once));
    ON_CALL(*this, com_util_sleep_ms(_)).WillByDefault(Invoke(delegate_real_com_util_sleep_ms));

    // runtime - module_info
    ON_CALL(*this, com_util_module_get_path(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_module_get_path));
    ON_CALL(*this, com_util_module_get_basename(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_module_get_basename));

    // runtime - memory_lock
    ON_CALL(*this, com_util_memory_lock_range(_, _)).WillByDefault(Invoke(delegate_real_com_util_memory_lock_range));
    ON_CALL(*this, com_util_memory_unlock_range(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_memory_unlock_range));
    ON_CALL(*this, com_util_memory_lock_self(_, _)).WillByDefault(Invoke(delegate_real_com_util_memory_lock_self));
    ON_CALL(*this, com_util_memory_lock_scope_release(_))
        .WillByDefault(Invoke(delegate_real_com_util_memory_lock_scope_release));

    // runtime - process_info
    ON_CALL(*this, com_util_process_get_executable_path(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_process_get_executable_path));
    ON_CALL(*this, com_util_elevated_process_is_elevated(_))
        .WillByDefault(Invoke(delegate_real_com_util_elevated_process_is_elevated));
    ON_CALL(*this, com_util_elevated_process_run_if_needed(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_elevated_process_run_if_needed));
    ON_CALL(*this, com_util_elevated_process_run_with_result(_, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_elevated_process_run_with_result));
    ON_CALL(*this, com_util_elevated_process_extract_result_target(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_elevated_process_extract_result_target));
    ON_CALL(*this, com_util_elevated_process_report_result(_))
        .WillByDefault(Invoke(delegate_real_com_util_elevated_process_report_result));
    ON_CALL(*this, com_util_process_start(_, _)).WillByDefault(Invoke(delegate_real_com_util_process_start));
    ON_CALL(*this, com_util_process_wait(_, _)).WillByDefault(Invoke(delegate_real_com_util_process_wait));
    ON_CALL(*this, com_util_process_get_exit_code(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_process_get_exit_code));
    ON_CALL(*this, com_util_process_terminate(_)).WillByDefault(Invoke(delegate_real_com_util_process_terminate));
    ON_CALL(*this, com_util_process_destroy(_)).WillByDefault(Invoke(delegate_real_com_util_process_destroy));
    ON_CALL(*this, com_util_process_run_sync(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_process_run_sync));

    // runtime - sym_loader
    ON_CALL(*this, com_util_sym_loader_resolve(_)).WillByDefault(Invoke(delegate_real_com_util_sym_loader_resolve));
    ON_CALL(*this, com_util_sym_loader_is_default(_))
        .WillByDefault(Invoke(delegate_real_com_util_sym_loader_is_default));
    ON_CALL(*this, com_util_sym_loader_init(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_sym_loader_init));
    ON_CALL(*this, com_util_sym_loader_dispose(_, _)).WillByDefault(Invoke(delegate_real_com_util_sym_loader_dispose));
    ON_CALL(*this, com_util_sym_loader_info(_, _)).WillByDefault(Invoke(delegate_real_com_util_sym_loader_info));

    // runtime - shutdown
    ON_CALL(*this, com_util_shutdown_register(_, _)).WillByDefault(Invoke(delegate_real_com_util_shutdown_register));
    ON_CALL(*this, com_util_shutdown_request_register(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_shutdown_request_register));

    // trace - log_file_sink
    ON_CALL(*this, com_util_trace_file_sink_create(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_trace_file_sink_create));
    ON_CALL(*this, com_util_trace_file_sink_write(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_trace_file_sink_write));
    ON_CALL(*this, com_util_trace_file_sink_dispose(_))
        .WillByDefault(Invoke(delegate_real_com_util_trace_file_sink_dispose));

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    ON_CALL(*this, com_util_syslog_sink_create(_, _)).WillByDefault(Invoke(delegate_real_com_util_syslog_sink_create));
    ON_CALL(*this, com_util_syslog_sink_write(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_syslog_sink_write));
    ON_CALL(*this, com_util_syslog_sink_rename(_, _)).WillByDefault(Invoke(delegate_real_com_util_syslog_sink_rename));
    ON_CALL(*this, com_util_syslog_sink_dispose(_)).WillByDefault(Invoke(delegate_real_com_util_syslog_sink_dispose));
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
    // crt - wchar_conv (Windows only)
    ON_CALL(*this, com_util_utf8_to_wpath(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_utf8_to_wpath));
    ON_CALL(*this, com_util_wpath_to_utf8(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_wpath_to_utf8));
    ON_CALL(*this, com_util_utf8_to_wstr_alloc(_)).WillByDefault(Invoke(delegate_real_com_util_utf8_to_wstr_alloc));
    ON_CALL(*this, com_util_wstr_to_utf8_alloc(_)).WillByDefault(Invoke(delegate_real_com_util_wstr_to_utf8_alloc));
#endif /* PLATFORM_WINDOWS */

#if defined(PLATFORM_WINDOWS)
    // trace - trace_etw (Windows only)
    ON_CALL(*this, com_util_etw_provider_create(_)).WillByDefault(Invoke(delegate_real_com_util_etw_provider_create));
    ON_CALL(*this, com_util_etw_provider_write(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_etw_provider_write));
    ON_CALL(*this, com_util_etw_provider_dispose(_)).WillByDefault(Invoke(delegate_real_com_util_etw_provider_dispose));
    ON_CALL(*this, com_util_etw_session_check_access())
        .WillByDefault(Invoke(delegate_real_com_util_etw_session_check_access));
    ON_CALL(*this, com_util_etw_session_start(_, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_etw_session_start));
    ON_CALL(*this, com_util_etw_session_stop(_)).WillByDefault(Invoke(delegate_real_com_util_etw_session_stop));

    // trace - trace_eventlog (Windows only)
    ON_CALL(*this, com_util_eventlog_sink_create(_)).WillByDefault(Invoke(delegate_real_com_util_eventlog_sink_create));
    ON_CALL(*this, com_util_eventlog_sink_write(_, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_eventlog_sink_write));
    ON_CALL(*this, com_util_eventlog_sink_dispose(_))
        .WillByDefault(Invoke(delegate_real_com_util_eventlog_sink_dispose));
    ON_CALL(*this, com_util_eventlog_register_source(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_eventlog_register_source));
    ON_CALL(*this, com_util_eventlog_unregister_source(_))
        .WillByDefault(Invoke(delegate_real_com_util_eventlog_unregister_source));
#endif /* PLATFORM_WINDOWS */

    // prompt
    ON_CALL(*this, com_util_prompt_create(_)).WillByDefault(Invoke(delegate_real_com_util_prompt_create));
    ON_CALL(*this, com_util_prompt_dispose(_)).WillByDefault(Invoke(delegate_real_com_util_prompt_dispose));
    ON_CALL(*this, com_util_prompt_readline_at(_, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_prompt_readline_at));
    ON_CALL(*this, com_util_prompt_readline_fmt_at(_, _, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_prompt_readline_fmt_at));
    ON_CALL(*this, com_util_pinned_prompt_create(_)).WillByDefault(Invoke(delegate_real_com_util_pinned_prompt_create));
    ON_CALL(*this, com_util_pinned_prompt_dispose(_))
        .WillByDefault(Invoke(delegate_real_com_util_pinned_prompt_dispose));
    ON_CALL(*this, _com_util_pinned_prompt_readline(_, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real__com_util_pinned_prompt_readline));
    ON_CALL(*this, _com_util_pinned_prompt_readline_fmt(_, _, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real__com_util_pinned_prompt_readline_fmt));
    ON_CALL(*this, com_util_pinned_prompt_write(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_pinned_prompt_write));
    ON_CALL(*this, com_util_pinned_prompt_printf(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_pinned_prompt_printf));
    ON_CALL(*this, com_util_pinned_prompt_status_enable(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_pinned_prompt_status_enable));
    ON_CALL(*this, com_util_pinned_prompt_status_set(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_pinned_prompt_status_set));

    // argparser
    ON_CALL(*this, com_util_argparser_create(_)).WillByDefault(Invoke(delegate_real_com_util_argparser_create));
    ON_CALL(*this, com_util_argparser_default(_)).WillByDefault(Invoke(delegate_real_com_util_argparser_default));
    ON_CALL(*this, com_util_argparser_dispose(_)).WillByDefault(Invoke(delegate_real_com_util_argparser_dispose));
    ON_CALL(*this, com_util_argparser_register_flag(_, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_flag));
    ON_CALL(*this, com_util_argparser_register_option_int(_, _, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_option_int));
    ON_CALL(*this, com_util_argparser_register_option_string(_, _, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_option_string));
    ON_CALL(*this, com_util_argparser_register_option_int_array(_, _, _, _, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_option_int_array));
    ON_CALL(*this, com_util_argparser_register_option_string_array(_, _, _, _, _, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_option_string_array));
    ON_CALL(*this, com_util_argparser_register_positional_int(_, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_positional_int));
    ON_CALL(*this, com_util_argparser_register_positional_string(_, _, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_register_positional_string));
    ON_CALL(*this, com_util_argparser_parse(_, _, _)).WillByDefault(Invoke(delegate_real_com_util_argparser_parse));
    ON_CALL(*this, com_util_argparser_get_error(_)).WillByDefault(Invoke(delegate_real_com_util_argparser_get_error));
    ON_CALL(*this, com_util_argparser_get_error_target(_))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_get_error_target));
    ON_CALL(*this, com_util_argparser_get_error_index(_))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_get_error_index));
    ON_CALL(*this, com_util_argparser_get_error_message(_, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_get_error_message));
    ON_CALL(*this, com_util_argparser_get_usage(_, _, _, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_get_usage));
    ON_CALL(*this, com_util_argparser_print_usage(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_print_usage));
    ON_CALL(*this, com_util_argparser_print_error_messages(_, _))
        .WillByDefault(Invoke(delegate_real_com_util_argparser_print_error_messages));

    _mock_com_util = this;
}

Mock_com_util::~Mock_com_util()
{
    _mock_com_util = nullptr;
}
