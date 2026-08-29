#ifndef MOCK_CPLAT_H
#define MOCK_CPLAT_H

#include <cplat/base/platform.h>
#include <testfw.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <vector>

#if defined(COMPILER_MSVC)
    #define MOCK_CPLAT_LINK_IMPL(func) __pragma(comment(linker, "/INCLUDE:_mock_impl_" #func))

// mock_cplat/<dir>/*.cc の弱リンク実装 (MOCK_WEAK_IMPL) は、参照側の翻訳単位が
// 存在しない場合に MSVC がオブジェクトをリンクへ取り込まず、cplat_* が未解決に
// なることがある。MOCK_CPLAT_LINK_IMPL(func) は /INCLUDE:_mock_impl_<func> を
// リンカーへ渡し、実体を明示的に含める。以下は mock_cplat/ 配下のディレクトリ
// 構成 (argparser/clock/compress/console/crt/crypto/hashtable/net/prompt/runtime/sync/trace/win32)
// に沿って、各 .cc ファイルが実装する関数を定義順に列挙する。
// cplat_syslog_sink_* は Linux 専用のため、この Windows 専用ブロックには含めない。

// argparser
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_flag)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_option_int)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_option_string)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_option_int_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_option_string_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_positional_int)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_positional_string)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_positional_int_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_register_positional_string_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_parse)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_error_message)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_usage)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_print_usage)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_print_error_messages)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_register_error)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_register_error_message)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_print_register_error_messages)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_create)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_error)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_error_target)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_error_index)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_register_error_count)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_handle_get_register_error_target)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_init)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_flag)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_option_int)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_option_string)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_option_int_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_option_string_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_positional_int)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_positional_string)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_positional_int_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_register_positional_string_array)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_parse)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_error_message)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_usage)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_print_usage)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_print_error_messages)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_register_error)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_register_error_message)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_print_register_error_messages)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_error)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_error_target)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_error_index)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_register_error_count)
MOCK_CPLAT_LINK_IMPL(cplat_argparser_get_register_error_target)

// clock
MOCK_CPLAT_LINK_IMPL(cplat_format_realtime_iso8601_local)
MOCK_CPLAT_LINK_IMPL(cplat_format_realtime_iso8601_utc)
MOCK_CPLAT_LINK_IMPL(cplat_get_monotonic)
MOCK_CPLAT_LINK_IMPL(cplat_get_monotonic_ms)
MOCK_CPLAT_LINK_IMPL(cplat_get_realtime)
MOCK_CPLAT_LINK_IMPL(cplat_get_realtime_deadline_ms)
MOCK_CPLAT_LINK_IMPL(cplat_get_realtime_utc)
MOCK_CPLAT_LINK_IMPL(cplat_timespec_add_ms)
MOCK_CPLAT_LINK_IMPL(cplat_timespec_cmp)
MOCK_CPLAT_LINK_IMPL(cplat_timespec_from_native)
MOCK_CPLAT_LINK_IMPL(cplat_timespec_to_native)

// hashtable
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_required_size)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_create)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_attach)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_validate)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_config_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_config_val)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_buffer_size)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_buffer_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_add)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_upsert)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_insert_direct)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_update)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_update_rec)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_find_value_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_find_value_copy)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_find_recno)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_key_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_key_copy)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_value_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_value_copy)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_status)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_next_record)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_timestamp_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_timestamp_val)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_generation)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_table_timestamp_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_table_timestamp_val)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_get_table_generation)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_find_timestamp_ref)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_find_timestamp_val)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_find_generation)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_count_status)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_count)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_deleted_count)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_empty_count)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_delete)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_delete_rec)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_push_deleted)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_purge_deleted)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_compact)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_resize)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_rebuild_into)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_clear)
MOCK_CPLAT_LINK_IMPL(cplat_hashtable_dispose)

// compress
MOCK_CPLAT_LINK_IMPL(cplat_compress)
MOCK_CPLAT_LINK_IMPL(cplat_decompress)

// console
MOCK_CPLAT_LINK_IMPL(cplat_console_attach_parent)
MOCK_CPLAT_LINK_IMPL(cplat_console_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_console_dispose_on_shutdown)
MOCK_CPLAT_LINK_IMPL(cplat_console_init)

// crt
MOCK_CPLAT_LINK_IMPL(cplat_access)
MOCK_CPLAT_LINK_IMPL(cplat_access_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_close)
MOCK_CPLAT_LINK_IMPL(cplat_ctime)
MOCK_CPLAT_LINK_IMPL(cplat_dup)
MOCK_CPLAT_LINK_IMPL(cplat_dup2)
MOCK_CPLAT_LINK_IMPL(cplat_fclose)
MOCK_CPLAT_LINK_IMPL(cplat_fflush)
MOCK_CPLAT_LINK_IMPL(cplat_fgets)
MOCK_CPLAT_LINK_IMPL(cplat_file_close)
MOCK_CPLAT_LINK_IMPL(cplat_file_flush)
MOCK_CPLAT_LINK_IMPL(cplat_file_get_id)
MOCK_CPLAT_LINK_IMPL(cplat_file_get_path_id)
MOCK_CPLAT_LINK_IMPL(cplat_file_get_modified_timestamp)
MOCK_CPLAT_LINK_IMPL(cplat_file_set_modified_timestamp)
MOCK_CPLAT_LINK_IMPL(cplat_file_get_path_modified_timestamp)
MOCK_CPLAT_LINK_IMPL(cplat_file_set_path_modified_timestamp)
MOCK_CPLAT_LINK_IMPL(cplat_file_stat_is_regular)
MOCK_CPLAT_LINK_IMPL(cplat_file_get_size)
MOCK_CPLAT_LINK_IMPL(cplat_file_init)
MOCK_CPLAT_LINK_IMPL(cplat_file_open)
MOCK_CPLAT_LINK_IMPL(cplat_file_read)
MOCK_CPLAT_LINK_IMPL(cplat_file_set_size)
MOCK_CPLAT_LINK_IMPL(cplat_file_write)
MOCK_CPLAT_LINK_IMPL(cplat_fopen)
MOCK_CPLAT_LINK_IMPL(cplat_fopen_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_fopen_temp)
MOCK_CPLAT_LINK_IMPL(cplat_fprintf)
MOCK_CPLAT_LINK_IMPL(cplat_fread)
MOCK_CPLAT_LINK_IMPL(cplat_freopen)
MOCK_CPLAT_LINK_IMPL(cplat_fscanf)
MOCK_CPLAT_LINK_IMPL(cplat_fseek)
MOCK_CPLAT_LINK_IMPL(cplat_ftell)
MOCK_CPLAT_LINK_IMPL(cplat_fwrite)
MOCK_CPLAT_LINK_IMPL(cplat_getenv)
MOCK_CPLAT_LINK_IMPL(cplat_gmtime)
MOCK_CPLAT_LINK_IMPL(cplat_isatty)
MOCK_CPLAT_LINK_IMPL(cplat_localtime)
MOCK_CPLAT_LINK_IMPL(cplat_lseek)
MOCK_CPLAT_LINK_IMPL(cplat_makedirs)
MOCK_CPLAT_LINK_IMPL(cplat_mkdir)
MOCK_CPLAT_LINK_IMPL(cplat_mkdir_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_normalize_path_sep)
MOCK_CPLAT_LINK_IMPL(cplat_open)
MOCK_CPLAT_LINK_IMPL(cplat_open_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_parse_double)
MOCK_CPLAT_LINK_IMPL(cplat_parse_int)
MOCK_CPLAT_LINK_IMPL(cplat_parse_int64)
MOCK_CPLAT_LINK_IMPL(cplat_parse_uint64)
MOCK_CPLAT_LINK_IMPL(cplat_path_basename)
MOCK_CPLAT_LINK_IMPL(cplat_path_dirname)
MOCK_CPLAT_LINK_IMPL(cplat_path_get_full)
MOCK_CPLAT_LINK_IMPL(cplat_path_join_n)
MOCK_CPLAT_LINK_IMPL(cplat_path_strip_extension)
MOCK_CPLAT_LINK_IMPL(cplat_vpath_join_n)
MOCK_CPLAT_LINK_IMPL(cplat_paths_equal)
MOCK_CPLAT_LINK_IMPL(cplat_read)
MOCK_CPLAT_LINK_IMPL(cplat_remove)
MOCK_CPLAT_LINK_IMPL(cplat_remove_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_rename)
MOCK_CPLAT_LINK_IMPL(cplat_rmdir)
MOCK_CPLAT_LINK_IMPL(cplat_scanf)
MOCK_CPLAT_LINK_IMPL(cplat_setenv)
MOCK_CPLAT_LINK_IMPL(cplat_snprintf)
MOCK_CPLAT_LINK_IMPL(cplat_sscanf)
MOCK_CPLAT_LINK_IMPL(cplat_stat)
MOCK_CPLAT_LINK_IMPL(cplat_stat_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_strcat)
MOCK_CPLAT_LINK_IMPL(cplat_strcpy)
MOCK_CPLAT_LINK_IMPL(cplat_strdup)
MOCK_CPLAT_LINK_IMPL(cplat_malloc)
MOCK_CPLAT_LINK_IMPL(cplat_calloc)
MOCK_CPLAT_LINK_IMPL(cplat_realloc)
MOCK_CPLAT_LINK_IMPL(cplat_realloc_zerofill)
MOCK_CPLAT_LINK_IMPL(cplat_free)
MOCK_CPLAT_LINK_IMPL(cplat_strncat)
MOCK_CPLAT_LINK_IMPL(cplat_strncpy)
MOCK_CPLAT_LINK_IMPL(cplat_strtok_r)
MOCK_CPLAT_LINK_IMPL(cplat_unsetenv)
MOCK_CPLAT_LINK_IMPL(cplat_vaccess_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_vfopen_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_vfprintf)
MOCK_CPLAT_LINK_IMPL(cplat_vfscanf)
MOCK_CPLAT_LINK_IMPL(cplat_vmkdir_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_vopen_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_vremove_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_vscanf)
MOCK_CPLAT_LINK_IMPL(cplat_vsnprintf)
MOCK_CPLAT_LINK_IMPL(cplat_vsscanf)
MOCK_CPLAT_LINK_IMPL(cplat_vstat_fmt)
MOCK_CPLAT_LINK_IMPL(cplat_wcscpy)
MOCK_CPLAT_LINK_IMPL(cplat_write)
MOCK_CPLAT_LINK_IMPL(cplat_utf8_to_wpath)
MOCK_CPLAT_LINK_IMPL(cplat_wpath_to_utf8)
MOCK_CPLAT_LINK_IMPL(cplat_utf8_to_wstr_alloc)
MOCK_CPLAT_LINK_IMPL(cplat_wstr_to_utf8_alloc)

// crypto
MOCK_CPLAT_LINK_IMPL(cplat_decrypt)
MOCK_CPLAT_LINK_IMPL(cplat_encrypt)
MOCK_CPLAT_LINK_IMPL(cplat_passphrase_to_key)
MOCK_CPLAT_LINK_IMPL(cplat_random_bytes)

// net
MOCK_CPLAT_LINK_IMPL(cplat_hton16)
MOCK_CPLAT_LINK_IMPL(cplat_ntoh16)
MOCK_CPLAT_LINK_IMPL(cplat_hton32)
MOCK_CPLAT_LINK_IMPL(cplat_ntoh32)
MOCK_CPLAT_LINK_IMPL(cplat_ipv4_parse)
MOCK_CPLAT_LINK_IMPL(cplat_ipv4_resolve)
MOCK_CPLAT_LINK_IMPL(cplat_ipv4_to_string)
MOCK_CPLAT_LINK_IMPL(cplat_socket_accept)
MOCK_CPLAT_LINK_IMPL(cplat_socket_bind)
MOCK_CPLAT_LINK_IMPL(cplat_socket_close)
MOCK_CPLAT_LINK_IMPL(cplat_socket_connect)
MOCK_CPLAT_LINK_IMPL(cplat_socket_get_pending_error)
MOCK_CPLAT_LINK_IMPL(cplat_socket_join_multicast_group)
MOCK_CPLAT_LINK_IMPL(cplat_socket_leave_multicast_group)
MOCK_CPLAT_LINK_IMPL(cplat_socket_listen)
MOCK_CPLAT_LINK_IMPL(cplat_socket_open)
MOCK_CPLAT_LINK_IMPL(cplat_socket_recv)
MOCK_CPLAT_LINK_IMPL(cplat_socket_recv_all)
MOCK_CPLAT_LINK_IMPL(cplat_socket_recvfrom)
MOCK_CPLAT_LINK_IMPL(cplat_socket_send)
MOCK_CPLAT_LINK_IMPL(cplat_socket_send_all)
MOCK_CPLAT_LINK_IMPL(cplat_socket_sendto)
MOCK_CPLAT_LINK_IMPL(cplat_socket_set_broadcast)
MOCK_CPLAT_LINK_IMPL(cplat_socket_set_multicast_interface)
MOCK_CPLAT_LINK_IMPL(cplat_socket_set_nonblocking)
MOCK_CPLAT_LINK_IMPL(cplat_socket_set_reuse_address)
MOCK_CPLAT_LINK_IMPL(cplat_socket_shutdown)
MOCK_CPLAT_LINK_IMPL(cplat_socket_shutdown_receive)
MOCK_CPLAT_LINK_IMPL(cplat_socket_wait_readable)
MOCK_CPLAT_LINK_IMPL(cplat_socket_wait_readable_multi)
MOCK_CPLAT_LINK_IMPL(cplat_socket_wait_writable)

// prompt
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_create)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_printf)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_readline_at)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_readline_fmt_at)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_status_enable)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_status_set)
MOCK_CPLAT_LINK_IMPL(cplat_pinned_prompt_write)
MOCK_CPLAT_LINK_IMPL(cplat_prompt_create)
MOCK_CPLAT_LINK_IMPL(cplat_prompt_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_prompt_readline_at)
MOCK_CPLAT_LINK_IMPL(cplat_prompt_readline_fmt_at)

// runtime
MOCK_CPLAT_LINK_IMPL(cplat_elevated_process_extract_result_target)
MOCK_CPLAT_LINK_IMPL(cplat_elevated_process_is_elevated)
MOCK_CPLAT_LINK_IMPL(cplat_elevated_process_report_result)
MOCK_CPLAT_LINK_IMPL(cplat_elevated_process_run_if_needed)
MOCK_CPLAT_LINK_IMPL(cplat_elevated_process_run_with_result)
MOCK_CPLAT_LINK_IMPL(cplat_memory_lock_range)
MOCK_CPLAT_LINK_IMPL(cplat_memory_lock_scope_release)
MOCK_CPLAT_LINK_IMPL(cplat_memory_lock_self)
MOCK_CPLAT_LINK_IMPL(cplat_memory_unlock_range)
MOCK_CPLAT_LINK_IMPL(cplat_module_get_basename)
MOCK_CPLAT_LINK_IMPL(cplat_module_get_path)
MOCK_CPLAT_LINK_IMPL(cplat_process_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_process_get_executable_path)
MOCK_CPLAT_LINK_IMPL(cplat_process_get_pid)
MOCK_CPLAT_LINK_IMPL(cplat_process_get_exit_code)
MOCK_CPLAT_LINK_IMPL(cplat_process_run_sync)
MOCK_CPLAT_LINK_IMPL(cplat_process_start)
MOCK_CPLAT_LINK_IMPL(cplat_process_terminate)
MOCK_CPLAT_LINK_IMPL(cplat_process_wait)
MOCK_CPLAT_LINK_IMPL(cplat_secure_zero)
MOCK_CPLAT_LINK_IMPL(cplat_shutdown_register)
MOCK_CPLAT_LINK_IMPL(cplat_shutdown_request_register)
MOCK_CPLAT_LINK_IMPL(cplat_sym_loader_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_sym_loader_info)
MOCK_CPLAT_LINK_IMPL(cplat_sym_loader_init)
MOCK_CPLAT_LINK_IMPL(cplat_sym_loader_is_default)
MOCK_CPLAT_LINK_IMPL(cplat_sym_loader_resolve)

// sync
MOCK_CPLAT_LINK_IMPL(cplat_call_once)
MOCK_CPLAT_LINK_IMPL(cplat_sleep_ms)
MOCK_CPLAT_LINK_IMPL(cplat_local_lock_create)
MOCK_CPLAT_LINK_IMPL(cplat_local_lock_lock)
MOCK_CPLAT_LINK_IMPL(cplat_local_lock_try_lock)
MOCK_CPLAT_LINK_IMPL(cplat_local_lock_unlock)
MOCK_CPLAT_LINK_IMPL(cplat_local_lock_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_condvar_create)
MOCK_CPLAT_LINK_IMPL(cplat_condvar_wait)
MOCK_CPLAT_LINK_IMPL(cplat_condvar_signal)
MOCK_CPLAT_LINK_IMPL(cplat_condvar_broadcast)
MOCK_CPLAT_LINK_IMPL(cplat_condvar_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_create)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_lock_shared)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_try_lock_shared)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_lock_exclusive)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_try_lock_exclusive)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_unlock_shared)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_unlock_exclusive)
MOCK_CPLAT_LINK_IMPL(cplat_local_rwlock_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_thread_create)
MOCK_CPLAT_LINK_IMPL(cplat_thread_join)
MOCK_CPLAT_LINK_IMPL(cplat_thread_detach)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_open)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_import_descriptor)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_export_descriptor)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_lock)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_try_lock)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_unlock)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_lock_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_open)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_import_descriptor)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_export_descriptor)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_lock_shared)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_try_lock_shared)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_lock_exclusive)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_try_lock_exclusive)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_unlock)
MOCK_CPLAT_LINK_IMPL(cplat_interprocess_rwlock_dispose)

// trace
MOCK_CPLAT_LINK_IMPL(cplat_etw_provider_create)
MOCK_CPLAT_LINK_IMPL(cplat_etw_provider_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_etw_provider_write)
MOCK_CPLAT_LINK_IMPL(cplat_etw_session_check_access)
MOCK_CPLAT_LINK_IMPL(cplat_etw_session_start)
MOCK_CPLAT_LINK_IMPL(cplat_etw_session_stop)
MOCK_CPLAT_LINK_IMPL(cplat_eventlog_register_source)
MOCK_CPLAT_LINK_IMPL(cplat_eventlog_sink_create)
MOCK_CPLAT_LINK_IMPL(cplat_eventlog_sink_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_eventlog_sink_write)
MOCK_CPLAT_LINK_IMPL(cplat_eventlog_unregister_source)
MOCK_CPLAT_LINK_IMPL(cplat_trace_file_sink_create)
MOCK_CPLAT_LINK_IMPL(cplat_trace_file_sink_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_trace_file_sink_write)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_call_next_hook)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_create)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_dispose)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_get_etw_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_get_file_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_get_os_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_get_state)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_get_stderr_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_remove_hook)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_set_etw_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_set_file_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_set_hook)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_set_name)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_set_os_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_set_stderr_level)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_start)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_stop)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_write_at)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_write_hex_at)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_write_hexf_at)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_writef_at)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_hex_sep)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_hex_msg)
MOCK_CPLAT_LINK_IMPL(cplat_tracer_write_with_source)

// win32
MOCK_CPLAT_LINK_IMPL(ChangeServiceConfig2U)
MOCK_CPLAT_LINK_IMPL(CreateFileU)
MOCK_CPLAT_LINK_IMPL(CreateNamedPipeU)
MOCK_CPLAT_LINK_IMPL(CreateProcessU)
MOCK_CPLAT_LINK_IMPL(CreateServiceU)
MOCK_CPLAT_LINK_IMPL(GetModuleFileNameU)
MOCK_CPLAT_LINK_IMPL(GetVolumeInformationU)
MOCK_CPLAT_LINK_IMPL(GetVolumePathNameU)
MOCK_CPLAT_LINK_IMPL(LoadLibraryU)
MOCK_CPLAT_LINK_IMPL(OpenSCManagerU)
MOCK_CPLAT_LINK_IMPL(OpenServiceU)
MOCK_CPLAT_LINK_IMPL(RegisterServiceCtrlHandlerExU)
MOCK_CPLAT_LINK_IMPL(StartServiceCtrlDispatcherU)
MOCK_CPLAT_LINK_IMPL(WriteConsoleU)

    #undef MOCK_CPLAT_LINK_IMPL
#endif /* COMPILER_MSVC */

#include <cplat/compress/compress.h>
#include <cplat/crypto/crypto.h>
#include <cplat/crypto/random.h>
#include <cplat/net/endpoint.h>
#include <cplat/net/socket.h>
#include <cplat/net/byteorder.h>
#include <cplat/crt/fcntl.h>
#include <cplat/crt/time.h>
#include <cplat/crt/stdio.h>
#include <cplat/crt/stdlib.h>
#include <cplat/crt/sys/stat.h>
#include <cplat/crt/string.h>
#include <cplat/crt/unistd.h>
#include <cplat/crt/file.h>
#include <cplat/crt/path.h>
#include <cplat/trace/tracer.h>
#include <cplat/clock/clock.h>
#include <cplat/console/console.h>
#include <cplat/sync/sync.h>
#include <cplat/runtime/module.h>
#include <cplat/runtime/memory_lock.h>
#include <cplat/runtime/elevated_process.h>
#include <cplat/runtime/process.h>
#include <cplat/runtime/sym_loader.h>
#include <cplat/runtime/shutdown.h>
#include <cplat/crt/wchar_conv.h>
#include <cplat/trace/trace_file.h>
#include <cplat/trace/syslog.h>
#include <cplat/trace/etw.h>
#include <cplat/trace/eventlog.h>
#include <cplat/prompt/prompt.h>
#include <cplat/prompt/pinned_prompt.h>
#include <cplat/argparser/argparser.h>
#include <cplat/hashtable/hashtable.h>
#if defined(PLATFORM_WINDOWS)
    #include <cplat/win32/win32.h>
#endif /* PLATFORM_WINDOWS */

inline constexpr char kLibCplatName[] = "libcplat" TESTFW_SHARED_LIBRARY_EXTENSION;

// 書式を展開した NUL 終端文字列を返す。
// 期待値の照合と実関数への委譲の双方で使用する。固定長バッファーで展開すると長い出力が
// 切り詰められ、被テスト側の切り詰め判定が実関数と食い違うため、testfw の allocvprintf で
// 必要な長さを確保する。戻り値は解放不要で、.data() は常に有効な NUL 終端文字列を指す。
extern std::vector<char> mock_cplat_expand_format(const char *format, va_list args);

// compress
extern int delegate_real_cplat_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);
extern int delegate_real_cplat_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);

// crypto
extern int delegate_real_cplat_encrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                          const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_cplat_decrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len,
                                          const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len);
extern int delegate_real_cplat_passphrase_to_key(uint8_t *key, const uint8_t *passphrase, size_t passphrase_len);
extern int delegate_real_cplat_random_bytes(void *buf, size_t size);
extern uint16_t delegate_real_cplat_hton16(uint16_t value);
extern uint16_t delegate_real_cplat_ntoh16(uint16_t value);
extern uint32_t delegate_real_cplat_hton32(uint32_t value);
extern uint32_t delegate_real_cplat_ntoh32(uint32_t value);
extern int delegate_real_cplat_ipv4_parse(const char *, uint32_t *);
extern int delegate_real_cplat_ipv4_resolve(const char *, uint32_t *, cplat_error *);
extern int delegate_real_cplat_ipv4_to_string(uint32_t, char *, size_t, cplat_error *);
extern int delegate_real_cplat_socket_open(cplat_socket_kind, cplat_socket *, cplat_error *);
extern void delegate_real_cplat_socket_close(cplat_socket);
extern void delegate_real_cplat_socket_shutdown(cplat_socket);
extern int delegate_real_cplat_socket_bind(cplat_socket, const cplat_ipv4_endpoint *, cplat_error *);
extern int delegate_real_cplat_socket_listen(cplat_socket, int, cplat_error *);
extern int delegate_real_cplat_socket_accept(cplat_socket, cplat_ipv4_endpoint *, cplat_socket *,
                                                cplat_error *);
extern int delegate_real_cplat_socket_connect(cplat_socket, const cplat_ipv4_endpoint *, cplat_error *);
extern int delegate_real_cplat_socket_get_pending_error(cplat_socket, cplat_error *);
extern int delegate_real_cplat_socket_set_nonblocking(cplat_socket, int, cplat_error *);
extern int delegate_real_cplat_socket_set_reuse_address(cplat_socket, int, cplat_error *);
extern int delegate_real_cplat_socket_set_broadcast(cplat_socket, int, cplat_error *);
extern int delegate_real_cplat_socket_set_multicast_interface(cplat_socket, uint32_t, cplat_error *);
extern int delegate_real_cplat_socket_join_multicast_group(cplat_socket, uint32_t, uint32_t, cplat_error *);
extern int delegate_real_cplat_socket_leave_multicast_group(cplat_socket, uint32_t, uint32_t, cplat_error *);
extern int delegate_real_cplat_socket_send(cplat_socket, const void *, size_t, size_t *, cplat_error *);
extern int delegate_real_cplat_socket_recv(cplat_socket, void *, size_t, size_t *, cplat_error *);
extern int delegate_real_cplat_socket_sendto(cplat_socket, const void *, size_t, const cplat_ipv4_endpoint *,
                                                size_t *, cplat_error *);
extern int delegate_real_cplat_socket_recvfrom(cplat_socket, void *, size_t, cplat_ipv4_endpoint *, size_t *,
                                                  cplat_error *);
extern int delegate_real_cplat_socket_send_all(cplat_socket, const void *, size_t, cplat_error *);
extern int delegate_real_cplat_socket_recv_all(cplat_socket, void *, size_t, cplat_error *);
extern int delegate_real_cplat_socket_wait_readable(cplat_socket, int, int *, cplat_error *);
extern int delegate_real_cplat_socket_wait_writable(cplat_socket, int, int *, cplat_error *);
extern int delegate_real_cplat_socket_wait_readable_multi(const cplat_socket *, size_t, int, unsigned char *,
                                                             cplat_error *);
extern int delegate_real_cplat_socket_shutdown_receive(cplat_socket *, cplat_error *);

// crt
extern FILE *delegate_real_cplat_fopen(const char *path, const char *modes, cplat_error *detail_out);
extern FILE *delegate_real_cplat_freopen(const char *path, const char *modes, FILE *stream,
                                            cplat_error *detail_out);
extern int delegate_real_cplat_fclose(FILE *stream, cplat_error *detail_out);
extern int delegate_real_cplat_fflush(FILE *stream, cplat_error *detail_out);
extern size_t delegate_real_cplat_fread(void *buffer, size_t size, size_t count, FILE *stream,
                                           cplat_error *detail_out);
extern size_t delegate_real_cplat_fwrite(const void *buffer, size_t size, size_t count, FILE *stream,
                                            cplat_error *detail_out);
extern int delegate_real_cplat_stat(cplat_file_stat_t *buf, cplat_error *detail_out, const char *path);
extern int delegate_real_cplat_open(const char *path, int flags, int mode, cplat_error *detail_out);
extern int delegate_real_cplat_access(const char *path, int mode, cplat_error *detail_out);
extern int delegate_real_cplat_mkdir(const char *path, cplat_error *detail_out);
extern int delegate_real_cplat_makedirs(const char *path, cplat_error *detail_out);
extern int delegate_real_cplat_rmdir(const char *path, cplat_error *detail_out);
extern int delegate_real_cplat_remove(const char *path, cplat_error *detail_out);
extern int delegate_real_cplat_sscanf(const char *buffer, const char *format, va_list args);
extern int delegate_real_cplat_vsscanf(const char *buffer, const char *format, va_list args);
extern int delegate_real_cplat_gmtime(struct tm *utc_tm, const time_t *timep);
extern int delegate_real_cplat_localtime(struct tm *local_tm, const time_t *timep);
extern int delegate_real_cplat_ctime(char *buf, size_t buf_size, const time_t *timep);
extern int delegate_real_cplat_getenv(const char *name, char *buf, size_t buf_size, int *exists_out,
                                         cplat_error *detail_out);
extern int delegate_real_cplat_setenv(const char *name, const char *value, int overwrite,
                                         cplat_error *detail_out);
extern int delegate_real_cplat_unsetenv(const char *name, cplat_error *detail_out);
extern int delegate_real_cplat_parse_int64(int64_t *value_out, const char *text, int base);
extern int delegate_real_cplat_parse_uint64(uint64_t *value_out, const char *text, int base);
extern int delegate_real_cplat_parse_int(int *value_out, const char *text, int base);
extern int delegate_real_cplat_parse_double(double *value_out, const char *text);
extern int delegate_real_cplat_path_get_full(char *path_out, size_t path_size, cplat_error *detail_out,
                                                const char *path);
extern char *delegate_real_cplat_normalize_path_sep(char *path);
extern int delegate_real_cplat_paths_equal(const char *lhs, const char *rhs, int *equal_out,
                                              cplat_error *detail_out);
extern const char *delegate_real_cplat_path_basename(const char *path);
extern int delegate_real_cplat_path_dirname(char *path_out, size_t path_size, cplat_error *detail_out,
                                               const char *path);
extern int delegate_real_cplat_path_strip_extension(char *path_out, size_t path_size, cplat_error *detail_out,
                                                       const char *path);
extern int delegate_real_cplat_path_join_n(char *path_out, size_t path_size, cplat_error *detail_out,
                                              size_t part_count, ...);
extern int delegate_real_cplat_vpath_join_n(char *path_out, size_t path_size, cplat_error *detail_out,
                                               size_t part_count, va_list args);

// crt - stdio
extern int delegate_real_cplat_scanf(const char *format, va_list args);
extern int delegate_real_cplat_vscanf(const char *format, va_list args);
extern int delegate_real_cplat_fscanf(FILE *stream, const char *format, va_list args);
extern int delegate_real_cplat_vfscanf(FILE *stream, const char *format, va_list args);
extern int delegate_real_cplat_snprintf(char *dest, size_t dest_size, const char *format, ...);
extern int delegate_real_cplat_vsnprintf(char *dest, size_t dest_size, const char *format, va_list args);
extern int delegate_real_cplat_fgets(char *dest, size_t dest_size, FILE *stream, cplat_error *detail_out);
extern int delegate_real_cplat_rename(const char *oldpath, const char *newpath, cplat_error *detail_out);
extern int delegate_real_cplat_fprintf(FILE *stream, const char *format, ...);
extern int delegate_real_cplat_vfprintf(FILE *stream, const char *format, va_list args);
extern int delegate_real_cplat_fseek(FILE *stream, int64_t offset, int whence);
extern int64_t delegate_real_cplat_ftell(FILE *stream);
extern FILE *delegate_real_cplat_fopen_fmt(const char *modes, cplat_error *detail_out, const char *format, ...);
extern FILE *delegate_real_cplat_vfopen_fmt(const char *modes, cplat_error *detail_out, const char *format,
                                               va_list args);
extern int delegate_real_cplat_remove_fmt(cplat_error *detail_out, const char *format, ...);
extern int delegate_real_cplat_vremove_fmt(cplat_error *detail_out, const char *format, va_list args);
extern FILE *delegate_real_cplat_fopen_temp(const char *prefix, const char *modes, char *path_out, size_t path_size,
                                               cplat_error *detail_out);

// crt - unistd
extern int delegate_real_cplat_isatty(cplat_stream stream);
extern int delegate_real_cplat_access_fmt(int mode, cplat_error *detail_out, const char *format, ...);
extern int delegate_real_cplat_vaccess_fmt(int mode, cplat_error *detail_out, const char *format, va_list args);
extern int64_t delegate_real_cplat_lseek(int fd, int64_t offset, int whence, cplat_error *detail_out);
extern int delegate_real_cplat_close(int fd, cplat_error *detail_out);
extern int delegate_real_cplat_dup(int fd, cplat_error *detail_out);
extern int delegate_real_cplat_dup2(int oldfd, int newfd, cplat_error *detail_out);
extern int64_t delegate_real_cplat_read(int fd, void *buf, size_t count, cplat_error *detail_out);
extern int64_t delegate_real_cplat_write(int fd, const void *buf, size_t count, cplat_error *detail_out);

// crt - fcntl
extern int delegate_real_cplat_open_fmt(int flags, int mode, cplat_error *detail_out, const char *format, ...);
extern int delegate_real_cplat_vopen_fmt(int flags, int mode, cplat_error *detail_out, const char *format,
                                            va_list args);

// crt - string
extern int delegate_real_cplat_strcpy(char *dest, size_t dest_size, const char *src);
extern int delegate_real_cplat_strncpy(char *dest, size_t dest_size, const char *src, size_t count);
extern int delegate_real_cplat_strcat(char *dest, size_t dest_size, const char *src);
extern int delegate_real_cplat_strncat(char *dest, size_t dest_size, const char *src, size_t count);
extern char *delegate_real_cplat_strtok_r(char *str, const char *delim, char **saveptr);
extern char *delegate_real_cplat_strdup(const char *src);
extern void *delegate_real_cplat_malloc(size_t size);
extern void *delegate_real_cplat_calloc(size_t count, size_t size);
extern void *delegate_real_cplat_realloc(void *ptr, size_t count, size_t size);
extern void *delegate_real_cplat_realloc_zerofill(void *ptr, size_t old_count, size_t count, size_t size);
extern void delegate_real_cplat_free(void *ptr);
extern int delegate_real_cplat_wcscpy(wchar_t *dest, size_t dest_size, const wchar_t *src);

// crt - sys/stat
extern int delegate_real_cplat_stat_fmt(cplat_file_stat_t *buf, cplat_error *detail_out, const char *format,
                                           ...);
extern int delegate_real_cplat_vstat_fmt(cplat_file_stat_t *buf, cplat_error *detail_out, const char *format,
                                            va_list args);
extern int delegate_real_cplat_mkdir_fmt(cplat_error *detail_out, const char *format, ...);
extern int delegate_real_cplat_vmkdir_fmt(cplat_error *detail_out, const char *format, va_list args);

// crt - file
extern void delegate_real_cplat_file_init(cplat_file *file);
extern int delegate_real_cplat_file_open(cplat_file *file, const char *path, int flags,
                                            cplat_error *detail_out);
extern int delegate_real_cplat_file_write(cplat_file *file, const void *buf, size_t len,
                                             cplat_error *detail_out);
extern int delegate_real_cplat_file_read(cplat_file *file, void *buf, size_t len, size_t *read_out,
                                            cplat_error *detail_out);
extern int delegate_real_cplat_file_get_size(const cplat_file *file, size_t *size_out,
                                                cplat_error *detail_out);
extern int delegate_real_cplat_file_set_size(cplat_file *file, size_t size, cplat_error *detail_out);
extern int delegate_real_cplat_file_get_id(const cplat_file *file, cplat_file_id *id_out,
                                              cplat_error *detail_out);
extern int delegate_real_cplat_file_get_path_id(const char *path, cplat_file_id *id_out,
                                                   cplat_error *detail_out);
extern int delegate_real_cplat_file_get_modified_timestamp(const cplat_file *file,
                                                              cplat_timespec *timestamp_out,
                                                              cplat_error *detail_out);
extern int delegate_real_cplat_file_set_modified_timestamp(cplat_file *file, const cplat_timespec *timestamp,
                                                              cplat_error *detail_out);
extern int delegate_real_cplat_file_get_path_modified_timestamp(const char *path, cplat_timespec *timestamp_out,
                                                                   cplat_error *detail_out);
extern int delegate_real_cplat_file_set_path_modified_timestamp(const char *path, const cplat_timespec *timestamp,
                                                                   cplat_error *detail_out);
extern int delegate_real_cplat_file_stat_is_regular(const cplat_file_stat_t *file_stat);
extern int delegate_real_cplat_file_flush(cplat_file *file, cplat_error *detail_out);
extern int delegate_real_cplat_file_close(cplat_file *file, cplat_error *detail_out);

// trace - tracer
extern cplat_tracer *delegate_real_cplat_tracer_create(cplat_tracer_concurrency_mode concurrency_mode);
extern void delegate_real_cplat_tracer_dispose(cplat_tracer **handle);
extern int delegate_real_cplat_tracer_start(cplat_tracer *handle);
extern int delegate_real_cplat_tracer_stop(cplat_tracer *handle);
extern int delegate_real_cplat_tracer_write_at(cplat_tracer *handle, cplat_trace_level level,
                                                  const cplat_timespec *timestamp, const char *message);
extern int delegate_real_cplat_tracer_write_hex_at(cplat_tracer *handle, cplat_trace_level level,
                                                      const cplat_timespec *timestamp, const void *data, size_t size,
                                                      const char *message);
extern int delegate_real_cplat_tracer_writef_at(cplat_tracer *handle, cplat_trace_level level,
                                                   const cplat_timespec *timestamp, const char *format, ...);
extern int delegate_real_cplat_tracer_write_hexf_at(cplat_tracer *handle, cplat_trace_level level,
                                                       const cplat_timespec *timestamp, const void *data,
                                                       size_t size, const char *format, ...);
extern const char *delegate_real_cplat_tracer_hex_sep(const char *message);
extern const char *delegate_real_cplat_tracer_hex_msg(const char *message);
extern int delegate_real_cplat_tracer_write_with_source(cplat_tracer *handle, cplat_trace_level level,
                                                           const cplat_timespec *timestamp, const char *file,
                                                           int line, const char *message);
extern int delegate_real_cplat_tracer_set_name(cplat_tracer *handle, const char *name, int64_t identifier);
extern int delegate_real_cplat_tracer_set_os_level(cplat_tracer *handle, cplat_trace_level level);
extern int delegate_real_cplat_tracer_set_etw_level(cplat_tracer *handle, cplat_trace_level level);
extern int delegate_real_cplat_tracer_set_file_level(cplat_tracer *handle, const char *path,
                                                        cplat_trace_level level, size_t max_bytes, int generations,
                                                        int flags);
extern int delegate_real_cplat_tracer_set_stderr_level(cplat_tracer *handle, cplat_trace_level level);
extern cplat_tracer_hook_entry *delegate_real_cplat_tracer_set_hook(cplat_tracer *handle,
                                                                          cplat_tracer_hook_fn fn, void *context);
extern void delegate_real_cplat_tracer_remove_hook(cplat_tracer *handle, cplat_tracer_hook_entry *hook_entry);
extern void delegate_real_cplat_tracer_call_next_hook(cplat_tracer_hook_entry *prev, cplat_tracer *handle,
                                                         cplat_trace_level level, const cplat_timespec *timestamp,
                                                         const char *message);
extern cplat_tracer_state delegate_real_cplat_tracer_get_state(cplat_tracer *handle);
extern cplat_trace_level delegate_real_cplat_tracer_get_os_level(cplat_tracer *handle);
extern cplat_trace_level delegate_real_cplat_tracer_get_etw_level(cplat_tracer *handle);
extern cplat_trace_level delegate_real_cplat_tracer_get_file_level(cplat_tracer *handle);
extern cplat_trace_level delegate_real_cplat_tracer_get_stderr_level(cplat_tracer *handle);

// clock
extern uint64_t delegate_real_cplat_get_monotonic_ms(void);
extern void delegate_real_cplat_get_monotonic(cplat_timespec *ts);
extern void delegate_real_cplat_get_realtime(cplat_timespec *ts);
extern void delegate_real_cplat_get_realtime_utc(struct tm *utc_tm, int32_t *tv_nsec);
extern int delegate_real_cplat_format_realtime_iso8601_local(char *buf, size_t buf_size,
                                                                const cplat_timespec *timestamp);
extern int delegate_real_cplat_format_realtime_iso8601_utc(char *buf, size_t buf_size,
                                                              const cplat_timespec *timestamp);
extern void delegate_real_cplat_get_realtime_deadline_ms(uint64_t timeout_ms, struct timespec *abs_timeout);
extern void delegate_real_cplat_timespec_from_native(const struct timespec *native, cplat_timespec *ts);
extern void delegate_real_cplat_timespec_to_native(const cplat_timespec *ts, struct timespec *native);
extern void delegate_real_cplat_timespec_add_ms(const cplat_timespec *ts, uint64_t timeout_ms,
                                                   cplat_timespec *result);
extern int delegate_real_cplat_timespec_cmp(const cplat_timespec *a, const cplat_timespec *b);

// console
extern void delegate_real_cplat_console_init(void);
extern void delegate_real_cplat_console_dispose(void);
extern int delegate_real_cplat_console_attach_parent(int *argc, char **argv, int *attached_out);
extern void delegate_real_cplat_console_dispose_on_shutdown(const cplat_shutdown_event *event, void *context);

// sync
extern int delegate_real_cplat_local_lock_create(cplat_local_lock **mtx);
extern int delegate_real_cplat_local_lock_lock(cplat_local_lock *mtx, int timeout_ms);
extern int delegate_real_cplat_local_lock_try_lock(cplat_local_lock *mtx);
extern int delegate_real_cplat_local_lock_unlock(cplat_local_lock *mtx);
extern void delegate_real_cplat_local_lock_dispose(cplat_local_lock *mtx);
extern int delegate_real_cplat_condvar_create(cplat_condvar **cv);
extern int delegate_real_cplat_condvar_wait(cplat_condvar *cv, cplat_local_lock *mtx, int timeout_ms);
extern int delegate_real_cplat_condvar_signal(cplat_condvar *cv);
extern int delegate_real_cplat_condvar_broadcast(cplat_condvar *cv);
extern void delegate_real_cplat_condvar_dispose(cplat_condvar *cv);
extern int delegate_real_cplat_local_rwlock_create(cplat_local_rwlock **rwlock);
extern int delegate_real_cplat_local_rwlock_lock_shared(cplat_local_rwlock *rwlock, int timeout_ms);
extern int delegate_real_cplat_local_rwlock_try_lock_shared(cplat_local_rwlock *rwlock);
extern int delegate_real_cplat_local_rwlock_lock_exclusive(cplat_local_rwlock *rwlock, int timeout_ms);
extern int delegate_real_cplat_local_rwlock_try_lock_exclusive(cplat_local_rwlock *rwlock);
extern int delegate_real_cplat_local_rwlock_unlock_shared(cplat_local_rwlock *rwlock);
extern int delegate_real_cplat_local_rwlock_unlock_exclusive(cplat_local_rwlock *rwlock);
extern void delegate_real_cplat_local_rwlock_dispose(cplat_local_rwlock *rwlock);
extern int delegate_real_cplat_thread_create(cplat_thread **thread, cplat_thread_fn func, void *arg);
extern int delegate_real_cplat_thread_join(cplat_thread *thread, int timeout_ms);
extern void delegate_real_cplat_thread_detach(cplat_thread *thread);
extern int delegate_real_cplat_interprocess_lock_open(const char *identity, cplat_interprocess_lock **lock);
extern int delegate_real_cplat_interprocess_lock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                                      cplat_interprocess_lock **lock);
extern int delegate_real_cplat_interprocess_lock_export_descriptor(const cplat_interprocess_lock *lock,
                                                                      void *descriptor, size_t *descriptor_size);
extern int delegate_real_cplat_interprocess_lock_lock(cplat_interprocess_lock *lock, int timeout_ms);
extern int delegate_real_cplat_interprocess_lock_try_lock(cplat_interprocess_lock *lock);
extern int delegate_real_cplat_interprocess_lock_unlock(cplat_interprocess_lock *lock);
extern void delegate_real_cplat_interprocess_lock_dispose(cplat_interprocess_lock *lock);
extern int delegate_real_cplat_interprocess_rwlock_open(const char *identity, cplat_interprocess_rwlock **lock);
extern int delegate_real_cplat_interprocess_rwlock_import_descriptor(const void *descriptor, size_t descriptor_size,
                                                                        cplat_interprocess_rwlock **lock);
extern int delegate_real_cplat_interprocess_rwlock_export_descriptor(const cplat_interprocess_rwlock *lock,
                                                                        void *descriptor, size_t *descriptor_size);
extern int delegate_real_cplat_interprocess_rwlock_lock_shared(cplat_interprocess_rwlock *lock, int timeout_ms);
extern int delegate_real_cplat_interprocess_rwlock_try_lock_shared(cplat_interprocess_rwlock *lock);
extern int delegate_real_cplat_interprocess_rwlock_lock_exclusive(cplat_interprocess_rwlock *lock,
                                                                     int timeout_ms);
extern int delegate_real_cplat_interprocess_rwlock_try_lock_exclusive(cplat_interprocess_rwlock *lock);
extern int delegate_real_cplat_interprocess_rwlock_unlock(cplat_interprocess_rwlock *lock);
extern void delegate_real_cplat_interprocess_rwlock_dispose(cplat_interprocess_rwlock *lock);
extern void delegate_real_cplat_call_once(cplat_once_flag *flag, cplat_once_fn func);
extern void delegate_real_cplat_sleep_ms(int ms);

// runtime - module_info
extern int delegate_real_cplat_module_get_path(char *path_out, size_t path_size, const void *func_addr);
extern int delegate_real_cplat_module_get_basename(char *basename_out, size_t basename_size, const void *func_addr);

// runtime - memory_lock
extern int delegate_real_cplat_memory_lock_range(const void *address, size_t size);
extern int delegate_real_cplat_memory_unlock_range(const void *address, size_t size);
extern int delegate_real_cplat_memory_lock_self(const cplat_memory_lock_self_options *options,
                                                   cplat_memory_lock_scope **scope);
extern int delegate_real_cplat_memory_lock_scope_release(cplat_memory_lock_scope *scope);
// hashtable
extern int delegate_real_cplat_hashtable_required_size(const cplat_hashtable_config *config,
                                                          size_t *mgmt_size_out, size_t *data_size_out);
extern int delegate_real_cplat_hashtable_create(const cplat_hashtable_config *config, void *buf_mgmt,
                                                   size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                                                   cplat_hashtable **ht_out);
extern int delegate_real_cplat_hashtable_attach(void *buf_mgmt, size_t buf_mgmt_size, void *buf_data,
                                                   size_t buf_data_size, cplat_hashtable **ht_out);
extern int delegate_real_cplat_hashtable_validate(const cplat_hashtable *ht);
extern int delegate_real_cplat_hashtable_get_config_ref(const cplat_hashtable *ht,
                                                           const cplat_hashtable_config **config_out);
extern int delegate_real_cplat_hashtable_get_config_val(const cplat_hashtable *ht,
                                                           cplat_hashtable_config *config_out);
extern int delegate_real_cplat_hashtable_buffer_size(const cplat_hashtable *ht, size_t *mgmt_size_out,
                                                        size_t *data_size_out);
extern int delegate_real_cplat_hashtable_buffer_ref(const cplat_hashtable *ht, const void **mgmt_out,
                                                       const void **data_out);
extern int delegate_real_cplat_hashtable_add(cplat_hashtable *ht, const void *key, const void *value,
                                                cplat_hashtable_add_deleted_policy deleted_policy);
extern int delegate_real_cplat_hashtable_upsert(cplat_hashtable *ht, const void *key, const void *value,
                                                   int *inserted_out);
extern int delegate_real_cplat_hashtable_insert_direct(cplat_hashtable *ht, uint64_t record, const void *key,
                                                          int status, const void *value,
                                                          const cplat_timespec *timestamp, uint64_t generation);
extern int delegate_real_cplat_hashtable_update(cplat_hashtable *ht, const void *key, const void *value);
extern int delegate_real_cplat_hashtable_update_rec(cplat_hashtable *ht, uint64_t record, const void *value);
extern int delegate_real_cplat_hashtable_find_value_ref(const cplat_hashtable *ht, const void *key,
                                                           const void **value_out);
extern int delegate_real_cplat_hashtable_find_value_copy(const cplat_hashtable *ht, const void *key, void *dest,
                                                            size_t dest_size, size_t *required_size_out);
extern int delegate_real_cplat_hashtable_find_recno(const cplat_hashtable *ht, const void *key,
                                                       uint64_t *record_out);
extern int delegate_real_cplat_hashtable_get_key_ref(const cplat_hashtable *ht, uint64_t record,
                                                        const void **key_out);
extern int delegate_real_cplat_hashtable_get_key_copy(const cplat_hashtable *ht, uint64_t record, void *dest,
                                                         size_t dest_size, size_t *required_size_out);
extern int delegate_real_cplat_hashtable_get_value_ref(const cplat_hashtable *ht, uint64_t record,
                                                          const void **value_out);
extern int delegate_real_cplat_hashtable_get_value_copy(const cplat_hashtable *ht, uint64_t record, void *dest,
                                                           size_t dest_size, size_t *required_size_out);
extern int delegate_real_cplat_hashtable_get_status(const cplat_hashtable *ht, uint64_t record, int *status_out);
extern int delegate_real_cplat_hashtable_next_record(const cplat_hashtable *ht, uint64_t from,
                                                        unsigned int status_mask, uint64_t *record_out,
                                                        int *has_record_out);
extern int delegate_real_cplat_hashtable_get_timestamp_ref(const cplat_hashtable *ht, uint64_t record,
                                                              const cplat_timespec **timestamp_out);
extern int delegate_real_cplat_hashtable_get_timestamp_val(const cplat_hashtable *ht, uint64_t record,
                                                              cplat_timespec *timestamp_out);
extern int delegate_real_cplat_hashtable_get_generation(const cplat_hashtable *ht, uint64_t record,
                                                           uint64_t *generation_out);
extern int delegate_real_cplat_hashtable_get_table_timestamp_ref(const cplat_hashtable *ht,
                                                                    const cplat_timespec **timestamp_out);
extern int delegate_real_cplat_hashtable_get_table_timestamp_val(const cplat_hashtable *ht,
                                                                    cplat_timespec *timestamp_out);
extern int delegate_real_cplat_hashtable_get_table_generation(const cplat_hashtable *ht,
                                                                 uint64_t *generation_out);
extern int delegate_real_cplat_hashtable_find_timestamp_ref(const cplat_hashtable *ht, const void *key,
                                                               const cplat_timespec **timestamp_out);
extern int delegate_real_cplat_hashtable_find_timestamp_val(const cplat_hashtable *ht, const void *key,
                                                               cplat_timespec *timestamp_out);
extern int delegate_real_cplat_hashtable_find_generation(const cplat_hashtable *ht, const void *key,
                                                            uint64_t *generation_out);
extern int delegate_real_cplat_hashtable_count_status(const cplat_hashtable *ht, size_t *in_use_out,
                                                         size_t *deleted_out, size_t *empty_out);
extern int delegate_real_cplat_hashtable_count(const cplat_hashtable *ht, size_t *count_out);
extern int delegate_real_cplat_hashtable_deleted_count(const cplat_hashtable *ht, size_t *count_out);
extern int delegate_real_cplat_hashtable_empty_count(const cplat_hashtable *ht, size_t *count_out);
extern int delegate_real_cplat_hashtable_delete(cplat_hashtable *ht, const void *key);
extern int delegate_real_cplat_hashtable_delete_rec(cplat_hashtable *ht, uint64_t record);
extern int delegate_real_cplat_hashtable_push_deleted(cplat_hashtable *ht);
extern int delegate_real_cplat_hashtable_purge_deleted(cplat_hashtable *ht);
extern int delegate_real_cplat_hashtable_compact(cplat_hashtable *ht);
extern int delegate_real_cplat_hashtable_resize(cplat_hashtable *ht, const cplat_hashtable_config *new_config);
extern int delegate_real_cplat_hashtable_rebuild_into(const cplat_hashtable *src,
                                                         const cplat_hashtable_config *new_config, void *buf_mgmt,
                                                         size_t buf_mgmt_size, void *buf_data, size_t buf_data_size,
                                                         cplat_hashtable **ht_out);
extern int delegate_real_cplat_hashtable_clear(cplat_hashtable *ht);
extern void delegate_real_cplat_hashtable_dispose(cplat_hashtable *ht);

extern void delegate_real_cplat_secure_zero(void *buf, size_t size);

// runtime - process_info
extern int delegate_real_cplat_process_get_executable_path(char *path_out, size_t path_size);
extern uint32_t delegate_real_cplat_process_get_pid(void);
extern int delegate_real_cplat_elevated_process_is_elevated(int *elevated);
extern int delegate_real_cplat_elevated_process_run_if_needed(const char *arguments, int *exit_code, int *handled);
extern int delegate_real_cplat_elevated_process_run_with_result(const char *arguments, int *exit_code, int *handled,
                                                                   char *result_message, size_t result_message_size);
extern int delegate_real_cplat_elevated_process_extract_result_target(int *argc, char **argv, int *detected_out);
extern int delegate_real_cplat_elevated_process_report_result(const char *message);
extern int delegate_real_cplat_process_start(const cplat_process_options *options, cplat_process **process);
extern int delegate_real_cplat_process_wait(cplat_process *process, int timeout_ms);
extern int delegate_real_cplat_process_get_exit_code(cplat_process *process, int *exit_code);
extern int delegate_real_cplat_process_terminate(cplat_process *process);
extern void delegate_real_cplat_process_dispose(cplat_process *process);
extern int delegate_real_cplat_process_run_sync(const cplat_process_options *options, int timeout_ms,
                                                   int *exit_code);

// runtime - sym_loader
extern void *delegate_real_cplat_sym_loader_resolve(cplat_sym_loader_entry *fobj);
extern int delegate_real_cplat_sym_loader_is_default(cplat_sym_loader_entry *fobj);
extern void delegate_real_cplat_sym_loader_init(cplat_sym_loader_entry *const *fobj_array, size_t fobj_length,
                                                   const char *configpath);
extern void delegate_real_cplat_sym_loader_dispose(cplat_sym_loader_entry *const *fobj_array, size_t fobj_length);
extern int delegate_real_cplat_sym_loader_info(cplat_sym_loader_entry *const *fobj_array, size_t fobj_length);

// runtime - shutdown
extern int delegate_real_cplat_shutdown_register(cplat_shutdown_fn callback, void *context);
extern int delegate_real_cplat_shutdown_request_register(cplat_shutdown_fn callback, void *context);

// trace - trace_file_sink
extern cplat_trace_file_sink *delegate_real_cplat_trace_file_sink_create(const char *path, size_t max_bytes,
                                                                               int generations, int flags);
extern int delegate_real_cplat_trace_file_sink_write(cplat_trace_file_sink *handle, int level,
                                                        const cplat_timespec *timestamp, const char *message);
extern void delegate_real_cplat_trace_file_sink_dispose(cplat_trace_file_sink *handle);

#if defined(PLATFORM_LINUX)
// trace - syslog_sink (Linux only)
extern cplat_syslog_sink *delegate_real_cplat_syslog_sink_create(const char *ident, int facility);
extern int delegate_real_cplat_syslog_sink_write(cplat_syslog_sink *handle, int level,
                                                    const cplat_timespec *timestamp, const char *message);
extern int delegate_real_cplat_syslog_sink_rename(cplat_syslog_sink *handle, const char *new_ident);
extern void delegate_real_cplat_syslog_sink_dispose(cplat_syslog_sink *handle);
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
extern BOOL delegate_real_StartServiceCtrlDispatcherU(const cplat_service_entry_u *service_table);

// crt - wchar_conv (Windows only)
extern int delegate_real_cplat_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path);
extern int delegate_real_cplat_wpath_to_utf8(char *dest, size_t dest_size, const wchar_t *wpath);
extern wchar_t *delegate_real_cplat_utf8_to_wstr_alloc(const char *utf8_text);
extern char *delegate_real_cplat_wstr_to_utf8_alloc(const wchar_t *wtext);
#endif /* PLATFORM_WINDOWS */

#if defined(PLATFORM_WINDOWS)
// trace - etw (Windows only)
extern cplat_etw_provider *delegate_real_cplat_etw_provider_create(cplat_etw_provider_ref_t provider_ref);
extern void delegate_real_cplat_etw_provider_dispose(cplat_etw_provider *handle);
extern int delegate_real_cplat_etw_provider_write(cplat_etw_provider *handle, int level, const char *service,
                                                     const char *message);
extern int delegate_real_cplat_etw_session_check_access(void);
extern int delegate_real_cplat_etw_session_start(const char *session_name, const char *provider_guid_str,
                                                    cplat_etw_event_fn callback, void *context,
                                                    cplat_etw_session **session_out);
extern void delegate_real_cplat_etw_session_stop(cplat_etw_session *session);

// trace - eventlog (Windows only)
extern cplat_eventlog_sink *delegate_real_cplat_eventlog_sink_create(const char *source_name);
extern int delegate_real_cplat_eventlog_sink_write(cplat_eventlog_sink *handle, int level,
                                                      int64_t file_identifier, const char *instance_name,
                                                      int64_t instance_identifier, const char *message);
extern void delegate_real_cplat_eventlog_sink_dispose(cplat_eventlog_sink *handle);
extern int delegate_real_cplat_eventlog_register_source(const char *source_name, const char *message_file_path);
extern int delegate_real_cplat_eventlog_unregister_source(const char *source_name);
#endif /* PLATFORM_WINDOWS */

// prompt
extern cplat_prompt *delegate_real_cplat_prompt_create(const cplat_prompt_options *options);
extern void delegate_real_cplat_prompt_dispose(cplat_prompt *prompt);
extern int delegate_real_cplat_prompt_readline_at(cplat_prompt *p, char *buf, size_t buf_size,
                                                     const char *prompt_str, const char *file, int line);
extern int delegate_real_cplat_prompt_readline_fmt_at(cplat_prompt *p, char *buf, size_t buf_size,
                                                         const char *file, int line, const char *fmt, va_list args);
extern cplat_pinned_prompt *
delegate_real_cplat_pinned_prompt_create(const cplat_pinned_prompt_options *options);
extern void delegate_real_cplat_pinned_prompt_dispose(cplat_pinned_prompt *screen);
extern int delegate_real_cplat_pinned_prompt_readline_at(cplat_pinned_prompt *screen, char *buf, size_t buf_size,
                                                            const char *prompt_str, const char *file, int line);
extern int delegate_real_cplat_pinned_prompt_readline_fmt_at(cplat_pinned_prompt *screen, char *buf,
                                                                size_t buf_size, const char *file, int line,
                                                                const char *fmt, va_list args);
extern int delegate_real_cplat_pinned_prompt_write(cplat_pinned_prompt *screen,
                                                      cplat_pinned_prompt_channel channel, const void *data,
                                                      size_t size, size_t *written_out);
extern int delegate_real_cplat_pinned_prompt_printf(cplat_pinned_prompt *screen,
                                                       cplat_pinned_prompt_channel channel, const char *fmt, ...);
extern int delegate_real_cplat_pinned_prompt_status_enable(cplat_pinned_prompt *screen,
                                                              cplat_pinned_prompt_status_position position,
                                                              int enable);
extern int delegate_real_cplat_pinned_prompt_status_set(cplat_pinned_prompt *screen,
                                                           cplat_pinned_prompt_status_position position,
                                                           cplat_pinned_prompt_status_align align,
                                                           const char *content);

// argparser
extern cplat_argparser *delegate_real_cplat_argparser_handle_create(int argc, char *const *argv,
                                                                          const cplat_argparser_options *options);
extern void delegate_real_cplat_argparser_handle_dispose(cplat_argparser *parser);
extern int delegate_real_cplat_argparser_handle_register_flag(cplat_argparser *parser, const char *short_name,
                                                          const char *long_name, const char *description, int *storage);
extern int delegate_real_cplat_argparser_handle_register_option_int(cplat_argparser *parser, const char *short_name,
                                                                const char *long_name, const char *value_name,
                                                                const char *description, unsigned int flags,
                                                                int *storage);
extern int delegate_real_cplat_argparser_handle_register_option_string(cplat_argparser *parser, const char *short_name,
                                                                   const char *long_name, const char *value_name,
                                                                   const char *description, unsigned int flags,
                                                                   const char **storage);
extern int delegate_real_cplat_argparser_handle_register_option_int_array(cplat_argparser *parser,
                                                                      const char *short_name, const char *long_name,
                                                                      const char *value_name, const char *description,
                                                                      unsigned int flags, int *storage, size_t capacity,
                                                                      size_t *count);
extern int delegate_real_cplat_argparser_handle_register_option_string_array(
    cplat_argparser *parser, const char *short_name, const char *long_name, const char *value_name,
    const char *description, unsigned int flags, const char **storage, size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_handle_register_positional_int(cplat_argparser *parser, const char *name,
                                                                    const char *description, unsigned int flags,
                                                                    int *storage);
extern int delegate_real_cplat_argparser_handle_register_positional_string(cplat_argparser *parser, const char *name,
                                                                       const char *description, unsigned int flags,
                                                                       const char **storage);
extern int delegate_real_cplat_argparser_handle_register_positional_int_array(cplat_argparser *parser, const char *name,
                                                                          const char *description, unsigned int flags,
                                                                          int *storage, size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_handle_register_positional_string_array(cplat_argparser *parser,
                                                                             const char *name, const char *description,
                                                                             unsigned int flags, const char **storage,
                                                                             size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_handle_parse(cplat_argparser *parser);
extern int delegate_real_cplat_argparser_handle_get_error(const cplat_argparser *parser);
extern const char *delegate_real_cplat_argparser_handle_get_error_target(const cplat_argparser *parser);
extern int delegate_real_cplat_argparser_handle_get_error_index(const cplat_argparser *parser);
extern int delegate_real_cplat_argparser_handle_get_error_message(const cplat_argparser *parser, char *buffer,
                                                              size_t buffer_size);
extern int delegate_real_cplat_argparser_handle_get_usage(const cplat_argparser *parser, char *buffer,
                                                      size_t buffer_size, size_t *required_size);
extern int delegate_real_cplat_argparser_handle_print_usage(const cplat_argparser *parser, FILE *stream);
extern int delegate_real_cplat_argparser_handle_print_error_messages(const cplat_argparser *parser, FILE *stream);
extern int delegate_real_cplat_argparser_handle_get_register_error(const cplat_argparser *parser, size_t index);
extern size_t delegate_real_cplat_argparser_handle_get_register_error_count(const cplat_argparser *parser);
extern const char *delegate_real_cplat_argparser_handle_get_register_error_target(const cplat_argparser *parser,
                                                                              size_t index);
extern int delegate_real_cplat_argparser_handle_get_register_error_message(const cplat_argparser *parser, size_t index,
                                                                       char *buffer, size_t buffer_size);
extern int delegate_real_cplat_argparser_handle_print_register_error_messages(const cplat_argparser *parser,
                                                                          FILE *stream);

// argparser (省略可能な単一インスタンス API)
extern void delegate_real_cplat_argparser_init(int argc, char *const *argv, const char *description);
extern int delegate_real_cplat_argparser_register_flag(const char *short_name, const char *long_name,
                                                                  const char *description, int *storage);
extern int delegate_real_cplat_argparser_register_option_int(const char *short_name, const char *long_name,
                                                                        const char *value_name, const char *description,
                                                                        unsigned int flags, int *storage);
extern int delegate_real_cplat_argparser_register_option_string(const char *short_name,
                                                                           const char *long_name,
                                                                           const char *value_name,
                                                                           const char *description, unsigned int flags,
                                                                           const char **storage);
extern int delegate_real_cplat_argparser_register_option_int_array(
    const char *short_name, const char *long_name, const char *value_name, const char *description, unsigned int flags,
    int *storage, size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_register_option_string_array(
    const char *short_name, const char *long_name, const char *value_name, const char *description, unsigned int flags,
    const char **storage, size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_register_positional_int(const char *name, const char *description,
                                                                            unsigned int flags, int *storage);
extern int delegate_real_cplat_argparser_register_positional_string(const char *name,
                                                                               const char *description,
                                                                               unsigned int flags,
                                                                               const char **storage);
extern int delegate_real_cplat_argparser_register_positional_int_array(const char *name,
                                                                                  const char *description,
                                                                                  unsigned int flags, int *storage,
                                                                                  size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_register_positional_string_array(const char *name,
                                                                                     const char *description,
                                                                                     unsigned int flags,
                                                                                     const char **storage,
                                                                                     size_t capacity, size_t *count);
extern int delegate_real_cplat_argparser_parse(void);
extern int delegate_real_cplat_argparser_get_error(void);
extern const char *delegate_real_cplat_argparser_get_error_target(void);
extern int delegate_real_cplat_argparser_get_error_index(void);
extern int delegate_real_cplat_argparser_get_error_message(char *buffer, size_t buffer_size);
extern int delegate_real_cplat_argparser_get_usage(char *buffer, size_t buffer_size, size_t *required_size);
extern int delegate_real_cplat_argparser_print_usage(FILE *stream);
extern int delegate_real_cplat_argparser_print_error_messages(FILE *stream);
extern int delegate_real_cplat_argparser_get_register_error(size_t index);
extern size_t delegate_real_cplat_argparser_get_register_error_count(void);
extern const char *delegate_real_cplat_argparser_get_register_error_target(size_t index);
extern int delegate_real_cplat_argparser_get_register_error_message(size_t index, char *buffer,
                                                                               size_t buffer_size);
extern int delegate_real_cplat_argparser_print_register_error_messages(FILE *stream);

class Mock_cplat
{
  public:
    // compress
    // hashtable
    MOCK_METHOD(int, cplat_hashtable_required_size, (const cplat_hashtable_config *, size_t *, size_t *));
    MOCK_METHOD(int, cplat_hashtable_create,
                (const cplat_hashtable_config *, void *, size_t, void *, size_t, cplat_hashtable **));
    MOCK_METHOD(int, cplat_hashtable_attach, (void *, size_t, void *, size_t, cplat_hashtable **));
    MOCK_METHOD(int, cplat_hashtable_validate, (const cplat_hashtable *));
    MOCK_METHOD(int, cplat_hashtable_get_config_ref,
                (const cplat_hashtable *, const cplat_hashtable_config **));
    MOCK_METHOD(int, cplat_hashtable_get_config_val, (const cplat_hashtable *, cplat_hashtable_config *));
    MOCK_METHOD(int, cplat_hashtable_buffer_size, (const cplat_hashtable *, size_t *, size_t *));
    MOCK_METHOD(int, cplat_hashtable_buffer_ref, (const cplat_hashtable *, const void **, const void **));
    MOCK_METHOD(int, cplat_hashtable_add,
                (cplat_hashtable *, const void *, const void *, cplat_hashtable_add_deleted_policy));
    MOCK_METHOD(int, cplat_hashtable_upsert, (cplat_hashtable *, const void *, const void *, int *));
    MOCK_METHOD(int, cplat_hashtable_insert_direct,
                (cplat_hashtable *, uint64_t, const void *, int, const void *, const cplat_timespec *, uint64_t));
    MOCK_METHOD(int, cplat_hashtable_update, (cplat_hashtable *, const void *, const void *));
    MOCK_METHOD(int, cplat_hashtable_update_rec, (cplat_hashtable *, uint64_t, const void *));
    MOCK_METHOD(int, cplat_hashtable_find_value_ref, (const cplat_hashtable *, const void *, const void **));
    MOCK_METHOD(int, cplat_hashtable_find_value_copy,
                (const cplat_hashtable *, const void *, void *, size_t, size_t *));
    MOCK_METHOD(int, cplat_hashtable_find_recno, (const cplat_hashtable *, const void *, uint64_t *));
    MOCK_METHOD(int, cplat_hashtable_get_key_ref, (const cplat_hashtable *, uint64_t, const void **));
    MOCK_METHOD(int, cplat_hashtable_get_key_copy, (const cplat_hashtable *, uint64_t, void *, size_t, size_t *));
    MOCK_METHOD(int, cplat_hashtable_get_value_ref, (const cplat_hashtable *, uint64_t, const void **));
    MOCK_METHOD(int, cplat_hashtable_get_value_copy,
                (const cplat_hashtable *, uint64_t, void *, size_t, size_t *));
    MOCK_METHOD(int, cplat_hashtable_get_status, (const cplat_hashtable *, uint64_t, int *));
    MOCK_METHOD(int, cplat_hashtable_next_record,
                (const cplat_hashtable *, uint64_t, unsigned int, uint64_t *, int *));
    MOCK_METHOD(int, cplat_hashtable_get_timestamp_ref,
                (const cplat_hashtable *, uint64_t, const cplat_timespec **));
    MOCK_METHOD(int, cplat_hashtable_get_timestamp_val, (const cplat_hashtable *, uint64_t, cplat_timespec *));
    MOCK_METHOD(int, cplat_hashtable_get_generation, (const cplat_hashtable *, uint64_t, uint64_t *));
    MOCK_METHOD(int, cplat_hashtable_get_table_timestamp_ref,
                (const cplat_hashtable *, const cplat_timespec **));
    MOCK_METHOD(int, cplat_hashtable_get_table_timestamp_val, (const cplat_hashtable *, cplat_timespec *));
    MOCK_METHOD(int, cplat_hashtable_get_table_generation, (const cplat_hashtable *, uint64_t *));
    MOCK_METHOD(int, cplat_hashtable_find_timestamp_ref,
                (const cplat_hashtable *, const void *, const cplat_timespec **));
    MOCK_METHOD(int, cplat_hashtable_find_timestamp_val,
                (const cplat_hashtable *, const void *, cplat_timespec *));
    MOCK_METHOD(int, cplat_hashtable_find_generation, (const cplat_hashtable *, const void *, uint64_t *));
    MOCK_METHOD(int, cplat_hashtable_count_status, (const cplat_hashtable *, size_t *, size_t *, size_t *));
    MOCK_METHOD(int, cplat_hashtable_count, (const cplat_hashtable *, size_t *));
    MOCK_METHOD(int, cplat_hashtable_deleted_count, (const cplat_hashtable *, size_t *));
    MOCK_METHOD(int, cplat_hashtable_empty_count, (const cplat_hashtable *, size_t *));
    MOCK_METHOD(int, cplat_hashtable_delete, (cplat_hashtable *, const void *));
    MOCK_METHOD(int, cplat_hashtable_delete_rec, (cplat_hashtable *, uint64_t));
    MOCK_METHOD(int, cplat_hashtable_push_deleted, (cplat_hashtable *));
    MOCK_METHOD(int, cplat_hashtable_purge_deleted, (cplat_hashtable *));
    MOCK_METHOD(int, cplat_hashtable_compact, (cplat_hashtable *));
    MOCK_METHOD(int, cplat_hashtable_resize, (cplat_hashtable *, const cplat_hashtable_config *));
    MOCK_METHOD(int, cplat_hashtable_rebuild_into,
                (const cplat_hashtable *, const cplat_hashtable_config *, void *, size_t, void *, size_t,
                 cplat_hashtable **));
    MOCK_METHOD(int, cplat_hashtable_clear, (cplat_hashtable *));
    MOCK_METHOD(void, cplat_hashtable_dispose, (cplat_hashtable *));

    MOCK_METHOD(int, cplat_compress, (uint8_t *, size_t *, const uint8_t *, size_t));
    MOCK_METHOD(int, cplat_decompress, (uint8_t *, size_t *, const uint8_t *, size_t));

    // crypto
    MOCK_METHOD(int, cplat_encrypt,
                (uint8_t *, size_t *, const uint8_t *, size_t, const uint8_t *, const uint8_t *, const uint8_t *,
                 size_t));
    MOCK_METHOD(int, cplat_decrypt,
                (uint8_t *, size_t *, const uint8_t *, size_t, const uint8_t *, const uint8_t *, const uint8_t *,
                 size_t));
    MOCK_METHOD(int, cplat_passphrase_to_key, (uint8_t *, const uint8_t *, size_t));
    MOCK_METHOD(int, cplat_random_bytes, (void *, size_t));
    MOCK_METHOD(uint16_t, cplat_hton16, (uint16_t));
    MOCK_METHOD(uint16_t, cplat_ntoh16, (uint16_t));
    MOCK_METHOD(uint32_t, cplat_hton32, (uint32_t));
    MOCK_METHOD(uint32_t, cplat_ntoh32, (uint32_t));
    MOCK_METHOD(int, cplat_ipv4_parse, (const char *, uint32_t *));
    MOCK_METHOD(int, cplat_ipv4_resolve, (const char *, uint32_t *, cplat_error *));
    MOCK_METHOD(int, cplat_ipv4_to_string, (uint32_t, char *, size_t, cplat_error *));
    MOCK_METHOD(int, cplat_socket_open, (cplat_socket_kind, cplat_socket *, cplat_error *));
    MOCK_METHOD(void, cplat_socket_close, (cplat_socket));
    MOCK_METHOD(void, cplat_socket_shutdown, (cplat_socket));
    MOCK_METHOD(int, cplat_socket_bind, (cplat_socket, const cplat_ipv4_endpoint *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_listen, (cplat_socket, int, cplat_error *));
    MOCK_METHOD(int, cplat_socket_accept,
                (cplat_socket, cplat_ipv4_endpoint *, cplat_socket *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_connect, (cplat_socket, const cplat_ipv4_endpoint *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_get_pending_error, (cplat_socket, cplat_error *));
    MOCK_METHOD(int, cplat_socket_set_nonblocking, (cplat_socket, int, cplat_error *));
    MOCK_METHOD(int, cplat_socket_set_reuse_address, (cplat_socket, int, cplat_error *));
    MOCK_METHOD(int, cplat_socket_set_broadcast, (cplat_socket, int, cplat_error *));
    MOCK_METHOD(int, cplat_socket_set_multicast_interface, (cplat_socket, uint32_t, cplat_error *));
    MOCK_METHOD(int, cplat_socket_join_multicast_group, (cplat_socket, uint32_t, uint32_t, cplat_error *));
    MOCK_METHOD(int, cplat_socket_leave_multicast_group, (cplat_socket, uint32_t, uint32_t, cplat_error *));
    MOCK_METHOD(int, cplat_socket_send, (cplat_socket, const void *, size_t, size_t *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_recv, (cplat_socket, void *, size_t, size_t *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_sendto,
                (cplat_socket, const void *, size_t, const cplat_ipv4_endpoint *, size_t *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_recvfrom,
                (cplat_socket, void *, size_t, cplat_ipv4_endpoint *, size_t *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_send_all, (cplat_socket, const void *, size_t, cplat_error *));
    MOCK_METHOD(int, cplat_socket_recv_all, (cplat_socket, void *, size_t, cplat_error *));
    MOCK_METHOD(int, cplat_socket_wait_readable, (cplat_socket, int, int *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_wait_writable, (cplat_socket, int, int *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_wait_readable_multi,
                (const cplat_socket *, size_t, int, unsigned char *, cplat_error *));
    MOCK_METHOD(int, cplat_socket_shutdown_receive, (cplat_socket *, cplat_error *));

    // crt
    MOCK_METHOD(FILE *, cplat_fopen, (const char *, const char *, cplat_error *));
    MOCK_METHOD(FILE *, cplat_freopen, (const char *, const char *, FILE *, cplat_error *));
    MOCK_METHOD(int, cplat_fclose, (FILE *, cplat_error *));
    MOCK_METHOD(int, cplat_fflush, (FILE *, cplat_error *));
    MOCK_METHOD(size_t, cplat_fread, (void *, size_t, size_t, FILE *, cplat_error *));
    MOCK_METHOD(size_t, cplat_fwrite, (const void *, size_t, size_t, FILE *, cplat_error *));
    MOCK_METHOD(int, cplat_stat, (cplat_file_stat_t *, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_open, (const char *, int, int, cplat_error *));
    MOCK_METHOD(int, cplat_access, (const char *, int, cplat_error *));
    MOCK_METHOD(int, cplat_mkdir, (const char *, cplat_error *));
    MOCK_METHOD(int, cplat_makedirs, (const char *, cplat_error *));
    MOCK_METHOD(int, cplat_rmdir, (const char *, cplat_error *));
    MOCK_METHOD(int, cplat_remove, (const char *, cplat_error *));
    MOCK_METHOD(int, cplat_sscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int, cplat_vsscanf, (const char *, const char *, va_list));
    MOCK_METHOD(int, cplat_gmtime, (struct tm *, const time_t *));
    MOCK_METHOD(int, cplat_localtime, (struct tm *, const time_t *));
    MOCK_METHOD(int, cplat_ctime, (char *, size_t, const time_t *));
    MOCK_METHOD(int, cplat_getenv, (const char *, char *, size_t, int *, cplat_error *));
    MOCK_METHOD(int, cplat_setenv, (const char *, const char *, int, cplat_error *));
    MOCK_METHOD(int, cplat_parse_int64, (int64_t *, const char *, int));
    MOCK_METHOD(int, cplat_parse_uint64, (uint64_t *, const char *, int));
    MOCK_METHOD(int, cplat_parse_int, (int *, const char *, int));
    MOCK_METHOD(int, cplat_parse_double, (double *, const char *));
    MOCK_METHOD(int, cplat_unsetenv, (const char *, cplat_error *));
    MOCK_METHOD(int, cplat_path_get_full, (char *, size_t, cplat_error *, const char *));
    MOCK_METHOD(char *, cplat_normalize_path_sep, (char *));
    MOCK_METHOD(int, cplat_paths_equal, (const char *, const char *, int *, cplat_error *));
    MOCK_METHOD(const char *, cplat_path_basename, (const char *));
    MOCK_METHOD(int, cplat_path_dirname, (char *, size_t, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_path_strip_extension, (char *, size_t, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_path_join_n, (char *, size_t, cplat_error *, size_t, va_list));
    MOCK_METHOD(int, cplat_vpath_join_n, (char *, size_t, cplat_error *, size_t, va_list));

    // crt - stdio
    MOCK_METHOD(int, cplat_scanf, (const char *, va_list));
    MOCK_METHOD(int, cplat_vscanf, (const char *, va_list));
    MOCK_METHOD(int, cplat_fscanf, (FILE *, const char *, va_list));
    MOCK_METHOD(int, cplat_vfscanf, (FILE *, const char *, va_list));
    MOCK_METHOD(int, cplat_snprintf, (char *, size_t, const char *));
    MOCK_METHOD(int, cplat_vsnprintf, (char *, size_t, const char *));
    MOCK_METHOD(int, cplat_fgets, (char *, size_t, FILE *, cplat_error *));
    MOCK_METHOD(int, cplat_rename, (const char *, const char *, cplat_error *));
    MOCK_METHOD(int, cplat_fprintf, (FILE *, const char *));
    MOCK_METHOD(int, cplat_vfprintf, (FILE *, const char *));
    MOCK_METHOD(int, cplat_fseek, (FILE *, int64_t, int));
    MOCK_METHOD(int64_t, cplat_ftell, (FILE *));
    MOCK_METHOD(FILE *, cplat_fopen_fmt, (const char *, cplat_error *, const char *));
    MOCK_METHOD(FILE *, cplat_vfopen_fmt, (const char *, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_remove_fmt, (cplat_error *, const char *));
    MOCK_METHOD(int, cplat_vremove_fmt, (cplat_error *, const char *));
    MOCK_METHOD(FILE *, cplat_fopen_temp, (const char *, const char *, char *, size_t, cplat_error *));

    // crt - unistd
    MOCK_METHOD(int, cplat_isatty, (cplat_stream));
    MOCK_METHOD(int, cplat_access_fmt, (int, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_vaccess_fmt, (int, cplat_error *, const char *));
    MOCK_METHOD(int64_t, cplat_lseek, (int, int64_t, int, cplat_error *));
    MOCK_METHOD(int, cplat_close, (int, cplat_error *));
    MOCK_METHOD(int, cplat_dup, (int, cplat_error *));
    MOCK_METHOD(int, cplat_dup2, (int, int, cplat_error *));
    MOCK_METHOD(int64_t, cplat_read, (int, void *, size_t, cplat_error *));
    MOCK_METHOD(int64_t, cplat_write, (int, const void *, size_t, cplat_error *));

    // crt - fcntl
    MOCK_METHOD(int, cplat_open_fmt, (int, int, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_vopen_fmt, (int, int, cplat_error *, const char *));

    // crt - string
    MOCK_METHOD(int, cplat_strcpy, (char *, size_t, const char *));
    MOCK_METHOD(int, cplat_strncpy, (char *, size_t, const char *, size_t));
    MOCK_METHOD(int, cplat_strcat, (char *, size_t, const char *));
    MOCK_METHOD(int, cplat_strncat, (char *, size_t, const char *, size_t));
    MOCK_METHOD(char *, cplat_strtok_r, (char *, const char *, char **));
    MOCK_METHOD(char *, cplat_strdup, (const char *));
    MOCK_METHOD(void *, cplat_malloc, (size_t));
    MOCK_METHOD(void *, cplat_calloc, (size_t, size_t));
    MOCK_METHOD(void *, cplat_realloc, (void *, size_t, size_t));
    MOCK_METHOD(void *, cplat_realloc_zerofill, (void *, size_t, size_t, size_t));
    MOCK_METHOD(void, cplat_free, (void *));
    MOCK_METHOD(int, cplat_wcscpy, (wchar_t *, size_t, const wchar_t *));

    // crt - sys/stat
    MOCK_METHOD(int, cplat_stat_fmt, (cplat_file_stat_t *, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_vstat_fmt, (cplat_file_stat_t *, cplat_error *, const char *));
    MOCK_METHOD(int, cplat_mkdir_fmt, (cplat_error *, const char *));
    MOCK_METHOD(int, cplat_vmkdir_fmt, (cplat_error *, const char *));

    // crt - file
    MOCK_METHOD(void, cplat_file_init, (cplat_file *));
    MOCK_METHOD(int, cplat_file_open, (cplat_file *, const char *, int, cplat_error *));
    MOCK_METHOD(int, cplat_file_write, (cplat_file *, const void *, size_t, cplat_error *));
    MOCK_METHOD(int, cplat_file_read, (cplat_file *, void *, size_t, size_t *, cplat_error *));
    MOCK_METHOD(int, cplat_file_get_size, (const cplat_file *, size_t *, cplat_error *));
    MOCK_METHOD(int, cplat_file_set_size, (cplat_file *, size_t, cplat_error *));
    MOCK_METHOD(int, cplat_file_get_id, (const cplat_file *, cplat_file_id *, cplat_error *));
    MOCK_METHOD(int, cplat_file_get_path_id, (const char *, cplat_file_id *, cplat_error *));
    MOCK_METHOD(int, cplat_file_get_modified_timestamp,
                (const cplat_file *, cplat_timespec *, cplat_error *));
    MOCK_METHOD(int, cplat_file_set_modified_timestamp,
                (cplat_file *, const cplat_timespec *, cplat_error *));
    MOCK_METHOD(int, cplat_file_get_path_modified_timestamp, (const char *, cplat_timespec *, cplat_error *));
    MOCK_METHOD(int, cplat_file_set_path_modified_timestamp,
                (const char *, const cplat_timespec *, cplat_error *));
    MOCK_METHOD(int, cplat_file_stat_is_regular, (const cplat_file_stat_t *));
    MOCK_METHOD(int, cplat_file_flush, (cplat_file *, cplat_error *));
    MOCK_METHOD(int, cplat_file_close, (cplat_file *, cplat_error *));

    // trace - tracer
    MOCK_METHOD(cplat_tracer *, cplat_tracer_create, (cplat_tracer_concurrency_mode));
    MOCK_METHOD(void, cplat_tracer_dispose, (cplat_tracer **));
    MOCK_METHOD(int, cplat_tracer_start, (cplat_tracer *));
    MOCK_METHOD(int, cplat_tracer_stop, (cplat_tracer *));
    MOCK_METHOD(int, cplat_tracer_write_at,
                (cplat_tracer *, cplat_trace_level, const cplat_timespec *, const char *));
    MOCK_METHOD(int, cplat_tracer_write_hex_at,
                (cplat_tracer *, cplat_trace_level, const cplat_timespec *, const void *, size_t,
                 const char *));
    MOCK_METHOD(int, cplat_tracer_writef_at,
                (cplat_tracer *, cplat_trace_level, const cplat_timespec *, const char *));
    MOCK_METHOD(int, cplat_tracer_write_hexf_at,
                (cplat_tracer *, cplat_trace_level, const cplat_timespec *, const void *, size_t,
                 const char *));
    MOCK_METHOD(const char *, cplat_tracer_hex_sep, (const char *));
    MOCK_METHOD(const char *, cplat_tracer_hex_msg, (const char *));
    MOCK_METHOD(int, cplat_tracer_write_with_source,
                (cplat_tracer *, cplat_trace_level, const cplat_timespec *, const char *, int, const char *));
    MOCK_METHOD(int, cplat_tracer_set_name, (cplat_tracer *, const char *, int64_t));
    MOCK_METHOD(int, cplat_tracer_set_os_level, (cplat_tracer *, cplat_trace_level));
    MOCK_METHOD(int, cplat_tracer_set_etw_level, (cplat_tracer *, cplat_trace_level));
    MOCK_METHOD(int, cplat_tracer_set_file_level,
                (cplat_tracer *, const char *, cplat_trace_level, size_t, int, int));
    MOCK_METHOD(int, cplat_tracer_set_stderr_level, (cplat_tracer *, cplat_trace_level));
    MOCK_METHOD(cplat_tracer_hook_entry *, cplat_tracer_set_hook,
                (cplat_tracer *, cplat_tracer_hook_fn, void *));
    MOCK_METHOD(void, cplat_tracer_remove_hook, (cplat_tracer *, cplat_tracer_hook_entry *));
    MOCK_METHOD(void, cplat_tracer_call_next_hook,
                (cplat_tracer_hook_entry *, cplat_tracer *, cplat_trace_level, const cplat_timespec *,
                 const char *));
    MOCK_METHOD(cplat_tracer_state, cplat_tracer_get_state, (cplat_tracer *));
    MOCK_METHOD(cplat_trace_level, cplat_tracer_get_os_level, (cplat_tracer *));
    MOCK_METHOD(cplat_trace_level, cplat_tracer_get_etw_level, (cplat_tracer *));
    MOCK_METHOD(cplat_trace_level, cplat_tracer_get_file_level, (cplat_tracer *));
    MOCK_METHOD(cplat_trace_level, cplat_tracer_get_stderr_level, (cplat_tracer *));

    // clock
    MOCK_METHOD(uint64_t, cplat_get_monotonic_ms, ());
    MOCK_METHOD(void, cplat_get_monotonic, (cplat_timespec *));
    MOCK_METHOD(void, cplat_get_realtime, (cplat_timespec *));
    MOCK_METHOD(void, cplat_get_realtime_utc, (struct tm *, int32_t *));
    MOCK_METHOD(int, cplat_format_realtime_iso8601_local, (char *, size_t, const cplat_timespec *));
    MOCK_METHOD(int, cplat_format_realtime_iso8601_utc, (char *, size_t, const cplat_timespec *));
    MOCK_METHOD(void, cplat_get_realtime_deadline_ms, (uint64_t, struct timespec *));
    MOCK_METHOD(void, cplat_timespec_from_native, (const struct timespec *, cplat_timespec *));
    MOCK_METHOD(void, cplat_timespec_to_native, (const cplat_timespec *, struct timespec *));
    MOCK_METHOD(void, cplat_timespec_add_ms, (const cplat_timespec *, uint64_t, cplat_timespec *));
    MOCK_METHOD(int, cplat_timespec_cmp, (const cplat_timespec *, const cplat_timespec *));

    // console
    MOCK_METHOD(void, cplat_console_init, ());
    MOCK_METHOD(void, cplat_console_dispose, ());
    MOCK_METHOD(int, cplat_console_attach_parent, (int *, char **, int *));
    MOCK_METHOD(void, cplat_console_dispose_on_shutdown, (const cplat_shutdown_event *, void *));

    // sync
    MOCK_METHOD(int, cplat_local_lock_create, (cplat_local_lock **));
    MOCK_METHOD(int, cplat_local_lock_lock, (cplat_local_lock *, int));
    MOCK_METHOD(int, cplat_local_lock_try_lock, (cplat_local_lock *));
    MOCK_METHOD(int, cplat_local_lock_unlock, (cplat_local_lock *));
    MOCK_METHOD(void, cplat_local_lock_dispose, (cplat_local_lock *));
    MOCK_METHOD(int, cplat_condvar_create, (cplat_condvar **));
    MOCK_METHOD(int, cplat_condvar_wait, (cplat_condvar *, cplat_local_lock *, int));
    MOCK_METHOD(int, cplat_condvar_signal, (cplat_condvar *));
    MOCK_METHOD(int, cplat_condvar_broadcast, (cplat_condvar *));
    MOCK_METHOD(void, cplat_condvar_dispose, (cplat_condvar *));
    MOCK_METHOD(int, cplat_local_rwlock_create, (cplat_local_rwlock **));
    MOCK_METHOD(int, cplat_local_rwlock_lock_shared, (cplat_local_rwlock *, int));
    MOCK_METHOD(int, cplat_local_rwlock_try_lock_shared, (cplat_local_rwlock *));
    MOCK_METHOD(int, cplat_local_rwlock_lock_exclusive, (cplat_local_rwlock *, int));
    MOCK_METHOD(int, cplat_local_rwlock_try_lock_exclusive, (cplat_local_rwlock *));
    MOCK_METHOD(int, cplat_local_rwlock_unlock_shared, (cplat_local_rwlock *));
    MOCK_METHOD(int, cplat_local_rwlock_unlock_exclusive, (cplat_local_rwlock *));
    MOCK_METHOD(void, cplat_local_rwlock_dispose, (cplat_local_rwlock *));
    MOCK_METHOD(int, cplat_thread_create, (cplat_thread **, cplat_thread_fn, void *));
    MOCK_METHOD(int, cplat_thread_join, (cplat_thread *, int));
    MOCK_METHOD(void, cplat_thread_detach, (cplat_thread *));
    MOCK_METHOD(int, cplat_interprocess_lock_open, (const char *, cplat_interprocess_lock **));
    MOCK_METHOD(int, cplat_interprocess_lock_import_descriptor,
                (const void *, size_t, cplat_interprocess_lock **));
    MOCK_METHOD(int, cplat_interprocess_lock_export_descriptor,
                (const cplat_interprocess_lock *, void *, size_t *));
    MOCK_METHOD(int, cplat_interprocess_lock_lock, (cplat_interprocess_lock *, int));
    MOCK_METHOD(int, cplat_interprocess_lock_try_lock, (cplat_interprocess_lock *));
    MOCK_METHOD(int, cplat_interprocess_lock_unlock, (cplat_interprocess_lock *));
    MOCK_METHOD(void, cplat_interprocess_lock_dispose, (cplat_interprocess_lock *));
    MOCK_METHOD(int, cplat_interprocess_rwlock_open, (const char *, cplat_interprocess_rwlock **));
    MOCK_METHOD(int, cplat_interprocess_rwlock_import_descriptor,
                (const void *, size_t, cplat_interprocess_rwlock **));
    MOCK_METHOD(int, cplat_interprocess_rwlock_export_descriptor,
                (const cplat_interprocess_rwlock *, void *, size_t *));
    MOCK_METHOD(int, cplat_interprocess_rwlock_lock_shared, (cplat_interprocess_rwlock *, int));
    MOCK_METHOD(int, cplat_interprocess_rwlock_try_lock_shared, (cplat_interprocess_rwlock *));
    MOCK_METHOD(int, cplat_interprocess_rwlock_lock_exclusive, (cplat_interprocess_rwlock *, int));
    MOCK_METHOD(int, cplat_interprocess_rwlock_try_lock_exclusive, (cplat_interprocess_rwlock *));
    MOCK_METHOD(int, cplat_interprocess_rwlock_unlock, (cplat_interprocess_rwlock *));
    MOCK_METHOD(void, cplat_interprocess_rwlock_dispose, (cplat_interprocess_rwlock *));
    MOCK_METHOD(void, cplat_call_once, (cplat_once_flag *, cplat_once_fn));
    MOCK_METHOD(void, cplat_sleep_ms, (int));

    // runtime - module_info
    MOCK_METHOD(int, cplat_module_get_path, (char *, size_t, const void *));
    MOCK_METHOD(int, cplat_module_get_basename, (char *, size_t, const void *));

    // runtime - memory_lock
    MOCK_METHOD(int, cplat_memory_lock_range, (const void *, size_t));
    MOCK_METHOD(int, cplat_memory_unlock_range, (const void *, size_t));
    MOCK_METHOD(int, cplat_memory_lock_self,
                (const cplat_memory_lock_self_options *, cplat_memory_lock_scope **));
    MOCK_METHOD(int, cplat_memory_lock_scope_release, (cplat_memory_lock_scope *));
    MOCK_METHOD(void, cplat_secure_zero, (void *, size_t));

    // runtime - process_info
    MOCK_METHOD(int, cplat_process_get_executable_path, (char *, size_t));
    MOCK_METHOD(uint32_t, cplat_process_get_pid, ());
    MOCK_METHOD(int, cplat_elevated_process_is_elevated, (int *));
    MOCK_METHOD(int, cplat_elevated_process_run_if_needed, (const char *, int *, int *));
    MOCK_METHOD(int, cplat_elevated_process_run_with_result, (const char *, int *, int *, char *, size_t));
    MOCK_METHOD(int, cplat_elevated_process_extract_result_target, (int *, char **, int *));
    MOCK_METHOD(int, cplat_elevated_process_report_result, (const char *));
    MOCK_METHOD(int, cplat_process_start, (const cplat_process_options *, cplat_process **));
    MOCK_METHOD(int, cplat_process_wait, (cplat_process *, int));
    MOCK_METHOD(int, cplat_process_get_exit_code, (cplat_process *, int *));
    MOCK_METHOD(int, cplat_process_terminate, (cplat_process *));
    MOCK_METHOD(void, cplat_process_dispose, (cplat_process *));
    MOCK_METHOD(int, cplat_process_run_sync, (const cplat_process_options *, int, int *));

    // runtime - sym_loader
    MOCK_METHOD(void *, cplat_sym_loader_resolve, (cplat_sym_loader_entry *));
    MOCK_METHOD(int, cplat_sym_loader_is_default, (cplat_sym_loader_entry *));
    MOCK_METHOD(void, cplat_sym_loader_init, (cplat_sym_loader_entry *const *, size_t, const char *));
    MOCK_METHOD(void, cplat_sym_loader_dispose, (cplat_sym_loader_entry *const *, size_t));
    MOCK_METHOD(int, cplat_sym_loader_info, (cplat_sym_loader_entry *const *, size_t));

    // runtime - shutdown
    MOCK_METHOD(int, cplat_shutdown_register, (cplat_shutdown_fn, void *));
    MOCK_METHOD(int, cplat_shutdown_request_register, (cplat_shutdown_fn, void *));

    // trace - trace_file_sink
    MOCK_METHOD(cplat_trace_file_sink *, cplat_trace_file_sink_create, (const char *, size_t, int, int));
    MOCK_METHOD(int, cplat_trace_file_sink_write,
                (cplat_trace_file_sink *, int, const cplat_timespec *, const char *));
    MOCK_METHOD(void, cplat_trace_file_sink_dispose, (cplat_trace_file_sink *));

#if defined(PLATFORM_LINUX)
    // trace - syslog_sink (Linux only)
    MOCK_METHOD(cplat_syslog_sink *, cplat_syslog_sink_create, (const char *, int));
    MOCK_METHOD(int, cplat_syslog_sink_write,
                (cplat_syslog_sink *, int, const cplat_timespec *, const char *));
    MOCK_METHOD(int, cplat_syslog_sink_rename, (cplat_syslog_sink *, const char *));
    MOCK_METHOD(void, cplat_syslog_sink_dispose, (cplat_syslog_sink *));
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
    MOCK_METHOD(BOOL, StartServiceCtrlDispatcherU, (const cplat_service_entry_u *));

    // crt - wchar_conv (Windows only)
    MOCK_METHOD(int, cplat_utf8_to_wpath, (wchar_t *, size_t, const char *));
    MOCK_METHOD(int, cplat_wpath_to_utf8, (char *, size_t, const wchar_t *));
    MOCK_METHOD(wchar_t *, cplat_utf8_to_wstr_alloc, (const char *));
    MOCK_METHOD(char *, cplat_wstr_to_utf8_alloc, (const wchar_t *));
#endif /* PLATFORM_WINDOWS */

#if defined(PLATFORM_WINDOWS)
    // trace - trace_etw (Windows only)
    MOCK_METHOD(cplat_etw_provider *, cplat_etw_provider_create, (cplat_etw_provider_ref_t));
    MOCK_METHOD(int, cplat_etw_provider_write, (cplat_etw_provider *, int, const char *, const char *));
    MOCK_METHOD(void, cplat_etw_provider_dispose, (cplat_etw_provider *));
    MOCK_METHOD(int, cplat_etw_session_check_access, ());
    MOCK_METHOD(int, cplat_etw_session_start,
                (const char *, const char *, cplat_etw_event_fn, void *, cplat_etw_session **));
    MOCK_METHOD(void, cplat_etw_session_stop, (cplat_etw_session *));

    // trace - trace_eventlog (Windows only)
    MOCK_METHOD(cplat_eventlog_sink *, cplat_eventlog_sink_create, (const char *));
    MOCK_METHOD(int, cplat_eventlog_sink_write,
                (cplat_eventlog_sink *, int, int64_t, const char *, int64_t, const char *));
    MOCK_METHOD(void, cplat_eventlog_sink_dispose, (cplat_eventlog_sink *));
    MOCK_METHOD(int, cplat_eventlog_register_source, (const char *, const char *));
    MOCK_METHOD(int, cplat_eventlog_unregister_source, (const char *));
#endif /* PLATFORM_WINDOWS */

    // prompt
    MOCK_METHOD(cplat_prompt *, cplat_prompt_create, (const cplat_prompt_options *));
    MOCK_METHOD(void, cplat_prompt_dispose, (cplat_prompt *));
    MOCK_METHOD(int, cplat_prompt_readline_at, (cplat_prompt *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, cplat_prompt_readline_fmt_at,
                (cplat_prompt *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(cplat_pinned_prompt *, cplat_pinned_prompt_create, (const cplat_pinned_prompt_options *));
    MOCK_METHOD(void, cplat_pinned_prompt_dispose, (cplat_pinned_prompt *));
    MOCK_METHOD(int, cplat_pinned_prompt_readline_at,
                (cplat_pinned_prompt *, char *, size_t, const char *, const char *, int));
    MOCK_METHOD(int, cplat_pinned_prompt_readline_fmt_at,
                (cplat_pinned_prompt *, char *, size_t, const char *, int, const char *, va_list));
    MOCK_METHOD(int, cplat_pinned_prompt_write,
                (cplat_pinned_prompt *, cplat_pinned_prompt_channel, const void *, size_t, size_t *));
    MOCK_METHOD(int, cplat_pinned_prompt_printf,
                (cplat_pinned_prompt *, cplat_pinned_prompt_channel, const char *));
    MOCK_METHOD(int, cplat_pinned_prompt_status_enable,
                (cplat_pinned_prompt *, cplat_pinned_prompt_status_position, int));
    MOCK_METHOD(int, cplat_pinned_prompt_status_set,
                (cplat_pinned_prompt *, cplat_pinned_prompt_status_position, cplat_pinned_prompt_status_align,
                 const char *));

    // argparser
    MOCK_METHOD(cplat_argparser *, cplat_argparser_handle_create,
                (int, char *const *, const cplat_argparser_options *));
    MOCK_METHOD(void, cplat_argparser_handle_dispose, (cplat_argparser *));
    MOCK_METHOD(int, cplat_argparser_handle_register_flag,
                (cplat_argparser *, const char *, const char *, const char *, int *));
    MOCK_METHOD(int, cplat_argparser_handle_register_option_int,
                (cplat_argparser *, const char *, const char *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, cplat_argparser_handle_register_option_string,
                (cplat_argparser *, const char *, const char *, const char *, const char *, unsigned int,
                 const char **));
    MOCK_METHOD(int, cplat_argparser_handle_register_option_int_array,
                (cplat_argparser *, const char *, const char *, const char *, const char *, unsigned int, int *,
                 size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_handle_register_option_string_array,
                (cplat_argparser *, const char *, const char *, const char *, const char *, unsigned int,
                 const char **, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_handle_register_positional_int,
                (cplat_argparser *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, cplat_argparser_handle_register_positional_string,
                (cplat_argparser *, const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, cplat_argparser_handle_register_positional_int_array,
                (cplat_argparser *, const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_handle_register_positional_string_array,
                (cplat_argparser *, const char *, const char *, unsigned int, const char **, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_handle_parse, (cplat_argparser *));
    MOCK_METHOD(int, cplat_argparser_handle_get_error, (const cplat_argparser *));
    MOCK_METHOD(const char *, cplat_argparser_handle_get_error_target, (const cplat_argparser *));
    MOCK_METHOD(int, cplat_argparser_handle_get_error_index, (const cplat_argparser *));
    MOCK_METHOD(int, cplat_argparser_handle_get_error_message, (const cplat_argparser *, char *, size_t));
    MOCK_METHOD(int, cplat_argparser_handle_get_usage, (const cplat_argparser *, char *, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_handle_print_usage, (const cplat_argparser *, FILE *));
    MOCK_METHOD(int, cplat_argparser_handle_print_error_messages, (const cplat_argparser *, FILE *));
    MOCK_METHOD(int, cplat_argparser_handle_get_register_error, (const cplat_argparser *, size_t));
    MOCK_METHOD(size_t, cplat_argparser_handle_get_register_error_count, (const cplat_argparser *));
    MOCK_METHOD(const char *, cplat_argparser_handle_get_register_error_target, (const cplat_argparser *, size_t));
    MOCK_METHOD(int, cplat_argparser_handle_get_register_error_message,
                (const cplat_argparser *, size_t, char *, size_t));
    MOCK_METHOD(int, cplat_argparser_handle_print_register_error_messages, (const cplat_argparser *, FILE *));

    // argparser (省略可能な単一インスタンス API)
    MOCK_METHOD(void, cplat_argparser_init, (int, char *const *, const char *));
    MOCK_METHOD(int, cplat_argparser_register_flag, (const char *, const char *, const char *, int *));
    MOCK_METHOD(int, cplat_argparser_register_option_int,
                (const char *, const char *, const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, cplat_argparser_register_option_string,
                (const char *, const char *, const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, cplat_argparser_register_option_int_array,
                (const char *, const char *, const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_register_option_string_array,
                (const char *, const char *, const char *, const char *, unsigned int, const char **, size_t,
                 size_t *));
    MOCK_METHOD(int, cplat_argparser_register_positional_int,
                (const char *, const char *, unsigned int, int *));
    MOCK_METHOD(int, cplat_argparser_register_positional_string,
                (const char *, const char *, unsigned int, const char **));
    MOCK_METHOD(int, cplat_argparser_register_positional_int_array,
                (const char *, const char *, unsigned int, int *, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_register_positional_string_array,
                (const char *, const char *, unsigned int, const char **, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_parse, ());
    MOCK_METHOD(int, cplat_argparser_get_error, ());
    MOCK_METHOD(const char *, cplat_argparser_get_error_target, ());
    MOCK_METHOD(int, cplat_argparser_get_error_index, ());
    MOCK_METHOD(int, cplat_argparser_get_error_message, (char *, size_t));
    MOCK_METHOD(int, cplat_argparser_get_usage, (char *, size_t, size_t *));
    MOCK_METHOD(int, cplat_argparser_print_usage, (FILE *));
    MOCK_METHOD(int, cplat_argparser_print_error_messages, (FILE *));
    MOCK_METHOD(int, cplat_argparser_get_register_error, (size_t));
    MOCK_METHOD(size_t, cplat_argparser_get_register_error_count, ());
    MOCK_METHOD(const char *, cplat_argparser_get_register_error_target, (size_t));
    MOCK_METHOD(int, cplat_argparser_get_register_error_message, (size_t, char *, size_t));
    MOCK_METHOD(int, cplat_argparser_print_register_error_messages, (FILE *));

    Mock_cplat();
    ~Mock_cplat();
};

extern Mock_cplat *_mock_cplat;

#endif /* MOCK_CPLAT_H */
