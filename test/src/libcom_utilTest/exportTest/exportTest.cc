#include <testfw.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <com_util/base/error.h>
#include <com_util/base/platform.h>
#include <com_util/argparser/argparser.h>
#include <com_util/clock/clock.h>
#include <com_util/clock/timespec.h>
#include <com_util/compress/compress.h>
#include <com_util/console/console.h>
#include <com_util/crt/fcntl.h>
#include <com_util/crt/file.h>
#include <com_util/crt/path.h>
#include <com_util/crt/stdio.h>
#include <com_util/crt/stdlib.h>
#include <com_util/crt/string.h>
#include <com_util/crt/sys/stat.h>
#include <com_util/crt/time.h>
#include <com_util/crt/unistd.h>
#include <com_util/base/error_message.h>
#include <com_util/crypto/crypto.h>
#include <com_util/crypto/random.h>
#include <com_util/mmap/mmap.h>
#include <com_util/prompt/pinned_prompt.h>
#include <com_util/prompt/prompt.h>
#include <com_util/runtime/elevated_process.h>
#include <com_util/runtime/memory_lock.h>
#include <com_util/regex/regex.h>

#include <com_util/runtime/module.h>
#include <com_util/runtime/process.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/runtime/sym_loader.h>
#include <com_util/sync/sync.h>
#include <com_util/trace/trace_file.h>
#include <com_util/trace/tracer.h>

#if defined(PLATFORM_WINDOWS)
    #include <com_util/crt/wchar_conv.h>
    #include <com_util/trace/etw.h>
    #include <com_util/trace/eventlog.h>
    #include <com_util/win32/win32.h>
#elif defined(PLATFORM_LINUX)
    #include <com_util/trace/syslog.h>
#endif /* PLATFORM_ */

// libcom_util が公開エクスポートすべき関数の一覧。
// 関数の追加・削除は、このテーブルのみを編集する。
// 名前一致チェックとシグネチャの static_assert は、いずれも本テーブルから生成する。
#define COM_UTIL_EXPORT_TABLE_COMMON(EXPORT_ENTRY) \
    /* com_util/argparser/argparser.h */ \
    EXPORT_ENTRY(_com_util_argparser_create, \
                 com_util_argparser *(COM_UTIL_API *)(const com_util_argparser_options *options)) \
    EXPORT_ENTRY(_com_util_argparser_default, \
                 com_util_argparser *(COM_UTIL_API *)(const com_util_argparser_options *options)) \
    EXPORT_ENTRY(com_util_argparser_init, void(COM_UTIL_API *)(const char *description)) \
    EXPORT_ENTRY(_com_util_argparser_dispose, void(COM_UTIL_API *)(com_util_argparser * parser)) \
    EXPORT_ENTRY(_com_util_argparser_register_flag, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *short_name, const char *long_name, \
                                     const char *description, int *storage)) \
    EXPORT_ENTRY(com_util_argparser_register_flag, int(COM_UTIL_API *)(const char *short_name, const char *long_name, \
                                                                       const char *description, int *storage)) \
    EXPORT_ENTRY(_com_util_argparser_register_option_int, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *short_name, const char *long_name, \
                                     const char *value_name, const char *description, unsigned int flags, \
                                     int *storage)) \
    EXPORT_ENTRY(com_util_argparser_register_option_int, \
                 int(COM_UTIL_API *)(const char *short_name, const char *long_name, const char *value_name, \
                                     const char *description, unsigned int flags, int *storage)) \
    EXPORT_ENTRY(_com_util_argparser_register_option_string, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *short_name, const char *long_name, \
                                     const char *value_name, const char *description, unsigned int flags, \
                                     const char **storage)) \
    EXPORT_ENTRY(com_util_argparser_register_option_string, \
                 int(COM_UTIL_API *)(const char *short_name, const char *long_name, const char *value_name, \
                                     const char *description, unsigned int flags, const char **storage)) \
    EXPORT_ENTRY(_com_util_argparser_register_option_int_array, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *short_name, const char *long_name, \
                                     const char *value_name, const char *description, unsigned int flags, \
                                     int *storage, size_t capacity, size_t *count)) \
    EXPORT_ENTRY(com_util_argparser_register_option_int_array, \
                 int(COM_UTIL_API *)(const char *short_name, const char *long_name, const char *value_name, \
                                     const char *description, unsigned int flags, int *storage, size_t capacity, \
                                     size_t *count)) \
    EXPORT_ENTRY(_com_util_argparser_register_option_string_array, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *short_name, const char *long_name, \
                                     const char *value_name, const char *description, unsigned int flags, \
                                     const char **storage, size_t capacity, size_t *count)) \
    EXPORT_ENTRY(com_util_argparser_register_option_string_array, \
                 int(COM_UTIL_API *)(const char *short_name, const char *long_name, const char *value_name, \
                                     const char *description, unsigned int flags, const char **storage, \
                                     size_t capacity, size_t *count)) \
    EXPORT_ENTRY(_com_util_argparser_register_positional_int, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *name, const char *description, \
                                     unsigned int flags, int *storage)) \
    EXPORT_ENTRY(com_util_argparser_register_positional_int, \
                 int(COM_UTIL_API *)(const char *name, const char *description, unsigned int flags, int *storage)) \
    EXPORT_ENTRY(_com_util_argparser_register_positional_string, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *name, const char *description, \
                                     unsigned int flags, const char **storage)) \
    EXPORT_ENTRY( \
        com_util_argparser_register_positional_string, \
        int(COM_UTIL_API *)(const char *name, const char *description, unsigned int flags, const char **storage)) \
    EXPORT_ENTRY(_com_util_argparser_register_positional_int_array, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *name, const char *description, \
                                     unsigned int flags, int *storage, size_t capacity, size_t *count)) \
    EXPORT_ENTRY(com_util_argparser_register_positional_int_array, \
                 int(COM_UTIL_API *)(const char *name, const char *description, unsigned int flags, int *storage, \
                                     size_t capacity, size_t *count)) \
    EXPORT_ENTRY(_com_util_argparser_register_positional_string_array, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, const char *name, const char *description, \
                                     unsigned int flags, const char **storage, size_t capacity, size_t *count)) \
    EXPORT_ENTRY(com_util_argparser_register_positional_string_array, \
                 int(COM_UTIL_API *)(const char *name, const char *description, unsigned int flags, \
                                     const char **storage, size_t capacity, size_t *count)) \
    EXPORT_ENTRY(_com_util_argparser_parse, \
                 int(COM_UTIL_API *)(com_util_argparser * parser, int argc, char *const *argv)) \
    EXPORT_ENTRY(com_util_argparser_parse, int(COM_UTIL_API *)(int argc, char *const *argv)) \
    EXPORT_ENTRY(_com_util_argparser_get_error, int(COM_UTIL_API *)(const com_util_argparser *parser)) \
    EXPORT_ENTRY(com_util_argparser_get_error, int(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(_com_util_argparser_get_error_target, const char *(COM_UTIL_API *)(const com_util_argparser *parser)) \
    EXPORT_ENTRY(com_util_argparser_get_error_target, const char *(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(_com_util_argparser_get_error_index, int(COM_UTIL_API *)(const com_util_argparser *parser)) \
    EXPORT_ENTRY(com_util_argparser_get_error_index, int(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(_com_util_argparser_get_error_message, \
                 int(COM_UTIL_API *)(const com_util_argparser *parser, char *buffer, size_t buffer_size)) \
    EXPORT_ENTRY(com_util_argparser_get_error_message, int(COM_UTIL_API *)(char *buffer, size_t buffer_size)) \
    EXPORT_ENTRY(_com_util_argparser_get_usage, int(COM_UTIL_API *)(const com_util_argparser *parser, char *buffer, \
                                                                    size_t buffer_size, size_t *required_size)) \
    EXPORT_ENTRY(com_util_argparser_get_usage, \
                 int(COM_UTIL_API *)(char *buffer, size_t buffer_size, size_t *required_size)) \
    EXPORT_ENTRY(_com_util_argparser_print_usage, int(COM_UTIL_API *)(const com_util_argparser *parser, FILE *stream)) \
    EXPORT_ENTRY(com_util_argparser_print_usage, int(COM_UTIL_API *)(FILE * stream)) \
    EXPORT_ENTRY(_com_util_argparser_print_error_messages, \
                 int(COM_UTIL_API *)(const com_util_argparser *parser, FILE *stream)) \
    EXPORT_ENTRY(com_util_argparser_print_error_messages, int(COM_UTIL_API *)(FILE * stream)) \
    EXPORT_ENTRY(_com_util_argparser_get_register_error_count, \
                 size_t(COM_UTIL_API *)(const com_util_argparser *parser)) \
    EXPORT_ENTRY(com_util_argparser_get_register_error_count, size_t(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(_com_util_argparser_get_register_error, \
                 int(COM_UTIL_API *)(const com_util_argparser *parser, size_t index)) \
    EXPORT_ENTRY(com_util_argparser_get_register_error, int(COM_UTIL_API *)(size_t index)) \
    EXPORT_ENTRY(_com_util_argparser_get_register_error_target, \
                 const char *(COM_UTIL_API *)(const com_util_argparser *parser, size_t index)) \
    EXPORT_ENTRY(com_util_argparser_get_register_error_target, const char *(COM_UTIL_API *)(size_t index)) \
    EXPORT_ENTRY( \
        _com_util_argparser_get_register_error_message, \
        int(COM_UTIL_API *)(const com_util_argparser *parser, size_t index, char *buffer, size_t buffer_size)) \
    EXPORT_ENTRY(com_util_argparser_get_register_error_message, \
                 int(COM_UTIL_API *)(size_t index, char *buffer, size_t buffer_size)) \
    EXPORT_ENTRY(_com_util_argparser_print_register_error_messages, \
                 int(COM_UTIL_API *)(const com_util_argparser *parser, FILE *stream)) \
    EXPORT_ENTRY(com_util_argparser_print_register_error_messages, int(COM_UTIL_API *)(FILE * stream)) \
    /* com_util/clock/clock.h */ \
    EXPORT_ENTRY(com_util_get_monotonic_ms, uint64_t(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(com_util_get_monotonic, void(COM_UTIL_API *)(com_util_timespec * ts)) \
    EXPORT_ENTRY(com_util_get_realtime, void(COM_UTIL_API *)(com_util_timespec * ts)) \
    EXPORT_ENTRY(com_util_format_realtime_iso8601_local, \
                 int(COM_UTIL_API *)(char *buf, size_t buf_size, const com_util_timespec *timestamp)) \
    EXPORT_ENTRY(com_util_format_realtime_iso8601_utc, \
                 int(COM_UTIL_API *)(char *buf, size_t buf_size, const com_util_timespec *timestamp)) \
    EXPORT_ENTRY(com_util_get_realtime_utc, void(COM_UTIL_API *)(struct tm * utc_tm, int32_t *tv_nsec)) \
    EXPORT_ENTRY(com_util_get_realtime_deadline_ms, \
                 void(COM_UTIL_API *)(uint64_t timeout_ms, struct timespec *abs_timeout)) \
    /* com_util/clock/timespec.h */ \
    EXPORT_ENTRY(com_util_timespec_normalize, void(COM_UTIL_API *)(com_util_timespec * ts)) \
    EXPORT_ENTRY(com_util_timespec_add, void(COM_UTIL_API *)(const com_util_timespec *a, const com_util_timespec *b, \
                                                             com_util_timespec *result)) \
    EXPORT_ENTRY(com_util_timespec_sub, void(COM_UTIL_API *)(const com_util_timespec *a, const com_util_timespec *b, \
                                                             com_util_timespec *result)) \
    EXPORT_ENTRY(com_util_timespec_cmp, int(COM_UTIL_API *)(const com_util_timespec *a, const com_util_timespec *b)) \
    EXPORT_ENTRY(com_util_timespec_add_ms, \
                 void(COM_UTIL_API *)(const com_util_timespec *ts, uint64_t timeout_ms, com_util_timespec *result)) \
    EXPORT_ENTRY(com_util_timespec_diff_ms, \
                 int64_t(COM_UTIL_API *)(const com_util_timespec *end, const com_util_timespec *start)) \
    EXPORT_ENTRY(com_util_timespec_to_native, \
                 void(COM_UTIL_API *)(const com_util_timespec *ts, struct timespec *native)) \
    EXPORT_ENTRY(com_util_timespec_from_native, \
                 void(COM_UTIL_API *)(const struct timespec *native, com_util_timespec *ts)) \
    /* com_util/compress/compress.h */ \
    EXPORT_ENTRY(com_util_compress, \
                 int(COM_UTIL_API *)(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)) \
    EXPORT_ENTRY(com_util_decompress, \
                 int(COM_UTIL_API *)(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)) \
    /* com_util/console/console.h */ \
    EXPORT_ENTRY(com_util_console_init, void(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(com_util_console_dispose, void(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(com_util_console_attach_parent, int(COM_UTIL_API *)(int *argc, char **argv, int *attached_out)) \
    EXPORT_ENTRY(com_util_console_write, int(COM_UTIL_API *)(com_util_stream stream, const char *text)) \
    /* com_util/crt/fcntl.h */ \
    EXPORT_ENTRY(com_util_open, \
                 int(COM_UTIL_API *)(const char *path, int flags, int mode, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_open_fmt, \
                 int(COM_UTIL_API *)(int flags, int mode, com_util_error *detail_out, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vopen_fmt, int(COM_UTIL_API *)(int flags, int mode, com_util_error *detail_out, \
                                                         const char *format, va_list args)) \
    /* com_util/crt/file.h */ \
    EXPORT_ENTRY(com_util_file_init, void(COM_UTIL_API *)(com_util_file * file)) \
    EXPORT_ENTRY(com_util_file_open, \
                 int(COM_UTIL_API *)(com_util_file * file, const char *path, int flags, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_write, \
                 int(COM_UTIL_API *)(com_util_file * file, const void *buf, size_t len, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_read, int(COM_UTIL_API *)(com_util_file * file, void *buf, size_t len, \
                                                         size_t *read_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_get_size, \
                 int(COM_UTIL_API *)(const com_util_file *file, size_t *size_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_set_size, \
                 int(COM_UTIL_API *)(com_util_file * file, size_t size, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_get_id, \
                 int(COM_UTIL_API *)(const com_util_file *file, com_util_file_id *id_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_get_path_id, \
                 int(COM_UTIL_API *)(const char *path, com_util_file_id *id_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_file_flush, int(COM_UTIL_API *)(com_util_file * file, com_util_error * detail_out)) \
    EXPORT_ENTRY(com_util_file_close, int(COM_UTIL_API *)(com_util_file * file, com_util_error * detail_out)) \
    /* com_util/crt/path.h */ \
    EXPORT_ENTRY(com_util_normalize_path_sep, char *(COM_UTIL_API *)(char *path)) \
    EXPORT_ENTRY(com_util_path_get_full, \
                 int(COM_UTIL_API *)(char *path_out, size_t path_size, com_util_error *detail_out, const char *path)) \
    EXPORT_ENTRY(com_util_paths_equal, \
                 int(COM_UTIL_API *)(const char *lhs, const char *rhs, int *equal_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_get_temp_dir, \
                 int(COM_UTIL_API *)(char *path_out, size_t path_size, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_path_concat_n, int(COM_UTIL_API *)(char *path_out, size_t path_size, \
                                                             com_util_error *detail_out, size_t part_count, ...)) \
    EXPORT_ENTRY(com_util_path_basename, const char *(COM_UTIL_API *)(const char *path)) \
    EXPORT_ENTRY(com_util_path_dirname, \
                 int(COM_UTIL_API *)(char *path_out, size_t path_size, com_util_error *detail_out, const char *path)) \
    EXPORT_ENTRY(com_util_path_extension, const char *(COM_UTIL_API *)(const char *path)) \
    EXPORT_ENTRY(com_util_path_strip_extension, \
                 int(COM_UTIL_API *)(char *path_out, size_t path_size, com_util_error *detail_out, const char *path)) \
    EXPORT_ENTRY(com_util_path_join_n, int(COM_UTIL_API *)(char *path_out, size_t path_size, \
                                                           com_util_error *detail_out, size_t part_count, ...)) \
    /* com_util/crt/stdio.h */ \
    EXPORT_ENTRY(com_util_fopen, \
                 FILE *(COM_UTIL_API *)(const char *path, const char *modes, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_freopen, FILE *(COM_UTIL_API *)(const char *path, const char *modes, FILE *stream, \
                                                          com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_fclose, int(COM_UTIL_API *)(FILE * stream, com_util_error * detail_out)) \
    EXPORT_ENTRY(com_util_fflush, int(COM_UTIL_API *)(FILE * stream, com_util_error * detail_out)) \
    EXPORT_ENTRY(com_util_fread, size_t(COM_UTIL_API *)(void *buffer, size_t size, size_t count, FILE *stream, \
                                                        com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_fwrite, size_t(COM_UTIL_API *)(const void *buffer, size_t size, size_t count, FILE *stream, \
                                                         com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_remove, int(COM_UTIL_API *)(const char *path, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_rename, \
                 int(COM_UTIL_API *)(const char *oldpath, const char *newpath, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_scanf, int(COM_UTIL_API *)(const char *format, ...)) \
    EXPORT_ENTRY(com_util_vscanf, int(COM_UTIL_API *)(const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_fscanf, int(COM_UTIL_API *)(FILE * stream, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vfscanf, int(COM_UTIL_API *)(FILE * stream, const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_snprintf, int(COM_UTIL_API *)(char *dest, size_t dest_size, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vsnprintf, \
                 int(COM_UTIL_API *)(char *dest, size_t dest_size, const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_fgets, \
                 int(COM_UTIL_API *)(char *dest, size_t dest_size, FILE *stream, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_fprintf, int(COM_UTIL_API *)(FILE * stream, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vfprintf, int(COM_UTIL_API *)(FILE * stream, const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_fseek, int(COM_UTIL_API *)(FILE * stream, int64_t offset, int whence)) \
    EXPORT_ENTRY(com_util_ftell, int64_t(COM_UTIL_API *)(FILE * stream)) \
    EXPORT_ENTRY(com_util_fopen_fmt, \
                 FILE *(COM_UTIL_API *)(const char *modes, com_util_error *detail_out, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vfopen_fmt, FILE *(COM_UTIL_API *)(const char *modes, com_util_error *detail_out, \
                                                             const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_remove_fmt, int(COM_UTIL_API *)(com_util_error * detail_out, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vremove_fmt, \
                 int(COM_UTIL_API *)(com_util_error * detail_out, const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_fopen_temp, FILE *(COM_UTIL_API *)(const char *prefix, const char *modes, char *path_out, \
                                                             size_t path_size, com_util_error *detail_out)) \
    /* com_util/crt/stdlib.h */ \
    EXPORT_ENTRY(com_util_getenv, int(COM_UTIL_API *)(const char *name, char *buf, size_t buf_size, int *exists_out, \
                                                      com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_setenv, \
                 int(COM_UTIL_API *)(const char *name, const char *value, int overwrite, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_unsetenv, int(COM_UTIL_API *)(const char *name, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_parse_int64, int(COM_UTIL_API *)(int64_t *value_out, const char *text, int base)) \
    EXPORT_ENTRY(com_util_parse_uint64, int(COM_UTIL_API *)(uint64_t *value_out, const char *text, int base)) \
    EXPORT_ENTRY(com_util_parse_int, int(COM_UTIL_API *)(int *value_out, const char *text, int base)) \
    EXPORT_ENTRY(com_util_parse_double, int(COM_UTIL_API *)(double *value_out, const char *text)) \
    EXPORT_ENTRY(com_util_malloc, void *(COM_UTIL_API *)(size_t size)) \
    EXPORT_ENTRY(com_util_malloc_zerofill, void *(COM_UTIL_API *)(size_t size)) \
    EXPORT_ENTRY(com_util_calloc, void *(COM_UTIL_API *)(size_t count, size_t size)) \
    EXPORT_ENTRY(com_util_realloc, void *(COM_UTIL_API *)(void *ptr, size_t count, size_t size)) \
    EXPORT_ENTRY(com_util_realloc_zerofill, \
                 void *(COM_UTIL_API *)(void *ptr, size_t old_count, size_t count, size_t size)) \
    EXPORT_ENTRY(com_util_free, void(COM_UTIL_API *)(void *ptr)) \
    /* com_util/crt/string.h */ \
    EXPORT_ENTRY(com_util_strcpy, int(COM_UTIL_API *)(char *dest, size_t dest_size, const char *src)) \
    EXPORT_ENTRY(com_util_strncpy, int(COM_UTIL_API *)(char *dest, size_t dest_size, const char *src, size_t count)) \
    EXPORT_ENTRY(com_util_strcat, int(COM_UTIL_API *)(char *dest, size_t dest_size, const char *src)) \
    EXPORT_ENTRY(com_util_strncat, int(COM_UTIL_API *)(char *dest, size_t dest_size, const char *src, size_t count)) \
    EXPORT_ENTRY(com_util_strtok_r, char *(COM_UTIL_API *)(char *str, const char *delim, char **saveptr)) \
    EXPORT_ENTRY(com_util_strdup, char *(COM_UTIL_API *)(const char *src)) \
    EXPORT_ENTRY(com_util_wcscpy, int(COM_UTIL_API *)(wchar_t * dest, size_t dest_size, const wchar_t *src)) \
    EXPORT_ENTRY(com_util_sscanf, int(COM_UTIL_API *)(const char *buffer, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vsscanf, int(COM_UTIL_API *)(const char *buffer, const char *format, va_list args)) \
    /* com_util/crt/sys/stat.h */ \
    EXPORT_ENTRY(com_util_stat, \
                 int(COM_UTIL_API *)(com_util_file_stat_t * buf, com_util_error * detail_out, const char *path)) \
    EXPORT_ENTRY(com_util_mkdir, int(COM_UTIL_API *)(const char *path, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_makedirs, int(COM_UTIL_API *)(const char *path, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_rmdir, int(COM_UTIL_API *)(const char *path, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_stat_fmt, int(COM_UTIL_API *)(com_util_file_stat_t * buf, com_util_error * detail_out, \
                                                        const char *format, ...)) \
    EXPORT_ENTRY(com_util_vstat_fmt, int(COM_UTIL_API *)(com_util_file_stat_t * buf, com_util_error * detail_out, \
                                                         const char *format, va_list args)) \
    EXPORT_ENTRY(com_util_mkdir_fmt, int(COM_UTIL_API *)(com_util_error * detail_out, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vmkdir_fmt, \
                 int(COM_UTIL_API *)(com_util_error * detail_out, const char *format, va_list args)) \
    /* com_util/crt/time.h */ \
    EXPORT_ENTRY(com_util_gmtime, int(COM_UTIL_API *)(struct tm * utc_tm, const time_t *timep)) \
    EXPORT_ENTRY(com_util_localtime, int(COM_UTIL_API *)(struct tm * local_tm, const time_t *timep)) \
    EXPORT_ENTRY(com_util_ctime, int(COM_UTIL_API *)(char *buf, size_t buf_size, const time_t *timep)) \
    /* com_util/crt/unistd.h */ \
    EXPORT_ENTRY(com_util_isatty, int(COM_UTIL_API *)(com_util_stream stream)) \
    EXPORT_ENTRY(com_util_lseek, \
                 int64_t(COM_UTIL_API *)(int fd, int64_t offset, int whence, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_close, int(COM_UTIL_API *)(int fd, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_dup, int(COM_UTIL_API *)(int fd, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_dup2, int(COM_UTIL_API *)(int oldfd, int newfd, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_read, int64_t(COM_UTIL_API *)(int fd, void *buf, size_t count, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_write, \
                 int64_t(COM_UTIL_API *)(int fd, const void *buf, size_t count, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_access, int(COM_UTIL_API *)(const char *path, int mode, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_access_fmt, \
                 int(COM_UTIL_API *)(int mode, com_util_error *detail_out, const char *format, ...)) \
    EXPORT_ENTRY(com_util_vaccess_fmt, \
                 int(COM_UTIL_API *)(int mode, com_util_error *detail_out, const char *format, va_list args)) \
    /* com_util/crypto/crypto.h */ \
    EXPORT_ENTRY(com_util_encrypt, \
                 int(COM_UTIL_API *)(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, \
                                     const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len)) \
    EXPORT_ENTRY(com_util_decrypt, \
                 int(COM_UTIL_API *)(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, \
                                     const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len)) \
    EXPORT_ENTRY(com_util_random_bytes, int(COM_UTIL_API *)(void *buf, size_t size)) \
    EXPORT_ENTRY(com_util_secure_zero, void(COM_UTIL_API *)(void *buf, size_t size)) \
    EXPORT_ENTRY(com_util_passphrase_to_key, \
                 int(COM_UTIL_API *)(uint8_t *key, const uint8_t *passphrase, size_t passphrase_len)) \
    /* com_util/base/error.h */ \
    EXPORT_ENTRY(com_util_error_clear, void(COM_UTIL_API *)(com_util_error * error)) \
    EXPORT_ENTRY(com_util_error_capture_errno, void(COM_UTIL_API *)(com_util_error * error, int errno_value)) \
    EXPORT_ENTRY(com_util_error_capture_current_errno, void(COM_UTIL_API *)(com_util_error * error)) \
    EXPORT_ENTRY(com_util_error_get_last, void(COM_UTIL_API *)(com_util_error * error_out)) \
    EXPORT_ENTRY(com_util_error_set_last, void(COM_UTIL_API *)(const com_util_error *error)) \
    EXPORT_ENTRY(com_util_error_clear_last, void(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(com_util_error_is_set, int(COM_UTIL_API *)(const com_util_error *error)) \
    EXPORT_ENTRY(com_util_error_get_domain, com_util_error_domain(COM_UTIL_API *)(const com_util_error *error)) \
    EXPORT_ENTRY(com_util_error_get_errno, int(COM_UTIL_API *)(const com_util_error *error)) \
    EXPORT_ENTRY(com_util_error_to_result, int(COM_UTIL_API *)(const com_util_error *error)) \
    EXPORT_ENTRY(com_util_error_get_cause, com_util_error_cause(COM_UTIL_API *)(const com_util_error *error)) \
    EXPORT_ENTRY(com_util_error_is, int(COM_UTIL_API *)(const com_util_error *error, com_util_error_cause cause)) \
    /* com_util/base/error_message.h */ \
    EXPORT_ENTRY(com_util_result_to_string, const char *(COM_UTIL_API *)(int result)) \
    EXPORT_ENTRY(com_util_error_message, int(COM_UTIL_API *)(char *buf, size_t buf_size, const com_util_error *error)) \
    /* com_util/crypto/random.h */ \
    EXPORT_ENTRY(com_util_random_bytes, int(COM_UTIL_API *)(void *buf, size_t size)) \
    /* com_util/mmap/mmap.h */ \
    EXPORT_ENTRY(com_util_mmap_attach, \
                 int(COM_UTIL_API *)(const char *path, com_util_mmap_access access, size_t create_size, \
                                     com_util_mmap **map, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_mmap_get_address, void *(COM_UTIL_API *)(const com_util_mmap *map)) \
    EXPORT_ENTRY(com_util_mmap_get_size, size_t(COM_UTIL_API *)(const com_util_mmap *map)) \
    EXPORT_ENTRY(com_util_mmap_get_rwlock, \
                 int(COM_UTIL_API *)(const com_util_mmap *map, com_util_interprocess_rwlock **lock_out, \
                                     com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_mmap_flush, \
                 int(COM_UTIL_API *)(com_util_mmap * map, void *address, size_t length, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_mmap_detach, int(COM_UTIL_API *)(com_util_mmap * map, com_util_error * detail_out)) \
    /* com_util/prompt/pinned_prompt.h */ \
    EXPORT_ENTRY(com_util_pinned_prompt_create, \
                 com_util_pinned_prompt *(COM_UTIL_API *)(const com_util_pinned_prompt_options *options)) \
    EXPORT_ENTRY(com_util_pinned_prompt_dispose, void(COM_UTIL_API *)(com_util_pinned_prompt * screen)) \
    EXPORT_ENTRY(_com_util_pinned_prompt_readline, \
                 int(COM_UTIL_API *)(com_util_pinned_prompt * screen, char *buf, size_t buf_size, \
                                     const char *prompt_str, const char *file, int line)) \
    EXPORT_ENTRY(_com_util_pinned_prompt_readline_fmt, \
                 int(COM_UTIL_API *)(com_util_pinned_prompt * screen, char *buf, size_t buf_size, const char *file, \
                                     int line, const char *fmt, ...)) \
    EXPORT_ENTRY(com_util_pinned_prompt_write, \
                 int(COM_UTIL_API *)(com_util_pinned_prompt * screen, com_util_pinned_prompt_channel channel, \
                                     const void *data, size_t size, size_t *written_out)) \
    EXPORT_ENTRY(com_util_pinned_prompt_printf, \
                 int(COM_UTIL_API *)(com_util_pinned_prompt * screen, com_util_pinned_prompt_channel channel, \
                                     const char *fmt, ...)) \
    EXPORT_ENTRY(com_util_pinned_prompt_status_enable, \
                 int(COM_UTIL_API *)(com_util_pinned_prompt * screen, com_util_pinned_prompt_status_position position, \
                                     int enable)) \
    EXPORT_ENTRY(com_util_pinned_prompt_status_set, \
                 int(COM_UTIL_API *)(com_util_pinned_prompt * screen, com_util_pinned_prompt_status_position position, \
                                     com_util_pinned_prompt_status_align align, const char *content)) \
    /* com_util/prompt/prompt.h */ \
    EXPORT_ENTRY(com_util_prompt_create, com_util_prompt *(COM_UTIL_API *)(const com_util_prompt_options *options)) \
    EXPORT_ENTRY(com_util_prompt_dispose, void(COM_UTIL_API *)(com_util_prompt * prompt)) \
    EXPORT_ENTRY(com_util_prompt_readline_at, \
                 int(COM_UTIL_API *)(com_util_prompt * prompt, char *buf, size_t buf_size, const char *prompt_str, \
                                     const char *file, int line)) \
    EXPORT_ENTRY(com_util_prompt_readline_fmt_at, \
                 int(COM_UTIL_API *)(com_util_prompt * p, char *buf, size_t buf_size, const char *file, int line, \
                                     const char *fmt, ...)) \
    /* com_util/runtime/elevated_process.h */ \
    EXPORT_ENTRY(com_util_elevated_process_is_elevated, int(COM_UTIL_API *)(int *elevated)) \
    EXPORT_ENTRY(com_util_elevated_process_run_if_needed, \
                 int(COM_UTIL_API *)(const char *arguments, int *exit_code, int *handled)) \
    EXPORT_ENTRY(com_util_elevated_process_run_with_result, \
                 int(COM_UTIL_API *)(const char *arguments, int *exit_code, int *handled, char *result_message, \
                                     size_t result_message_size)) \
    EXPORT_ENTRY(com_util_elevated_process_extract_result_target, \
                 int(COM_UTIL_API *)(int *argc, char **argv, int *detected_out)) \
    EXPORT_ENTRY(com_util_elevated_process_report_result, int(COM_UTIL_API *)(const char *message)) \
    /* com_util/regex/regex.h */ \
    EXPORT_ENTRY(com_util_regex_create, int(COM_UTIL_API *)(const char *pattern, unsigned int flags, \
                                                            com_util_regex **regex_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_regex_dispose, void(COM_UTIL_API *)(com_util_regex * regex)) \
    EXPORT_ENTRY(com_util_regex_get_group_count, size_t(COM_UTIL_API *)(const com_util_regex *regex)) \
    EXPORT_ENTRY(com_util_regex_search, \
                 int(COM_UTIL_API *)(const com_util_regex *regex, const char *text, size_t text_len, \
                                     size_t start_offset, unsigned int match_flags, com_util_regex_match *matches_out, \
                                     size_t matches_capacity, int *matched_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_regex_matches, \
                 int(COM_UTIL_API *)(const com_util_regex *regex, const char *text, size_t text_len, \
                                     unsigned int match_flags, com_util_regex_match *matches_out, \
                                     size_t matches_capacity, int *matched_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_regex_replace, \
                 int(COM_UTIL_API *)(const com_util_regex *regex, const char *text, size_t text_len, \
                                     const char *replacement, unsigned int flags, char *result_out, \
                                     size_t result_size, size_t *required_size_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_regex_iter_create, \
                 int(COM_UTIL_API *)(const com_util_regex *regex, const char *text, size_t text_len, \
                                     unsigned int match_flags, com_util_regex_iter **iter_out, \
                                     com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_regex_iter_next, \
                 int(COM_UTIL_API *)(com_util_regex_iter * iter, com_util_regex_match * matches_out, \
                                     size_t matches_capacity, int *has_match_out, com_util_error *detail_out)) \
    EXPORT_ENTRY(com_util_regex_iter_dispose, void(COM_UTIL_API *)(com_util_regex_iter * iter)) \
    EXPORT_ENTRY(com_util_regex_split, \
                 int(COM_UTIL_API *)(const com_util_regex *regex, const char *text, size_t text_len, size_t max_parts, \
                                     unsigned int match_flags, com_util_regex_match *parts_out, size_t parts_capacity, \
                                     size_t *part_count_out, com_util_error *detail_out)) \
    /* com_util/runtime/memory_lock.h */ \
    EXPORT_ENTRY(com_util_memory_lock_range, int(COM_UTIL_API *)(const void *address, size_t size)) \
    EXPORT_ENTRY(com_util_memory_unlock_range, int(COM_UTIL_API *)(const void *address, size_t size)) \
    EXPORT_ENTRY(com_util_memory_lock_self, int(COM_UTIL_API *)(const com_util_memory_lock_self_options *options, \
                                                                com_util_memory_lock_scope **scope)) \
    EXPORT_ENTRY(com_util_memory_lock_scope_release, int(COM_UTIL_API *)(com_util_memory_lock_scope * scope)) \
    EXPORT_ENTRY(com_util_secure_zero, void(COM_UTIL_API *)(void *buf, size_t size)) \
    /* com_util/runtime/module.h */ \
    EXPORT_ENTRY(com_util_module_get_path, \
                 int(COM_UTIL_API *)(char *out_path, size_t out_path_sz, const void *func_addr)) \
    EXPORT_ENTRY(com_util_module_get_basename, \
                 int(COM_UTIL_API *)(char *out_basename, size_t out_basename_sz, const void *func_addr)) \
    /* com_util/runtime/process.h */ \
    EXPORT_ENTRY(com_util_process_get_executable_path, int(COM_UTIL_API *)(char *out_path, size_t out_path_sz)) \
    EXPORT_ENTRY(com_util_process_start, \
                 int(COM_UTIL_API *)(const com_util_process_options *options, com_util_process **process)) \
    EXPORT_ENTRY(com_util_process_wait, int(COM_UTIL_API *)(com_util_process * process, int timeout_ms)) \
    EXPORT_ENTRY(com_util_process_get_exit_code, int(COM_UTIL_API *)(com_util_process * process, int *exit_code)) \
    EXPORT_ENTRY(com_util_process_terminate, int(COM_UTIL_API *)(com_util_process * process)) \
    EXPORT_ENTRY(com_util_process_destroy, void(COM_UTIL_API *)(com_util_process * process)) \
    EXPORT_ENTRY(com_util_process_run_sync, \
                 int(COM_UTIL_API *)(const com_util_process_options *options, int timeout_ms, int *exit_code)) \
    /* com_util/runtime/shutdown.h */ \
    EXPORT_ENTRY(com_util_shutdown_register, int(COM_UTIL_API *)(com_util_shutdown_fn callback, void *context)) \
    EXPORT_ENTRY(com_util_shutdown_request_register, \
                 int(COM_UTIL_API *)(com_util_shutdown_fn callback, void *context)) \
    EXPORT_ENTRY(com_util_exit, void(COM_UTIL_API *)(int code)) \
    EXPORT_ENTRY(_com_util_shutdown_invoke_for_test, \
                 int(COM_UTIL_API *)(const com_util_shutdown_event *event, int *invoked_out)) \
    EXPORT_ENTRY(_com_util_shutdown_request_invoke_for_test, \
                 int(COM_UTIL_API *)(const com_util_shutdown_event *event, int *invoked_out)) \
    EXPORT_ENTRY(_com_util_shutdown_reset_for_test, void(COM_UTIL_API *)(void)) \
    /* com_util/runtime/sym_loader.h */ \
    EXPORT_ENTRY(com_util_sym_loader_resolve, void *(COM_UTIL_API *)(com_util_sym_loader_entry * fobj)) \
    EXPORT_ENTRY(com_util_sym_loader_is_default, int(COM_UTIL_API *)(com_util_sym_loader_entry * fobj)) \
    EXPORT_ENTRY(com_util_sym_loader_init, void(COM_UTIL_API *)(com_util_sym_loader_entry *const *fobj_array, \
                                                                size_t fobj_length, const char *configpath)) \
    EXPORT_ENTRY(com_util_sym_loader_dispose, \
                 void(COM_UTIL_API *)(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length)) \
    EXPORT_ENTRY(com_util_sym_loader_info, \
                 int(COM_UTIL_API *)(com_util_sym_loader_entry *const *fobj_array, size_t fobj_length)) \
    /* com_util/sync/sync.h */ \
    EXPORT_ENTRY(com_util_local_lock_create, int(COM_UTIL_API *)(com_util_local_lock * *mtx)) \
    EXPORT_ENTRY(com_util_local_lock_lock, int(COM_UTIL_API *)(com_util_local_lock * mtx, int timeout_ms)) \
    EXPORT_ENTRY(com_util_local_lock_try_lock, int(COM_UTIL_API *)(com_util_local_lock * mtx)) \
    EXPORT_ENTRY(com_util_local_lock_unlock, int(COM_UTIL_API *)(com_util_local_lock * mtx)) \
    EXPORT_ENTRY(com_util_local_lock_destroy, void(COM_UTIL_API *)(com_util_local_lock * mtx)) \
    EXPORT_ENTRY(com_util_condvar_create, int(COM_UTIL_API *)(com_util_condvar * *cv)) \
    EXPORT_ENTRY(com_util_condvar_wait, \
                 int(COM_UTIL_API *)(com_util_condvar * cv, com_util_local_lock * mtx, int timeout_ms)) \
    EXPORT_ENTRY(com_util_condvar_signal, int(COM_UTIL_API *)(com_util_condvar * cv)) \
    EXPORT_ENTRY(com_util_condvar_broadcast, int(COM_UTIL_API *)(com_util_condvar * cv)) \
    EXPORT_ENTRY(com_util_condvar_destroy, void(COM_UTIL_API *)(com_util_condvar * cv)) \
    EXPORT_ENTRY(com_util_local_rwlock_create, int(COM_UTIL_API *)(com_util_local_rwlock * *rwlock)) \
    EXPORT_ENTRY(com_util_local_rwlock_lock_shared, \
                 int(COM_UTIL_API *)(com_util_local_rwlock * rwlock, int timeout_ms)) \
    EXPORT_ENTRY(com_util_local_rwlock_try_lock_shared, int(COM_UTIL_API *)(com_util_local_rwlock * rwlock)) \
    EXPORT_ENTRY(com_util_local_rwlock_lock_exclusive, \
                 int(COM_UTIL_API *)(com_util_local_rwlock * rwlock, int timeout_ms)) \
    EXPORT_ENTRY(com_util_local_rwlock_try_lock_exclusive, int(COM_UTIL_API *)(com_util_local_rwlock * rwlock)) \
    EXPORT_ENTRY(com_util_local_rwlock_unlock_shared, int(COM_UTIL_API *)(com_util_local_rwlock * rwlock)) \
    EXPORT_ENTRY(com_util_local_rwlock_unlock_exclusive, int(COM_UTIL_API *)(com_util_local_rwlock * rwlock)) \
    EXPORT_ENTRY(com_util_local_rwlock_destroy, void(COM_UTIL_API *)(com_util_local_rwlock * rwlock)) \
    EXPORT_ENTRY(com_util_thread_create, \
                 int(COM_UTIL_API *)(com_util_thread * *thread, com_util_thread_fn func, void *arg)) \
    EXPORT_ENTRY(com_util_thread_join, int(COM_UTIL_API *)(com_util_thread * thread, int timeout_ms)) \
    EXPORT_ENTRY(com_util_thread_detach, void(COM_UTIL_API *)(com_util_thread * thread)) \
    EXPORT_ENTRY(com_util_interprocess_lock_open, \
                 int(COM_UTIL_API *)(const char *identity, com_util_interprocess_lock **lock)) \
    EXPORT_ENTRY( \
        com_util_interprocess_lock_import_descriptor, \
        int(COM_UTIL_API *)(const void *descriptor, size_t descriptor_size, com_util_interprocess_lock **lock)) \
    EXPORT_ENTRY( \
        com_util_interprocess_lock_export_descriptor, \
        int(COM_UTIL_API *)(const com_util_interprocess_lock *lock, void *descriptor, size_t *descriptor_size)) \
    EXPORT_ENTRY(com_util_interprocess_lock_lock, \
                 int(COM_UTIL_API *)(com_util_interprocess_lock * lock, int timeout_ms)) \
    EXPORT_ENTRY(com_util_interprocess_lock_try_lock, int(COM_UTIL_API *)(com_util_interprocess_lock * lock)) \
    EXPORT_ENTRY(com_util_interprocess_lock_unlock, int(COM_UTIL_API *)(com_util_interprocess_lock * lock)) \
    EXPORT_ENTRY(com_util_interprocess_lock_destroy, void(COM_UTIL_API *)(com_util_interprocess_lock * lock)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_open, \
                 int(COM_UTIL_API *)(const char *identity, com_util_interprocess_rwlock **lock)) \
    EXPORT_ENTRY( \
        com_util_interprocess_rwlock_import_descriptor, \
        int(COM_UTIL_API *)(const void *descriptor, size_t descriptor_size, com_util_interprocess_rwlock **lock)) \
    EXPORT_ENTRY( \
        com_util_interprocess_rwlock_export_descriptor, \
        int(COM_UTIL_API *)(const com_util_interprocess_rwlock *lock, void *descriptor, size_t *descriptor_size)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_lock_shared, \
                 int(COM_UTIL_API *)(com_util_interprocess_rwlock * lock, int timeout_ms)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_try_lock_shared, \
                 int(COM_UTIL_API *)(com_util_interprocess_rwlock * lock)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_lock_exclusive, \
                 int(COM_UTIL_API *)(com_util_interprocess_rwlock * lock, int timeout_ms)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_try_lock_exclusive, \
                 int(COM_UTIL_API *)(com_util_interprocess_rwlock * lock)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_unlock, int(COM_UTIL_API *)(com_util_interprocess_rwlock * lock)) \
    EXPORT_ENTRY(com_util_interprocess_rwlock_destroy, void(COM_UTIL_API *)(com_util_interprocess_rwlock * lock)) \
    EXPORT_ENTRY(com_util_call_once, void(COM_UTIL_API *)(com_util_once_flag * flag, com_util_once_fn func)) \
    EXPORT_ENTRY(com_util_sleep_ms, void(COM_UTIL_API *)(int ms)) \
    /* com_util/trace/trace_file.h */ \
    EXPORT_ENTRY( \
        com_util_trace_file_sink_create, \
        com_util_trace_file_sink *(COM_UTIL_API *)(const char *path, size_t max_bytes, int generations, int flags)) \
    EXPORT_ENTRY(com_util_trace_file_sink_write, \
                 int(COM_UTIL_API *)(com_util_trace_file_sink * handle, int level, const com_util_timespec *timestamp, \
                                     const char *message)) \
    EXPORT_ENTRY(com_util_trace_file_sink_dispose, void(COM_UTIL_API *)(com_util_trace_file_sink * handle)) \
    /* com_util/trace/tracer.h */ \
    EXPORT_ENTRY(com_util_tracer_create, com_util_tracer *(COM_UTIL_API *)(void)) \
    EXPORT_ENTRY(com_util_tracer_start, int(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_stop, int(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_get_state, com_util_tracer_state(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(_com_util_tracer_write, int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level, \
                                                             const com_util_timespec *timestamp, const char *message)) \
    EXPORT_ENTRY(_com_util_tracer_writef, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level, \
                                     const com_util_timespec *timestamp, const char *format, ...)) \
    EXPORT_ENTRY(_com_util_tracer_write_hex, int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level, \
                                                                 const com_util_timespec *timestamp, const void *data, \
                                                                 size_t size, const char *message)) \
    EXPORT_ENTRY(_com_util_tracer_write_hexf, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level, \
                                     const com_util_timespec *timestamp, const void *data, size_t size, \
                                     const char *format, ...)) \
    EXPORT_ENTRY(com_util_tracer_set_name, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, const char *name, int64_t identifier)) \
    EXPORT_ENTRY(com_util_tracer_get_name, int(COM_UTIL_API *)(com_util_tracer * handle, char *out, size_t out_size)) \
    EXPORT_ENTRY(com_util_tracer_get_identifier, int64_t(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_set_file_name, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, const char *name, int64_t identifier)) \
    EXPORT_ENTRY(com_util_tracer_get_file_name, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, char *out, size_t out_size)) \
    EXPORT_ENTRY(com_util_tracer_get_file_identifier, int64_t(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_get_os_level, com_util_trace_level(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_set_os_level, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level)) \
    EXPORT_ENTRY(com_util_tracer_get_etw_level, com_util_trace_level(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_set_etw_level, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level)) \
    EXPORT_ENTRY(com_util_tracer_get_file_level, com_util_trace_level(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_set_file_level, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, const char *path, com_util_trace_level level, \
                                     size_t max_bytes, int generations, int flags)) \
    EXPORT_ENTRY(com_util_tracer_get_stderr_level, com_util_trace_level(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_set_stderr_level, \
                 int(COM_UTIL_API *)(com_util_tracer * handle, com_util_trace_level level)) \
    EXPORT_ENTRY(com_util_tracer_dispose, void(COM_UTIL_API *)(com_util_tracer * handle)) \
    EXPORT_ENTRY(com_util_tracer_set_hook, \
                 com_util_tracer_hook_entry *(COM_UTIL_API *)(com_util_tracer * handle, com_util_tracer_hook_fn fn, \
                                                              void *context)) \
    EXPORT_ENTRY(com_util_tracer_remove_hook, \
                 void(COM_UTIL_API *)(com_util_tracer * handle, com_util_tracer_hook_entry * hook_entry)) \
    EXPORT_ENTRY(com_util_tracer_call_next_hook, \
                 void(COM_UTIL_API *)(com_util_tracer_hook_entry * prev, com_util_tracer * handle, \
                                      com_util_trace_level level, const com_util_timespec *timestamp, \
                                      const char *message))

#if defined(PLATFORM_WINDOWS)
    #define COM_UTIL_EXPORT_TABLE_PLATFORM(EXPORT_ENTRY) \
        /* com_util/crt/wchar_conv.h */ \
        EXPORT_ENTRY(com_util_utf8_to_wpath, \
                     int(COM_UTIL_API *)(wchar_t * wbuf, size_t wbuf_count, const char *utf8_path)) \
        EXPORT_ENTRY(com_util_wpath_to_utf8, int(COM_UTIL_API *)(char *out, size_t out_size, const wchar_t *wpath)) \
        EXPORT_ENTRY(com_util_utf8_to_wstr_alloc, wchar_t *(COM_UTIL_API *)(const char *utf8_text)) \
        EXPORT_ENTRY(com_util_wstr_to_utf8_alloc, char *(COM_UTIL_API *)(const wchar_t *wtext)) \
        /* com_util/base/error.h */ \
        EXPORT_ENTRY(com_util_error_capture_windows_error, \
                     void(COM_UTIL_API *)(com_util_error * error, unsigned long error_code)) \
        EXPORT_ENTRY(com_util_error_capture_current_windows_error, void(COM_UTIL_API *)(com_util_error * error)) \
        EXPORT_ENTRY(com_util_error_get_windows_error, unsigned long(COM_UTIL_API *)(const com_util_error *error)) \
        /* com_util/trace/etw.h */ \
        EXPORT_ENTRY(com_util_etw_provider_create, \
                     com_util_etw_provider *(COM_UTIL_API *)(com_util_etw_provider_ref_t provider_ref)) \
        EXPORT_ENTRY(com_util_etw_provider_write, int(COM_UTIL_API *)(com_util_etw_provider * handle, int level, \
                                                                      const char *service, const char *message)) \
        EXPORT_ENTRY(com_util_etw_provider_dispose, void(COM_UTIL_API *)(com_util_etw_provider * handle)) \
        EXPORT_ENTRY(com_util_etw_session_check_access, int(COM_UTIL_API *)(void)) \
        EXPORT_ENTRY(com_util_etw_session_start, \
                     int(COM_UTIL_API *)(const char *session_name, const char *provider_guid_str, \
                                         com_util_etw_event_fn callback, void *context, \
                                         com_util_etw_session **session_out)) \
        EXPORT_ENTRY(com_util_etw_session_stop, void(COM_UTIL_API *)(com_util_etw_session * session)) \
        /* com_util/trace/eventlog.h */ \
        EXPORT_ENTRY(com_util_eventlog_sink_create, com_util_eventlog_sink *(COM_UTIL_API *)(const char *source_name)) \
        EXPORT_ENTRY(com_util_eventlog_sink_write, \
                     int(COM_UTIL_API *)(com_util_eventlog_sink * handle, int level, int64_t file_identifier, \
                                         const char *instance_name, int64_t instance_identifier, const char *message)) \
        EXPORT_ENTRY(com_util_eventlog_sink_dispose, void(COM_UTIL_API *)(com_util_eventlog_sink * handle)) \
        EXPORT_ENTRY(com_util_eventlog_register_source, \
                     int(COM_UTIL_API *)(const char *source_name, const char *message_file_path)) \
        EXPORT_ENTRY(com_util_eventlog_unregister_source, int(COM_UTIL_API *)(const char *source_name)) \
        /* com_util/win32/win32.h */ \
        EXPORT_ENTRY(CreateFileU, \
                     HANDLE(COM_UTIL_API *)(const char *utf8_path, DWORD desired_access, DWORD share_mode, \
                                            LPSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, \
                                            DWORD flags_and_attributes, HANDLE template_file)) \
        EXPORT_ENTRY(CreateNamedPipeU, \
                     HANDLE(COM_UTIL_API *)(const char *utf8_name, DWORD open_mode, DWORD pipe_mode, \
                                            DWORD max_instances, DWORD out_buffer_size, DWORD in_buffer_size, \
                                            DWORD default_timeout, LPSECURITY_ATTRIBUTES security_attributes)) \
        EXPORT_ENTRY(GetModuleFileNameU, DWORD(COM_UTIL_API *)(HMODULE module, char *utf8_buf, DWORD size)) \
        EXPORT_ENTRY(WriteConsoleU, BOOL(COM_UTIL_API *)(HANDLE console, const char *utf8_text, DWORD utf8_length, \
                                                         DWORD *written_length, void *reserved)) \
        EXPORT_ENTRY(GetVolumePathNameU, \
                     BOOL(COM_UTIL_API *)(const char *utf8_path, char *utf8_volume_root, DWORD size)) \
        EXPORT_ENTRY(GetVolumeInformationU, \
                     BOOL(COM_UTIL_API *)(const char *utf8_root_path, char *utf8_volume_name, DWORD volume_name_size, \
                                          DWORD *serial_number, DWORD *max_component_length, DWORD *file_system_flags, \
                                          char *utf8_file_system_name, DWORD file_system_name_size)) \
        EXPORT_ENTRY(LoadLibraryU, HMODULE(COM_UTIL_API *)(const char *utf8_file_name)) \
        EXPORT_ENTRY(CreateProcessU, BOOL(COM_UTIL_API *)( \
                                         const char *utf8_application_name, const char *utf8_command_line, \
                                         LPSECURITY_ATTRIBUTES process_attributes, \
                                         LPSECURITY_ATTRIBUTES thread_attributes, BOOL inherit_handles, \
                                         DWORD creation_flags, LPVOID environment, const char *utf8_current_directory, \
                                         LPSTARTUPINFOW startup_info, LPPROCESS_INFORMATION process_information)) \
        EXPORT_ENTRY(OpenSCManagerU, SC_HANDLE(COM_UTIL_API *)(const char *utf8_machine_name, \
                                                               const char *utf8_database_name, DWORD desired_access)) \
        EXPORT_ENTRY(CreateServiceU, \
                     SC_HANDLE(COM_UTIL_API *)(SC_HANDLE scm, const char *utf8_service_name, \
                                               const char *utf8_display_name, DWORD desired_access, \
                                               DWORD service_type, DWORD start_type, DWORD error_control, \
                                               const char *utf8_binary_path_name, const char *utf8_load_order_group, \
                                               LPDWORD tag_id, const char *utf8_dependencies, \
                                               const char *utf8_service_start_name, const char *utf8_password)) \
        EXPORT_ENTRY(OpenServiceU, \
                     SC_HANDLE(COM_UTIL_API *)(SC_HANDLE scm, const char *utf8_service_name, DWORD desired_access)) \
        EXPORT_ENTRY(ChangeServiceConfig2U, \
                     BOOL(COM_UTIL_API *)(SC_HANDLE service, DWORD info_level, const char *utf8_text)) \
        EXPORT_ENTRY(RegisterServiceCtrlHandlerExU, \
                     SERVICE_STATUS_HANDLE(COM_UTIL_API *)(const char *utf8_service_name, \
                                                           LPHANDLER_FUNCTION_EX handler_proc, LPVOID context)) \
        EXPORT_ENTRY(StartServiceCtrlDispatcherU, BOOL(COM_UTIL_API *)(const com_util_service_entry_u *service_table))
#elif defined(PLATFORM_LINUX)
    #define COM_UTIL_EXPORT_TABLE_PLATFORM(EXPORT_ENTRY) \
        /* com_util/trace/syslog.h */ \
        EXPORT_ENTRY(com_util_syslog_sink_create, \
                     com_util_syslog_sink *(COM_UTIL_API *)(const char *ident, int facility)) \
        EXPORT_ENTRY(com_util_syslog_sink_write, \
                     int(COM_UTIL_API *)(com_util_syslog_sink * handle, int level, const com_util_timespec *timestamp, \
                                         const char *message)) \
        EXPORT_ENTRY(com_util_syslog_sink_rename, \
                     int(COM_UTIL_API *)(com_util_syslog_sink * handle, const char *new_ident)) \
        EXPORT_ENTRY(com_util_syslog_sink_dispose, void(COM_UTIL_API *)(com_util_syslog_sink * handle))
#endif /* PLATFORM_ */

// libcom_util が公開エクスポートすべき変数の一覧。
// 現時点ではエントリなし (公開ヘッダーに dllexport 付きの変数エクスポートが存在しないため)。
// 公開ヘッダーへ変数エクスポートを追加する場合は、ここへ X(変数名, 型 *) の形で登録する。
// decltype(&name) はオブジェクトに対しても型 * を返すため、関数と同じ
// static_assert / kExpectedExportNames / symbol_names_match の仕組みがそのまま使える。
#define COM_UTIL_EXPORT_VARIABLE_TABLE(EXPORT_ENTRY)

#define COM_UTIL_EXPORT_TABLE(EXPORT_ENTRY) \
    COM_UTIL_EXPORT_TABLE_COMMON(EXPORT_ENTRY) \
    COM_UTIL_EXPORT_TABLE_PLATFORM(EXPORT_ENTRY) \
    COM_UTIL_EXPORT_VARIABLE_TABLE(EXPORT_ENTRY)

// テーブルからシグネチャの static_assert と期待シンボル名一覧を生成する。
// 定型マクロ (TESTFW_EXPORT_STATIC_ASSERT_ENTRY/TESTFW_EXPORT_NAME_ENTRY) は
// framework/testfw/include/export_check.h 側の共通定義を使う。
#if defined(PLATFORM_LINUX)
    // GCC は format 属性付き関数ポインター型を std::is_same のテンプレート引数にすると、
    // ABI シグネチャ比較の対象外である format 属性を無視したことを警告するため、この比較だけ抑制する。
    // see: https://gcc.gnu.org/onlinedocs/gcc-8.5.0/gcc/Warning-Options.html#index-Wignored-attributes
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wignored-attributes"
#endif /* PLATFORM_LINUX */
COM_UTIL_EXPORT_TABLE(TESTFW_EXPORT_STATIC_ASSERT_ENTRY)
#if defined(PLATFORM_LINUX)
    #pragma GCC diagnostic pop
#endif /* PLATFORM_LINUX */

static const char *const kExpectedExportNames[] = {COM_UTIL_EXPORT_TABLE(TESTFW_EXPORT_NAME_ENTRY)};

static const std::map<std::string, std::string> kExpectedExportSignatures = {
    COM_UTIL_EXPORT_TABLE(TESTFW_EXPORT_SIGNATURE_ENTRY)};

class exportTest : public Test
{
  protected:
    std::string workspace_root;
    std::string dll_path;

    void SetUp() override
    {
        workspace_root = findWorkspaceRoot();
        ASSERT_FALSE(workspace_root.empty()) << "ワークスペースルートが見つかりません";
        dll_path = workspace_root + "/app/com_util/prod/lib/libcom_util" TESTFW_SHARED_LIBRARY_EXTENSION;
    }
};

// libcom_util のエクスポート シンボル名一致テスト
TEST_F(exportTest, symbol_names_match)
{
    // Arrange
    std::set<std::string> expected(
        std::begin(kExpectedExportNames),
        std::end(kExpectedExportNames)); // [状態] - COM_UTIL_EXPORT_TABLE から期待シンボル名一覧を構築する。
#if defined(PLATFORM_WINDOWS)
    // _ident_manifest_libcom_util_dll は gen_ident_manifest.py が自動生成するビルド識別データであり、
    // 関数ではないためシグネチャ検証の対象外としつつ、名前一致の期待値には含める。
    expected.insert(testing::identManifestSymbolName(
        "libcom_util" TESTFW_SHARED_LIBRARY_EXTENSION)); // [状態] - IDENT manifest シンボル名を期待値へ追加する (Windows のみ実際にエクスポートされる)。
#endif                                                   /* PLATFORM_WINDOWS */

    // Pre-Assert

    // Act
    std::set<std::string> actual = testing::getActualExportNames(
        dll_path); // [手順] - dumpbin/nm で libcom_util の実際のエクスポート一覧を取得する。

    // Assert
    testing::expectExportNamesMatch(
        expected, actual,
        kExpectedExportSignatures); // [確認_正常系] - 期待シンボルとの不足/想定外がないこと (Windows / Linux とも完全一致)。
}

// 公開ヘッダーの変数宣言が dllexport マクロ (COM_UTIL_EXPORT) を
// 伴わずに追加されていないことの確認
TEST_F(exportTest, public_header_variables_declare_export_macro)
{
    // Arrange
    std::string include_dir =
        workspace_root +
        "/app/com_util/prod/include"; // [状態] - 公開ヘッダーのディレクトリを "/app/com_util/prod/include" に設定する。

    // Pre-Assert

    // Act
    std::vector<std::string> undecorated = testing::findUndecoratedExternVariables(
        include_dir,
        "COM_UTIL_EXPORT"); // [手順] - prod/include 配下を走査し、COM_UTIL_EXPORT を伴わない extern 変数宣言を集める。

    // Assert
    EXPECT_TRUE(undecorated.empty()) << "COM_UTIL_EXPORT を伴わない変数宣言: "
                                     << testing::joinNames(
                                            undecorated); // [確認_正常系] - 該当する宣言が 1 件もないこと。
}
