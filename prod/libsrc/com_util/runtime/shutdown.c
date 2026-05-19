/**
 *******************************************************************************
 *  @file           shutdown.c
 *  @brief          プロセス終了ハンドリング共通実装。
 *  @author         Tetsuo Honda
 *  @date           2026/05/06
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

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
    com_util_shutdown_callback_t callback;
    void *context;
    struct shutdown_callback_entry *next;
} shutdown_callback_entry_t;

static shutdown_callback_entry_t *s_shutdown_callbacks = NULL;
static shutdown_callback_entry_t *s_shutdown_request_callbacks = NULL;
static com_util_local_lock_t *s_shutdown_lock;
static com_util_once_flag_t s_shutdown_lock_once = {0};
static com_util_once_flag_t s_shutdown_hook_once = {0};
static volatile int s_shutdown_started = 0;
static volatile int s_shutdown_request_started = 0;
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

static com_util_shutdown_event_t make_normal_exit_event(void)
{
    com_util_shutdown_event_t event;

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

static void free_callback_list(shutdown_callback_entry_t *entry)
{
    while (entry != NULL)
    {
        shutdown_callback_entry_t *next = entry->next;
        free(entry);
        entry = next;
    }
}

static int invoke_shutdown_callbacks_once(const com_util_shutdown_event_t *event)
{
    shutdown_callback_entry_t *callbacks;
    shutdown_callback_entry_t *entry;

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
        shutdown_callback_entry_t *next = entry->next;
        entry->callback(event, entry->context);
        free(entry);
        entry = next;
    }

    return 0;
}

static int invoke_shutdown_request_callbacks_once(const com_util_shutdown_event_t *event, int *handled_out)
{
    shutdown_callback_entry_t *callbacks;
    shutdown_callback_entry_t *entry;

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
        shutdown_callback_entry_t *next = entry->next;
        entry->callback(event, entry->context);
        free(entry);
        entry = next;
    }

    return 0;
}

static void shutdown_atexit_callback(void)
{
    com_util_shutdown_event_t event = make_normal_exit_event();
    (void)invoke_shutdown_callbacks_once(&event);
}

#if defined(PLATFORM_LINUX)
static void shutdown_signal_handler(int signal_number)
{
    com_util_shutdown_event_t event;
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
    com_util_shutdown_event_t event;
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

COM_UTIL_EXPORT int COM_UTIL_API
    com_util_shutdown_register(com_util_shutdown_callback_t callback, void *context)
{
    shutdown_callback_entry_t *entry;
    int result = 0;

    if (callback == NULL)
    {
        return -1;
    }

    com_util_call_once(&s_shutdown_hook_once, install_shutdown_hooks);

    entry = (shutdown_callback_entry_t *)malloc(sizeof(*entry));
    if (entry == NULL)
    {
        return -1;
    }

    entry->callback = callback;
    entry->context = context;

    shutdown_lock();
    if (s_shutdown_started)
    {
        result = -1;
    }
    else
    {
        entry->next = s_shutdown_callbacks;
        s_shutdown_callbacks = entry;
    }
    shutdown_unlock();

    if (result != 0)
    {
        free(entry);
    }

    return result;
}

COM_UTIL_EXPORT void COM_UTIL_API com_util_exit(const int code)
{
    s_exit_code = code;
    s_exit_code_valid = 1;
    exit(code);
}

COM_UTIL_EXPORT int COM_UTIL_API
    com_util_shutdown_request_register(com_util_shutdown_callback_t callback, void *context)
{
    shutdown_callback_entry_t *entry;
    int result = 0;

    if (callback == NULL)
    {
        return -1;
    }

    com_util_call_once(&s_shutdown_hook_once, install_shutdown_hooks);

    entry = (shutdown_callback_entry_t *)malloc(sizeof(*entry));
    if (entry == NULL)
    {
        return -1;
    }

    entry->callback = callback;
    entry->context = context;

    shutdown_lock();
    if (s_shutdown_started || s_shutdown_request_started)
    {
        result = -1;
    }
    else
    {
        entry->next = s_shutdown_request_callbacks;
        s_shutdown_request_callbacks = entry;
    }
    shutdown_unlock();

    if (result != 0)
    {
        free(entry);
    }

    return result;
}

COM_UTIL_EXPORT int COM_UTIL_API
    _com_util_shutdown_invoke_for_test(const com_util_shutdown_event_t *event)
{
    return invoke_shutdown_callbacks_once(event);
}

COM_UTIL_EXPORT int COM_UTIL_API
    _com_util_shutdown_request_invoke_for_test(const com_util_shutdown_event_t *event)
{
    return invoke_shutdown_request_callbacks_once(event, NULL);
}

COM_UTIL_EXPORT void COM_UTIL_API _com_util_shutdown_reset_for_test(void)
{
    shutdown_callback_entry_t *shutdown_entry;
    shutdown_callback_entry_t *request_entry;

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
