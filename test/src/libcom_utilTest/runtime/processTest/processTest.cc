#include <testfw.h>
#include <mock_com_util.h>
#include <mock_unistd.h>

#include <com_util/base/platform.h>
#include <com_util/base/result_internal.h>
#include <com_util/crt/path.h>
#include <com_util/runtime/process.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;

#if defined(PLATFORM_LINUX)
    #include <fcntl.h>
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
    int other_result = com_util_result_from_errno(ENOENT);      // [手順] - ENOENT を共通結果コードへ変換する。

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
    char arg2[] = "ping -n 3 127.0.0.1 > NUL";
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
#endif /* PLATFORM_LINUX */
