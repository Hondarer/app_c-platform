/**
 *******************************************************************************
 *  @file           process.c
 *  @brief          プロセス情報取得・子プロセス起動 実装。
 *  @author         Tetsuo Honda
 *  @date           2026/06/07
 *  @version        1.1.0
 *
 *  現在のプロセスの実行ファイル本体の絶対パス取得と、子プロセス起動を提供します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/runtime/process.h>
#include <com_util/crt/path.h>
#include <com_util/crt/wchar_conv.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <time.h>
    #include <unistd.h>

extern char **environ;
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
    #include <shellapi.h>
    #include <wchar.h>
    #pragma comment(lib, "Shell32.lib")
#endif /* PLATFORM_ */

/* Doxygen コメントは、ヘッダーに記載 */

struct com_util_process
{
#if defined(PLATFORM_LINUX)
    pid_t pid;
#elif defined(PLATFORM_WINDOWS)
    HANDLE process;
#endif /* PLATFORM_ */
    int exited;
    int exit_code;
};

static int has_valid_argv(char *const *argv)
{
    if (argv == NULL)
    {
        return 0;
    }
    if (argv[0] == NULL)
    {
        return 0;
    }
    if (argv[0][0] == '\0')
    {
        return 0;
    }
    return 1;
}

static void process_set_exit_code(com_util_process *process, const int exit_code)
{
    process->exited = 1;
    process->exit_code = exit_code;
}

#if defined(PLATFORM_LINUX)
static uint64_t monotonic_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static char *string_duplicate(const char *text)
{
    char *copy;
    size_t len;

    if (text == NULL)
    {
        return NULL;
    }

    len = strlen(text) + 1;
    copy = (char *)malloc(len);
    if (copy != NULL)
    {
        memcpy(copy, text, len);
    }
    return copy;
}

static size_t env_key_len(const char *entry)
{
    const char *sep;

    sep = strchr(entry, '=');
    if (sep == NULL)
    {
        return 0;
    }
    return (size_t)(sep - entry);
}

static int env_key_matches(const char *entry, const char *key, const size_t key_len)
{
    if (strncmp(entry, key, key_len) != 0)
    {
        return 0;
    }
    if (entry[key_len] != '=')
    {
        return 0;
    }
    return 1;
}

static void free_envp(char **envp)
{
    size_t i;

    if (envp == NULL)
    {
        return;
    }

    i = 0;
    while (envp[i] != NULL)
    {
        free(envp[i]);
        i++;
    }
    free(envp);
}

static int set_env_entry(char **envp, const size_t capacity, const char *entry)
{
    size_t key_len;
    size_t i;
    char *copy;

    key_len = env_key_len(entry);
    if (key_len == 0)
    {
        return -1;
    }

    copy = string_duplicate(entry);
    if (copy == NULL)
    {
        return -1;
    }

    i = 0;
    while (envp[i] != NULL)
    {
        if (env_key_matches(envp[i], entry, key_len) != 0)
        {
            free(envp[i]);
            envp[i] = copy;
            return 0;
        }
        i++;
    }

    if (i + 1 >= capacity)
    {
        free(copy);
        return -1;
    }

    envp[i] = copy;
    envp[i + 1] = NULL;
    return 0;
}

static char **build_environment(char *const *overrides)
{
    size_t env_count;
    size_t override_count;
    size_t capacity;
    size_t i;
    char **envp;

    env_count = 0;
    while (environ[env_count] != NULL)
    {
        env_count++;
    }

    override_count = 0;
    if (overrides != NULL)
    {
        while (overrides[override_count] != NULL)
        {
            if (env_key_len(overrides[override_count]) == 0)
            {
                return NULL;
            }
            override_count++;
        }
    }

    capacity = env_count + override_count + 1;
    envp = (char **)calloc(capacity, sizeof(*envp));
    if (envp == NULL)
    {
        return NULL;
    }

    i = 0;
    while (i < env_count)
    {
        envp[i] = string_duplicate(environ[i]);
        if (envp[i] == NULL)
        {
            free_envp(envp);
            return NULL;
        }
        i++;
    }
    envp[i] = NULL;

    i = 0;
    while (overrides != NULL && overrides[i] != NULL)
    {
        if (set_env_entry(envp, capacity, overrides[i]) != 0)
        {
            free_envp(envp);
            return NULL;
        }
        i++;
    }

    return envp;
}

static const char *find_env_value(char *const *envp, const char *key)
{
    size_t key_len;
    size_t i;

    key_len = strlen(key);
    i = 0;
    while (envp != NULL && envp[i] != NULL)
    {
        if (env_key_matches(envp[i], key, key_len) != 0)
        {
            return envp[i] + key_len + 1;
        }
        i++;
    }
    return NULL;
}

static int setup_child_stdio_one(const com_util_process_stdio_t *spec, const int target_fd, const int null_flags)
{
    int source_fd;

    if (spec->mode == COM_UTIL_PROCESS_STDIO_INHERIT)
    {
        return 0;
    }

    if (spec->mode == COM_UTIL_PROCESS_STDIO_NULL_DEVICE)
    {
        source_fd = open("/dev/null", null_flags);
        if (source_fd < 0)
        {
            return -1;
        }
    }
    else if (spec->mode == COM_UTIL_PROCESS_STDIO_NATIVE_HANDLE)
    {
        if (spec->native_handle < 0)
        {
            return -1;
        }
        source_fd = (int)spec->native_handle;
    }
    else
    {
        return -1;
    }

    if (dup2(source_fd, target_fd) < 0)
    {
        if (spec->mode == COM_UTIL_PROCESS_STDIO_NULL_DEVICE)
        {
            close(source_fd);
        }
        return -1;
    }

    if (spec->mode == COM_UTIL_PROCESS_STDIO_NULL_DEVICE)
    {
        close(source_fd);
    }
    return 0;
}

static int setup_child_stdio(const com_util_process_options_t *options)
{
    if (setup_child_stdio_one(&options->stdin_spec, STDIN_FILENO, O_RDONLY) != 0)
    {
        return -1;
    }
    if (setup_child_stdio_one(&options->stdout_spec, STDOUT_FILENO, O_WRONLY) != 0)
    {
        return -1;
    }
    if (setup_child_stdio_one(&options->stderr_spec, STDERR_FILENO, O_WRONLY) != 0)
    {
        return -1;
    }
    return 0;
}

static void exec_with_path(char *const *argv, char *const *envp)
{
    const char *path_value;
    const char *segment;
    const char *segment_end;
    size_t name_len;

    if (strchr(argv[0], '/') != NULL)
    {
        execve(argv[0], argv, envp);
        return;
    }

    path_value = find_env_value(envp, "PATH");
    if (path_value == NULL || path_value[0] == '\0')
    {
        path_value = "/bin:/usr/bin";
    }

    name_len = strlen(argv[0]);
    segment = path_value;
    while (*segment != '\0')
    {
        char candidate[PLATFORM_PATH_MAX];
        size_t dir_len;

        segment_end = strchr(segment, ':');
        if (segment_end == NULL)
        {
            segment_end = segment + strlen(segment);
        }
        dir_len = (size_t)(segment_end - segment);
        if (dir_len == 0)
        {
            dir_len = 1;
            segment = ".";
        }
        if (dir_len + 1 + name_len + 1 <= sizeof(candidate))
        {
            memcpy(candidate, segment, dir_len);
            candidate[dir_len] = '/';
            memcpy(candidate + dir_len + 1, argv[0], name_len + 1);
            execve(candidate, argv, envp);
        }
        if (*segment_end == '\0')
        {
            break;
        }
        segment = segment_end + 1;
    }
}

#elif defined(PLATFORM_WINDOWS)
static int is_process_elevated(int *elevated)
{
    HANDLE token = NULL;
    TOKEN_ELEVATION elevation;
    DWORD returned_length;
    int rc;

    if (elevated == NULL)
    {
        return -1;
    }

    *elevated = 0;
    rc = -1;
    returned_length = 0;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &returned_length))
        {
            if (elevation.TokenIsElevated != 0)
            {
                *elevated = 1;
            }
            rc = 0;
        }
        CloseHandle(token);
    }

    return rc;
}

typedef struct wide_buffer
{
    wchar_t *data;
    size_t len;
    size_t cap;
} wide_buffer;

static int wide_buffer_reserve(wide_buffer *buf, const size_t need)
{
    wchar_t *new_data;
    size_t new_cap;

    if (need <= buf->cap)
    {
        return 0;
    }

    new_cap = buf->cap;
    if (new_cap == 0)
    {
        new_cap = 64;
    }
    while (new_cap < need)
    {
        new_cap *= 2;
    }

    new_data = (wchar_t *)realloc(buf->data, new_cap * sizeof(*new_data));
    if (new_data == NULL)
    {
        return -1;
    }
    buf->data = new_data;
    buf->cap = new_cap;
    return 0;
}

static int wide_buffer_append_char(wide_buffer *buf, const wchar_t ch)
{
    if (wide_buffer_reserve(buf, buf->len + 2) != 0)
    {
        return -1;
    }
    buf->data[buf->len] = ch;
    buf->len++;
    buf->data[buf->len] = L'\0';
    return 0;
}

static int wide_buffer_append_text(wide_buffer *buf, const wchar_t *text)
{
    size_t len;

    len = wcslen(text);
    if (wide_buffer_reserve(buf, buf->len + len + 1) != 0)
    {
        return -1;
    }
    memcpy(buf->data + buf->len, text, (len + 1) * sizeof(*text));
    buf->len += len;
    return 0;
}

static int append_windows_quoted_arg(wide_buffer *buf, const wchar_t *arg)
{
    size_t i;
    int need_quote;

    need_quote = 0;
    if (arg[0] == L'\0')
    {
        need_quote = 1;
    }
    i = 0;
    while (arg[i] != L'\0')
    {
        if (arg[i] == L' ' || arg[i] == L'\t' || arg[i] == L'"')
        {
            need_quote = 1;
        }
        i++;
    }

    if (need_quote == 0)
    {
        return wide_buffer_append_text(buf, arg);
    }

    if (wide_buffer_append_char(buf, L'"') != 0)
    {
        return -1;
    }

    i = 0;
    while (arg[i] != L'\0')
    {
        size_t slash_count;

        slash_count = 0;
        while (arg[i] == L'\\')
        {
            slash_count++;
            i++;
        }

        if (arg[i] == L'"')
        {
            while (slash_count > 0)
            {
                if (wide_buffer_append_char(buf, L'\\') != 0)
                {
                    return -1;
                }
                slash_count--;
            }
            if (wide_buffer_append_char(buf, L'\\') != 0 || wide_buffer_append_char(buf, L'"') != 0)
            {
                return -1;
            }
            i++;
        }
        else
        {
            while (slash_count > 0)
            {
                if (wide_buffer_append_char(buf, L'\\') != 0)
                {
                    return -1;
                }
                slash_count--;
            }
            if (arg[i] != L'\0')
            {
                if (wide_buffer_append_char(buf, arg[i]) != 0)
                {
                    return -1;
                }
                i++;
            }
        }
    }

    i = wcslen(arg);
    while (i > 0 && arg[i - 1] == L'\\')
    {
        if (wide_buffer_append_char(buf, L'\\') != 0)
        {
            return -1;
        }
        i--;
    }

    return wide_buffer_append_char(buf, L'"');
}

static wchar_t *build_command_line(char *const *argv)
{
    wide_buffer buf;
    size_t i;

    memset(&buf, 0, sizeof(buf));
    i = 0;
    while (argv[i] != NULL)
    {
        wchar_t *wide_arg;
        int rc;

        wide_arg = com_util_utf8_to_wstr_alloc(argv[i]);
        if (wide_arg == NULL)
        {
            free(buf.data);
            return NULL;
        }
        if (i > 0)
        {
            if (wide_buffer_append_char(&buf, L' ') != 0)
            {
                free(wide_arg);
                free(buf.data);
                return NULL;
            }
        }
        rc = append_windows_quoted_arg(&buf, wide_arg);
        free(wide_arg);
        if (rc != 0)
        {
            free(buf.data);
            return NULL;
        }
        i++;
    }
    return buf.data;
}

static size_t wide_env_key_len(const wchar_t *entry)
{
    const wchar_t *sep;

    sep = wcschr(entry, L'=');
    if (sep == NULL)
    {
        return 0;
    }
    if (sep == entry)
    {
        return 0;
    }
    return (size_t)(sep - entry);
}

static int wide_env_key_matches(const wchar_t *entry, const wchar_t *key, const size_t key_len)
{
    if (_wcsnicmp(entry, key, key_len) != 0)
    {
        return 0;
    }
    if (entry[key_len] != L'=')
    {
        return 0;
    }
    return 1;
}

static void free_wide_env_list(wchar_t **list)
{
    size_t i;

    if (list == NULL)
    {
        return;
    }

    i = 0;
    while (list[i] != NULL)
    {
        free(list[i]);
        i++;
    }
    free(list);
}

static wchar_t *wide_string_duplicate(const wchar_t *text)
{
    wchar_t *copy;
    size_t len;

    len = wcslen(text) + 1;
    copy = (wchar_t *)malloc(len * sizeof(*copy));
    if (copy != NULL)
    {
        memcpy(copy, text, len * sizeof(*copy));
    }
    return copy;
}

static int set_wide_env_entry(wchar_t **list, const size_t capacity, const wchar_t *entry)
{
    size_t key_len;
    size_t i;
    wchar_t *copy;

    key_len = wide_env_key_len(entry);
    if (key_len == 0)
    {
        return -1;
    }

    copy = wide_string_duplicate(entry);
    if (copy == NULL)
    {
        return -1;
    }

    i = 0;
    while (list[i] != NULL)
    {
        if (wide_env_key_matches(list[i], entry, key_len) != 0)
        {
            free(list[i]);
            list[i] = copy;
            return 0;
        }
        i++;
    }

    if (i + 1 >= capacity)
    {
        free(copy);
        return -1;
    }
    list[i] = copy;
    list[i + 1] = NULL;
    return 0;
}

static wchar_t *build_environment_block(char *const *overrides)
{
    LPWCH current;
    LPWCH cursor;
    size_t env_count;
    size_t override_count;
    size_t capacity;
    wchar_t **list;
    size_t i;
    size_t block_len;
    wchar_t *block;
    wchar_t *out;

    if (overrides == NULL)
    {
        return NULL;
    }

    current = GetEnvironmentStringsW();
    if (current == NULL)
    {
        return NULL;
    }

    env_count = 0;
    cursor = current;
    while (*cursor != L'\0')
    {
        env_count++;
        cursor += wcslen(cursor) + 1;
    }

    override_count = 0;
    while (overrides[override_count] != NULL)
    {
        if (strchr(overrides[override_count], '=') == NULL)
        {
            FreeEnvironmentStringsW(current);
            return NULL;
        }
        override_count++;
    }

    capacity = env_count + override_count + 1;
    list = (wchar_t **)calloc(capacity, sizeof(*list));
    if (list == NULL)
    {
        FreeEnvironmentStringsW(current);
        return NULL;
    }

    i = 0;
    cursor = current;
    while (*cursor != L'\0')
    {
        list[i] = wide_string_duplicate(cursor);
        if (list[i] == NULL)
        {
            free_wide_env_list(list);
            FreeEnvironmentStringsW(current);
            return NULL;
        }
        i++;
        cursor += wcslen(cursor) + 1;
    }
    list[i] = NULL;
    FreeEnvironmentStringsW(current);

    i = 0;
    while (overrides[i] != NULL)
    {
        wchar_t *wide_entry;

        wide_entry = com_util_utf8_to_wstr_alloc(overrides[i]);
        if (wide_entry == NULL)
        {
            free_wide_env_list(list);
            return NULL;
        }
        if (set_wide_env_entry(list, capacity, wide_entry) != 0)
        {
            free(wide_entry);
            free_wide_env_list(list);
            return NULL;
        }
        free(wide_entry);
        i++;
    }

    block_len = 1;
    i = 0;
    while (list[i] != NULL)
    {
        block_len += wcslen(list[i]) + 1;
        i++;
    }

    block = (wchar_t *)calloc(block_len, sizeof(*block));
    if (block == NULL)
    {
        free_wide_env_list(list);
        return NULL;
    }

    out = block;
    i = 0;
    while (list[i] != NULL)
    {
        size_t len;

        len = wcslen(list[i]) + 1;
        memcpy(out, list[i], len * sizeof(*out));
        out += len;
        i++;
    }
    *out = L'\0';
    free_wide_env_list(list);
    return block;
}

static HANDLE open_windows_null_device(const DWORD access)
{
    return CreateFileW(L"NUL", access, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       NULL);
}

static int duplicate_inheritable_handle(HANDLE source, HANDLE *duplicated)
{
    HANDLE current;

    current = GetCurrentProcess();
    if (!DuplicateHandle(current, source, current, duplicated, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
        return -1;
    }
    return 0;
}

static int prepare_stdio_handle(const com_util_process_stdio_t *spec, const DWORD std_id, const DWORD null_access,
                                HANDLE *prepared)
{
    HANDLE source;

    *prepared = NULL;
    if (spec->mode == COM_UTIL_PROCESS_STDIO_INHERIT)
    {
        source = GetStdHandle(std_id);
        if (source == NULL || source == INVALID_HANDLE_VALUE)
        {
            source = open_windows_null_device(null_access);
            if (source == INVALID_HANDLE_VALUE)
            {
                return -1;
            }
            *prepared = source;
            return 0;
        }
        return duplicate_inheritable_handle(source, prepared);
    }

    if (spec->mode == COM_UTIL_PROCESS_STDIO_NULL_DEVICE)
    {
        source = open_windows_null_device(null_access);
        if (source == INVALID_HANDLE_VALUE)
        {
            return -1;
        }
        *prepared = source;
        return 0;
    }

    if (spec->mode == COM_UTIL_PROCESS_STDIO_NATIVE_HANDLE)
    {
        source = (HANDLE)spec->native_handle;
        if (source == NULL || source == INVALID_HANDLE_VALUE)
        {
            return -1;
        }
        return duplicate_inheritable_handle(source, prepared);
    }

    return -1;
}

static void close_stdio_handles(HANDLE handles[3])
{
    size_t i;

    i = 0;
    while (i < 3)
    {
        if (handles[i] != NULL && handles[i] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handles[i]);
        }
        i++;
    }
}
#endif /* PLATFORM_ */

COM_UTIL_EXPORT int COM_UTIL_API com_util_process_get_executable_path(char *out_path, const size_t out_path_sz)
{
    if (out_path == NULL || out_path_sz == 0)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    {
        ssize_t len;

        len = readlink("/proc/self/exe", out_path, out_path_sz - 1);
        if (len < 0 || (size_t)len >= out_path_sz)
        {
            out_path[0] = '\0';
            return -1;
        }
        out_path[len] = '\0';
        return 0;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wbuf[PLATFORM_PATH_MAX];
        DWORD n;

        n = GetModuleFileNameW(NULL, wbuf, (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])));
        if (n == 0 || n >= (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])))
        {
            out_path[0] = '\0';
            return -1;
        }
        wbuf[n] = L'\0';
        if (com_util_wpath_to_utf8(out_path, out_path_sz, wbuf) < 0)
        {
            out_path[0] = '\0';
            return -1;
        }
        return 0;
    }
#else
    out_path[0] = '\0';
    return -1;
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_start(const com_util_process_options_t *options,
                                                                              com_util_process **process)
{
    com_util_process *new_process;

    if (options == NULL || process == NULL)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    if (has_valid_argv(options->argv) == 0)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    *process = NULL;

#if defined(PLATFORM_LINUX)
    {
        char **envp;
        pid_t pid;

        envp = build_environment(options->env_overrides);
        if (envp == NULL)
        {
            return COM_UTIL_PROCESS_INVALID_ARGUMENT;
        }

        new_process = (com_util_process *)calloc(1, sizeof(*new_process));
        if (new_process == NULL)
        {
            free_envp(envp);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        pid = fork();
        if (pid < 0)
        {
            free_envp(envp);
            free(new_process);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        if (pid == 0)
        {
            if (options->working_directory != NULL)
            {
                if (chdir(options->working_directory) != 0)
                {
                    _exit(127);
                }
            }
            if (setup_child_stdio(options) != 0)
            {
                _exit(127);
            }
            exec_with_path(options->argv, envp);
            fprintf(stderr, "エラー: exec(\"%s\") が失敗しました: %s\n", options->argv[0], strerror(errno));
            _exit(127);
        }

        free_envp(envp);
        new_process->pid = pid;
        *process = new_process;
        return COM_UTIL_PROCESS_OK;
    }
#elif defined(PLATFORM_WINDOWS)
    {
        STARTUPINFOEXW startup;
        PROCESS_INFORMATION process_info;
        SIZE_T attr_size;
        HANDLE stdio_handles[3];
        HANDLE inherit_handles[3];
        wchar_t *command_line;
        wchar_t *environment_block;
        wchar_t *working_directory;
        DWORD create_flags;
        BOOL created;

        memset(&startup, 0, sizeof(startup));
        memset(&process_info, 0, sizeof(process_info));
        memset(stdio_handles, 0, sizeof(stdio_handles));
        memset(inherit_handles, 0, sizeof(inherit_handles));
        command_line = build_command_line(options->argv);
        if (command_line == NULL)
        {
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        environment_block = build_environment_block(options->env_overrides);
        if (options->env_overrides != NULL && environment_block == NULL)
        {
            free(command_line);
            return COM_UTIL_PROCESS_INVALID_ARGUMENT;
        }

        working_directory = NULL;
        if (options->working_directory != NULL)
        {
            working_directory = com_util_utf8_to_wstr_alloc(options->working_directory);
            if (working_directory == NULL)
            {
                free(environment_block);
                free(command_line);
                return COM_UTIL_PROCESS_SYSTEM_ERROR;
            }
        }

        if (prepare_stdio_handle(&options->stdin_spec, STD_INPUT_HANDLE, GENERIC_READ, &stdio_handles[0]) != 0 ||
            prepare_stdio_handle(&options->stdout_spec, STD_OUTPUT_HANDLE, GENERIC_WRITE, &stdio_handles[1]) != 0 ||
            prepare_stdio_handle(&options->stderr_spec, STD_ERROR_HANDLE, GENERIC_WRITE, &stdio_handles[2]) != 0)
        {
            close_stdio_handles(stdio_handles);
            free(working_directory);
            free(environment_block);
            free(command_line);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdInput = stdio_handles[0];
        startup.StartupInfo.hStdOutput = stdio_handles[1];
        startup.StartupInfo.hStdError = stdio_handles[2];

        attr_size = 0;
        InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
        startup.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)calloc(1, attr_size);
        if (startup.lpAttributeList == NULL)
        {
            close_stdio_handles(stdio_handles);
            free(working_directory);
            free(environment_block);
            free(command_line);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }
        if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attr_size))
        {
            free(startup.lpAttributeList);
            close_stdio_handles(stdio_handles);
            free(working_directory);
            free(environment_block);
            free(command_line);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        inherit_handles[0] = stdio_handles[0];
        inherit_handles[1] = stdio_handles[1];
        inherit_handles[2] = stdio_handles[2];
        if (!UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherit_handles,
                                       sizeof(inherit_handles), NULL, NULL))
        {
            DeleteProcThreadAttributeList(startup.lpAttributeList);
            free(startup.lpAttributeList);
            close_stdio_handles(stdio_handles);
            free(working_directory);
            free(environment_block);
            free(command_line);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        create_flags = EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT;
        created = CreateProcessW(NULL, command_line, NULL, NULL, TRUE, create_flags, environment_block,
                                 working_directory, &startup.StartupInfo, &process_info);

        DeleteProcThreadAttributeList(startup.lpAttributeList);
        free(startup.lpAttributeList);
        close_stdio_handles(stdio_handles);
        free(working_directory);
        free(environment_block);
        free(command_line);

        if (!created)
        {
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        CloseHandle(process_info.hThread);
        new_process = (com_util_process *)calloc(1, sizeof(*new_process));
        if (new_process == NULL)
        {
            CloseHandle(process_info.hProcess);
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }
        new_process->process = process_info.hProcess;
        *process = new_process;
        return COM_UTIL_PROCESS_OK;
    }
#else
    return COM_UTIL_PROCESS_UNSUPPORTED;
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_wait(com_util_process *process,
                                                                             const int timeout_ms)
{
    if (process == NULL || timeout_ms < 0)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    if (process->exited != 0)
    {
        return COM_UTIL_PROCESS_OK;
    }

#if defined(PLATFORM_LINUX)
    {
        int status;
        int wait_options;
        uint64_t deadline;

        wait_options = 0;
        if (timeout_ms != COM_UTIL_PROCESS_WAIT_FOREVER)
        {
            wait_options = WNOHANG;
            deadline = monotonic_ms() + (uint64_t)timeout_ms;
        }
        else
        {
            deadline = 0;
        }

        while (1)
        {
            pid_t rc;

            rc = waitpid(process->pid, &status, wait_options);
            if (rc == process->pid)
            {
                if (WIFEXITED(status))
                {
                    process_set_exit_code(process, WEXITSTATUS(status));
                }
                else
                {
                    process_set_exit_code(process, -1);
                }
                return COM_UTIL_PROCESS_OK;
            }
            if (rc < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return COM_UTIL_PROCESS_SYSTEM_ERROR;
            }
            if (timeout_ms == COM_UTIL_PROCESS_NO_WAIT)
            {
                return COM_UTIL_PROCESS_TIMEOUT;
            }
            if (timeout_ms != COM_UTIL_PROCESS_WAIT_FOREVER && monotonic_ms() >= deadline)
            {
                return COM_UTIL_PROCESS_TIMEOUT;
            }
            usleep(1000);
        }
    }
#elif defined(PLATFORM_WINDOWS)
    {
        DWORD wait_ms;
        DWORD wait_result;
        DWORD child_exit_code;

        if (timeout_ms == COM_UTIL_PROCESS_WAIT_FOREVER)
        {
            wait_ms = INFINITE;
        }
        else
        {
            wait_ms = (DWORD)timeout_ms;
        }

        wait_result = WaitForSingleObject(process->process, wait_ms);
        if (wait_result == WAIT_TIMEOUT)
        {
            return COM_UTIL_PROCESS_TIMEOUT;
        }
        if (wait_result != WAIT_OBJECT_0)
        {
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }

        child_exit_code = EXIT_FAILURE;
        if (!GetExitCodeProcess(process->process, &child_exit_code))
        {
            return COM_UTIL_PROCESS_SYSTEM_ERROR;
        }
        if (child_exit_code > INT_MAX)
        {
            process_set_exit_code(process, EXIT_FAILURE);
        }
        else
        {
            process_set_exit_code(process, (int)child_exit_code);
        }
        return COM_UTIL_PROCESS_OK;
    }
#else
    return COM_UTIL_PROCESS_UNSUPPORTED;
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_get_exit_code(com_util_process *process,
                                                                                      int *exit_code)
{
    if (process == NULL || exit_code == NULL)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    if (process->exited == 0)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    *exit_code = process->exit_code;
    return COM_UTIL_PROCESS_OK;
}

COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API com_util_process_terminate(com_util_process *process)
{
    if (process == NULL)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    if (process->exited != 0)
    {
        return COM_UTIL_PROCESS_OK;
    }

#if defined(PLATFORM_LINUX)
    if (kill(process->pid, SIGTERM) != 0)
    {
        return COM_UTIL_PROCESS_SYSTEM_ERROR;
    }
    return COM_UTIL_PROCESS_OK;
#elif defined(PLATFORM_WINDOWS)
    if (!TerminateProcess(process->process, EXIT_FAILURE))
    {
        return COM_UTIL_PROCESS_SYSTEM_ERROR;
    }
    return COM_UTIL_PROCESS_OK;
#else
    return COM_UTIL_PROCESS_UNSUPPORTED;
#endif /* PLATFORM_ */
}

COM_UTIL_EXPORT void COM_UTIL_API com_util_process_destroy(com_util_process *process)
{
    if (process == NULL)
    {
        return;
    }

#if defined(PLATFORM_WINDOWS)
    if (process->process != NULL)
    {
        CloseHandle(process->process);
    }
#endif /* PLATFORM_WINDOWS */
    free(process);
}

COM_UTIL_EXPORT com_util_process_result_t COM_UTIL_API
com_util_process_run_sync(const com_util_process_options_t *options, const int timeout_ms, int *exit_code)
{
    com_util_process *process;
    com_util_process_result_t result;

    if (exit_code == NULL)
    {
        return COM_UTIL_PROCESS_INVALID_ARGUMENT;
    }
    *exit_code = EXIT_FAILURE;

    process = NULL;
    result = com_util_process_start(options, &process);
    if (result != COM_UTIL_PROCESS_OK)
    {
        return result;
    }

    result = com_util_process_wait(process, timeout_ms);
    if (result == COM_UTIL_PROCESS_OK)
    {
        result = com_util_process_get_exit_code(process, exit_code);
    }
    com_util_process_destroy(process);
    return result;
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_process_run_elevated_if_needed(const char *arguments, int *exit_code,
                                                                         int *handled)
{
    if (exit_code == NULL || handled == NULL)
    {
        return -1;
    }

    *exit_code = 0;
    *handled = 0;

#if defined(PLATFORM_WINDOWS)
    {
        SHELLEXECUTEINFOW exec_info;
        char exe_path[PLATFORM_PATH_MAX];
        wchar_t *wide_arguments = NULL;
        wchar_t *wide_exe_path;
        com_util_process child_process;
        int child_exit_code;
        int elevated;

        if (is_process_elevated(&elevated) != 0)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        if (elevated != 0)
        {
            return 0;
        }

        if (com_util_process_get_executable_path(exe_path, sizeof(exe_path)) != 0)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }

        wide_exe_path = com_util_utf8_to_wstr_alloc(exe_path);
        if (wide_exe_path == NULL)
        {
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        if (arguments != NULL)
        {
            wide_arguments = com_util_utf8_to_wstr_alloc(arguments);
            if (wide_arguments == NULL)
            {
                free(wide_exe_path);
                *exit_code = EXIT_FAILURE;
                return -1;
            }
        }

        ZeroMemory(&exec_info, sizeof(exec_info));
        exec_info.cbSize = sizeof(exec_info);
        exec_info.fMask = SEE_MASK_NOCLOSEPROCESS;
        exec_info.hwnd = NULL;
        exec_info.lpVerb = L"runas";
        exec_info.lpFile = wide_exe_path;
        exec_info.lpParameters = wide_arguments;
        exec_info.nShow = SW_SHOWNORMAL;

        *handled = 1;

        if (!ShellExecuteExW(&exec_info))
        {
            free(wide_arguments);
            free(wide_exe_path);
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        free(wide_arguments);
        free(wide_exe_path);

        memset(&child_process, 0, sizeof(child_process));
        child_process.process = exec_info.hProcess;
        if (com_util_process_wait(&child_process, COM_UTIL_PROCESS_WAIT_FOREVER) != COM_UTIL_PROCESS_OK ||
            com_util_process_get_exit_code(&child_process, &child_exit_code) != COM_UTIL_PROCESS_OK)
        {
            CloseHandle(exec_info.hProcess);
            *exit_code = EXIT_FAILURE;
            return -1;
        }
        CloseHandle(exec_info.hProcess);

        *exit_code = child_exit_code;
        return 0;
    }
#elif defined(PLATFORM_LINUX)
    if (geteuid() != 0)
    {
        (void)arguments;
        *exit_code = EXIT_FAILURE;
        return -1;
    }
    (void)arguments;
    return 0;
#else
    (void)arguments;
    *exit_code = EXIT_FAILURE;
    return -1;
#endif /* PLATFORM_ */
}
