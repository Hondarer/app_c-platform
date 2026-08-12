#include <testfw.h>
#include <mock_com_util.h>
#include <mock_fcntl.h>
#include <mock_stdlib.h>
#include <mock_time.h>
#include <mock_unistd.h>
#include <sys/mock_wait.h>

#include <com_util/base/platform.h>
#include <com_util/base/result_internal.h>
#include <com_util/crt/path.h>
#include <com_util/runtime/process.h>
#include <com_util/runtime/process_internal.h>

#include "process.inject.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

using testing::_;
using testing::DoAll;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
    #include <signal.h>
    #include <unistd.h>
static void make_temp_path(char *buf, size_t size, const char *tag)
{
    snprintf(buf, size, "/tmp/com_util_process_%s_%ld.txt", tag, (long)getpid());
}

static intptr_t open_output_handle(const char *path)
{
    return (intptr_t)open(path, O_CREAT | O_TRUNC | O_WRONLY, 0600);
}

static void close_output_handle(intptr_t handle)
{
    close((int)handle);
}

static int is_invalid_output_handle(intptr_t handle)
{
    if (handle == (intptr_t)-1)
    {
        return 1;
    }
    return 0;
}

static void remove_temp_path(const char *path)
{
    unlink(path);
}
#elif defined(PLATFORM_WINDOWS)
    #include <com_util/base/windows_sdk.h>
static void make_temp_path(char *buf, size_t size, const char *tag)
{
    char tmp_dir[PLATFORM_PATH_MAX];
    DWORD len;

    len = GetTempPathA((DWORD)sizeof(tmp_dir), tmp_dir);
    if (len == 0)
    {
        snprintf(buf, size, "com_util_process_%s_%lu.txt", tag, (unsigned long)GetCurrentProcessId());
        return;
    }
    snprintf(buf, size, "%scom_util_process_%s_%lu.txt", tmp_dir, tag, (unsigned long)GetCurrentProcessId());
}

static intptr_t open_output_handle(const char *path)
{
    HANDLE handle;

    handle = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return (intptr_t)INVALID_HANDLE_VALUE;
    }
    return (intptr_t)handle;
}

static void close_output_handle(intptr_t handle)
{
    CloseHandle((HANDLE)handle);
}

static int is_invalid_output_handle(intptr_t handle)
{
    if ((HANDLE)handle == INVALID_HANDLE_VALUE)
    {
        return 1;
    }
    return 0;
}

static void remove_temp_path(const char *path)
{
    DeleteFileA(path);
}
#endif /* PLATFORM_ */

static int read_text_file(char *buf, size_t size, const char *path)
{
    FILE *fp;
    size_t nread;

#if defined(PLATFORM_WINDOWS)
    if (fopen_s(&fp, path, "rb") != 0)
    {
        return -1;
    }
#else
    fp = fopen(path, "rb");
    if (fp == NULL)
    {
        return -1;
    }
#endif /* PLATFORM_WINDOWS */
    nread = fread(buf, 1, size - 1, fp);
    buf[nread] = '\0';
    fclose(fp);
    return 0;
}

static void trim_trailing_newline(char *buf)
{
    size_t len;

    len = strlen(buf);
    while (len > 0)
    {
        if (buf[len - 1] != '\n' && buf[len - 1] != '\r')
        {
            return;
        }
        buf[len - 1] = '\0';
        len--;
    }
}

// errno が共通結果コードへ分類されることの確認
TEST(processTest, MapsErrnoToCommonResults)
{
    // Arrange

    // Pre-Assert

    // Act
    int invalid_result = com_util_result_from_errno(EINVAL);    // [手順] - EINVAL を共通結果コードへ変換する。
    int permission_result = com_util_result_from_errno(EACCES); // [手順] - EACCES を共通結果コードへ変換する。
    int timeout_result = com_util_result_from_errno(ETIMEDOUT); // [手順] - ETIMEDOUT を共通結果コードへ変換する。
    int busy_result = com_util_result_from_errno(EBUSY);        // [手順] - EBUSY を共通結果コードへ変換する。
    int memory_result = com_util_result_from_errno(ENOMEM);     // [手順] - ENOMEM を共通結果コードへ変換する。
    int not_found_result = com_util_result_from_errno(ENOENT);   // [手順] - ENOENT を共通結果コードへ変換する。
    int other_result = com_util_result_from_errno(EDOM);        // [手順] - EDOM を共通結果コードへ変換する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              invalid_result); // [確認_正常系] - EINVAL の変換結果が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_PERMISSION_DENIED,
              permission_result); // [確認_正常系] - EACCES の変換結果が COM_UTIL_ERR_PERMISSION_DENIED であること。
    EXPECT_EQ(COM_UTIL_ERR_TIMEOUT,
              timeout_result); // [確認_正常系] - ETIMEDOUT の変換結果が COM_UTIL_ERR_TIMEOUT であること。
    EXPECT_EQ(COM_UTIL_ERR_BUSY,
              busy_result); // [確認_正常系] - EBUSY の変換結果が COM_UTIL_ERR_BUSY であること。
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              memory_result); // [確認_正常系] - ENOMEM の変換結果が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(COM_UTIL_ERR_NOT_FOUND,
              not_found_result); // [確認_正常系] - ENOENT の変換結果が COM_UTIL_ERR_NOT_FOUND であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              other_result); // [確認_正常系] - 未分類の errno の変換結果が COM_UTIL_ERR_UNKNOWN であること。
}

#if defined(PLATFORM_WINDOWS)
// Windows のバッファー不足が共通結果コードへ分類されることの確認
TEST(processTest, MapsWindowsInsufficientBuffer)
{
    // Arrange

    // Pre-Assert

    // Act
    int result = com_util_result_from_windows_error(
        ERROR_INSUFFICIENT_BUFFER); // [手順] - ERROR_INSUFFICIENT_BUFFER を共通結果コードへ変換する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        result); // [確認_正常系] - ERROR_INSUFFICIENT_BUFFER の変換結果が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
}
#endif

// 同期実行が子プロセスの終了コードを返すことの確認
TEST(processTest, RunSyncReturnsChildExitCode)
{
    // Arrange
    com_util_process_options options;
    int exit_code;
#if defined(PLATFORM_LINUX)
    char arg0[] = "/bin/sh";
    char arg1[] = "-c";
    char arg2[] = "exit 7";
#elif defined(PLATFORM_WINDOWS)
    char arg0[] = "cmd.exe";
    char arg1[] = "/C";
    char arg2[] = "exit /B 7";
#endif /* PLATFORM_ */
    char *argv[] = {arg0, arg1, arg2, NULL};

    memset(&options, 0, sizeof(options));
    options.argv = argv; // [状態] - 終了コード 7 で終了するシェル コマンドを子プロセスとする。
    exit_code = 0;

    // Pre-Assert

    // Act
    int result = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER,
                                           &exit_code); // [手順] - com_util_process_run_sync を無期限待機で呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);       // [確認_正常系] - com_util_process_run_sync の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(7, exit_code); // [確認_正常系] - 子プロセスの終了コード 7 が取得できること。
}

// 環境変数の上書きが子プロセスから参照できることの確認
TEST(processTest, EnvironmentOverridesAreVisibleToChild)
{
    // Arrange
    com_util_process_options options;
    com_util_process_stdio stdout_spec;
    char path[PLATFORM_PATH_MAX];
    char output[64];
    intptr_t handle;
    int exit_code;
    char env_value[] = "COM_UTIL_PROCESS_TEST_VALUE=override-value";
    char *env_overrides[] = {env_value, NULL};
#if defined(PLATFORM_LINUX)
    char arg0[] = "/bin/sh";
    char arg1[] = "-c";
    char arg2[] = "printf %s \"$COM_UTIL_PROCESS_TEST_VALUE\"";
#elif defined(PLATFORM_WINDOWS)
    char arg0[] = "cmd.exe";
    char arg1[] = "/C";
    char arg2[] = "echo %COM_UTIL_PROCESS_TEST_VALUE%";
#endif /* PLATFORM_ */
    char *argv[] = {arg0, arg1, arg2, NULL};

    make_temp_path(path, sizeof(path), "env");
    remove_temp_path(path);
    handle = open_output_handle(path); // [状態] - 子プロセスの stdout を受けるテンポラリ ファイルを開く。
    ASSERT_EQ(0, is_invalid_output_handle(handle));

    memset(&stdout_spec, 0, sizeof(stdout_spec));
    stdout_spec.mode = COM_UTIL_PROCESS_STDIO_NATIVE_HANDLE;
    stdout_spec.native_handle = handle; // [状態] - stdout をネイティブ ハンドルへリダイレクトする指定とする。
    memset(&options, 0, sizeof(options));
    options.argv = argv;
    options.env_overrides =
        env_overrides; // [状態] - 環境変数 COM_UTIL_PROCESS_TEST_VALUE を "override-value" に上書きする。
    options.stdout_spec = stdout_spec;
    exit_code = 0;

    // Pre-Assert

    // Act
    int result = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER,
                                           &exit_code); // [手順] - 環境変数を出力する子プロセスを同期実行する。
    close_output_handle(handle);
    int read_result =
        read_text_file(output, sizeof(output), path); // [手順] - リダイレクト先ファイルから子プロセスの出力を読み取る。
    trim_trailing_newline(output);

    // Assert
    EXPECT_EQ(COM_UTIL_OK, result);         // [確認_正常系] - com_util_process_run_sync の戻り値が OK であること。
    EXPECT_EQ(0, exit_code);                // [確認_正常系] - 子プロセスの終了コードが 0 であること。
    EXPECT_EQ(0, read_result);              // [確認_正常系] - 出力ファイルが読み取れること。
    EXPECT_STREQ("override-value", output); // [確認_正常系] - 子プロセスの出力が上書き値 "override-value" であること。

    // Cleanup
    remove_temp_path(path);
}

// 実行中プロセスへの NO_WAIT 待機が TIMEOUT を報告することの確認
TEST(processTest, WaitNoWaitReportsTimeoutForRunningProcess)
{
    // Arrange
    com_util_process_options options;
    com_util_process *process;
    int exit_code;
#if defined(PLATFORM_LINUX)
    char arg0[] = "/bin/sh";
    char arg1[] = "-c";
    char arg2[] = "sleep 2";
#elif defined(PLATFORM_WINDOWS)
    char arg0[] = "cmd.exe";
    char arg1[] = "/C";
    char arg2[] = "ping -n 3 127.0.0.1 > " PLATFORM_NULL_DEVICE_PATH;
#endif /* PLATFORM_ */
    char *argv[] = {arg0, arg1, arg2, NULL};

    memset(&options, 0, sizeof(options));
    options.argv = argv; // [状態] - 2 秒程度実行し続けるシェル コマンドを子プロセスとする。
    process = NULL;
    exit_code = 0;

    // Pre-Assert

    // Act
    int start_result =
        com_util_process_start(&options, &process); // [手順] - com_util_process_start で子プロセスを起動する。
    ASSERT_EQ(COM_UTIL_OK, start_result);
    ASSERT_NE(nullptr, process);

    int wait_result = com_util_process_wait(process, COM_UTIL_PROCESS_NO_WAIT); // [手順] - NO_WAIT で待機する。
    int terminate_result = com_util_process_terminate(process); // [手順] - 子プロセスを terminate する。
    int final_wait = com_util_process_wait(process, COM_UTIL_PROCESS_WAIT_FOREVER); // [手順] - 無期限待機で終了を待つ。
    int exit_result = com_util_process_get_exit_code(process, &exit_code);          // [手順] - 終了コードを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_TIMEOUT, wait_result); // [確認_正常系] - 実行中の NO_WAIT 待機が TIMEOUT を返すこと。
    EXPECT_EQ(COM_UTIL_OK, terminate_result);     // [確認_正常系] - terminate が OK を返すこと。
    EXPECT_EQ(COM_UTIL_OK, final_wait);           // [確認_正常系] - terminate 後の待機が OK を返すこと。
    EXPECT_EQ(
        COM_UTIL_OK,
        exit_result); // [確認_正常系] - com_util_process_get_exit_code の戻り値として、終了コードの取得が OK を返すこと。

    // Cleanup
    com_util_process_destroy(process);
}

// start が不正引数を検出することの確認
TEST(processTest, RejectsInvalidArguments)
{
    // Arrange
    com_util_process *process = NULL; // [状態] - プロセス ハンドルの受け取り先を NULL で初期化する。

    // Pre-Assert

    // Act
    int result =
        com_util_process_start(NULL, &process); // [手順] - options に NULL を渡して com_util_process_start を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result); // [確認_異常系] - com_util_process_start の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(nullptr, process); // [確認_異常系] - ハンドルが NULL のままであること。
}

// 実行ファイルのパスを取得できることの確認
TEST(processTest, GetsExecutablePath)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {};

    // Pre-Assert

    // Act
    int result = com_util_process_get_executable_path(
        path, sizeof(path)); // [手順] - 十分な容量のバッファーを渡して実行ファイルのパスを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_process_get_executable_path の戻り値が COM_UTIL_OK であること。
    EXPECT_NE('\0', path[0]); // [確認_正常系] - 取得した実行ファイルのパスが空文字列でないこと。
}

// 実行ファイルのパス取得が不正な出力引数を拒否することの確認
TEST(processTest, ExecutablePathRejectsInvalidOutputArguments)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {};

    // Pre-Assert

    // Act
    int null_result = com_util_process_get_executable_path(
        NULL, sizeof(path)); // [手順] - 出力先に NULL を渡して実行ファイルのパスを取得する。
    int zero_size_result = com_util_process_get_executable_path(
        path, 0); // [手順] - 出力先サイズに 0 を渡して実行ファイルのパスを取得する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_result); // [確認_異常系] - 出力先が NULL の呼び出しの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        zero_size_result); // [確認_異常系] - 出力先サイズが 0 の呼び出しの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 実行ファイルのパス取得がバッファー不足を報告することの確認
TEST(processTest, ExecutablePathReportsSmallBuffer)
{
    // Arrange
    char path[1] = {'x'};

    // Pre-Assert

    // Act
    int result = com_util_process_get_executable_path(
        path, sizeof(path)); // [手順] - 1 バイトの出力先へ実行ファイルのパスを取得する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        result); // [確認_異常系] - com_util_process_get_executable_path の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - バッファー不足時に出力先が空文字列であること。
}

#if defined(PLATFORM_LINUX)
// 実行ファイルのパス取得が readlink の OS エラーを共通結果へ変換することの確認
TEST(processTest, ExecutablePathReportsReadlinkFailure)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    char path[PLATFORM_PATH_MAX] = {'x'};
    errno = EACCES;
    EXPECT_CALL(mock_unistd, readlink(_, _, _, StrEq("/proc/self/exe"), _, _))
        .WillOnce(Return(static_cast<ssize_t>(-1)));

    // Pre-Assert

    // Act
    int result = com_util_process_get_executable_path(
        path, sizeof(path)); // [手順] - readlink が EACCES で失敗する状態で実行ファイルのパスを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_PERMISSION_DENIED,
              result);        // [確認_異常系] - readlink の EACCES が PERMISSION_DENIED へ変換されること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - readlink 失敗時に出力先が空文字列であること。
}

// readlink が出力バッファーを超える長さを返した場合に不足を報告することの確認
TEST(processTest, ExecutablePathReportsReadlinkLengthOverflow)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    char path[8] = {'x'};
    EXPECT_CALL(mock_unistd, readlink(_, _, _, StrEq("/proc/self/exe"), _, _))
        .WillOnce(Return(static_cast<ssize_t>(8)));

    // Pre-Assert

    // Act
    int result = com_util_process_get_executable_path(
        path, sizeof(path)); // [手順] - readlink が出力先容量以上の長さを返す状態でパスを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              result);        // [確認_異常系] - readlink の長さ超過が BUFFER_TOO_SMALL になること。
    EXPECT_EQ('\0', path[0]); // [確認_異常系] - 長さ超過時に出力先が空文字列であること。
}

// argv の NULL、空文字列を process_start が拒否することの確認
TEST(processTest, RejectsInvalidArgumentVectors)
{
    // Arrange
    com_util_process_options options = {};
    com_util_process *process = nullptr;
    char empty_arg0[] = "";
    char *null_first_argv[] = {nullptr};
    char *empty_argv[] = {empty_arg0, nullptr};

    // Pre-Assert

    // Act
    options.argv = nullptr;
    int null_argv_result =
        com_util_process_start(&options, &process); // [手順] - argv 自体が NULL の options でプロセスを開始する。
    options.argv = null_first_argv;
    int null_first_result =
        com_util_process_start(&options, &process); // [手順] - argv[0] が NULL の options でプロセスを開始する。
    options.argv = empty_argv;
    int empty_first_result =
        com_util_process_start(&options, &process); // [手順] - argv[0] が空文字列の options でプロセスを開始する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_argv_result); // [確認_異常系] - argv NULL の戻り値が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_first_result); // [確認_異常系] - argv[0] NULL の戻り値が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              empty_first_result); // [確認_異常系] - argv[0] 空文字列の戻り値が INVALID_ARGUMENT であること。
    EXPECT_EQ(nullptr, process);   // [確認_異常系] - 不正な argv で process が NULL のままであること。
}

// 環境変数上書きの形式が不正な場合に process_start が拒否することの確認
TEST(processTest, RejectsInvalidEnvironmentOverride)
{
    // Arrange
    com_util_process_options options = {};
    com_util_process *process = nullptr;
    char invalid_override[] = "INVALID_ENVIRONMENT_ENTRY";
    char *overrides[] = {invalid_override, nullptr};
    char arg0[] = "/bin/true";
    char *argv[] = {arg0, nullptr};
    options.argv = argv;
    options.env_overrides = overrides; // [状態] - '=' を含まない不正な環境変数上書きを指定する。

    // Pre-Assert

    // Act
    int result = com_util_process_start(&options, &process); // [手順] - 不正な環境変数上書きでプロセスを開始する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              result); // [確認_異常系] - com_util_process_start の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(nullptr, process); // [確認_異常系] - 不正な環境変数上書きで process が NULL のままであること。
}

// プロセス ハンドル確保に失敗した場合に process_start が失敗することの確認
TEST(processTest, StartReportsProcessAllocationFailure)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_process_options options = {};
    com_util_process *process = nullptr;
    char arg0[] = "/bin/true";
    char *argv[] = {arg0, nullptr};
    options.argv = argv;
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(DoDefault())
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - 環境配列後の process ハンドル確保で calloc が失敗すること。

    // Pre-Assert

    // Act
    int result = com_util_process_start(&options, &process); // [手順] - process ハンドル確保失敗を注入して開始する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result);           // [確認_異常系] - com_util_process_start の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(nullptr, process); // [確認_異常系] - 確保失敗時に process が NULL のままであること。
}

// fork が失敗した場合に process_start が失敗することの確認
TEST(processTest, StartReportsForkFailure)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    com_util_process_options options = {};
    com_util_process *process = nullptr;
    char arg0[] = "/bin/true";
    char *argv[] = {arg0, nullptr};
    options.argv = argv;
    EXPECT_CALL(mock_unistd, fork(_, _, _))
        .WillOnce(Return(static_cast<pid_t>(-1))); // [Pre-Assert確認_異常系] - fork が失敗すること。

    // Pre-Assert

    // Act
    int result = com_util_process_start(&options, &process); // [手順] - fork 失敗を注入してプロセスを開始する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result);           // [確認_異常系] - com_util_process_start の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(nullptr, process); // [確認_異常系] - fork 失敗時に process が NULL のままであること。
}

// 待機が終了コードとシグナル終了を分類し、割り込みを再試行することの確認
TEST(processTest, WaitMapsExitStatesAndRetriesEintr)
{
    // Arrange
    NiceMock<Mock_sys_wait> mock_sys_wait;
    com_util_process *normal_process = com_util_process_adopt_native(123);
    com_util_process *signaled_process = com_util_process_adopt_native(124);
    int normal_status = 7 << 8;
    int signaled_status = SIGTERM;
    int normal_exit_code = 0;
    int signaled_exit_code = 0;
    ASSERT_NE(nullptr, normal_process);
    ASSERT_NE(nullptr, signaled_process);
    EXPECT_CALL(mock_sys_wait, waitpid(_, _, _, 123, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(normal_status), Return(static_cast<pid_t>(-1))))
        .WillOnce(DoAll(SetArgPointee<4>(normal_status), Return(static_cast<pid_t>(123))));
    EXPECT_CALL(mock_sys_wait, waitpid(_, _, _, 124, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(signaled_status), Return(static_cast<pid_t>(124))));
    errno = EINTR; // [状態] - 1 回目の waitpid が EINTR を返す状態とする。

    // Pre-Assert

    // Act
    int normal_wait = com_util_process_wait(
        normal_process, COM_UTIL_PROCESS_WAIT_FOREVER); // [手順] - EINTR 後に正常終了する process を待機する。
    int normal_get = com_util_process_get_exit_code(
        normal_process, &normal_exit_code); // [手順] - 正常終了 process の終了コードを取得する。
    int signaled_wait = com_util_process_wait(
        signaled_process, COM_UTIL_PROCESS_WAIT_FOREVER); // [手順] - シグナル終了 process を待機する。
    int signaled_get = com_util_process_get_exit_code(
        signaled_process, &signaled_exit_code); // [手順] - シグナル終了 process の終了コードを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              normal_wait); // [確認_正常系] - 1 回目の EINTR 後の com_util_process_wait が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        normal_get); // [確認_正常系] - 正常終了 process の com_util_process_get_exit_code が COM_UTIL_OK であること。
    EXPECT_EQ(7, normal_exit_code); // [確認_正常系] - 正常終了 process の終了コードが 7 であること。
    EXPECT_EQ(
        COM_UTIL_OK,
        signaled_wait); // [確認_正常系] - シグナル終了 process の com_util_process_wait が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK,
              signaled_get); // [確認_正常系] - シグナル終了 process の終了コード取得が COM_UTIL_OK であること。
    EXPECT_EQ(-1, signaled_exit_code); // [確認_正常系] - シグナル終了 process の終了コードが -1 であること。

    // Cleanup
    com_util_process_destroy(normal_process);
    com_util_process_destroy(signaled_process);
}

// waitpid の OS エラーを未知エラーへ変換することの確認
TEST(processTest, WaitReportsWaitpidFailure)
{
    // Arrange
    NiceMock<Mock_sys_wait> mock_sys_wait;
    com_util_process *process = com_util_process_adopt_native(125);
    ASSERT_NE(nullptr, process);
    EXPECT_CALL(mock_sys_wait, waitpid(_, _, _, 125, _, _)).WillOnce(Return(static_cast<pid_t>(-1)));
    errno = ECHILD; // [状態] - waitpid が子プロセスなしで失敗する状態とする。

    // Pre-Assert

    // Act
    int result =
        com_util_process_wait(process, COM_UTIL_PROCESS_WAIT_FOREVER); // [手順] - waitpid 失敗を注入して待機する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_process_wait の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_process_destroy(process);
}

// terminate の kill 失敗を未知エラーへ変換することの確認
TEST(processTest, TerminateReportsKillFailure)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    com_util_process *process = com_util_process_adopt_native(126);
    ASSERT_NE(nullptr, process);
    EXPECT_CALL(mock_unistd, kill(_, _, _, 126, SIGTERM)).WillOnce(Return(-1));
    errno = ESRCH; // [状態] - terminate 対象が存在せず kill が失敗する状態とする。

    // Pre-Assert

    // Act
    int result = com_util_process_terminate(process); // [手順] - kill 失敗を注入して process を terminate する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result); // [確認_異常系] - com_util_process_terminate の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_process_destroy(process);
}

// process API が NULL、負値、終了前の状態を拒否することの確認
TEST(processTest, RejectsInvalidWaitAndExitArguments)
{
    // Arrange
    com_util_process *process = com_util_process_adopt_native(127);
    int exit_code = 0;
    ASSERT_NE(nullptr, process);

    // Pre-Assert

    // Act
    int null_wait = com_util_process_wait(NULL, COM_UTIL_PROCESS_NO_WAIT); // [手順] - NULL process で待機する。
    int negative_wait = com_util_process_wait(process, -1);                // [手順] - 負の timeout で待機する。
    int null_exit_process =
        com_util_process_get_exit_code(NULL, &exit_code); // [手順] - NULL process から終了コードを取得する。
    int null_exit_output =
        com_util_process_get_exit_code(process, NULL); // [手順] - NULL 出力先へ終了コードを取得する。
    int running_exit =
        com_util_process_get_exit_code(process, &exit_code); // [手順] - 待機前 process から終了コードを取得する。
    int null_terminate = com_util_process_terminate(NULL);   // [手順] - NULL process を terminate する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_wait); // [確認_異常系] - NULL process の com_util_process_wait が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              negative_wait); // [確認_異常系] - 負の timeout の com_util_process_wait が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_exit_process); // [確認_異常系] - NULL process の終了コード取得が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_exit_output); // [確認_異常系] - NULL 出力先の終了コード取得が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              running_exit); // [確認_異常系] - 実行中 process の終了コード取得が INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              null_terminate); // [確認_異常系] - NULL process の terminate が INVALID_ARGUMENT であること。

    // Cleanup
    com_util_process_destroy(process);
    com_util_process_destroy(NULL);
}

#if defined(PLATFORM_LINUX)

// Linux の環境変数補助関数がキー境界、上書き、容量不足を処理することの確認
TEST(processTest, environment_helpers_handle_keys_and_capacity)
{
    // Arrange
    char *old_entry = strdup("KEY=old");
    char *envp[3] = {old_entry, NULL, NULL};
    char replace_entry[] = "KEY=new";
    char add_entry[] = "OTHER=value";
    char overflow_entry[] = "THIRD=value";
    char override_one[] = "PATH=/custom/bin";
    char override_two[] = "COM_UTIL_PROCESS_TEST_HELPER=helper";
    char *overrides[] = {override_one, override_two, NULL};
    char invalid_override[] = "INVALID_HELPER_ENTRY";
    char *invalid_overrides[] = {invalid_override, NULL};

    // Pre-Assert

    // Act
    size_t key_len = test_process_env_key_len("KEY=value"); // [手順] - 環境変数エントリのキー長を取得する。
    size_t no_key_len = test_process_env_key_len("KEY"); // [手順] - 区切りを持たないエントリのキー長を取得する。
    int matching_key = test_process_env_key_matches("KEY=value", "KEY", 3U); // [手順] - 一致するキーを判定する。
    int prefix_key = test_process_env_key_matches("KEY=value", "KE", 2U); // [手順] - 接頭辞だけのキーを判定する。
    int different_key = test_process_env_key_matches("KEY=value", "OTHER", 5U); // [手順] - 異なるキーを判定する。
    int replace_result = test_process_set_env_entry(envp, 3U, replace_entry); // [手順] - 既存キーを上書きする。
    int add_result = test_process_set_env_entry(envp, 3U, add_entry); // [手順] - 新しいキーを追加する。
    int overflow_result = test_process_set_env_entry(envp, 3U, overflow_entry); // [手順] - 容量超過のキーを追加する。
    char **built_env = test_process_build_environment(overrides); // [手順] - 現在の環境へ上書きを適用する。
    char **invalid_env = test_process_build_environment(invalid_overrides); // [手順] - 不正な上書きを適用する。
    const char *updated_value = test_process_find_env_value(envp, "KEY"); // [手順] - 上書き後の値を検索する。
    const char *missing_value = test_process_find_env_value(envp, "MISSING"); // [手順] - 存在しない値を検索する。

    // Assert
    EXPECT_EQ(3U, key_len); // [確認_正常系] - KEY の長さが 3 であること。
    EXPECT_EQ(0U, no_key_len); // [確認_異常系] - 区切りなしエントリの長さが 0 であること。
    EXPECT_EQ(1, matching_key); // [確認_正常系] - 一致するキーが 1 になること。
    EXPECT_EQ(0, prefix_key); // [確認_異常系] - 接頭辞だけのキーが不一致になること。
    EXPECT_EQ(0, different_key); // [確認_異常系] - 異なるキーが不一致になること。
    EXPECT_EQ(0, replace_result); // [確認_正常系] - 既存キーの上書きが成功すること。
    EXPECT_EQ(0, add_result); // [確認_正常系] - 新しいキーの追加が成功すること。
    EXPECT_EQ(-1, overflow_result); // [確認_異常系] - 容量超過の追加が失敗すること。
    EXPECT_STREQ("new", updated_value); // [確認_正常系] - 上書き後の KEY が new になること。
    EXPECT_EQ(static_cast<const char *>(NULL), missing_value); // [確認_異常系] - 未登録キーが NULL になること。
    ASSERT_NE(static_cast<char **>(NULL), built_env); // [確認_正常系] - 環境配列が生成されること。
    EXPECT_STREQ("/custom/bin", test_process_find_env_value(built_env, "PATH")); // [確認_正常系] - PATH が上書きされること。
    EXPECT_STREQ("helper", test_process_find_env_value(built_env, "COM_UTIL_PROCESS_TEST_HELPER")); // [確認_正常系] - 追加変数が検索できること。
    EXPECT_EQ(static_cast<char **>(NULL), invalid_env); // [確認_異常系] - 不正な上書きで NULL が返ること。

    // Cleanup
    test_process_free_envp(built_env);
}

// Linux の子プロセス標準入出力設定が各モードと OS エラーを分類することの確認
TEST(processTest, child_stdio_helpers_handle_modes_and_errors)
{
    // Arrange
    NiceMock<Mock_fcntl> mock_fcntl;
    NiceMock<Mock_unistd> mock_unistd;
    com_util_process_stdio spec = {};
    com_util_process_options options = {};
    EXPECT_CALL(mock_fcntl, open(_, _, _, _, _, _))
        .WillOnce(Return(10))
        .WillOnce(Return(13))
        .WillOnce(Return(-1));
    EXPECT_CALL(mock_unistd, dup2(_, _, _, 10, STDIN_FILENO)).WillOnce(Return(0));
    EXPECT_CALL(mock_unistd, close(_, _, _, 10)).WillOnce(Return(0));
    EXPECT_CALL(mock_unistd, dup2(_, _, _, 13, STDOUT_FILENO)).WillOnce(Return(-1));
    EXPECT_CALL(mock_unistd, close(_, _, _, 13)).WillOnce(Return(0));
    EXPECT_CALL(mock_unistd, dup2(_, _, _, 11, STDOUT_FILENO)).WillOnce(Return(-1));
    EXPECT_CALL(mock_unistd, dup2(_, _, _, 12, STDERR_FILENO)).WillOnce(Return(0));

    // Pre-Assert

    // Act
    int inherit_result = test_process_setup_child_stdio_one(&spec, STDIN_FILENO, O_RDONLY); // [手順] - 継承モードを設定する。
    spec.mode = COM_UTIL_PROCESS_STDIO_NULL_DEVICE;
    int null_result = test_process_setup_child_stdio_one(&spec, STDIN_FILENO, O_RDONLY); // [手順] - NULL デバイスへ接続する。
    int null_dup_failure_result = test_process_setup_child_stdio_one(&spec, STDOUT_FILENO, O_WRONLY); // [手順] - NULL デバイスの dup2 失敗を処理する。
    int null_open_result = test_process_setup_child_stdio_one(&spec, STDOUT_FILENO, O_WRONLY); // [手順] - NULL デバイスの open 失敗を処理する。
    spec.mode = COM_UTIL_PROCESS_STDIO_NATIVE_HANDLE;
    spec.native_handle = -1;
    int invalid_handle_result = test_process_setup_child_stdio_one(&spec, STDOUT_FILENO, O_WRONLY); // [手順] - 負のネイティブハンドルを設定する。
    spec.native_handle = 11;
    int dup_failure_result = test_process_setup_child_stdio_one(&spec, STDOUT_FILENO, O_WRONLY); // [手順] - dup2 の失敗を処理する。
    spec.native_handle = 12;
    int native_result = test_process_setup_child_stdio_one(&spec, STDERR_FILENO, O_WRONLY); // [手順] - ネイティブハンドルを接続する。
    int invalid_mode_value = 99;
    spec.mode = static_cast<com_util_process_stdio_mode>(invalid_mode_value);
    int invalid_mode_result = test_process_setup_child_stdio_one(&spec, STDERR_FILENO, O_WRONLY); // [手順] - 不正な標準入出力モードを設定する。
    options.stdin_spec.mode = COM_UTIL_PROCESS_STDIO_INHERIT;
    options.stdout_spec.mode = COM_UTIL_PROCESS_STDIO_INHERIT;
    options.stderr_spec.mode = COM_UTIL_PROCESS_STDIO_INHERIT;
    int all_inherit_result = test_process_setup_child_stdio(&options); // [手順] - 3 標準ストリームを継承する。

    // Assert
    EXPECT_EQ(0, inherit_result); // [確認_正常系] - 継承モードが成功すること。
    EXPECT_EQ(0, null_result); // [確認_正常系] - NULL デバイス接続が成功すること。
    EXPECT_EQ(-1, null_dup_failure_result); // [確認_異常系] - NULL デバイスの dup2 失敗が -1 になること。
    EXPECT_EQ(-1, null_open_result); // [確認_異常系] - NULL デバイス open 失敗が -1 になること。
    EXPECT_EQ(-1, invalid_handle_result); // [確認_異常系] - 負のハンドルが -1 になること。
    EXPECT_EQ(-1, dup_failure_result); // [確認_異常系] - dup2 失敗が -1 になること。
    EXPECT_EQ(0, native_result); // [確認_正常系] - ネイティブハンドル接続が成功すること。
    EXPECT_EQ(-1, invalid_mode_result); // [確認_異常系] - 不正モードが -1 になること。
    EXPECT_EQ(0, all_inherit_result); // [確認_正常系] - 全ストリーム継承が成功すること。
}

// Linux の PATH 探索が絶対パス、空要素、既定値、候補長を処理することの確認
TEST(processTest, exec_path_helper_searches_path_segments)
{
    // Arrange
    NiceMock<Mock_unistd> mock_unistd;
    char absolute_arg[] = "/bin/tool";
    char relative_arg[] = "tool";
    char long_arg[PLATFORM_PATH_MAX] = {};
    char path_value[] = "PATH=/one::/two";
    char *absolute_argv[] = {absolute_arg, NULL};
    char *relative_argv[] = {relative_arg, NULL};
    char *long_argv[] = {long_arg, NULL};
    char *envp[] = {path_value, NULL};
    memset(long_arg, 'x', sizeof(long_arg) - 1U);
    long_arg[sizeof(long_arg) - 1U] = '\0';
    EXPECT_CALL(mock_unistd, execve(_, _, _, _, _, _)).WillRepeatedly(Return(-1));

    // Pre-Assert

    // Act
    test_process_exec_with_path(absolute_argv, envp); // [手順] - 絶対パスを exec する。
    test_process_exec_with_path(relative_argv, envp); // [手順] - PATH の各要素から相対パスを探索する。
    char *empty_path_env[] = {NULL};
    test_process_exec_with_path(relative_argv, empty_path_env); // [手順] - PATH 不在時の既定値から探索する。
    test_process_exec_with_path(long_argv, envp); // [手順] - 長すぎる候補を exec しない。

    // Assert
    SUCCEED(); // [確認_正常系] - exec 失敗後も PATH 探索ヘルパーが戻ること。
}

// Linux の単調時間取得が timespec をミリ秒へ変換することの確認
TEST(processTest, monotonic_time_converts_timespec_to_milliseconds)
{
    // Arrange
    NiceMock<Mock_time> mock_time;
    struct timespec value = {};
    value.tv_sec = 12;
    value.tv_nsec = 345000000L;
    EXPECT_CALL(mock_time, clock_gettime(_, _, _, CLOCK_MONOTONIC, _))
        .WillOnce(DoAll(SetArgPointee<4>(value), Return(0)));

    // Pre-Assert

    // Act
    uint64_t result = test_process_monotonic_ms(); // [手順] - 単調時計をミリ秒へ変換する。

    // Assert
    EXPECT_EQ(12345U, result); // [確認_正常系] - 12 秒 345 ミリ秒が 12345 ミリ秒になること。
}

// Linux の有限待機が deadline 到達時に timeout を返すことの確認
TEST(processTest, wait_reports_timeout_at_finite_deadline)
{
    // Arrange
    NiceMock<Mock_sys_wait> mock_sys_wait;
    NiceMock<Mock_time> mock_time;
    NiceMock<Mock_unistd> mock_unistd;
    com_util_process *process = com_util_process_adopt_native(128);
    struct timespec first = {};
    struct timespec second = {};
    int clock_count = 0;
    ASSERT_NE(nullptr, process);
    first.tv_sec = 1;
    second.tv_sec = 2;
    EXPECT_CALL(mock_time, clock_gettime(_, _, _, CLOCK_MONOTONIC, _))
        .Times(2)
        .WillRepeatedly(Invoke([&clock_count, first, second](const char *, const int, const char *, const clockid_t,
                                                              struct timespec *arg)
                               {
                                   *arg = (clock_count++ == 0) ? first : second;
                                   return 0;
                               }));
    EXPECT_CALL(mock_sys_wait, waitpid(_, _, _, 128, _, WNOHANG)).WillOnce(Return(static_cast<pid_t>(0)));

    // Pre-Assert

    // Act
    int result = com_util_process_wait(process, 500); // [手順] - 終了しないプロセスを有限時間待機する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_TIMEOUT, result); // [確認_正常系] - deadline 到達時の wait が TIMEOUT になること。

    // Cleanup
    com_util_process_destroy(process);
}

// Linux の有限待機が期限前にスリープしてから終了を検出することの確認
TEST(processTest, wait_sleeps_before_finite_deadline_and_detects_exit)
{
    // Arrange
    NiceMock<Mock_sys_wait> mock_sys_wait;
    NiceMock<Mock_time> mock_time;
    NiceMock<Mock_unistd> mock_unistd;
    com_util_process *process = com_util_process_adopt_native(131);
    struct timespec now = {};
    int status = 4 << 8;
    int exit_code = 0;
    ASSERT_NE(nullptr, process);
    now.tv_sec = 1;
    EXPECT_CALL(mock_time, clock_gettime(_, _, _, CLOCK_MONOTONIC, _))
        .Times(2)
        .WillRepeatedly(DoAll(SetArgPointee<4>(now), Return(0)));
    EXPECT_CALL(mock_sys_wait, waitpid(_, _, _, 131, _, WNOHANG))
        .WillOnce(Return(static_cast<pid_t>(0)))
        .WillOnce(DoAll(SetArgPointee<4>(status), Return(static_cast<pid_t>(131))));
    EXPECT_CALL(mock_unistd, usleep(_, _, _, 1000U)).WillOnce(Return(0));

    // Pre-Assert

    // Act
    int wait_result = com_util_process_wait(process, 500); // [手順] - 期限前のプロセスを有限時間待機する。
    int exit_result = com_util_process_get_exit_code(process, &exit_code); // [手順] - 終了コードを取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, wait_result); // [確認_正常系] - 有限待機が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, exit_result); // [確認_正常系] - 終了コード取得が COM_UTIL_OK であること。
    EXPECT_EQ(4, exit_code); // [確認_正常系] - 子プロセスの終了コードが 4 であること。

    // Cleanup
    com_util_process_destroy(process);
}

// Linux の環境変数補助関数が不正エントリとメモリ確保失敗を処理することの確認
TEST(processTest, environment_helpers_report_invalid_and_allocation_failures)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    char *envp[] = {NULL, NULL};
    char invalid_entry[] = "INVALID";
    char valid_entry[] = "KEY=value";
    char *invalid_overrides[] = {invalid_entry, NULL};
    int set_result;
    char **invalid_result;
    char **calloc_result;
    char **duplicate_result;

    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _))
        .WillOnce(Return(nullptr))
        .WillOnce(DoDefault());

    // Pre-Assert

    // Act
    set_result = test_process_set_env_entry(envp, 2U, valid_entry); // [手順] - 環境変数エントリの確保失敗を処理する。
    test_process_free_envp(NULL); // [手順] - NULL の環境配列を解放する。
    invalid_result = test_process_build_environment(invalid_overrides); // [手順] - 不正な上書き形式を処理する。
    calloc_result = test_process_build_environment(NULL); // [手順] - 環境配列の確保失敗を処理する。
    duplicate_result = test_process_build_environment(NULL); // [手順] - 環境エントリの複製失敗を処理する。

    // Assert
    EXPECT_EQ(-1, set_result); // [確認_異常系] - エントリ確保失敗が -1 になること。
    EXPECT_EQ(static_cast<char **>(NULL), invalid_result); // [確認_異常系] - 不正な上書きで NULL が返ること。
    EXPECT_EQ(static_cast<char **>(NULL), calloc_result); // [確認_異常系] - 環境配列の確保失敗で NULL が返ること。
    EXPECT_EQ(static_cast<char **>(NULL), duplicate_result); // [確認_異常系] - エントリ複製失敗で NULL が返ること。
}

// Linux の終了済みプロセスに対する待機と終了要求が冪等であることの確認
TEST(processTest, completed_process_wait_and_terminate_are_idempotent)
{
    // Arrange
    NiceMock<Mock_sys_wait> mock_sys_wait;
    com_util_process *process = com_util_process_adopt_native(129);
    int status = 3 << 8;
    ASSERT_NE(nullptr, process);
    EXPECT_CALL(mock_sys_wait, waitpid(_, _, _, 129, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(status), Return(static_cast<pid_t>(129))));

    // Pre-Assert

    // Act
    int first_wait = com_util_process_wait(process, COM_UTIL_PROCESS_WAIT_FOREVER); // [手順] - プロセスの終了を待機する。
    int second_wait = com_util_process_wait(process, COM_UTIL_PROCESS_WAIT_FOREVER); // [手順] - 終了済みプロセスを再度待機する。
    int terminate_result = com_util_process_terminate(process); // [手順] - 終了済みプロセスを terminate する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, first_wait); // [確認_正常系] - 1 回目の com_util_process_wait が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, second_wait); // [確認_正常系] - 終了済みプロセスの 2 回目の待機が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, terminate_result); // [確認_正常系] - 終了済みプロセスの terminate が COM_UTIL_OK であること。

    // Cleanup
    com_util_process_destroy(process);
}

// Linux の adopt_native がプロセス構造体の確保失敗を返すことの確認
TEST(processTest, adopt_native_reports_allocation_failure)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, _, _)).WillOnce(Return(nullptr));

    // Pre-Assert

    // Act
    com_util_process *process = com_util_process_adopt_native(130); // [手順] - プロセス構造体の確保失敗を注入する。

    // Assert
    EXPECT_EQ(nullptr, process); // [確認_異常系] - 確保失敗時に NULL が返ること。
}

// Linux の run_sync が出力引数と start 失敗を検出することの確認
TEST(processTest, run_sync_rejects_invalid_output_and_start_failure)
{
    // Arrange
    com_util_process_options options = {};
    com_util_process *process = nullptr;
    char arg0[] = "/bin/true";
    char *argv[] = {arg0, nullptr};
    int exit_code = 0;
    options.argv = argv;

    // Pre-Assert

    // Act
    int null_output = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER, NULL); // [手順] - 終了コード出力先に NULL を渡す。
    options.argv = nullptr;
    int start_failure = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER, &exit_code); // [手順] - 不正な options で同期実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, null_output); // [確認_異常系] - NULL 出力先が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, start_failure); // [確認_異常系] - start 失敗が INVALID_ARGUMENT になること。
    EXPECT_EQ(nullptr, process); // [確認_異常系] - 使用していない process が NULL のままであること。
}

// Linux の子プロセス起動失敗が終了コード 127 へ分類されることの確認
TEST(processTest, child_start_failures_return_exit_code_127)
{
    // Arrange
    com_util_process_options options = {};
    int chdir_exit_code = 0;
    int stdio_exit_code = 0;
    int exec_exit_code = 0;
    char true_arg0[] = "/bin/true";
    char missing_arg0[] = "/com_util/process/path/does/not/exist";
    char *true_argv[] = {true_arg0, nullptr};
    char *missing_argv[] = {missing_arg0, nullptr};
    options.argv = true_argv;
    options.working_directory = "/com_util/process/directory/does/not/exist";

    // Pre-Assert

    // Act
    int chdir_result = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER, &chdir_exit_code); // [手順] - 存在しない作業ディレクトリで子プロセスを起動する。
    options.working_directory = nullptr;
    options.stdout_spec.mode = COM_UTIL_PROCESS_STDIO_NATIVE_HANDLE;
    options.stdout_spec.native_handle = -1;
    int stdio_result = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER, &stdio_exit_code); // [手順] - 不正な標準出力ハンドルで子プロセスを起動する。
    options.stdout_spec.mode = COM_UTIL_PROCESS_STDIO_INHERIT;
    options.argv = missing_argv;
    int exec_result = com_util_process_run_sync(&options, COM_UTIL_PROCESS_WAIT_FOREVER, &exec_exit_code); // [手順] - 存在しない実行ファイルで子プロセスを起動する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, chdir_result); // [確認_正常系] - chdir 失敗後も親側の run_sync が COM_UTIL_OK であること。
    EXPECT_EQ(127, chdir_exit_code); // [確認_異常系] - chdir 失敗時の子プロセス終了コードが 127 であること。
    EXPECT_EQ(COM_UTIL_OK, stdio_result); // [確認_正常系] - stdio 設定失敗後も親側の run_sync が COM_UTIL_OK であること。
    EXPECT_EQ(127, stdio_exit_code); // [確認_異常系] - stdio 設定失敗時の子プロセス終了コードが 127 であること。
    EXPECT_EQ(COM_UTIL_OK, exec_result); // [確認_正常系] - exec 失敗後も親側の run_sync が COM_UTIL_OK であること。
    EXPECT_EQ(127, exec_exit_code); // [確認_異常系] - exec 失敗時の子プロセス終了コードが 127 であること。
}

#endif /* PLATFORM_LINUX */
#endif /* PLATFORM_LINUX */
