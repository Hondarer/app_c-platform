#include <com_util/console/console.h>
#include <com_util/crt/string.h>
#include <com_util/prompt/pinned_prompt.h>
#include <com_util/sync/sync.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PINNED_PROMPT_CLI_LINE_MAX 1024U
#define PINNED_PROMPT_CLI_TICK_MS 1000U

typedef struct
{
    com_util_mutex_t mutex;
    com_util_condvar_t condvar;
    com_util_thread_t thread;

    com_util_pinned_prompt_t *screen;
    const char *name;

    uint64_t tick_count;

    com_util_pinned_prompt_channel_t channel;
    int sync_initialized;
    int thread_running;
    int stop_requested;
} pinned_prompt_cli_worker_t;

typedef struct
{
    com_util_pinned_prompt_t *screen;
    pinned_prompt_cli_worker_t stdout_worker;
    pinned_prompt_cli_worker_t stderr_worker;
    int exit_requested;
    int reserved;
} pinned_prompt_cli_session_t;

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
    *arg = (*cursor != '\0') ? cursor : NULL;
    return 0;
}

static void print_usage(const char *argv0)
{
    fprintf(stderr, "使用方法: %s\n", argv0);
    fprintf(stderr, "起動後の対話入力で help を実行してください。\n");
}

static void print_help(com_util_pinned_prompt_t *screen)
{
    com_util_pinned_prompt_printf(screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                  "commands:\n"
                                  "  help          show this help\n"
                                  "  echo TEXT     write TEXT to stdout\n"
                                  "  start stdout  start stdout tick output\n"
                                  "  start stderr  start stderr tick output\n"
                                  "  stop stdout   stop stdout tick output\n"
                                  "  stop stderr   stop stderr tick output\n"
                                  "  stop all      stop all tick output\n"
                                  "  status show [top|bottom|all]    show status area(s)\n"
                                  "  status hide [top|bottom|all]    hide status area(s)\n"
                                  "  status set-top-left TEXT    set top-left status\n"
                                  "  status set-top-right TEXT   set top-right status\n"
                                  "  status set-bottom-left TEXT    set bottom-left status\n"
                                  "  status set-bottom-right TEXT   set bottom-right status\n"
                                  "  exit, quit    exit pinned-prompt\n");
}

static void worker_init(pinned_prompt_cli_worker_t *worker,
                        com_util_pinned_prompt_t *screen,
                        com_util_pinned_prompt_channel_t channel,
                        const char *name)
{
    memset(worker, 0, sizeof(*worker));
    worker->screen = screen;
    worker->channel = channel;
    worker->name = name;
}

static int worker_ensure_sync(pinned_prompt_cli_worker_t *worker)
{
    if (worker->sync_initialized)
    {
        return 0;
    }
    if (com_util_mutex_init(&worker->mutex) != 0)
    {
        return -1;
    }
    if (com_util_condvar_init(&worker->condvar) != 0)
    {
        (void)com_util_mutex_destroy(&worker->mutex);
        return -1;
    }
    worker->sync_initialized = 1;
    return 0;
}

static void worker_thread_proc(void *arg)
{
    pinned_prompt_cli_worker_t *worker;
    int stop;

    worker = (pinned_prompt_cli_worker_t *)arg;
    (void)com_util_mutex_lock(&worker->mutex);
    for (;;)
    {
        if (worker->stop_requested)
        {
            break;
        }

        (void)com_util_condvar_timedwait(&worker->condvar, &worker->mutex,
                                         PINNED_PROMPT_CLI_TICK_MS);
        if (worker->stop_requested)
        {
            break;
        }

        worker->tick_count++;
        stop = worker->stop_requested;
        (void)com_util_mutex_unlock(&worker->mutex);

        if (!stop)
        {
            com_util_pinned_prompt_printf(worker->screen, worker->channel,
                                          "[%s tick %llu]\n",
                                          worker->name,
                                          (unsigned long long)worker->tick_count);
        }

        (void)com_util_mutex_lock(&worker->mutex);
    }
    worker->thread_running = 0;
    (void)com_util_mutex_unlock(&worker->mutex);
}

static void worker_start(pinned_prompt_cli_worker_t *worker)
{
    int running;
    int started;
    int failed;

    if (worker_ensure_sync(worker) != 0)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "failed to initialize %s worker\n", worker->name);
        return;
    }

    started = 0;
    failed = 0;
    (void)com_util_mutex_lock(&worker->mutex);
    running = worker->thread_running;
    if (!running)
    {
        worker->stop_requested = 0;
        worker->tick_count = 0U;
        if (com_util_thread_create(&worker->thread, worker_thread_proc, worker) == 0)
        {
            worker->thread_running = 1;
            started = 1;
        }
        else
        {
            failed = 1;
        }
    }
    (void)com_util_mutex_unlock(&worker->mutex);

    if (running)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                      "%s worker already running\n", worker->name);
    }
    else if (started)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                      "%s worker started\n", worker->name);
    }
    else if (failed)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "failed to start %s worker\n", worker->name);
    }
}

static void worker_stop(pinned_prompt_cli_worker_t *worker, int announce)
{
    int running;

    if (!worker->sync_initialized)
    {
        if (announce)
        {
            com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "%s worker not running\n", worker->name);
        }
        return;
    }

    (void)com_util_mutex_lock(&worker->mutex);
    running = worker->thread_running;
    if (running)
    {
        worker->stop_requested = 1;
        (void)com_util_condvar_signal(&worker->condvar);
    }
    (void)com_util_mutex_unlock(&worker->mutex);

    if (running)
    {
        com_util_thread_join(&worker->thread);
        if (announce)
        {
            com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "%s worker stopped\n", worker->name);
        }
    }
    else if (announce)
    {
        com_util_pinned_prompt_printf(worker->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                      "%s worker not running\n", worker->name);
    }
}

static void worker_dispose(pinned_prompt_cli_worker_t *worker)
{
    if (worker->sync_initialized)
    {
        worker_stop(worker, 0);
        (void)com_util_condvar_destroy(&worker->condvar);
        (void)com_util_mutex_destroy(&worker->mutex);
        worker->sync_initialized = 0;
    }
}

static pinned_prompt_cli_worker_t *find_worker(pinned_prompt_cli_session_t *session,
                                               const char *name)
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

static void process_start(pinned_prompt_cli_session_t *session, const char *arg)
{
    pinned_prompt_cli_worker_t *worker;

    worker = find_worker(session, arg);
    if (worker == NULL)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "usage: start stdout|stderr\n");
        return;
    }
    worker_start(worker);
}

static void process_stop(pinned_prompt_cli_session_t *session, const char *arg)
{
    pinned_prompt_cli_worker_t *worker;

    if (arg != NULL && strcmp(arg, "all") == 0)
    {
        worker_stop(&session->stdout_worker, 1);
        worker_stop(&session->stderr_worker, 1);
        return;
    }

    worker = find_worker(session, arg);
    if (worker == NULL)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "usage: stop stdout|stderr|all\n");
        return;
    }
    worker_stop(worker, 1);
}

static void process_status(pinned_prompt_cli_session_t *session, const char *arg)
{
    char *subcmd;
    char *subarg;
    char  arg_copy[PINNED_PROMPT_CLI_LINE_MAX];

    if (arg == NULL)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "usage: status show|hide|set-top-left|set-top-right|set-bottom-left|set-bottom-right\n");
        return;
    }

    (void)com_util_strcpy(arg_copy, sizeof(arg_copy), arg);
    (void)split_command(arg_copy, &subcmd, &subarg);

    if (strcmp(subcmd, "show") == 0)
    {
        if (subarg == NULL || strcmp(subarg, "all") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_TOP,
                                                        1);
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM,
                                                        1);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "status areas enabled\n");
        }
        else if (strcmp(subarg, "top") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_TOP,
                                                        1);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "top status area enabled\n");
        }
        else if (strcmp(subarg, "bottom") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM,
                                                        1);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "bottom status area enabled\n");
        }
        else
        {
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                          "usage: status show [top|bottom|all]\n");
        }
    }
    else if (strcmp(subcmd, "hide") == 0)
    {
        if (subarg == NULL || strcmp(subarg, "all") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_TOP,
                                                        0);
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM,
                                                        0);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "status areas disabled\n");
        }
        else if (strcmp(subarg, "top") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_TOP,
                                                        0);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "top status area disabled\n");
        }
        else if (strcmp(subarg, "bottom") == 0)
        {
            (void)com_util_pinned_prompt_status_enable(session->screen,
                                                        COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM,
                                                        0);
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                          "bottom status area disabled\n");
        }
        else
        {
            com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                          "usage: status hide [top|bottom|all]\n");
        }
    }
    else if (strcmp(subcmd, "set-top-left") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen,
                                                COM_UTIL_PINNED_PROMPT_STATUS_TOP,
                                                COM_UTIL_PINNED_PROMPT_STATUS_LEFT,
                                                subarg);
    }
    else if (strcmp(subcmd, "set-top-right") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen,
                                                COM_UTIL_PINNED_PROMPT_STATUS_TOP,
                                                COM_UTIL_PINNED_PROMPT_STATUS_RIGHT,
                                                subarg);
    }
    else if (strcmp(subcmd, "set-bottom-left") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen,
                                                COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM,
                                                COM_UTIL_PINNED_PROMPT_STATUS_LEFT,
                                                subarg);
    }
    else if (strcmp(subcmd, "set-bottom-right") == 0)
    {
        (void)com_util_pinned_prompt_status_set(session->screen,
                                                COM_UTIL_PINNED_PROMPT_STATUS_BOTTOM,
                                                COM_UTIL_PINNED_PROMPT_STATUS_RIGHT,
                                                subarg);
    }
    else
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "unknown status subcommand: %s\n", subcmd);
    }
}

static void process_line(pinned_prompt_cli_session_t *session, char *line)
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

    com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                  "echo: %s\n", original_line);

    if (strcmp(command, "help") == 0)
    {
        print_help(session->screen);
    }
    else if (strcmp(command, "echo") == 0)
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDOUT,
                                      "%s\n", arg != NULL ? arg : "");
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
        process_status(session, arg);
    }
    else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
    {
        session->exit_requested = 1;
    }
    else
    {
        com_util_pinned_prompt_printf(session->screen, COM_UTIL_PINNED_PROMPT_STDERR,
                                      "unknown command: %s\n", command);
    }
}

static int session_init(pinned_prompt_cli_session_t *session)
{
    memset(session, 0, sizeof(*session));
    session->screen = com_util_pinned_prompt_create(NULL);
    if (session->screen == NULL)
    {
        fprintf(stderr, "pinned-prompt の作成に失敗しました。\n");
        return -1;
    }

    worker_init(&session->stdout_worker, session->screen,
                COM_UTIL_PINNED_PROMPT_STDOUT, "stdout");
    worker_init(&session->stderr_worker, session->screen,
                COM_UTIL_PINNED_PROMPT_STDERR, "stderr");
    return 0;
}

static void session_dispose(pinned_prompt_cli_session_t *session)
{
    worker_dispose(&session->stdout_worker);
    worker_dispose(&session->stderr_worker);
    com_util_pinned_prompt_dispose(session->screen);
    session->screen = NULL;
}

int main(int argc, char *argv[])
{
    pinned_prompt_cli_session_t session;
    char line[PINNED_PROMPT_CLI_LINE_MAX];
    int rc;

    com_util_console_init();

    if (argc != 1)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (session_init(&session) != 0)
    {
        return EXIT_FAILURE;
    }

    print_help(session.screen);
    while (!session.exit_requested)
    {
        rc = com_util_pinned_prompt_readline(session.screen, line, sizeof(line),
                                             "pinned-prompt> ");
        if (rc == 0)
        {
            break;
        }
        process_line(&session, line);
    }

    session_dispose(&session);
    return EXIT_SUCCESS;
}
