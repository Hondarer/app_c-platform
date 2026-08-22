#ifndef MOCK_UTIL_H
#define MOCK_UTIL_H

#include <com_util/base/platform.h>
#include <testfw.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <vector>

#if defined(COMPILER_MSVC)
    #define MOCK_COM_UTIL_LINK_IMPL(func) __pragma(comment(linker, "/INCLUDE:_mock_impl_" #func))

// mock_com_util/<dir>/*.cc の弱リンク実装 (MOCK_WEAK_IMPL) は、参照側の翻訳単位が
// 存在しない場合に MSVC がオブジェクトをリンクへ取り込まず、com_util_* が未解決に
// なることがある。MOCK_COM_UTIL_LINK_IMPL(func) は /INCLUDE:_mock_impl_<func> を
// リンカーへ渡し、実体を明示的に含める。以下は mock_com_util/ 配下のディレクトリ
// 構成 (argparser/clock/compress/console/crt/crypto/hashtable/net/prompt/runtime/sync/trace/win32)
// に沿って、各 .cc ファイルが実装する関数を定義順に列挙する。
// com_util_syslog_sink_* は Linux 専用のため、この Windows 専用ブロックには含めない。

// argparser
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_flag)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_option_int)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_option_string)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_option_int_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_option_string_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_positional_int)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_positional_string)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_positional_int_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_register_positional_string_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_parse)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_error_message)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_usage)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_print_usage)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_print_error_messages)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_register_error)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_register_error_message)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_print_register_error_messages)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_error)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_error_target)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_error_index)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_register_error_count)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_get_register_error_target)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_init)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_flag)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_option_int)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_option_string)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_option_int_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_option_string_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_positional_int)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_positional_string)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_positional_int_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_register_positional_string_array)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_parse)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_error_message)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_usage)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_print_usage)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_print_error_messages)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_register_error)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_register_error_message)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_print_register_error_messages)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_error)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_error_target)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_error_index)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_register_error_count)
MOCK_COM_UTIL_LINK_IMPL(com_util_argparser_default_get_register_error_target)

// clock
MOCK_COM_UTIL_LINK_IMPL(com_util_format_realtime_iso8601_local)
MOCK_COM_UTIL_LINK_IMPL(com_util_format_realtime_iso8601_utc)
MOCK_COM_UTIL_LINK_IMPL(com_util_get_monotonic)
MOCK_COM_UTIL_LINK_IMPL(com_util_get_monotonic_ms)
MOCK_COM_UTIL_LINK_IMPL(com_util_get_realtime)
MOCK_COM_UTIL_LINK_IMPL(com_util_get_realtime_deadline_ms)
MOCK_COM_UTIL_LINK_IMPL(com_util_get_realtime_utc)
MOCK_COM_UTIL_LINK_IMPL(com_util_timespec_add_ms)
MOCK_COM_UTIL_LINK_IMPL(com_util_timespec_cmp)
MOCK_COM_UTIL_LINK_IMPL(com_util_timespec_from_native)
MOCK_COM_UTIL_LINK_IMPL(com_util_timespec_to_native)

// hashtable
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_required_size)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_attach)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_validate)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_config_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_config_val)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_buffer_size)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_buffer_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_add)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_upsert)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_insert_direct)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_update)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_update_rec)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_find_value_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_find_value_copy)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_find_recno)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_key_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_key_copy)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_value_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_value_copy)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_status)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_next_record)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_timestamp_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_timestamp_val)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_generation)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_table_timestamp_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_table_timestamp_val)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_get_table_generation)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_find_timestamp_ref)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_find_timestamp_val)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_find_generation)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_count_status)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_count)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_deleted_count)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_empty_count)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_delete)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_delete_rec)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_push_deleted)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_purge_deleted)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_compact)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_resize)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_rebuild_into)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_clear)
MOCK_COM_UTIL_LINK_IMPL(com_util_hashtable_dispose)

// compress
MOCK_COM_UTIL_LINK_IMPL(com_util_compress)
MOCK_COM_UTIL_LINK_IMPL(com_util_decompress)

// console
MOCK_COM_UTIL_LINK_IMPL(com_util_console_attach_parent)
MOCK_COM_UTIL_LINK_IMPL(com_util_console_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_console_dispose_on_shutdown)
MOCK_COM_UTIL_LINK_IMPL(com_util_console_init)

// crt
MOCK_COM_UTIL_LINK_IMPL(com_util_access)
MOCK_COM_UTIL_LINK_IMPL(com_util_access_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_close)
MOCK_COM_UTIL_LINK_IMPL(com_util_ctime)
MOCK_COM_UTIL_LINK_IMPL(com_util_dup)
MOCK_COM_UTIL_LINK_IMPL(com_util_dup2)
MOCK_COM_UTIL_LINK_IMPL(com_util_fclose)
MOCK_COM_UTIL_LINK_IMPL(com_util_fflush)
MOCK_COM_UTIL_LINK_IMPL(com_util_fgets)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_close)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_flush)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_get_id)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_get_path_id)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_get_size)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_init)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_open)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_read)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_set_size)
MOCK_COM_UTIL_LINK_IMPL(com_util_file_write)
MOCK_COM_UTIL_LINK_IMPL(com_util_fopen)
MOCK_COM_UTIL_LINK_IMPL(com_util_fopen_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_fopen_temp)
MOCK_COM_UTIL_LINK_IMPL(com_util_fprintf)
MOCK_COM_UTIL_LINK_IMPL(com_util_fread)
MOCK_COM_UTIL_LINK_IMPL(com_util_freopen)
MOCK_COM_UTIL_LINK_IMPL(com_util_fscanf)
MOCK_COM_UTIL_LINK_IMPL(com_util_fseek)
MOCK_COM_UTIL_LINK_IMPL(com_util_ftell)
MOCK_COM_UTIL_LINK_IMPL(com_util_fwrite)
MOCK_COM_UTIL_LINK_IMPL(com_util_getenv)
MOCK_COM_UTIL_LINK_IMPL(com_util_gmtime)
MOCK_COM_UTIL_LINK_IMPL(com_util_isatty)
MOCK_COM_UTIL_LINK_IMPL(com_util_localtime)
MOCK_COM_UTIL_LINK_IMPL(com_util_lseek)
MOCK_COM_UTIL_LINK_IMPL(com_util_makedirs)
MOCK_COM_UTIL_LINK_IMPL(com_util_mkdir)
MOCK_COM_UTIL_LINK_IMPL(com_util_mkdir_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_normalize_path_sep)
MOCK_COM_UTIL_LINK_IMPL(com_util_open)
MOCK_COM_UTIL_LINK_IMPL(com_util_open_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_parse_double)
MOCK_COM_UTIL_LINK_IMPL(com_util_parse_int)
MOCK_COM_UTIL_LINK_IMPL(com_util_parse_int64)
MOCK_COM_UTIL_LINK_IMPL(com_util_parse_uint64)
MOCK_COM_UTIL_LINK_IMPL(com_util_path_basename)
MOCK_COM_UTIL_LINK_IMPL(com_util_path_dirname)
MOCK_COM_UTIL_LINK_IMPL(com_util_path_get_full)
MOCK_COM_UTIL_LINK_IMPL(com_util_path_join_n)
MOCK_COM_UTIL_LINK_IMPL(com_util_path_strip_extension)
MOCK_COM_UTIL_LINK_IMPL(com_util_vpath_join_n)
MOCK_COM_UTIL_LINK_IMPL(com_util_paths_equal)
MOCK_COM_UTIL_LINK_IMPL(com_util_read)
MOCK_COM_UTIL_LINK_IMPL(com_util_remove)
MOCK_COM_UTIL_LINK_IMPL(com_util_remove_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_rename)
MOCK_COM_UTIL_LINK_IMPL(com_util_rmdir)
MOCK_COM_UTIL_LINK_IMPL(com_util_scanf)
MOCK_COM_UTIL_LINK_IMPL(com_util_setenv)
MOCK_COM_UTIL_LINK_IMPL(com_util_snprintf)
MOCK_COM_UTIL_LINK_IMPL(com_util_sscanf)
MOCK_COM_UTIL_LINK_IMPL(com_util_stat)
MOCK_COM_UTIL_LINK_IMPL(com_util_stat_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_strcat)
MOCK_COM_UTIL_LINK_IMPL(com_util_strcpy)
MOCK_COM_UTIL_LINK_IMPL(com_util_strdup)
MOCK_COM_UTIL_LINK_IMPL(com_util_malloc)
MOCK_COM_UTIL_LINK_IMPL(com_util_calloc)
MOCK_COM_UTIL_LINK_IMPL(com_util_realloc)
MOCK_COM_UTIL_LINK_IMPL(com_util_realloc_zerofill)
MOCK_COM_UTIL_LINK_IMPL(com_util_free)
MOCK_COM_UTIL_LINK_IMPL(com_util_strncat)
MOCK_COM_UTIL_LINK_IMPL(com_util_strncpy)
MOCK_COM_UTIL_LINK_IMPL(com_util_strtok_r)
MOCK_COM_UTIL_LINK_IMPL(com_util_unsetenv)
MOCK_COM_UTIL_LINK_IMPL(com_util_vaccess_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_vfopen_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_vfprintf)
MOCK_COM_UTIL_LINK_IMPL(com_util_vfscanf)
MOCK_COM_UTIL_LINK_IMPL(com_util_vmkdir_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_vopen_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_vremove_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_vscanf)
MOCK_COM_UTIL_LINK_IMPL(com_util_vsnprintf)
MOCK_COM_UTIL_LINK_IMPL(com_util_vsscanf)
MOCK_COM_UTIL_LINK_IMPL(com_util_vstat_fmt)
MOCK_COM_UTIL_LINK_IMPL(com_util_wcscpy)
MOCK_COM_UTIL_LINK_IMPL(com_util_write)
MOCK_COM_UTIL_LINK_IMPL(com_util_utf8_to_wpath)
MOCK_COM_UTIL_LINK_IMPL(com_util_wpath_to_utf8)
MOCK_COM_UTIL_LINK_IMPL(com_util_utf8_to_wstr_alloc)
MOCK_COM_UTIL_LINK_IMPL(com_util_wstr_to_utf8_alloc)

// crypto
MOCK_COM_UTIL_LINK_IMPL(com_util_decrypt)
MOCK_COM_UTIL_LINK_IMPL(com_util_encrypt)
MOCK_COM_UTIL_LINK_IMPL(com_util_passphrase_to_key)
MOCK_COM_UTIL_LINK_IMPL(com_util_random_bytes)

// net
MOCK_COM_UTIL_LINK_IMPL(com_util_hton16)
MOCK_COM_UTIL_LINK_IMPL(com_util_ntoh16)
MOCK_COM_UTIL_LINK_IMPL(com_util_hton32)
MOCK_COM_UTIL_LINK_IMPL(com_util_ntoh32)
MOCK_COM_UTIL_LINK_IMPL(com_util_ipv4_parse)
MOCK_COM_UTIL_LINK_IMPL(com_util_ipv4_resolve)
MOCK_COM_UTIL_LINK_IMPL(com_util_ipv4_to_string)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_accept)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_bind)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_close)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_connect)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_get_pending_error)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_join_multicast_group)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_leave_multicast_group)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_listen)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_open)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_recv)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_recv_all)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_recvfrom)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_send)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_send_all)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_sendto)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_set_broadcast)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_set_multicast_interface)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_set_nonblocking)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_set_reuse_address)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_shutdown)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_shutdown_receive)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_wait_readable)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_wait_readable_multi)
MOCK_COM_UTIL_LINK_IMPL(com_util_socket_wait_writable)

// prompt
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_printf)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_readline_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_readline_fmt_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_status_enable)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_status_set)
MOCK_COM_UTIL_LINK_IMPL(com_util_pinned_prompt_write)
MOCK_COM_UTIL_LINK_IMPL(com_util_prompt_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_prompt_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_prompt_readline_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_prompt_readline_fmt_at)

// runtime
MOCK_COM_UTIL_LINK_IMPL(com_util_elevated_process_extract_result_target)
MOCK_COM_UTIL_LINK_IMPL(com_util_elevated_process_is_elevated)
MOCK_COM_UTIL_LINK_IMPL(com_util_elevated_process_report_result)
MOCK_COM_UTIL_LINK_IMPL(com_util_elevated_process_run_if_needed)
MOCK_COM_UTIL_LINK_IMPL(com_util_elevated_process_run_with_result)
MOCK_COM_UTIL_LINK_IMPL(com_util_memory_lock_range)
MOCK_COM_UTIL_LINK_IMPL(com_util_memory_lock_scope_release)
MOCK_COM_UTIL_LINK_IMPL(com_util_memory_lock_self)
MOCK_COM_UTIL_LINK_IMPL(com_util_memory_unlock_range)
MOCK_COM_UTIL_LINK_IMPL(com_util_module_get_basename)
MOCK_COM_UTIL_LINK_IMPL(com_util_module_get_path)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_get_executable_path)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_get_pid)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_get_exit_code)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_run_sync)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_start)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_terminate)
MOCK_COM_UTIL_LINK_IMPL(com_util_process_wait)
MOCK_COM_UTIL_LINK_IMPL(com_util_secure_zero)
MOCK_COM_UTIL_LINK_IMPL(com_util_shutdown_register)
MOCK_COM_UTIL_LINK_IMPL(com_util_shutdown_request_register)
MOCK_COM_UTIL_LINK_IMPL(com_util_sym_loader_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_sym_loader_info)
MOCK_COM_UTIL_LINK_IMPL(com_util_sym_loader_init)
MOCK_COM_UTIL_LINK_IMPL(com_util_sym_loader_is_default)
MOCK_COM_UTIL_LINK_IMPL(com_util_sym_loader_resolve)

// sync
MOCK_COM_UTIL_LINK_IMPL(com_util_call_once)
MOCK_COM_UTIL_LINK_IMPL(com_util_sleep_ms)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_lock_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_lock_lock)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_lock_try_lock)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_lock_unlock)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_lock_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_condvar_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_condvar_wait)
MOCK_COM_UTIL_LINK_IMPL(com_util_condvar_signal)
MOCK_COM_UTIL_LINK_IMPL(com_util_condvar_broadcast)
MOCK_COM_UTIL_LINK_IMPL(com_util_condvar_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_lock_shared)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_try_lock_shared)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_lock_exclusive)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_try_lock_exclusive)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_unlock_shared)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_unlock_exclusive)
MOCK_COM_UTIL_LINK_IMPL(com_util_local_rwlock_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_thread_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_thread_join)
MOCK_COM_UTIL_LINK_IMPL(com_util_thread_detach)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_open)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_import_descriptor)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_export_descriptor)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_lock)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_try_lock)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_unlock)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_lock_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_open)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_import_descriptor)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_export_descriptor)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_lock_shared)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_try_lock_shared)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_lock_exclusive)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_try_lock_exclusive)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_unlock)
MOCK_COM_UTIL_LINK_IMPL(com_util_interprocess_rwlock_dispose)

// trace
MOCK_COM_UTIL_LINK_IMPL(com_util_etw_provider_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_etw_provider_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_etw_provider_write)
MOCK_COM_UTIL_LINK_IMPL(com_util_etw_session_check_access)
MOCK_COM_UTIL_LINK_IMPL(com_util_etw_session_start)
MOCK_COM_UTIL_LINK_IMPL(com_util_etw_session_stop)
MOCK_COM_UTIL_LINK_IMPL(com_util_eventlog_register_source)
MOCK_COM_UTIL_LINK_IMPL(com_util_eventlog_sink_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_eventlog_sink_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_eventlog_sink_write)
MOCK_COM_UTIL_LINK_IMPL(com_util_eventlog_unregister_source)
MOCK_COM_UTIL_LINK_IMPL(com_util_trace_file_sink_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_trace_file_sink_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_trace_file_sink_write)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_call_next_hook)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_create)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_dispose)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_get_etw_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_get_file_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_get_os_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_get_state)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_get_stderr_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_remove_hook)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_set_etw_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_set_file_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_set_hook)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_set_name)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_set_os_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_set_stderr_level)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_start)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_stop)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_write_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_write_hex_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_write_hexf_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_writef_at)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_hex_sep)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_hex_msg)
MOCK_COM_UTIL_LINK_IMPL(com_util_tracer_write_with_source)

// win32
MOCK_COM_UTIL_LINK_IMPL(ChangeServiceConfig2U)
MOCK_COM_UTIL_LINK_IMPL(CreateFileU)
MOCK_COM_UTIL_LINK_IMPL(CreateNamedPipeU)
MOCK_COM_UTIL_LINK_IMPL(CreateProcessU)
MOCK_COM_UTIL_LINK_IMPL(CreateServiceU)
MOCK_COM_UTIL_LINK_IMPL(GetModuleFileNameU)
MOCK_COM_UTIL_LINK_IMPL(GetVolumeInformationU)
MOCK_COM_UTIL_LINK_IMPL(GetVolumePathNameU)
MOCK_COM_UTIL_LINK_IMPL(LoadLibraryU)
MOCK_COM_UTIL_LINK_IMPL(OpenSCManagerU)
MOCK_COM_UTIL_LINK_IMPL(OpenServiceU)
MOCK_COM_UTIL_LINK_IMPL(RegisterServiceCtrlHandlerExU)
MOCK_COM_UTIL_LINK_IMPL(StartServiceCtrlDispatcherU)
MOCK_COM_UTIL_LINK_IMPL(WriteConsoleU)

    #undef MOCK_COM_UTIL_LINK_IMPL
#endif /* COMPILER_MSVC */

#include <com_util/compress/compress.h>
#include <com_util/crypto/crypto.h>
#include <com_util/crypto/random.h>
#include <com_util/net/endpoint.h>
#include <com_util/net/socket.h>
#include <com_util/net/byteorder.h>
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
#include <com_util/hashtable/hashtable.h>
#if defined(PLATFORM_WINDOWS)
    #include <com_util/win32/win32.h>
#endif /* PLATFORM_WINDOWS */

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
extern uint16_t delegate_real_com_util_hton16(uint16_t value);
extern uint16_t delegate_real_com_util_ntoh16(uint16_t value);
extern uint32_t delegate_real_com_util_hton32(uint32_t value);
extern uint32_t delegate_real_com_util_ntoh32(uint32_t value);
extern int delegate_real_com_util_ipv4_parse(const char *, uint32_t *);
extern int delegate_real_com_util_ipv4_resolve(const char *, uint32_t *, com_util_error *);
extern int delegate_real_com_util_ipv4_to_string(uint32_t, char *, size_t, com_util_error *);
extern int delegate_real_com_util_socket_open(com_util_socket_kind, com_util_socket *, com_util_error *);
extern void delegate_real_com_util_socket_close(com_util_socket);
extern void delegate_real_com_util_socket_shutdown(com_util_socket);
extern int delegate_real_com_util_socket_bind(com_util_socket, const com_util_ipv4_endpoint *, com_util_error *);
extern int delegate_real_com_util_socket_listen(com_util_socket, int, com_util_error *);
extern int delegate_real_com_util_socket_accept(com_util_socket, com_util_ipv4_endpoint *, com_util_socket *,
                                                com_util_error *);
extern int delegate_real_com_util_socket_connect(com_util_socket, const com_util_ipv4_endpoint *, com_util_error *);
extern int delegate_real_com_util_socket_get_pending_error(com_util_socket, com_util_error *);
extern int delegate_real_com_util_socket_set_nonblocking(com_util_socket, int, com_util_error *);
extern int delegate_real_com_util_socket_set_reuse_address(com_util_socket, int, com_util_error *);
extern int delegate_real_com_util_socket_set_broadcast(com_util_socket, int, com_util_error *);
extern int delegate_real_com_util_socket_set_multicast_interface(com_util_socket, uint32_t, com_util_error *);
extern int delegate_real_com_util_socket_join_multicast_group(com_util_socket, uint32_t, uint32_t, com_util_error *);
extern int delegate_real_com_util_socket_leave_multicast_group(com_util_socket, uint32_t, uint32_t, com_util_error *);
extern int delegate_real_com_util_socket_send(com_util_socket, const void *, size_t, size_t *, com_util_error *);
extern int delegate_real_com_util_socket_recv(com_util_socket, void *, size_t, size_t *, com_util_error *);
extern int delegate_real_com_util_socket_sendto(com_util_socket, const void *, size_t, const com_util_ipv4_endpoint *,
                                                size_t *, com_util_error *);
extern int delegate_real_com_util_socket_recvfrom(com_util_socket, void *, size_t, com_util_ipv4_endpoint *, size_t *,
                                                  com_util_error *);
extern int delegate_real_com_util_socket_send_all(com_util_socket, const void *, size_t, com_util_error *);
extern int delegate_real_com_util_socket_recv_all(com_util_socket, void *, size_t, com_util_error *);
extern int delegate_real_com_util_socket_wait_readable(com_util_socket, int, int *, com_util_error *);
extern int delegate_real_com_util_socket_wait_writable(com_util_socket, int, int *, com_util_error *);
extern int delegate_real_com_util_socket_wait_readable_multi(const com_util_socket *, size_t, int, unsigned char *,
                                                             com_util_error *);
extern int delegate_real_com_util_socket_shutdown_receive(com_util_socket *, com_util_error *);

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
extern char *delegate_real_com_util_normalize_path_sep(char *path);
extern int delegate_real_com_util_paths_equal(const char *lhs, const char *rhs, int *equal_out,
                                              com_util_error *detail_out);
extern const char *delegate_real_com_util_path_basename(const char *path);
extern int delegate_real_com_util_path_dirname(char *path_out, size_t path_size, com_util_error *detail_out,
                                               const char *path);
extern int delegate_real_com_util_path_strip_extension(char *path_out, size_t path_size, com_util_error *detail_out,
                                                       const char *path);
extern int delegate_real_com_util_path_join_n(char *path_out, size_t path_size, com_util_error *detail_out,
                                              size_t part_count, ...);
extern int delegate_real_com_util_vpath_join_n(char *path_out, size_t path_size, com_util_error *detail_out,
                                               size_t part_count, va_list args);

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
extern void *delegate_real_com_util_malloc(size_t size);
extern void *delegate_real_com_util_calloc(size_t count, size_t size);
extern void *delegate_real_com_util_realloc(void *ptr, size_t count, size_t size);
extern void *delegate_real_com_util_realloc_zerofill(void *ptr, size_t old_count, size_t count, size_t size);
extern void delegate_real_com_util_free(void *ptr);
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
extern com_util_tracer *delegate_real_com_util_tracer_create(com_util_tracer_concurrency_mode concurrency_mode);
extern void delegate_real_com_util_tracer_dispose(com_util_tracer **handle);
extern int delegate_real_com_util_tracer_start(com_util_tracer *handle);
extern int delegate_real_com_util_tracer_stop(com_util_tracer *handle);
extern int delegate_real_com_util_tracer_write_at(com_util_tracer *handle, com_util_trace_level level,
                                                  const com_util_timespec *timestamp, const char *message);
extern int delegate_real_com_util_tracer_write_hex_at(com_util_tracer *handle, com_util_trace_level level,
                                                      const com_util_timespec *timestamp, const void *data, size_t size,
                                                      const char *message);
extern int delegate_real_com_util_tracer_writef_at(com_util_tracer *handle, com_util_trace_level level,
                                                   const com_util_timespec *timestamp, const char *format, ...);
extern int delegate_real_com_util_tracer_write_hexf_at(com_util_tracer *handle, com_util_trace_level level,
                                                       const com_util_timespec *timestamp, const void *data,
                                                       size_t size, const char *format, ...);
extern const char *delegate_real_com_util_tracer_hex_sep(const char *message);
extern const char *delegate_real_com_util_tracer_hex_msg(const char *message);
extern int delegate_real_com_util_tracer_write_with_source(com_util_tracer *handle, com_util_trace_level level,
                                                           const com_util_timespec *timestamp, const char *file,
                                                           int line, const char *message);
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
                                                         com_util_trace_level level, const com_util_timespec *timestamp,
                                                         const char *message);
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
extern void delegate_real_com_util_timespec_from_native(const struct timespec *native, com_util_timespec *ts);
extern void delegate_real_com_util_timespec_to_native(const com_util_timespec *ts, struct timespec *native);
extern void delegate_real_com_util_timespec_add_ms(const com_util_timespec *ts, uint64_t timeout_ms,
                                                   com_util_timespec *result);
extern int delegate_real_com_util_timespec_cmp(const com_util_timespec *a, const com_util_timespec *b);

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
extern void delegate_real_com_util_local_lock_dispose(com_util_local_lock *mtx);
extern int delegate_real_com_util_condvar_create(com_util_condvar **cv);
extern int delegate_real_com_util_condvar_wait(com_util_condvar *cv, com_util_local_lock *mtx, int timeout_ms);
extern int delegate_real_com_util_condvar_signal(com_util_condvar *cv);
extern int delegate_real_com_util_condvar_broadcast(com_util_condvar *cv);
extern void delegate_real_com_util_condvar_dispose(com_util_condvar *cv);
extern int delegate_real_com_util_local_rwlock_create(com_util_local_rwlock **rwlock);
extern int delegate_real_com_util_local_rwlock_lock_shared(com_util_local_rwlock *rwlock, int timeout_ms);
extern int delegate_real_com_util_local_rwlock_try_lock_shared(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_local_rwlock_lock_exclusive(com_util_local_rwlock *rwlock, int timeout_ms);
extern int delegate_real_com_util_local_rwlock_try_lock_exclusive(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_local_rwlock_unlock_shared(com_util_local_rwlock *rwlock);
extern int delegate_real_com_util_local_rwlock_unlock_exclusive(com_util_local_rwlock *rwlock);
extern void delegate_real_com_util_local_rwlock_dispose(com_util_local_rwlock *rwlock);
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
extern void delegate_real_com_util_interprocess_lock_dispose(com_util_interprocess_lock *lock);
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
extern void delegate_real_com_util_interprocess_rwlock_dispose(com_util_interprocess_rwlock *lock);
extern void delegate_real_com_util_call_once(com_util_once_flag *flag, com_util_once_fn func);
extern void delegate_real_com_util_sleep_ms(int ms);

// runtime - module_info
extern int delegate_real_com_util_module_get_path(char *path_out, size_t path_size, const void *func_addr);
extern int delegate_real_com_util_module_get_basename(char *basename_out, size_t basename_size, const void *func_addr);

// runtime - memory_lock
extern int delegate_real_com_util_memory_lock_range(const void *address, size_t size);
extern int delegate_real_com_util_memory_unlock_range(const void *address, size_t size);
extern int delegate_real_com_util_memory_lock_self(const com_util_memory_lock_self_options *options,
                                                   com_util_memory_lock_scope **scope);
extern int delegate_real_com_util_memory_lock_scope_release(com_util_memory_lock_scope *scope);
// hashtable
extern int delegate_real_com_util_hashtable_required_size(const com_util_hashtable_config *config,
                                                          size_t *mgmt_size_out, size_t *data_size_out);
extern int delegate_real_com_util_hashtable_create(const com_util_hashtable_config *config, void *buf_mgmt,
                                                   size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                                                   com_util_hashtable **ht_out);
extern int delegate_real_com_util_hashtable_attach(void *buf_mgmt, size_t buf_mgmt_size, void *buf_data,
                                                   size_t buf_data_size, com_util_hashtable **ht_out);
extern int delegate_real_com_util_hashtable_validate(const com_util_hashtable *ht);
extern int delegate_real_com_util_hashtable_get_config_ref(const com_util_hashtable *ht,
                                                           const com_util_hashtable_config **config_out);
extern int delegate_real_com_util_hashtable_get_config_val(const com_util_hashtable *ht,
                                                           com_util_hashtable_config *config_out);
extern int delegate_real_com_util_hashtable_buffer_size(const com_util_hashtable *ht, size_t *mgmt_size_out,
                                                        size_t *data_size_out);
extern int delegate_real_com_util_hashtable_buffer_ref(const com_util_hashtable *ht, const void **mgmt_out,
                                                       const void **data_out);
extern int delegate_real_com_util_hashtable_add(com_util_hashtable *ht, const void *key, const void *value,
                                                com_util_hashtable_add_deleted_policy deleted_policy);
extern int delegate_real_com_util_hashtable_upsert(com_util_hashtable *ht, const void *key, const void *value,
                                                   int *inserted_out);
extern int delegate_real_com_util_hashtable_insert_direct(com_util_hashtable *ht, uint64_t record, const void *key,
                                                          int status, const void *value,
                                                          const com_util_timespec *timestamp, uint64_t generation);
extern int delegate_real_com_util_hashtable_update(com_util_hashtable *ht, const void *key, const void *value);
extern int delegate_real_com_util_hashtable_update_rec(com_util_hashtable *ht, uint64_t record, const void *value);
extern int delegate_real_com_util_hashtable_find_value_ref(const com_util_hashtable *ht, const void *key,
                                                           const void **value_out);
extern int delegate_real_com_util_hashtable_find_value_copy(const com_util_hashtable *ht, const void *key, void *dest,
                                                            size_t dest_size, size_t *required_size_out);
extern int delegate_real_com_util_hashtable_find_recno(const com_util_hashtable *ht, const void *key,
                                                       uint64_t *record_out);
extern int delegate_real_com_util_hashtable_get_key_ref(const com_util_hashtable *ht, uint64_t record,
                                                        const void **key_out);
extern int delegate_real_com_util_hashtable_get_key_copy(const com_util_hashtable *ht, uint64_t record, void *dest,
                                                         size_t dest_size, size_t *required_size_out);
extern int delegate_real_com_util_hashtable_get_value_ref(const com_util_hashtable *ht, uint64_t record,
                                                          const void **value_out);
extern int delegate_real_com_util_hashtable_get_value_copy(const com_util_hashtable *ht, uint64_t record, void *dest,
                                                           size_t dest_size, size_t *required_size_out);
extern int delegate_real_com_util_hashtable_get_status(const com_util_hashtable *ht, uint64_t record, int *status_out);
extern int delegate_real_com_util_hashtable_next_record(const com_util_hashtable *ht, uint64_t from,
                                                        unsigned int status_mask, uint64_t *record_out,
                                                        int *has_record_out);
extern int delegate_real_com_util_hashtable_get_timestamp_ref(const com_util_hashtable *ht, uint64_t record,
                                                              const com_util_timespec **timestamp_out);
extern int delegate_real_com_util_hashtable_get_timestamp_val(const com_util_hashtable *ht, uint64_t record,
                                                              com_util_timespec *timestamp_out);
extern int delegate_real_com_util_hashtable_get_generation(const com_util_hashtable *ht, uint64_t record,
                                                           uint64_t *generation_out);
extern int delegate_real_com_util_hashtable_get_table_timestamp_ref(const com_util_hashtable *ht,
                                                                    const com_util_timespec **timestamp_out);
extern int delegate_real_com_util_hashtable_get_table_timestamp_val(const com_util_hashtable *ht,
                                                                    com_util_timespec *timestamp_out);
extern int delegate_real_com_util_hashtable_get_table_generation(const com_util_hashtable *ht,
                                                                 uint64_t *generation_out);
extern int delegate_real_com_util_hashtable_find_timestamp_ref(const com_util_hashtable *ht, const void *key,
                                                               const com_util_timespec **timestamp_out);
extern int delegate_real_com_util_hashtable_find_timestamp_val(const com_util_hashtable *ht, const void *key,
                                                               com_util_timespec *timestamp_out);
extern int delegate_real_com_util_hashtable_find_generation(const com_util_hashtable *ht, const void *key,
                                                            uint64_t *generation_out);
extern int delegate_real_com_util_hashtable_count_status(const com_util_hashtable *ht, size_t *in_use_out,
                                                         size_t *deleted_out, size_t *empty_out);
extern int delegate_real_com_util_hashtable_count(const com_util_hashtable *ht, size_t *count_out);
extern int delegate_real_com_util_hashtable_deleted_count(const com_util_hashtable *ht, size_t *count_out);
extern int delegate_real_com_util_hashtable_empty_count(const com_util_hashtable *ht, size_t *count_out);
extern int delegate_real_com_util_hashtable_delete(com_util_hashtable *ht, const void *key);
extern int delegate_real_com_util_hashtable_delete_rec(com_util_hashtable *ht, uint64_t record);
extern int delegate_real_com_util_hashtable_push_deleted(com_util_hashtable *ht);
extern int delegate_real_com_util_hashtable_purge_deleted(com_util_hashtable *ht);
extern int delegate_real_com_util_hashtable_compact(com_util_hashtable *ht);
extern int delegate_real_com_util_hashtable_resize(com_util_hashtable *ht,
                                                   const com_util_hashtable_config *new_config);
extern int delegate_real_com_util_hashtable_rebuild_into(const com_util_hashtable *src,
                                                         const com_util_hashtable_config *new_config, void *buf_mgmt,
                                                         size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                                                         com_util_hashtable **ht_out);
extern int delegate_real_com_util_hashtable_clear(com_util_hashtable *ht);
extern void delegate_real_com_util_hashtable_dispose(com_util_hashtable *ht);

extern void delegate_real_com_util_secure_zero(void *buf, size_t size);

// runtime - process_info
extern int delegate_real_com_util_process_get_executable_path(char *path_out, size_t path_size);
extern uint32_t delegate_real_com_util_process_get_pid(void);
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
extern void delegate_real_com_util_process_dispose(com_util_process *process);
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
// win32 - file_api (Windows only)
extern HANDLE delegate_real_CreateFileU(const char *utf8_path, DWORD desired_access, DWORD share_mode,
                                        LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition,
                                        DWORD flags_and_attributes, HANDLE template_file);
extern HANDLE delegate_real_CreateNamedPipeU(const char *utf8_name, DWORD open_mode, DWORD pipe_mode,
                                             DWORD max_instances, DWORD out_buffer_size, DWORD in_buffer_size,
                                             DWORD default_timeout, LPSECURITY_ATTRIBUTES security_attributes);
extern DWORD delegate_real_GetModuleFileNameU(HMODULE module, char *utf8_buf, DWORD size);
extern BOOL delegate_real_GetVolumePathNameU(const char *utf8_path, char *utf8_volume_root, DWORD size);
extern BOOL delegate_real_GetVolumeInformationU(const char *utf8_root_path, char *utf8_volume_name,
                                                DWORD volume_name_size, DWORD *serial_number,
                                                DWORD *max_component_length, DWORD *file_system_flags,
                                                char *utf8_file_system_name, DWORD file_system_name_size);
extern HMODULE delegate_real_LoadLibraryU(const char *utf8_file_name);
extern BOOL delegate_real_WriteConsoleU(HANDLE console, const char *utf8_text, DWORD utf8_length, DWORD *written_length,
                                        void *reserved);
extern BOOL delegate_real_CreateProcessU(const char *utf8_application_name, const char *utf8_command_line,
                                         LPSECURITY_ATTRIBUTES process_attributes,
                                         LPSECURITY_ATTRIBUTES thread_attributes, BOOL inherit_handles,
                                         DWORD creation_flags, LPVOID environment, const char *utf8_current_directory,
                                         LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION process_information);
extern SC_HANDLE delegate_real_OpenSCManagerU(const char *utf8_machine_name, const char *utf8_database_name,
                                              DWORD desired_access);
extern SC_HANDLE delegate_real_CreateServiceU(SC_HANDLE scm, const char *utf8_service_name,
                                              const char *utf8_display_name, DWORD desired_access, DWORD service_type,
                                              DWORD start_type, DWORD error_control, const char *utf8_binary_path_name,
                                              const char *utf8_load_order_group, LPDWORD tag_id,
                                              const char *utf8_dependencies, const char *utf8_service_start_name,
                                              const char *utf8_password);
extern SC_HANDLE delegate_real_OpenServiceU(SC_HANDLE scm, const char *utf8_service_name, DWORD desired_access);
extern BOOL delegate_real_ChangeServiceConfig2U(SC_HANDLE service, DWORD info_level, const char *utf8_text);
extern SERVICE_STATUS_HANDLE delegate_real_RegisterServiceCtrlHandlerExU(const char *utf8_service_name,
                                                                         LPHANDLER_FUNCTION_EX handler_proc,
                                                                         LPVOID context);
extern BOOL delegate_real_StartServiceCtrlDispatcherU(const com_util_service_entry_u *service_table);

// crt - wchar_conv (Windows only)
extern int delegate_real_com_util_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path);
extern int delegate_real_com_util_wpath_to_utf8(char *dest, size_t dest_size, const wchar_t *wpath);
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
extern int delegate_real_com_util_pinned_prompt_readline_at(com_util_pinned_prompt *screen, char *buf, size_t buf_size,
                                                            const char *prompt_str, const char *file, int line);
extern int delegate_real_com_util_pinned_prompt_readline_fmt_at(com_util_pinned_prompt *screen, char *buf,
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
extern com_util_argparser *delegate_real_com_util_argparser_create(const com_util_argparser_options *options);
extern com_util_argparser *delegate_real_com_util_argparser_default(const com_util_argparser_options *options);
extern void delegate_real_com_util_argparser_dispose(com_util_argparser *parser);
extern int delegate_real_com_util_argparser_register_flag(com_util_argparser *parser, const char *short_name,
                                                          const char *long_name, const char *description, int *storage);
extern int delegate_real_com_util_argparser_register_option_int(com_util_argparser *parser, const char *short_name,
                                                                const char *long_name, const char *value_name,
                                                                const char *description, unsigned int flags,
                                                                int *storage);
extern int delegate_real_com_util_argparser_register_option_string(com_util_argparser *parser, const char *short_name,
                                                                   const char *long_name, const char *value_name,
                                                                   const char *description, unsigned int flags,
                                                                   const char **storage);
extern int delegate_real_com_util_argparser_register_option_int_array(com_util_argparser *parser,
                                                                      const char *short_name, const char *long_name,
                                                                      const char *value_name, const char *description,
                                                                      unsigned int flags, int *storage, size_t capacity,
                                                                      size_t *count);
extern int delegate_real_com_util_argparser_register_option_string_array(
    com_util_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
    const char *description, unsigned int flags, const char **storage, size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_register_positional_int(com_util_argparser *parser, const char *name,
                                                                    const char *description, unsigned int flags,
                                                                    int *storage);
extern int delegate_real_com_util_argparser_register_positional_string(com_util_argparser *parser, const char *name,
                                                                       const char *description, unsigned int flags,
                                                                       const char **storage);
extern int delegate_real_com_util_argparser_register_positional_int_array(com_util_argparser *parser, const char *name,
                                                                          const char *description, unsigned int flags,
                                                                          int *storage, size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_register_positional_string_array(com_util_argparser *parser,
                                                                             const char *name, const char *description,
                                                                             unsigned int flags, const char **storage,
                                                                             size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_parse(com_util_argparser *parser, int argc, char *const *argv);
extern int delegate_real_com_util_argparser_get_error(const com_util_argparser *parser);
extern const char *delegate_real_com_util_argparser_get_error_target(const com_util_argparser *parser);
extern int delegate_real_com_util_argparser_get_error_index(const com_util_argparser *parser);
extern int delegate_real_com_util_argparser_get_error_message(const com_util_argparser *parser, char *buffer,
                                                              size_t buffer_size);
extern int delegate_real_com_util_argparser_get_usage(const com_util_argparser *parser, char *buffer,
                                                      size_t buffer_size, size_t *required_size);
extern int delegate_real_com_util_argparser_print_usage(const com_util_argparser *parser, FILE *stream);
extern int delegate_real_com_util_argparser_print_error_messages(const com_util_argparser *parser, FILE *stream);
extern int delegate_real_com_util_argparser_get_register_error(const com_util_argparser *parser, size_t index);
extern size_t delegate_real_com_util_argparser_get_register_error_count(const com_util_argparser *parser);
extern const char *delegate_real_com_util_argparser_get_register_error_target(const com_util_argparser *parser,
                                                                              size_t index);
extern int delegate_real_com_util_argparser_get_register_error_message(const com_util_argparser *parser, size_t index,
                                                                       char *buffer, size_t buffer_size);
extern int delegate_real_com_util_argparser_print_register_error_messages(const com_util_argparser *parser,
                                                                          FILE *stream);

// argparser (省略可能な単一インスタンス API)
extern void delegate_real_com_util_argparser_default_init(const char *description);
extern int delegate_real_com_util_argparser_default_register_flag(const char *short_name, const char *long_name,
                                                                  const char *description, int *storage);
extern int delegate_real_com_util_argparser_default_register_option_int(const char *short_name, const char *long_name,
                                                                        const char *value_name, const char *description,
                                                                        unsigned int flags, int *storage);
extern int delegate_real_com_util_argparser_default_register_option_string(const char *short_name,
                                                                           const char *long_name,
                                                                           const char *value_name,
                                                                           const char *description, unsigned int flags,
                                                                           const char **storage);
extern int delegate_real_com_util_argparser_default_register_option_int_array(
    const char *short_name, const char *long_name, const char *value_name, const char *description, unsigned int flags,
    int *storage, size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_default_register_option_string_array(
    const char *short_name, const char *long_name, const char *value_name, const char *description, unsigned int flags,
    const char **storage, size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_default_register_positional_int(const char *name, const char *description,
                                                                            unsigned int flags, int *storage);
extern int delegate_real_com_util_argparser_default_register_positional_string(const char *name,
                                                                               const char *description,
                                                                               unsigned int flags,
                                                                               const char **storage);
extern int delegate_real_com_util_argparser_default_register_positional_int_array(const char *name,
                                                                                  const char *description,
                                                                                  unsigned int flags, int *storage,
                                                                                  size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_default_register_positional_string_array(const char *name,
                                                                                     const char *description,
                                                                                     unsigned int flags,
                                                                                     const char **storage,
                                                                                     size_t capacity, size_t *count);
extern int delegate_real_com_util_argparser_default_parse(int argc, char *const *argv);
extern int delegate_real_com_util_argparser_default_get_error(void);
extern const char *delegate_real_com_util_argparser_default_get_error_target(void);
extern int delegate_real_com_util_argparser_default_get_error_index(void);
extern int delegate_real_com_util_argparser_default_get_error_message(char *buffer, size_t buffer_size);
extern int delegate_real_com_util_argparser_default_get_usage(char *buffer, size_t buffer_size, size_t *required_size);
extern int delegate_real_com_util_argparser_default_print_usage(FILE *stream);
extern int delegate_real_com_util_argparser_default_print_error_messages(FILE *stream);
extern int delegate_real_com_util_argparser_default_get_register_error(size_t index);
extern size_t delegate_real_com_util_argparser_default_get_register_error_count(void);
extern const char *delegate_real_com_util_argparser_default_get_register_error_target(size_t index);
extern int delegate_real_com_util_argparser_default_get_register_error_message(size_t index, char *buffer,
                                                                               size_t buffer_size);
extern int delegate_real_com_util_argparser_default_print_register_error_messages(FILE *stream);

class Mock_com_util
{
  public:
    // compress
    // hashtable
    MOCK_METHOD(int, com_util_hashtable_required_size, (const com_util_hashtable_config *, size_t *, size_t *));
    MOCK_METHOD(int, com_util_hashtable_create,
                (const com_util_hashtable_config *, void *, size_t, void *, size_t, com_util_hashtable **));
    MOCK_METHOD(int, com_util_hashtable_attach, (void *, size_t, void *, size_t, com_util_hashtable **));
    MOCK_METHOD(int, com_util_hashtable_validate, (const com_util_hashtable *));
    MOCK_METHOD(int, com_util_hashtable_get_config_ref,
                (const com_util_hashtable *, const com_util_hashtable_config **));
    MOCK_METHOD(int, com_util_hashtable_get_config_val, (const com_util_hashtable *, com_util_hashtable_config *));
    MOCK_METHOD(int, com_util_hashtable_buffer_size, (const com_util_hashtable *, size_t *, size_t *));
    MOCK_METHOD(int, com_util_hashtable_buffer_ref, (const com_util_hashtable *, const void **, const void **));
    MOCK_METHOD(int, com_util_hashtable_add,
                (com_util_hashtable *, const void *, const void *, com_util_hashtable_add_deleted_policy));
    MOCK_METHOD(int, com_util_hashtable_upsert, (com_util_hashtable *, const void *, const void *, int *));
    MOCK_METHOD(int, com_util_hashtable_insert_direct,
                (com_util_hashtable *, uint64_t, const void *, int, const void *, const com_util_timespec *,
                 uint64_t));
    MOCK_METHOD(int, com_util_hashtable_update, (com_util_hashtable *, const void *, const void *));
    MOCK_METHOD(int, com_util_hashtable_update_rec, (com_util_hashtable *, uint64_t, const void *));
    MOCK_METHOD(int, com_util_hashtable_find_value_ref, (const com_util_hashtable *, const void *, const void **));
    MOCK_METHOD(int, com_util_hashtable_find_value_copy,
                (const com_util_hashtable *, const void *, void *, size_t, size_t *));
    MOCK_METHOD(int, com_util_hashtable_find_recno, (const com_util_hashtable *, const void *, uint64_t *));
    MOCK_METHOD(int, com_util_hashtable_get_key_ref, (const com_util_hashtable *, uint64_t, const void **));
    MOCK_METHOD(int, com_util_hashtable_get_key_copy, (const com_util_hashtable *, uint64_t, void *, size_t, size_t *));
    MOCK_METHOD(int, com_util_hashtable_get_value_ref, (const com_util_hashtable *, uint64_t, const void **));
    MOCK_METHOD(int, com_util_hashtable_get_value_copy,
                (const com_util_hashtable *, uint64_t, void *, size_t, size_t *));
    MOCK_METHOD(int, com_util_hashtable_get_status, (const com_util_hashtable *, uint64_t, int *));
    MOCK_METHOD(int, com_util_hashtable_next_record,
                (const com_util_hashtable *, uint64_t, unsigned int, uint64_t *, int *));
    MOCK_METHOD(int, com_util_hashtable_get_timestamp_ref,
                (const com_util_hashtable *, uint64_t, const com_util_timespec **));
    MOCK_METHOD(int, com_util_hashtable_get_timestamp_val, (const com_util_hashtable *, uint64_t, com_util_timespec *));
    MOCK_METHOD(int, com_util_hashtable_get_generation, (const com_util_hashtable *, uint64_t, uint64_t *));
    MOCK_METHOD(int, com_util_hashtable_get_table_timestamp_ref,
                (const com_util_hashtable *, const com_util_timespec **));
    MOCK_METHOD(int, com_util_hashtable_get_table_timestamp_val, (const com_util_hashtable *, com_util_timespec *));
    MOCK_METHOD(int, com_util_hashtable_get_table_generation, (const com_util_hashtable *, uint64_t *));
    MOCK_METHOD(int, com_util_hashtable_find_timestamp_ref,
                (const com_util_hashtable *, const void *, const com_util_timespec **));
    MOCK_METHOD(int, com_util_hashtable_find_timestamp_val,
                (const com_util_hashtable *, const void *, com_util_timespec *));
    MOCK_METHOD(int, com_util_hashtable_find_generation, (const com_util_hashtable *, const void *, uint64_t *));
    MOCK_METHOD(int, com_util_hashtable_count_status, (const com_util_hashtable *, size_t *, size_t *, size_t *));
    MOCK_METHOD(int, com_util_hashtable_count, (const com_util_hashtable *, size_t *));
    MOCK_METHOD(int, com_util_hashtable_deleted_count, (const com_util_hashtable *, size_t *));
    MOCK_METHOD(int, com_util_hashtable_empty_count, (const com_util_hashtable *, size_t *));
    MOCK_METHOD(int, com_util_hashtable_delete, (com_util_hashtable *, const void *));
    MOCK_METHOD(int, com_util_hashtable_delete_rec, (com_util_hashtable *, uint64_t));
    MOCK_METHOD(int, com_util_hashtable_push_deleted, (com_util_hashtable *));
    MOCK_METHOD(int, com_util_hashtable_purge_deleted, (com_util_hashtable *));
    MOCK_METHOD(int, com_util_hashtable_compact, (com_util_hashtable *));
    MOCK_METHOD(int, com_util_hashtable_resize, (com_util_hashtable *, const com_util_hashtable_config *));
    MOCK_METHOD(int, com_util_hashtable_rebuild_into,
                (const com_util_hashtable *, const com_util_hashtable_config *, void *, size_t, void *, size_t,
                 com_util_hashtable **));
    MOCK_METHOD(int, com_util_hashtable_clear, (com_util_hashtable *));
    MOCK_METHOD(void, com_util_hashtable_dispose, (com_util_hashtable *));

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
    MOCK_METHOD(uint16_t, com_util_hton16, (uint16_t));
    MOCK_METHOD(uint16_t, com_util_ntoh16, (uint16_t));
    MOCK_METHOD(uint32_t, com_util_hton32, (uint32_t));
    MOCK_METHOD(uint32_t, com_util_ntoh32, (uint32_t));
    MOCK_METHOD(int, com_util_ipv4_parse, (const char *, uint32_t *));
    MOCK_METHOD(int, com_util_ipv4_resolve, (const char *, uint32_t *, com_util_error *));
    MOCK_METHOD(int, com_util_ipv4_to_string, (uint32_t, char *, size_t, com_util_error *));
    MOCK_METHOD(int, com_util_socket_open, (com_util_socket_kind, com_util_socket *, com_util_error *));
    MOCK_METHOD(void, com_util_socket_close, (com_util_socket));
    MOCK_METHOD(void, com_util_socket_shutdown, (com_util_socket));
    MOCK_METHOD(int, com_util_socket_bind, (com_util_socket, const com_util_ipv4_endpoint *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_listen, (com_util_socket, int, com_util_error *));
    MOCK_METHOD(int, com_util_socket_accept,
                (com_util_socket, com_util_ipv4_endpoint *, com_util_socket *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_connect, (com_util_socket, const com_util_ipv4_endpoint *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_get_pending_error, (com_util_socket, com_util_error *));
    MOCK_METHOD(int, com_util_socket_set_nonblocking, (com_util_socket, int, com_util_error *));
    MOCK_METHOD(int, com_util_socket_set_reuse_address, (com_util_socket, int, com_util_error *));
    MOCK_METHOD(int, com_util_socket_set_broadcast, (com_util_socket, int, com_util_error *));
    MOCK_METHOD(int, com_util_socket_set_multicast_interface, (com_util_socket, uint32_t, com_util_error *));
    MOCK_METHOD(int, com_util_socket_join_multicast_group, (com_util_socket, uint32_t, uint32_t, com_util_error *));
    MOCK_METHOD(int, com_util_socket_leave_multicast_group, (com_util_socket, uint32_t, uint32_t, com_util_error *));
    MOCK_METHOD(int, com_util_socket_send, (com_util_socket, const void *, size_t, size_t *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_recv, (com_util_socket, void *, size_t, size_t *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_sendto,
                (com_util_socket, const void *, size_t, const com_util_ipv4_endpoint *, size_t *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_recvfrom,
                (com_util_socket, void *, size_t, com_util_ipv4_endpoint *, size_t *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_send_all, (com_util_socket, const void *, size_t, com_util_error *));
    MOCK_METHOD(int, com_util_socket_recv_all, (com_util_socket, void *, size_t, com_util_error *));
    MOCK_METHOD(int, com_util_socket_wait_readable, (com_util_socket, int, int *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_wait_writable, (com_util_socket, int, int *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_wait_readable_multi,
                (const com_util_socket *, size_t, int, unsigned char *, com_util_error *));
    MOCK_METHOD(int, com_util_socket_shutdown_receive, (com_util_socket *, com_util_error *));

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
    MOCK_METHOD(char *, com_util_normalize_path_sep, (char *));
    MOCK_METHOD(int, com_util_paths_equal, (const char *, const char *, int *, com_util_error *));
    MOCK_METHOD(const char *, com_util_path_basename, (const char *));
    MOCK_METHOD(int, com_util_path_dirname, (char *, size_t, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_path_strip_extension, (char *, size_t, com_util_error *, const char *));
    MOCK_METHOD(int, com_util_path_join_n, (char *, size_t, com_util_error *, size_t, va_list));
    MOCK_METHOD(int, com_util_vpath_join_n, (char *, size_t, com_util_error *, size_t, va_list));

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
    MOCK_METHOD(void *, com_util_malloc, (size_t));
    MOCK_METHOD(void *, com_util_calloc, (size_t, size_t));
    MOCK_METHOD(void *, com_util_realloc, (void *, size_t, size_t));
    MOCK_METHOD(void *, com_util_realloc_zerofill, (void *, size_t, size_t, size_t));
    MOCK_METHOD(void, com_util_free, (void *));
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
    MOCK_METHOD(com_util_tracer *, com_util_tracer_create, (com_util_tracer_concurrency_mode));
    MOCK_METHOD(void, com_util_tracer_dispose, (com_util_tracer **));
    MOCK_METHOD(int, com_util_tracer_start, (com_util_tracer *));
    MOCK_METHOD(int, com_util_tracer_stop, (com_util_tracer *));
    MOCK_METHOD(int, com_util_tracer_write_at,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const char *));
    MOCK_METHOD(int, com_util_tracer_write_hex_at,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const void *, size_t,
                 const char *));
    MOCK_METHOD(int, com_util_tracer_writef_at,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const char *));
    MOCK_METHOD(int, com_util_tracer_write_hexf_at,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const void *, size_t,
                 const char *));
    MOCK_METHOD(const char *, com_util_tracer_hex_sep, (const char *));
    MOCK_METHOD(const char *, com_util_tracer_hex_msg, (const char *));
    MOCK_METHOD(int, com_util_tracer_write_with_source,
                (com_util_tracer *, com_util_trace_level, const com_util_timespec *, const char *, int, const char *));
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
    MOCK_METHOD(void, com_util_timespec_from_native, (const struct timespec *, com_util_timespec *));
    MOCK_METHOD(void, com_util_timespec_to_native, (const com_util_timespec *, struct timespec *));
    MOCK_METHOD(void, com_util_timespec_add_ms, (const com_util_timespec *, uint64_t, com_util_timespec *));
    MOCK_METHOD(int, com_util_timespec_cmp, (const com_util_timespec *, const com_util_timespec *));

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
    MOCK_METHOD(void, com_util_local_lock_dispose, (com_util_local_lock *));
    MOCK_METHOD(int, com_util_condvar_create, (com_util_condvar **));
    MOCK_METHOD(int, com_util_condvar_wait, (com_util_condvar *, com_util_local_lock *, int));
    MOCK_METHOD(int, com_util_condvar_signal, (com_util_condvar *));
    MOCK_METHOD(int, com_util_condvar_broadcast, (com_util_condvar *));
    MOCK_METHOD(void, com_util_condvar_dispose, (com_util_condvar *));
    MOCK_METHOD(int, com_util_local_rwlock_create, (com_util_local_rwlock **));
    MOCK_METHOD(int, com_util_local_rwlock_lock_shared, (com_util_local_rwlock *, int));
    MOCK_METHOD(int, com_util_local_rwlock_try_lock_shared, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_local_rwlock_lock_exclusive, (com_util_local_rwlock *, int));
    MOCK_METHOD(int, com_util_local_rwlock_try_lock_exclusive, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_local_rwlock_unlock_shared, (com_util_local_rwlock *));
    MOCK_METHOD(int, com_util_local_rwlock_unlock_exclusive, (com_util_local_rwlock *));
    MOCK_METHOD(void, com_util_local_rwlock_dispose, (com_util_local_rwlock *));
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
    MOCK_METHOD(void, com_util_interprocess_lock_dispose, (com_util_interprocess_lock *));
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
    MOCK_METHOD(void, com_util_interprocess_rwlock_dispose, (com_util_interprocess_rwlock *));
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
    MOCK_METHOD(uint32_t, com_util_process_get_pid, ());
    MOCK_METHOD(int, com_util_elevated_process_is_elevated, (int *));
    MOCK_METHOD(int, com_util_elevated_process_run_if_needed, (const char *, int *, int *));
    MOCK_METHOD(int, com_util_elevated_process_run_with_result, (const char *, int *, int *, char *, size_t));
    MOCK_METHOD(int, com_util_elevated_process_extract_result_target, (int *, char **, int *));
    MOCK_METHOD(int, com_util_elevated_process_report_result, (const char *));
    MOCK_METHOD(int, com_util_process_start, (const com_util_process_options *, com_util_process **));
    MOCK_METHOD(int, com_util_process_wait, (com_util_process *, int));
    MOCK_METHOD(int, com_util_process_get_exit_code, (com_util_process *, int *));
    MOCK_METHOD(int, com_util_process_terminate, (com_util_process *));
    MOCK_METHOD(void, com_util_process_dispose, (com_util_process *));
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
    // win32 - file_api (Windows only)
    MOCK_METHOD(HANDLE, CreateFileU, (const char *, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE));
    MOCK_METHOD(HANDLE, CreateNamedPipeU,
                (const char *, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPSECURITY_ATTRIBUTES));
    MOCK_METHOD(DWORD, GetModuleFileNameU, (HMODULE, char *, DWORD));
    MOCK_METHOD(BOOL, GetVolumePathNameU, (const char *, char *, DWORD));
    MOCK_METHOD(BOOL, GetVolumeInformationU, (const char *, char *, DWORD, DWORD *, DWORD *, DWORD *, char *, DWORD));
    MOCK_METHOD(HMODULE, LoadLibraryU, (const char *));
    MOCK_METHOD(BOOL, WriteConsoleU, (HANDLE, const char *, DWORD, DWORD *, void *));
    MOCK_METHOD(BOOL, CreateProcessU,
                (const char *, const char *, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID,
                 const char *, LPSTARTUPINFOW, LPPROCESS_INFORMATION));
    MOCK_METHOD(SC_HANDLE, OpenSCManagerU, (const char *, const char *, DWORD));
    MOCK_METHOD(SC_HANDLE, CreateServiceU,
                (SC_HANDLE, const char *, const char *, DWORD, DWORD, DWORD, DWORD, const char *, const char *, LPDWORD,
                 const char *, const char *, const char *));
    MOCK_METHOD(SC_HANDLE, OpenServiceU, (SC_HANDLE, const char *, DWORD));
    MOCK_METHOD(BOOL, ChangeServiceConfig2U, (SC_HANDLE, DWORD, const char *));
    MOCK_METHOD(SERVICE_STATUS_HANDLE, RegisterServiceCtrlHandlerExU, (const char *, LPHANDLER_FUNCTION_EX, LPVOID));
    MOCK_METHOD(BOOL, StartServiceCtrlDispatcherU, (const com_util_service_entry_u *));

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
    MOCK_METHOD(int, com_util_pinned_prompt_readline_at,
                (com_util_pinned_prompt *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, com_util_pinned_prompt_readline_fmt_at,
                (com_util_pinned_prompt *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(int, com_util_pinned_prompt_write,
                (com_util_pinned_prompt *, com_util_pinned_prompt_channel, const void *, size_t, size_t *));
    MOCK_METHOD(int, com_util_pinned_prompt_printf,
                (com_util_pinned_prompt *, com_util_pinned_prompt_channel, const char *));
    MOCK_METHOD(int, com_util_pinned_prompt_status_enable,
                (com_util_pinned_prompt *, com_util_pinned_prompt_status_position, int));
    MOCK_METHOD(int, com_util_pinned_prompt_status_set,
                (com_util_pinned_prompt *, com_util_pinned_prompt_status_position, com_util_pinned_prompt_status_align,
                 const char *));

    // argparser
    MOCK_METHOD(com_util_argparser *, com_util_argparser_create, (const com_util_argparser_options *));
    MOCK_METHOD(com_util_argparser *, com_util_argparser_default, (const com_util_argparser_options *));
    MOCK_METHOD(void, com_util_argparser_dispose, (com_util_argparser *));
    MOCK_METHOD(int, com_util_argparser_register_flag,
                (com_util_argparser *, const char *, const char *, const char *, int *));
    MOCK_METHOD(int, com_util_argparser_register_option_int,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, com_util_argparser_register_option_string,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int,
                 const char **));
    MOCK_METHOD(int, com_util_argparser_register_option_int_array,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int, int *,
                 size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_register_option_string_array,
                (com_util_argparser *, const char *, const char *, const char *, const char *, unsigned int,
                 const char **, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_register_positional_int,
                (com_util_argparser *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, com_util_argparser_register_positional_string,
                (com_util_argparser *, const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, com_util_argparser_register_positional_int_array,
                (com_util_argparser *, const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_register_positional_string_array,
                (com_util_argparser *, const char *, const char *, unsigned int, const char **, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_parse, (com_util_argparser *, int, char *const *));
    MOCK_METHOD(int, com_util_argparser_get_error, (const com_util_argparser *));
    MOCK_METHOD(const char *, com_util_argparser_get_error_target, (const com_util_argparser *));
    MOCK_METHOD(int, com_util_argparser_get_error_index, (const com_util_argparser *));
    MOCK_METHOD(int, com_util_argparser_get_error_message, (const com_util_argparser *, char *, size_t));
    MOCK_METHOD(int, com_util_argparser_get_usage, (const com_util_argparser *, char *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_print_usage, (const com_util_argparser *, FILE *));
    MOCK_METHOD(int, com_util_argparser_print_error_messages, (const com_util_argparser *, FILE *));
    MOCK_METHOD(int, com_util_argparser_get_register_error, (const com_util_argparser *, size_t));
    MOCK_METHOD(size_t, com_util_argparser_get_register_error_count, (const com_util_argparser *));
    MOCK_METHOD(const char *, com_util_argparser_get_register_error_target, (const com_util_argparser *, size_t));
    MOCK_METHOD(int, com_util_argparser_get_register_error_message,
                (const com_util_argparser *, size_t, char *, size_t));
    MOCK_METHOD(int, com_util_argparser_print_register_error_messages, (const com_util_argparser *, FILE *));

    // argparser (省略可能な単一インスタンス API)
    MOCK_METHOD(void, com_util_argparser_default_init, (const char *));
    MOCK_METHOD(int, com_util_argparser_default_register_flag, (const char *, const char *, const char *, int *));
    MOCK_METHOD(int, com_util_argparser_default_register_option_int,
                (const char *, const char *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, com_util_argparser_default_register_option_string,
                (const char *, const char *, const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, com_util_argparser_default_register_option_int_array,
                (const char *, const char *, const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_default_register_option_string_array,
                (const char *, const char *, const char *, const char *, unsigned int, const char **, size_t,
                 size_t *));
    MOCK_METHOD(int, com_util_argparser_default_register_positional_int,
                (const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, com_util_argparser_default_register_positional_string,
                (const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, com_util_argparser_default_register_positional_int_array,
                (const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_default_register_positional_string_array,
                (const char *, const char *, unsigned int, const char **, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_default_parse, (int, char *const *));
    MOCK_METHOD(int, com_util_argparser_default_get_error, ());
    MOCK_METHOD(const char *, com_util_argparser_default_get_error_target, ());
    MOCK_METHOD(int, com_util_argparser_default_get_error_index, ());
    MOCK_METHOD(int, com_util_argparser_default_get_error_message, (char *, size_t));
    MOCK_METHOD(int, com_util_argparser_default_get_usage, (char *, size_t, size_t *));
    MOCK_METHOD(int, com_util_argparser_default_print_usage, (FILE *));
    MOCK_METHOD(int, com_util_argparser_default_print_error_messages, (FILE *));
    MOCK_METHOD(int, com_util_argparser_default_get_register_error, (size_t));
    MOCK_METHOD(size_t, com_util_argparser_default_get_register_error_count, ());
    MOCK_METHOD(const char *, com_util_argparser_default_get_register_error_target, (size_t));
    MOCK_METHOD(int, com_util_argparser_default_get_register_error_message, (size_t, char *, size_t));
    MOCK_METHOD(int, com_util_argparser_default_print_register_error_messages, (FILE *));

    Mock_com_util();
    ~Mock_com_util();
};

extern Mock_com_util *_mock_com_util;

#endif /* MOCK_UTIL_H */
