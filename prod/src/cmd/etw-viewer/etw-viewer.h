#ifndef COM_UTIL_ETW_VIEWER_H
#define COM_UTIL_ETW_VIEWER_H

#include <com_util/trace/etw.h>
#include <stddef.h>
#include <stdint.h>

#define ETW_VIEWER_SESSION_NAME_MAX 128

typedef struct etw_viewer_options
{
    uint32_t process_id_filter;
    int has_process_id_filter;
} etw_viewer_options;

typedef struct etw_viewer_context
{
    uint32_t process_id_filter;
    int has_process_id_filter;
} etw_viewer_context;

#ifdef __cplusplus
extern "C"
{
#endif

    void etw_viewer_options_init(etw_viewer_options *options);
    int etw_viewer_parse_args(int argc, char *argv[], etw_viewer_options *options);
    int etw_viewer_build_default_session_name(unsigned long process_id, char *buffer, size_t buffer_size);
    const char *etw_viewer_level_name(int level);
    void etw_viewer_print_usage(const char *argv0, int is_windows);
    int etw_viewer_format_timestamp_utc(int64_t timestamp_100ns, char *buffer, size_t buffer_size);
    void etw_viewer_handle_event(const com_util_etw_event *event, void *context);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_ETW_VIEWER_H */
