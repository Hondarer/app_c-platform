#ifndef COM_UTIL_TRACE_CLI_H
#define COM_UTIL_TRACE_CLI_H

#include <com_util/trace/tracer.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct trace_cli_session
    {
        com_util_tracer *handle;
        int prompt_state;
        int exit_requested;
    } trace_cli_session;

    void trace_cli_session_init(trace_cli_session *session);
    void trace_cli_session_dispose(trace_cli_session *session);
    void trace_cli_print_help(void);
    int trace_cli_process_line(trace_cli_session *session, const char *line);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_TRACE_CLI_H */
