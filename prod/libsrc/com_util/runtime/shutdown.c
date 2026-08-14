/**
 *******************************************************************************
 *  @file           shutdown.c
 *  @brief          プロセスを終了する共通処理を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/05/06
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/result.h>
#include <com_util/crt/stdlib.h>
#include <com_util/runtime/shutdown.h>
#include <com_util/sync/sync.h>

#include <stdlib.h>

#if defined(PLATFORM_LINUX)
    #include <signal.h>
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
#endif

typedef struct shutdown_callback_entry
{
    com_util_shutdown_fn callback;
    void *context;
    struct shutdown_callback_entry *next;
} shutdown_callback_entry;

static shutdown_callback_entry *s_shutdown_callbacks = NULL;
static shutdown_callback_entry *s_shutdown_request_callbacks = NULL;
static com_util_local_lock *s_shutdown_lock;
static com_util_once_flag s_shutdown_lock_once = {0};
static com_util_once_flag s_shutdown_hook_once = {0};
static int s_shutdown_started = 0;
static int s_shutdown_request_started = 0;
/* com_util_exit はロックなしで書き、atexit 経路が読むため可視性を残す。 */
static volatile int s_exit_code = 0;
static volatile int s_exit_code_valid = 0;

static void init_shutdown_lock(void)
{
    (void)com_util_local_lock_create(&s_shutdown_lock);
}

static void shutdown_lock(void)
{
    com_util_call_once(&s_shutdown_lock_once, init_shutdown_lock);
    com_util_local_lock_lock(s_shutdown_lock, COM_UTIL_SYNC_WAIT_FOREVER);
}

static void shutdown_unlock(void)
{
    com_util_local_lock_unlock(s_shutdown_lock);
}

static com_util_shutdown_event make_normal_exit_event(void)
{
    com_util_shutdown_event event;

    event.reason = COM_UTIL_SHUTDOWN_REASON_NORMAL_EXIT;
    if (s_exit_code_valid)
    {
        event.code_kind = COM_UTIL_SHUTDOWN_CODE_KIND_EXIT_CODE;
    }
    else
    {
        event.code_kind = COM_UTIL_SHUTDOWN_CODE_KIND_NONE;
    }
    if (s_exit_code_valid)
    {
        event.code = s_exit_code;
    }
    else
    {
        event.code = 0;
    }
    return event;
}

static void free_callback_list(shutdown_callback_entry *entry)
{
    while (entry != NULL)
    {
        shutdown_callback_entry *next = entry->next;
        com_util_free(entry);
        entry = next;
    }
}

static int invoke_shutdown_callbacks_once(const com_util_shutdown_event *event)
{
    shutdown_callback_entry *callbacks;
    shutdown_callback_entry *entry;

    if (event == NULL)
    {
        return -1;
    }

    shutdown_lock();
    if (s_shutdown_started)
    {
        shutdown_unlock();
        return 1;
    }

    s_shutdown_started = 1;
    callbacks = s_shutdown_callbacks;
    s_shutdown_callbacks = NULL;
    shutdown_unlock();

    entry = callbacks;
    while (entry != NULL)
    {
        shutdown_callback_entry *next = entry->next;
        entry->callback(event, entry->context);
        com_util_free(entry);
        entry = next;
    }

    return 0;
}

static int invoke_shutdown_request_callbacks_once(const com_util_shutdown_event *event, int *handled_out)
{
    shutdown_callback_entry *callbacks;
    shutdown_callback_entry *entry;

    if (event == NULL)
    {
        return -1;
    }

    if (handled_out != NULL)
    {
        *handled_out = 0;
    }

    shutdown_lock();
    if (s_shutdown_started || s_shutdown_request_started)
    {
        shutdown_unlock();
        return 1;
    }

    s_shutdown_request_started = 1;
    callbacks = s_shutdown_request_callbacks;
    s_shutdown_request_callbacks = NULL;
    shutdown_unlock();

    if (handled_out != NULL && callbacks != NULL)
    {
        *handled_out = 1;
    }

    entry = callbacks;
    while (entry != NULL)
    {
        shutdown_callback_entry *next = entry->next;
        entry->callback(event, entry->context);
        com_util_free(entry);
        entry = next;
    }

    return 0;
}

static void shutdown_atexit_callback(void)
{
    com_util_shutdown_event event = make_normal_exit_event();
    (void)invoke_shutdown_callbacks_once(&event);
}

#if defined(PLATFORM_LINUX)
static void shutdown_signal_handler(int signal_number)
{
    com_util_shutdown_event event;
    int handled = 0;
    int request_result;

    event.reason = COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT;
    event.code_kind = COM_UTIL_SHUTDOWN_CODE_KIND_SIGNAL_NUMBER;
    event.code = signal_number;

    request_result = invoke_shutdown_request_callbacks_once(&event, &handled);
    if (request_result == 0 && handled)
    {
        return;
    }
    if (request_result == 1)
    {
        return;
    }

    (void)invoke_shutdown_callbacks_once(&event);

    signal(signal_number, SIG_DFL);
    raise(signal_number);
}
#elif defined(PLATFORM_WINDOWS)
static BOOL WINAPI shutdown_console_ctrl_handler(DWORD ctrl_type)
{
    com_util_shutdown_event event;
    int handled = 0;
    int request_result;

    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT)
    {
        event.reason = COM_UTIL_SHUTDOWN_REASON_SIGNAL_OR_CONSOLE_EVENT;
    }
    else
    {
        event.reason = COM_UTIL_SHUTDOWN_REASON_PROCESS_TERMINATING;
    }
    event.code_kind = COM_UTIL_SHUTDOWN_CODE_KIND_CONSOLE_CTRL_TYPE;
    event.code = (int)ctrl_type;

    request_result = invoke_shutdown_request_callbacks_once(&event, &handled);
    if (request_result == 0 && handled)
    {
        return TRUE;
    }
    if (request_result == 1)
    {
        return TRUE;
    }

    (void)invoke_shutdown_callbacks_once(&event);
    return FALSE;
}
#endif

static void install_shutdown_hooks(void)
{
    (void)atexit(shutdown_atexit_callback);

#if defined(PLATFORM_LINUX)
    signal(SIGINT, shutdown_signal_handler);
    signal(SIGTERM, shutdown_signal_handler);
#elif defined(PLATFORM_WINDOWS)
    SetConsoleCtrlHandler(shutdown_console_ctrl_handler, TRUE);
#endif
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_shutdown_register(com_util_shutdown_fn callback, void *context)
{
    shutdown_callback_entry *entry;
    int result = COM_UTIL_OK;

    if (callback == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    com_util_call_once(&s_shutdown_hook_once, install_shutdown_hooks);

    entry = (shutdown_callback_entry *)com_util_malloc(sizeof(*entry));
    if (entry == NULL)
    {
        return COM_UTIL_ERR_OUT_OF_MEMORY;
    }

    entry->callback = callback;
    entry->context = context;

    shutdown_lock();
    if (s_shutdown_started)
    {
        result = COM_UTIL_ERR_UNKNOWN;
    }
    else
    {
        entry->next = s_shutdown_callbacks;
        s_shutdown_callbacks = entry;
    }
    shutdown_unlock();

    if (result != COM_UTIL_OK)
    {
        com_util_free(entry);
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_exit(const int code)
{
    int effective_code = code;

    if (code < 0 || code > (COM_UTIL_EXIT_CODE_RESERVED_OUT_OF_RANGE - 1))
    {
        effective_code = COM_UTIL_EXIT_CODE_RESERVED_OUT_OF_RANGE;
    }

    s_exit_code = effective_code;
    s_exit_code_valid = 1;
    exit(effective_code);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_shutdown_request_register(com_util_shutdown_fn callback, void *context)
{
    shutdown_callback_entry *entry;
    int result = COM_UTIL_OK;

    if (callback == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    com_util_call_once(&s_shutdown_hook_once, install_shutdown_hooks);

    entry = (shutdown_callback_entry *)com_util_malloc(sizeof(*entry));
    if (entry == NULL)
    {
        return COM_UTIL_ERR_OUT_OF_MEMORY;
    }

    entry->callback = callback;
    entry->context = context;

    shutdown_lock();
    if (s_shutdown_started || s_shutdown_request_started)
    {
        result = COM_UTIL_ERR_UNKNOWN;
    }
    else
    {
        entry->next = s_shutdown_request_callbacks;
        s_shutdown_request_callbacks = entry;
    }
    shutdown_unlock();

    if (result != COM_UTIL_OK)
    {
        com_util_free(entry);
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_shutdown_invoke_for_test(const com_util_shutdown_event *event, int *invoked_out)
{
    int rc = invoke_shutdown_callbacks_once(event);

    if (rc < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (invoked_out != NULL)
    {
        if (rc == 0)
        {
            *invoked_out = 1;
        }
        else
        {
            *invoked_out = 0;
        }
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_shutdown_request_invoke_for_test(const com_util_shutdown_event *event, int *invoked_out)
{
    int rc = invoke_shutdown_request_callbacks_once(event, NULL);

    if (rc < 0)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    if (invoked_out != NULL)
    {
        if (rc == 0)
        {
            *invoked_out = 1;
        }
        else
        {
            *invoked_out = 0;
        }
    }
    return COM_UTIL_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_shutdown_reset_for_test(void)
{
    shutdown_callback_entry *shutdown_entry;
    shutdown_callback_entry *request_entry;

    shutdown_lock();
    shutdown_entry = s_shutdown_callbacks;
    s_shutdown_callbacks = NULL;
    request_entry = s_shutdown_request_callbacks;
    s_shutdown_request_callbacks = NULL;
    s_shutdown_started = 0;
    s_shutdown_request_started = 0;
    s_exit_code = 0;
    s_exit_code_valid = 0;
    shutdown_unlock();

    free_callback_list(shutdown_entry);
    free_callback_list(request_entry);
}
