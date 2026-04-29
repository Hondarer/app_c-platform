#include <testfw.h>
#include <mock_com_util.h>

Mock_com_util *_mock_com_util = nullptr;

Mock_com_util::Mock_com_util()
{
    ON_CALL(*this, com_util_compress(_, _, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_decompress(_, _, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_encrypt(_, _, _, _, _, _, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_decrypt(_, _, _, _, _, _, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_passphrase_to_key(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fopen(_, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_stat(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_open(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_access(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_mkdir(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_remove(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_sscanf(_, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_vsscanf(_, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_gmtime(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_rename(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fclose(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fread(_, _, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_fwrite(_, _, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_fgets(_, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_fputs(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fprintf(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_vfprintf(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fflush(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_feof(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_ferror(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_clearerr(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_rewind(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_fseek(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_ftell(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fopen_fmt(_, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_vfopen_fmt(_, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_remove_fmt(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_vremove_fmt(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_fopen_temp(_, _, _, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_access_fmt(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_vaccess_fmt(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_open_fmt(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_vopen_fmt(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_strcpy(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_strncpy(_, _, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_strcat(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_wcscpy(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_stat_fmt(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_vstat_fmt(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_mkdir_fmt(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_vmkdir_fmt(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_file_init(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_file_open(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_file_write(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_file_get_size(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_file_close(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_tracer_create())
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_tracer_dispose(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_tracer_start(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_stop(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_write(_, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_write_hex(_, _, _, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_writef(_, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_write_hexf(_, _, _, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_set_name(_, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_set_os_level(_, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_set_file_level(_, _, _, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_set_stderr_level(_, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_tracer_get_state(_))
        .WillByDefault(Return(COM_UTIL_TRACER_STATE_STOPPED));
    ON_CALL(*this, com_util_tracer_get_os_level(_))
        .WillByDefault(Return(COM_UTIL_TRACE_LEVEL_NONE));
    ON_CALL(*this, com_util_tracer_get_file_level(_))
        .WillByDefault(Return(COM_UTIL_TRACE_LEVEL_NONE));
    ON_CALL(*this, com_util_tracer_get_stderr_level(_))
        .WillByDefault(Return(COM_UTIL_TRACE_LEVEL_NONE));

    // clock
    ON_CALL(*this, com_util_get_monotonic_ms())
        .WillByDefault(Return((uint64_t)0));
    ON_CALL(*this, com_util_get_monotonic(_, _))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_get_realtime(_, _))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_get_realtime_utc(_, _))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_get_realtime_deadline_ms(_, _))
        .WillByDefault(Return());

    // console
    ON_CALL(*this, com_util_console_init())
        .WillByDefault(Return());
    ON_CALL(*this, com_util_console_dispose())
        .WillByDefault(Return());

    // sync
    ON_CALL(*this, com_util_mutex_init(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_mutex_lock(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_mutex_timedlock(_, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_mutex_unlock(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_mutex_destroy(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_condvar_init(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_condvar_wait(_, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_condvar_timedwait(_, _, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_condvar_signal(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_condvar_broadcast(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_condvar_destroy(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_init(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_lock_shared(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_timedlock_shared(_, _))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_lock_exclusive(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_unlock_shared(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_unlock_exclusive(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_rwlock_destroy(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_thread_create(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_thread_join(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_thread_join_timed(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_thread_detach(_))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_call_once(_, _))
        .WillByDefault(Invoke([](com_util_once_flag_t *flag, com_util_once_func_t func) {
            if ((flag != nullptr) && (flag->state == 0))
            {
                flag->state = 1;
                if (func != nullptr)
                {
                    func();
                }
            }
        }));

    // runtime - module_info
    ON_CALL(*this, com_util_module_get_path(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_module_get_basename(_, _, _))
        .WillByDefault(Return(-1));

    // runtime - sym_loader
    ON_CALL(*this, com_util_sym_loader_resolve(_))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_sym_loader_is_default(_))
        .WillByDefault(Return(0));
    ON_CALL(*this, com_util_sym_loader_init(_, _, _))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_sym_loader_dispose(_, _))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_sym_loader_info(_, _))
        .WillByDefault(Return(-1));

    // trace - log_file_sink
    ON_CALL(*this, com_util_trace_file_sink_create(_, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_trace_file_sink_write(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_trace_file_sink_dispose(_))
        .WillByDefault(Return());

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    ON_CALL(*this, com_util_syslog_sink_create(_, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_syslog_sink_write(_, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_syslog_sink_rename(_, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_syslog_sink_dispose(_))
        .WillByDefault(Return());
#endif /* PLATFORM_LINUX */

#if defined(PLATFORM_WINDOWS)
    // trace - trace_etw (Windows only)
    ON_CALL(*this, com_util_etw_provider_create(_))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_etw_provider_write(_, _, _, _))
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_etw_provider_dispose(_))
        .WillByDefault(Return());
    ON_CALL(*this, com_util_etw_session_check_access())
        .WillByDefault(Return(-1));
    ON_CALL(*this, com_util_etw_session_start(_, _, _, _, _))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, com_util_etw_session_stop(_))
        .WillByDefault(Return());
#endif /* PLATFORM_WINDOWS */

    _mock_com_util = this;
}

Mock_com_util::~Mock_com_util()
{
    _mock_com_util = nullptr;
}
