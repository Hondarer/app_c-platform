/**
 *******************************************************************************
 *  @file           trace-cli.c
 *  @brief          tracer API の動作確認用対話 CLI です。
 *  @author         Tetsuo Honda
 *  @date           2026/04/28
 *  @version        1.0.0
 *
 *  `com_util/trace/tracer.h` の公開 API を対話的に呼び出すための確認用 CLI です。\n
 *  起動後に interactive CLI として動作し、1 セッションにつき 1 個の tracer handle を保持します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include "trace-cli.h"

#include <com_util/console/console.h>
#include <com_util/crt/string.h>
#include <com_util/crt/unistd.h>
#include <com_util/prompt/prompt.h>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_CLI_LINE_MAX 4096

static const char *const g_trace_level_names[] = {
    "CRITICAL", "ERROR", "WARNING", "INFO", "VERBOSE", "DEBUG", "NONE",
};

enum trace_cli_prompt_state
{
    TRACE_CLI_PROMPT_STATE_UNCREATED = 0,
    TRACE_CLI_PROMPT_STATE_DISPOSED = 1
};

static char *skip_spaces(char *p)
{
    while (p != NULL && *p != '\0' && isspace((unsigned char)*p))
    {
        p++;
    }
    return p;
}

static void trim_right(char *s)
{
    size_t len;

    if (s == NULL)
    {
        return;
    }

    len = strlen(s);
    while (len > 0U && isspace((unsigned char)s[len - 1U]))
    {
        len--;
    }
    s[len] = '\0';
}

static int str_case_equal(const char *lhs, const char *rhs)
{
    size_t i;

    if (lhs == NULL || rhs == NULL)
    {
        return 0;
    }

    for (i = 0U; lhs[i] != '\0' && rhs[i] != '\0'; i++)
    {
        if (toupper((unsigned char)lhs[i]) != toupper((unsigned char)rhs[i]))
        {
            return 0;
        }
    }

    return lhs[i] == '\0' && rhs[i] == '\0';
}

static int is_null_keyword(const char *token)
{
    return str_case_equal(token, "null");
}

static char *next_token(char **cursor)
{
    char *p;
    char quote;
    char *start;

    if (cursor == NULL || *cursor == NULL)
    {
        return NULL;
    }

    p = skip_spaces(*cursor);
    if (*p == '\0')
    {
        *cursor = p;
        return NULL;
    }

    if (*p == '"' || *p == '\'')
    {
        quote = *p;
        start = ++p;
        while (*p != '\0' && *p != quote)
        {
            p++;
        }
        if (*p == '\0')
        {
            *cursor = p;
            return NULL;
        }
        *p = '\0';
        *cursor = p + 1;
        return start;
    }

    start = p;
    while (*p != '\0' && !isspace((unsigned char)*p))
    {
        p++;
    }
    if (*p != '\0')
    {
        *p = '\0';
        p++;
    }
    *cursor = p;
    return start;
}

static char *rest_argument(char **cursor)
{
    char *p;
    char *quoted;

    if (cursor == NULL || *cursor == NULL)
    {
        return NULL;
    }

    p = skip_spaces(*cursor);
    if (*p == '\0')
    {
        *cursor = p;
        return NULL;
    }

    if (*p == '"' || *p == '\'')
    {
        quoted = next_token(&p);
        if (quoted == NULL)
        {
            *cursor = p;
            return NULL;
        }
        p = skip_spaces(p);
        if (*p != '\0')
        {
            *cursor = p;
            return NULL;
        }
        *cursor = p;
        return quoted;
    }

    *cursor = p + strlen(p);
    return p;
}

static const char *level_to_name(com_util_trace_level_t level)
{
    if ((int)level >= 0 && (size_t)level < sizeof(g_trace_level_names) / sizeof(g_trace_level_names[0]))
    {
        return g_trace_level_names[(size_t)level];
    }

    return "UNKNOWN";
}

static int parse_trace_level(const char *token, com_util_trace_level_t *level)
{
    size_t i;

    if (token == NULL || level == NULL)
    {
        return 0;
    }

    for (i = 0U; i < sizeof(g_trace_level_names) / sizeof(g_trace_level_names[0]); i++)
    {
        if (str_case_equal(token, g_trace_level_names[i]))
        {
            *level = (com_util_trace_level_t)i;
            return 1;
        }
    }

    return 0;
}

static int parse_int64_value(const char *token, int64_t *value)
{
    char *end = NULL;
    long long parsed;

    if (token == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtoll(token, &end, 10);
    if (errno != 0 || end == token || *end != '\0')
    {
        return 0;
    }

    *value = (int64_t)parsed;
    return 1;
}

static int parse_int_value(const char *token, int *value)
{
    char *end = NULL;
    long parsed;

    if (token == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtol(token, &end, 10);
    if (errno != 0 || end == token || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX)
    {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static const char *tracer_state_to_name(com_util_tracer_state_t state)
{
    switch (state)
    {
    case COM_UTIL_TRACER_STATE_STARTED:
        return "started";
    case COM_UTIL_TRACER_STATE_STOPPED:
        return "stopped";
    case COM_UTIL_TRACER_STATE_DISPOSED:
    default:
        return "disposed";
    }
}

static const char *session_prompt_state_to_name(const trace_cli_session *session)
{
    if (session == NULL)
    {
        return "disposed";
    }
    if (session->handle != NULL)
    {
        return tracer_state_to_name(com_util_tracer_get_state(session->handle));
    }

    if (session->prompt_state == TRACE_CLI_PROMPT_STATE_DISPOSED)
    {
        return "disposed";
    }

    return "uncreated";
}

static int parse_size_value(const char *token, size_t *value)
{
    char *end = NULL;
    unsigned long long parsed;

    if (token == NULL || value == NULL)
    {
        return 0;
    }

    errno = 0;
    parsed = strtoull(token, &end, 10);
    if (errno != 0 || end == token || *end != '\0')
    {
        return 0;
    }

    *value = (size_t)parsed;
    return 1;
}

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    return -1;
}

static int parse_hex_bytes(const char *text, unsigned char **data, size_t *size)
{
    size_t digits = 0U;
    size_t index = 0U;
    const char *p;
    unsigned char *buf = NULL;
    int high = -1;

    if (text == NULL || data == NULL || size == NULL)
    {
        return 0;
    }

    for (p = text; *p != '\0'; p++)
    {
        if (isspace((unsigned char)*p))
        {
            continue;
        }
        if (hex_nibble(*p) < 0)
        {
            return 0;
        }
        digits++;
    }

    if ((digits % 2U) != 0U)
    {
        return 0;
    }

    if (digits == 0U)
    {
        *data = NULL;
        *size = 0U;
        return 1;
    }

    buf = (unsigned char *)malloc(digits / 2U);
    if (buf == NULL)
    {
        return 0;
    }

    for (p = text; *p != '\0'; p++)
    {
        int nibble;

        if (isspace((unsigned char)*p))
        {
            continue;
        }

        nibble = hex_nibble(*p);
        if (nibble < 0)
        {
            free(buf);
            return 0;
        }

        if (high < 0)
        {
            high = nibble;
        }
        else
        {
            buf[index++] = (unsigned char)((high << 4) | nibble);
            high = -1;
        }
    }

    *data = buf;
    *size = digits / 2U;
    return 1;
}

static void print_level_result(com_util_trace_level_t level)
{
    printf("level=%s(%d)\n", level_to_name(level), (int)level);
}

static void print_rc_result(int rc)
{
    if (!com_util_isatty(COM_UTIL_STREAM_STDOUT))
    {
        printf("rc=%d\n", rc);
        return;
    }

    if (rc == 0)
    {
        printf("\x1b[32mrc=%d\x1b[0m\n", rc);
        return;
    }

    printf("\x1b[31mrc=%d\x1b[0m\n", rc);
}

static void print_command_usage(const char *command)
{
    if (command == NULL)
    {
        return;
    }

    if (strcmp(command, "create") == 0)
    {
        fprintf(stderr, "使用方法: create\n");
    }
    else if (strcmp(command, "dispose") == 0)
    {
        fprintf(stderr, "使用方法: dispose\n");
    }
    else if (strcmp(command, "start") == 0)
    {
        fprintf(stderr, "使用方法: start\n");
    }
    else if (strcmp(command, "stop") == 0)
    {
        fprintf(stderr, "使用方法: stop\n");
    }
    else if (strcmp(command, "set-name") == 0)
    {
        fprintf(stderr, "使用方法: set-name <name|null> [identifier]\n");
    }
    else if (strcmp(command, "get-os-level") == 0)
    {
        fprintf(stderr, "使用方法: get-os-level\n");
    }
    else if (strcmp(command, "set-os-level") == 0)
    {
        fprintf(stderr, "使用方法: set-os-level <level>\n");
    }
    else if (strcmp(command, "get-file-level") == 0)
    {
        fprintf(stderr, "使用方法: get-file-level\n");
    }
    else if (strcmp(command, "set-file-level") == 0)
    {
        fprintf(stderr, "使用方法: set-file-level <path|null> <level> [max-bytes] [generations]\n");
    }
    else if (strcmp(command, "get-stderr-level") == 0)
    {
        fprintf(stderr, "使用方法: get-stderr-level\n");
    }
    else if (strcmp(command, "set-stderr-level") == 0)
    {
        fprintf(stderr, "使用方法: set-stderr-level <level>\n");
    }
    else if (strcmp(command, "write") == 0)
    {
        fprintf(stderr, "使用方法: write <level> <message...>\n");
    }
    else if (strcmp(command, "writef") == 0)
    {
        fprintf(stderr, "使用方法: writef <level> <message...>\n");
    }
    else if (strcmp(command, "write-hex") == 0)
    {
        fprintf(stderr, "使用方法: write-hex <level> <hex> [label...]\n");
    }
    else if (strcmp(command, "write-hexf") == 0)
    {
        fprintf(stderr, "使用方法: write-hexf <level> <hex> [label...]\n");
    }
    else if (strcmp(command, "help") == 0)
    {
        fprintf(stderr, "使用方法: help\n");
    }
    else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
    {
        fprintf(stderr, "使用方法: %s\n", command);
    }
}

void trace_cli_session_init(trace_cli_session *session)
{
    if (session == NULL)
    {
        return;
    }

    session->handle = NULL;
    session->prompt_state = TRACE_CLI_PROMPT_STATE_UNCREATED;
    session->exit_requested = 0;
}

void trace_cli_session_dispose(trace_cli_session *session)
{
    if (session == NULL)
    {
        return;
    }

    com_util_tracer_dispose(session->handle);
    session->handle = NULL;
    session->prompt_state = TRACE_CLI_PROMPT_STATE_DISPOSED;
}

void trace_cli_print_help(void)
{
    printf("trace-cli: com_util tracer interactive CLI\n");
    printf("使用可能な level: CRITICAL ERROR WARNING INFO VERBOSE DEBUG NONE\n");
    printf("コマンド:\n");
    printf("  help\n");
    printf("  exit\n");
    printf("  quit\n");
    printf("  create\n");
    printf("  dispose\n");
    printf("  start\n");
    printf("  stop\n");
    printf("  set-name <name|null> [identifier]\n");
    printf("  get-os-level\n");
    printf("  set-os-level <level>\n");
    printf("  get-file-level\n");
    printf("  set-file-level <path|null> <level> [max-bytes] [generations]\n");
    printf("  get-stderr-level\n");
    printf("  set-stderr-level <level>\n");
    printf("  write <level> <message...>\n");
    printf("  writef <level> <message...>\n");
    printf("  write-hex <level> <hex> [label...]\n");
    printf("  write-hexf <level> <hex> [label...]\n");
    printf("hex は 01ABFF または \"01 AB FF\" の形式で指定できます。\n");
}

static void print_interactive_hint(void)
{
    printf("help でコマンド一覧を表示します。exit で終了します。\n");
}

int trace_cli_process_line(trace_cli_session *session, const char *line)
{
    char buffer[TRACE_CLI_LINE_MAX];
    char *cursor;
    char *command;

    if (session == NULL || line == NULL)
    {
        return -1;
    }

    (void)com_util_strncpy(buffer, sizeof(buffer), line, sizeof(buffer) - 1U);
    trim_right(buffer);

    cursor = buffer;
    command = next_token(&cursor);
    if (command == NULL)
    {
        return 0;
    }

    if (strcmp(command, "help") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        trace_cli_print_help();
        return 0;
    }

    if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        session->exit_requested = 1;
        return 1;
    }

    if (strcmp(command, "create") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        if (session->handle != NULL)
        {
            fprintf(stderr, "エラー: 既存の handle を dispose してから create を実行してください。\n");
            return -1;
        }
        session->handle = com_util_tracer_create();
        if (session->handle == NULL)
        {
            session->prompt_state = TRACE_CLI_PROMPT_STATE_UNCREATED;
            printf("handle=NULL\n");
        }
        else
        {
            printf("handle=created\n");
        }
        return 0;
    }

    if (strcmp(command, "dispose") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        com_util_tracer_dispose(session->handle);
        session->handle = NULL;
        session->prompt_state = TRACE_CLI_PROMPT_STATE_DISPOSED;
        printf("handle=disposed\n");
        return 0;
    }

    if (strcmp(command, "start") == 0)
    {
        int rc;

        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        rc = com_util_tracer_start(session->handle);
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "stop") == 0)
    {
        int rc;

        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        rc = com_util_tracer_stop(session->handle);
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "set-name") == 0)
    {
        char *name_token;
        char *identifier_token;
        const char *name = NULL;
        int64_t identifier = 0;
        int rc;

        name_token = next_token(&cursor);
        if (name_token == NULL)
        {
            print_command_usage(command);
            return -1;
        }
        identifier_token = next_token(&cursor);
        if (identifier_token != NULL && !parse_int64_value(identifier_token, &identifier))
        {
            fprintf(stderr, "エラー: identifier は整数で指定してください。\n");
            return -1;
        }
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }

        if (!is_null_keyword(name_token))
        {
            name = name_token;
        }
        rc = com_util_tracer_set_name(session->handle, name, identifier);
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "get-os-level") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        print_level_result(com_util_tracer_get_os_level(session->handle));
        return 0;
    }

    if (strcmp(command, "set-os-level") == 0)
    {
        char *level_token;
        com_util_trace_level_t level;
        int rc;

        level_token = next_token(&cursor);
        if (level_token == NULL || next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        if (!parse_trace_level(level_token, &level))
        {
            fprintf(stderr, "エラー: level が不正です。\n");
            return -1;
        }
        rc = com_util_tracer_set_os_level(session->handle, level);
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "get-file-level") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        print_level_result(com_util_tracer_get_file_level(session->handle));
        return 0;
    }

    if (strcmp(command, "set-file-level") == 0)
    {
        char *path_token;
        char *level_token;
        char *max_bytes_token;
        char *generations_token;
        const char *path = NULL;
        com_util_trace_level_t level;
        size_t max_bytes = 0U;
        int generations = 0;
        int rc;

        path_token = next_token(&cursor);
        level_token = next_token(&cursor);
        if (path_token == NULL || level_token == NULL)
        {
            print_command_usage(command);
            return -1;
        }
        if (!parse_trace_level(level_token, &level))
        {
            fprintf(stderr, "エラー: level が不正です。\n");
            return -1;
        }

        max_bytes_token = next_token(&cursor);
        if (max_bytes_token != NULL && !parse_size_value(max_bytes_token, &max_bytes))
        {
            fprintf(stderr, "エラー: max-bytes は 0 以上の整数で指定してください。\n");
            return -1;
        }

        generations_token = next_token(&cursor);
        if (generations_token != NULL && !parse_int_value(generations_token, &generations))
        {
            fprintf(stderr, "エラー: generations は整数で指定してください。\n");
            return -1;
        }

        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }

        if (!is_null_keyword(path_token))
        {
            path = path_token;
        }
        rc = com_util_tracer_set_file_level(session->handle, path, level, max_bytes, generations, 0);
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "get-stderr-level") == 0)
    {
        if (next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        print_level_result(com_util_tracer_get_stderr_level(session->handle));
        return 0;
    }

    if (strcmp(command, "set-stderr-level") == 0)
    {
        char *level_token;
        com_util_trace_level_t level;
        int rc;

        level_token = next_token(&cursor);
        if (level_token == NULL || next_token(&cursor) != NULL)
        {
            print_command_usage(command);
            return -1;
        }
        if (!parse_trace_level(level_token, &level))
        {
            fprintf(stderr, "エラー: level が不正です。\n");
            return -1;
        }
        rc = com_util_tracer_set_stderr_level(session->handle, level);
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "write") == 0 || strcmp(command, "writef") == 0)
    {
        char *level_token;
        char *message;
        com_util_trace_level_t level;
        int rc;

        level_token = next_token(&cursor);
        if (level_token == NULL)
        {
            print_command_usage(command);
            return -1;
        }
        if (!parse_trace_level(level_token, &level))
        {
            fprintf(stderr, "エラー: level が不正です。\n");
            return -1;
        }
        message = rest_argument(&cursor);
        if (message == NULL)
        {
            print_command_usage(command);
            return -1;
        }

        if (strcmp(command, "write") == 0)
        {
            rc = _com_util_tracer_write(session->handle, level, NULL, message);
        }
        else
        {
            rc = _com_util_tracer_writef(session->handle, level, NULL, "%s", message);
        }
        print_rc_result(rc);
        return 0;
    }

    if (strcmp(command, "write-hex") == 0 || strcmp(command, "write-hexf") == 0)
    {
        char *level_token;
        char *hex_token;
        char *label_cursor;
        char *label;
        com_util_trace_level_t level;
        unsigned char *data = NULL;
        size_t size = 0U;
        int rc;

        level_token = next_token(&cursor);
        hex_token = next_token(&cursor);
        if (level_token == NULL || hex_token == NULL)
        {
            print_command_usage(command);
            return -1;
        }
        if (!parse_trace_level(level_token, &level))
        {
            fprintf(stderr, "エラー: level が不正です。\n");
            return -1;
        }
        if (!parse_hex_bytes(hex_token, &data, &size))
        {
            fprintf(stderr, "エラー: hex は 16 進文字列で指定してください。\n");
            return -1;
        }

        label_cursor = cursor;
        label = rest_argument(&cursor);
        if (label == NULL && *skip_spaces(label_cursor) != '\0')
        {
            free(data);
            print_command_usage(command);
            return -1;
        }

        if (strcmp(command, "write-hex") == 0)
        {
            rc = _com_util_tracer_write_hex(session->handle, level, NULL, data, size, label);
        }
        else
        {
            const char *label_str;
            if (label != NULL)
            {
                label_str = label;
            }
            else
            {
                label_str = "";
            }
            rc = _com_util_tracer_write_hexf(session->handle, level, NULL, data, size, "%s", label_str);
        }
        print_rc_result(rc);
        free(data);
        return 0;
    }

    fprintf(stderr, "エラー: 不明なコマンドです: %s\n", command);
    return -1;
}

int main(int argc, char *argv[])
{
    trace_cli_session session;
    char line[TRACE_CLI_LINE_MAX];
    com_util_prompt *prompt;

    com_util_console_init();

    if (argc != 1)
    {
        fprintf(stderr, "使用方法: %s\n", argv[0]);
        fprintf(stderr, "起動後の対話入力で help を実行してください。\n");
        return EXIT_FAILURE;
    }

    trace_cli_session_init(&session);
    prompt = com_util_prompt_create(NULL);
    print_interactive_hint();

    while (!session.exit_requested)
    {
        if (com_util_prompt_readline_fmt(prompt, line, sizeof(line), "trace-cli[%s]> ",
                                         session_prompt_state_to_name(&session)) == 0)
        {
            break;
        }
        if (line[0] == '\0')
        {
            continue;
        }
        trace_cli_process_line(&session, line);
    }

    trace_cli_session_dispose(&session);
    com_util_prompt_dispose(prompt);
    return EXIT_SUCCESS;
}
