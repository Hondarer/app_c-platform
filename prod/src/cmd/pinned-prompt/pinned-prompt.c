/**
 *******************************************************************************
 *  @file           pinned-prompt.c
 *  @brief          固定プロンプト API の動作を確認するコマンドを実装します。
 *
 *  入力、並行出力、ステータス領域を組み合わせた対話操作を提供します。
 *
 *******************************************************************************
 */

#include <com_util/argparser/argparser.h>
#include <com_util/console/console.h>
#include <com_util/crt/string.h>
#include <com_util/prompt/pinned_prompt.h>
#include <com_util/sync/sync.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PINNED_PROMPT_CLI_LINE_MAX 1024U
#define PINNED_PROMPT_CLI_TICK_MS  1000U

typedef struct pinned_prompt_cli_worker
{
    com_util_local_lock *mutex;
    com_util_condvar *condvar;
    com_util_thread *thread;

    com_util_pinned_prompt *screen;
    const char *name;

    uint64_t tick_count;

    com_util_pinned_prompt_channel channel;
    int sync_initialized;
    int thread_running;
    int stop_requested;
} pinned_prompt_cli_worker;

typedef struct pinned_prompt_cli_session
{
    com_util_pinned_prompt *screen;
    pinned_prompt_cli_worker stdout_worker;
    pinned_prompt_cli_worker stderr_worker;
    int exit_requested;
    int reserved;
} pinned_prompt_cli_session;

static char *skip_spaces(char *s)
{
    while (*s == ' ' || *s == '\t')
    {
        s++;
    }
    return s;
}

static int split_command(char *line, char **command, char **arg)
{
    char *cursor;

    cursor = line;
    cursor = skip_spaces(cursor);
    if (*cursor == '\0')
    {
        *command = NULL;
        *arg = NULL;
        return 0;
    }

    *command = cursor;
    while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t')
    {
        cursor++;
    }
    if (*cursor != '\0')
    {
        *cursor = '\0';
        cursor++;
    }
    cursor = skip_spaces(cursor);
    if (*cursor != '\0')
    {
        *arg = cursor;
    }
    else
    {
        *arg = NULL;
    }
    return 0;
}

static void decode_cli_escapes(char *s)
{
    char *read_pos;
    char *write_pos;

    if (s == NULL)
    {
        return;
    }

    read_pos = s;
    write_pos = s;
    while (*read_pos != '\0')
    {
        if (*read_pos == '\\')
        {
            read_pos++;
            if (*read_pos == 'e' || *read_pos == 'E')
            {
                *write_pos = '\033';
                write_pos++;
                read_pos++;
                continue;
            }
            if (*read_pos == '\\')
            {
                *write_pos = '\\';
                write_pos++;
                read_pos++;
                continue;
            }
            *write_pos = '\\';
            write_pos++;
            if (*read_pos == '\0')
            {
                break;
            }
        }
        *write_pos = *read_pos;
        write_pos++;
        read_pos++;
    }
    *write_pos = '\0';
}

static void print_help(com_util_pinned_prompt *screen)
{
    com_util_pinned_prompt_printf(screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                  "commands:\n"
                                  "  help          show this help\n"
                                  "  echo TEXT     write TEXT to stdout (\\e emits ESC)\n"
                                  "  read primary  read one line with primary history\n"
                                  "  read secondary read one line with secondary history\n"
                                  "  read formatted read one line with formatted prompt\n"
                                  "  start stdout  start stdout tick output\n"
                                  "  start stderr  start stderr tick output\n"
                                  "  stop stdout   stop stdout tick output\n"
                                  "  stop stderr   stop stderr tick output\n"
                                  "  stop all      stop all tick output\n"
                                  "  status show [top|bottom|all]    show status area(s)\n"
                                  "  status hide [top|bottom|all]    hide status area(s)\n"
                                  "  status set-top-left TEXT    set top-left status (\\e emits ESC)\n"
                                  "  status set-top-right TEXT   set top-right status (\\e emits ESC)\n"
                                  "  status set-bottom-left TEXT    set bottom-left status (\\e emits ESC)\n"
                                  "  status set-bottom-right TEXT   set bottom-right status (\\e emits ESC)\n"
                                  "  exit, quit    exit pinned-prompt\n");
}

static void worker_init(pinned_prompt_cli_worker *worker, com_util_pinned_prompt *screen,
                        com_util_pinned_prompt_channel channel, const char *name)
{
    memset(worker, 0, sizeof(*worker));
    worker->screen = screen;
    worker->channel = channel;
    worker->name = name;
}

static int worker_ensure_sync(pinned_prompt_cli_worker *worker)
{
    if (worker->sync_initialized)
    {
        return 0;
    }
    if (com_util_local_lock_create(&worker->mutex) != COM_UTIL_OK)
    {
        return -1;
    }
    if (com_util_condvar_create(&worker->condvar) != COM_UTIL_OK)
    {
        (void)com_util_local_lock_destroy(worker->mutex);
        return -1;
    }
    worker->sync_initialized = 1;
    return 0;
}

static void worker_thread_proc(void *arg)
{
    pinned_prompt_cli_worker *worker;
    int stop;

    worker = (pinned_prompt_cli_worker *)arg;
    (void)com_util_local_lock_lock(worker->mutex, COM_UTIL_SYNC_WAIT_FOREVER);
    for (;;)
    {
        if (worker->stop_requested)
        {
            break;
        }

        (void)com_util_condvar_wait(worker->condvar, worker->mutex, PINNED_PROMPT_CLI_TICK_MS);
        if (worker->stop_requested)
        {
            break;
        }

        worker->tick_count++;
        stop = worker->stop_requested;
        (void)com_util_local_lock_unlock(worker->mutex);

        if (!stop)
        {
            com_util_pinned_prompt_printf(worker->screen, worker->channel, "[%s tick %llu]\n", worker->name,
                                          (unsigned long long)worker->tick_count);
        }

        (void)com_util_local_lock_lock(worker->mutex, COM_UTIL_SYNC_WAIT_FOREVER);
    }
    worker->thread_running = 0;
    (void)com_util_local_lock_unlock(worker->mutex);
}

static void worker_start(pinned_prompt_cli_worker *worker)
{
    int running;
    int started;
    int failed;

    if (worker_ensure_sync(worker) != 0)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "failed to initialize %s worker\n", worker->name);
        return;
    }

    started = 0;
    failed = 0;
    (void)com_util_local_lock_lock(worker->mutex, COM_UTIL_SYNC_WAIT_FOREVER);
    running = worker->thread_running;
    if (!running)
    {
        worker->stop_requested = 0;
        worker->tick_count = 0U;
        if (com_util_thread_create(&worker->thread, worker_thread_proc, worker) == COM_UTIL_OK)
        {
            worker->thread_running = 1;
            started = 1;
        }
        else
        {
            failed = 1;
        }
    }
    (void)com_util_local_lock_unlock(worker->mutex);

    if (running)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                      "%s worker already running\n", worker->name);
    }
    else if (started)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s worker started\n",
                                      worker->name);
    }
    else if (failed)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "failed to start %s worker\n", worker->name);
    }
}

static void worker_stop(pinned_prompt_cli_worker *worker, int announce)
{
    int running;

    if (!worker->sync_initialized)
    {
        if (announce)
        {
            com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "%s worker not running\n", worker->name);
        }
        return;
    }

    (void)com_util_local_lock_lock(worker->mutex, COM_UTIL_SYNC_WAIT_FOREVER);
    running = worker->thread_running;
    if (running)
    {
        worker->stop_requested = 1;
        (void)com_util_condvar_signal(worker->condvar);
    }
    (void)com_util_local_lock_unlock(worker->mutex);

    if (running)
    {
        com_util_thread_join(worker->thread, COM_UTIL_SYNC_WAIT_FOREVER);
        if (announce)
        {
            com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s worker stopped\n",
                                          worker->name);
        }
    }
    else if (announce)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s worker not running\n",
                                      worker->name);
    }
}

static void worker_dispose(pinned_prompt_cli_worker *worker)
{
    if (worker->sync_initialized)
    {
        worker_stop(worker, 0);
        (void)com_util_condvar_destroy(worker->condvar);
        (void)com_util_local_lock_destroy(worker->mutex);
        worker->sync_initialized = 0;
    }
}

static pinned_prompt_cli_worker *find_worker(pinned_prompt_cli_session *session, const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }
    if (strcmp(name, "stdout") == 0)
    {
        return &session->stdout_worker;
    }
    if (strcmp(name, "stderr") == 0)
    {
        return &session->stderr_worker;
    }
    return NULL;
}

static void process_start(pinned_prompt_cli_session *session, const char *arg)
{
    pinned_prompt_cli_worker *worker;

    worker = find_worker(session, arg);
    if (worker == NULL)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "usage: start stdout|stderr\n");
        return;
    }
    worker_start(worker);
}

static void process_stop(pinned_prompt_cli_session *session, const char *arg)
{
    pinned_prompt_cli_worker *worker;

    if (arg != NULL && strcmp(arg, "all") == 0)
    {
        worker_stop(&session->stdout_worker, 1);
        worker_stop(&session->stderr_worker, 1);
        return;
    }

    worker = find_worker(session, arg);
    if (worker == NULL)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "usage: stop stdout|stderr|all\n");
        return;
    }
    worker_stop(worker, 1);
}

static void process_status(pinned_prompt_cli_session *session, const char *arg)
{
    char *subcmd;
    char *subarg;
    char arg_copy[PINNED_PROMPT_CLI_LINE_MAX];

    if (arg == NULL)
    {
        com_util_pinned_prompt_printf(
            session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
            "usage: status show|hide|set-top-left|set-top-right|set-bottom-left|set-bottom-right\n");
        return;
    }

    (void)com_util_strcpy(arg_copy, sizeof(arg_copy), arg);
    (void)split_command(arg_copy, &subcmd, &subarg);

    if (strcmp(subcmd, "show") == 0)
    {
        if (subarg == NULL || strcmp(subarg, "all") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, 1);
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                       1);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "status areas enabled\n");
        }
        else if (strcmp(subarg, "top") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, 1);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "top status area enabled\n");
        }
        else if (strcmp(subarg, "bottom") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                       1);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "bottom status area enabled\n");
        }
        else
        {
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                          "usage: status show [top|bottom|all]\n");
        }
    }
    else if (strcmp(subcmd, "hide") == 0)
    {
        if (subarg == NULL || strcmp(subarg, "all") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, 0);
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                       0);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "status areas disabled\n");
        }
        else if (strcmp(subarg, "top") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP, 0);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "top status area disabled\n");
        }
        else if (strcmp(subarg, "bottom") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                       0);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT,
                                          "bottom status area disabled\n");
        }
        else
        {
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                          "usage: status hide [top|bottom|all]\n");
        }
    }
    else if (strcmp(subcmd, "set-top-left") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP,
                                                COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT, subarg);
    }
    else if (strcmp(subcmd, "set-top-right") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP,
                                                COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_RIGHT, subarg);
    }
    else if (strcmp(subcmd, "set-bottom-left") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT, subarg);
    }
    else if (strcmp(subcmd, "set-bottom-right") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen, COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM,
                                                COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_RIGHT, subarg);
    }
    else
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "unknown status subcommand: %s\n", subcmd);
    }
}

static int read_primary_history(com_util_pinned_prompt *screen, char *buf, size_t buf_size)
{
    return com_util_pinned_prompt_readline(screen, buf, buf_size, "primary> ");
}

static int read_secondary_history(com_util_pinned_prompt *screen, char *buf, size_t buf_size)
{
    return com_util_pinned_prompt_readline(screen, buf, buf_size, "secondary> ");
}

static int read_formatted_history(com_util_pinned_prompt *screen, char *buf, size_t buf_size)
{
    return com_util_pinned_prompt_readline_fmt(screen, buf, buf_size, "%s> ", "formatted");
}

static void process_read(pinned_prompt_cli_session *session, const char *arg)
{
    char buf[PINNED_PROMPT_CLI_LINE_MAX];
    int ret;

    if (arg == NULL)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "usage: read primary|secondary|formatted\n");
        return;
    }

    if (strcmp(arg, "primary") == 0)
    {
        ret = read_primary_history(session->screen, buf, sizeof(buf));
    }
    else if (strcmp(arg, "secondary") == 0)
    {
        ret = read_secondary_history(session->screen, buf, sizeof(buf));
    }
    else if (strcmp(arg, "formatted") == 0)
    {
        ret = read_formatted_history(session->screen, buf, sizeof(buf));
    }
    else
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR,
                                      "usage: read primary|secondary|formatted\n");
        return;
    }

    if (ret == COM_UTIL_OK)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "read %s: %s\n", arg,
                                      buf);
    }
    else
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "read %s cancelled\n",
                                      arg);
    }
}

static void process_line(pinned_prompt_cli_session *session, char *line)
{
    char original_line[PINNED_PROMPT_CLI_LINE_MAX];
    char *command;
    char *arg;

    (void)com_util_strcpy(original_line, sizeof(original_line), line);

    (void)split_command(line, &command, &arg);
    if (command == NULL)
    {
        return;
    }

    com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "echo: %s\n", original_line);

    if (strcmp(command, "help") == 0)
    {
        print_help(session->screen);
    }
    else if (strcmp(command, "echo") == 0)
    {
        decode_cli_escapes(arg);
        {
            const char *echo_str;
            if (arg != NULL)
            {
                echo_str = arg;
            }
            else
            {
                echo_str = "";
            }
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "%s\n", echo_str);
        }
    }
    else if (strcmp(command, "start") == 0)
    {
        process_start(session, arg);
    }
    else if (strcmp(command, "stop") == 0)
    {
        process_stop(session, arg);
    }
    else if (strcmp(command, "status") == 0)
    {
        decode_cli_escapes(arg);
        process_status(session, arg);
    }
    else if (strcmp(command, "read") == 0)
    {
        process_read(session, arg);
    }
    else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
    {
        session->exit_requested = 1;
    }
    else
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR, "unknown command: %s\n",
                                      command);
    }
}

static int session_init(pinned_prompt_cli_session *session)
{
    memset(session, 0, sizeof(*session));
    session->screen = com_util_pinned_prompt_create(NULL);
    if (session->screen == NULL)
    {
        fprintf(stderr, "pinned-prompt の作成に失敗しました。\n");
        return -1;
    }

    worker_init(&session->stdout_worker, session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT, "stdout");
    worker_init(&session->stderr_worker, session->screen, COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR, "stderr");
    return 0;
}

static void session_dispose(pinned_prompt_cli_session *session)
{
    worker_dispose(&session->stdout_worker);
    worker_dispose(&session->stderr_worker);
    com_util_pinned_prompt_dispose(session->screen);
    session->screen = NULL;
}

int main(int argc, char *argv[])
{
    pinned_prompt_cli_session session;
    char line[PINNED_PROMPT_CLI_LINE_MAX];
    int ret;

    com_util_console_init();

    int need_help = 0;

    com_util_argparser_default_init("固定プロンプト API を対話的に確認します。");
    com_util_argparser_default_register_flag("-h", "--help", "ヘルプを表示します。", &need_help);

    if (com_util_argparser_default_get_register_error_count() > 0)
    {
        com_util_argparser_default_print_register_error_messages(stderr);
        return EXIT_FAILURE;
    }

    int parse_result = com_util_argparser_default_parse(argc, argv);

    if (need_help != 0)
    {
        com_util_argparser_default_print_usage(stdout);
        return EXIT_SUCCESS;
    }

    if (parse_result != COM_UTIL_OK)
    {
        com_util_argparser_default_print_error_messages(stderr);
        com_util_argparser_default_print_usage(stderr);
        return EXIT_FAILURE;
    }

    if (session_init(&session) != 0)
    {
        return EXIT_FAILURE;
    }

    print_help(session.screen);
    while (!session.exit_requested)
    {
        ret = com_util_pinned_prompt_readline(session.screen, line, sizeof(line), "pinned-prompt> ");
        if (ret != COM_UTIL_OK)
        {
            break;
        }
        process_line(&session, line);
    }

    session_dispose(&session);
    return EXIT_SUCCESS;
}
