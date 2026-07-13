#include <testfw.h>
#include <com_util/crt/path.h>
#include <com_util/runtime/process.h>
#include <com_util/trace/tracer.h>
#include <string>

class traceFileIntegrationTest : public Test
{
};

// file trace を有効化すると実ファイルへメッセージが書き込まれることの確認
TEST_F(traceFileIntegrationTest, test_enable_file_trace_writes_messages)
{
    // Arrange
    std::string ws = findWorkspaceRoot();
    std::string path = ws + "/app/com_util/test/src/integration/trace/traceFileIntegrationTest/results/trace_test.log";
    remove(path.c_str());

    com_util_tracer *handle = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, handle);
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert

    // Act
    int rtc_tracer_set_file_level = com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_INFO, 0,
                                                                   0, 0); // [手順] - file trace を有効化する。
    ASSERT_EQ(
        0,
        rtc_tracer_set_file_level); // [確認_正常系] - file trace を有効化した com_util_tracer_set_file_level の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int rtc_tracer_write = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                  "file error message"); // [手順] - ERROR 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write); // [確認_正常系] - _com_util_tracer_write の戻り値として、ERROR 行を書き込んだ結果が 0 であること。
    int rtc_tracer_write_2 = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                    "file info message"); // [手順] - INFO 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write_2); // [確認_正常系] - _com_util_tracer_write の戻り値として、INFO 行を書き込んだ結果が 0 であること。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path);                         // [確認_正常系] - 実ファイルが生成されること。
    EXPECT_FILE_CONTAINS(path, "file error message"); // [確認_正常系] - ERROR メッセージが含まれること。
    EXPECT_FILE_CONTAINS(path, "file info message");  // [確認_正常系] - INFO メッセージが含まれること。

    // Cleanup
    remove(path.c_str());
}

// file level が WARNING 以下の出力を抑止することの確認
TEST_F(traceFileIntegrationTest, test_file_level_filters_messages)
{
    // Arrange
    std::string ws = findWorkspaceRoot();
    std::string path =
        ws + "/app/com_util/test/src/integration/trace/traceFileIntegrationTest/results/trace_filter.log";
    remove(path.c_str());

    com_util_tracer *handle = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, handle);
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert

    // Act
    int rtc_tracer_set_file_level = com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_ERROR, 0,
                                                                   0, 0); // [手順] - file level を ERROR に設定する。
    ASSERT_EQ(
        0,
        rtc_tracer_set_file_level); // [確認_正常系] - file level を ERROR に設定した com_util_tracer_set_file_level の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int rtc_tracer_write = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                  "should be in file"); // [手順] - ERROR 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write); // [確認_正常系] - _com_util_tracer_write の戻り値として、ERROR 行を書き込んだ結果が 0 であること。
    int rtc_tracer_write_2 = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_WARNING, NULL,
                                                    "should not be in file"); // [手順] - WARNING 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write_2); // [確認_正常系] - _com_util_tracer_write の戻り値として、WARNING 行を書き込んだ結果が 0 であること。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path);                                           // [確認_正常系] - 実ファイルが生成されること。
    EXPECT_FILE_CONTAINS(path, "should be in file");                    // [確認_正常系] - ERROR 行が含まれること。
    EXPECT_FALSE(testing::FileContains(path, "should not be in file")); // [確認_正常系] - WARNING 行が含まれないこと。

    // Cleanup
    remove(path.c_str());
}

// DEBUG 設定時に V と D の marker が実ファイルへ出力されることの確認
TEST_F(traceFileIntegrationTest, test_debug_level_outputs_verbose_and_debug_markers)
{
    // Arrange
    std::string ws = findWorkspaceRoot();
    std::string path = ws + "/app/com_util/test/src/integration/trace/traceFileIntegrationTest/results/trace_debug.log";
    remove(path.c_str());

    com_util_tracer *handle = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, handle);
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert

    // Act
    int rtc_tracer_set_file_level = com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_DEBUG, 0,
                                                                   0, 0); // [手順] - file level を DEBUG に設定する。
    ASSERT_EQ(
        0,
        rtc_tracer_set_file_level); // [確認_正常系] - file level を DEBUG に設定した com_util_tracer_set_file_level の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int rtc_tracer_write = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_VERBOSE, NULL,
                                                  "verbose in debug file"); // [手順] - VERBOSE 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write); // [確認_正常系] - _com_util_tracer_write の戻り値として、VERBOSE 行を書き込んだ結果が 0 であること。
    int rtc_tracer_write_2 = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL,
                                                    "debug in debug file"); // [手順] - DEBUG 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write_2); // [確認_正常系] - _com_util_tracer_write の戻り値として、DEBUG 行を書き込んだ結果が 0 であること。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path);                               // [確認_正常系] - 実ファイルが生成されること。
    EXPECT_FILE_CONTAINS(path, " V verbose in debug file"); // [確認_正常系] - VERBOSE が V marker で出力されること。
    EXPECT_FILE_CONTAINS(path, " D debug in debug file");   // [確認_正常系] - DEBUG が D marker で出力されること。

    // Cleanup
    remove(path.c_str());
}

// level NONE 指定で file trace を無効化できることの確認
TEST_F(traceFileIntegrationTest, test_level_none_disables_file_trace)
{
    // Arrange
    std::string ws = findWorkspaceRoot();
    std::string path =
        ws + "/app/com_util/test/src/integration/trace/traceFileIntegrationTest/results/trace_disable.log";
    remove(path.c_str());

    com_util_tracer *handle = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, handle);
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert

    // Act
    int rtc_tracer_set_file_level = com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_INFO, 0,
                                                                   0, 0); // [手順] - file trace を有効化する。
    ASSERT_EQ(
        0,
        rtc_tracer_set_file_level); // [確認_正常系] - file trace を有効化した com_util_tracer_set_file_level の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int rtc_tracer_write = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                  "before disable"); // [手順] - 無効化前に 1 行書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write); // [確認_正常系] - _com_util_tracer_write の戻り値として、無効化前に 1 行書き込んだ結果が 0 であること。
    ASSERT_EQ(0, com_util_tracer_stop(handle));
    int rtc_tracer_set_file_level_2 = com_util_tracer_set_file_level(
        handle, path.c_str(), COM_UTIL_TRACE_LEVEL_NONE, 0, 0, 0); // [手順] - level NONE で file trace を無効化する。
    ASSERT_EQ(
        0,
        rtc_tracer_set_file_level_2); // [確認_正常系] - level NONE で file trace を無効化した com_util_tracer_set_file_level の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    int rtc_tracer_write_2 = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                    "after disable"); // [手順] - 無効化後に 1 行書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write_2); // [確認_正常系] - _com_util_tracer_write の戻り値として、無効化後に 1 行書き込んだ結果が 0 であること。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path);                                   // [確認_正常系] - 最初の実ファイルが残ること。
    EXPECT_FILE_CONTAINS(path, "before disable");               // [確認_正常系] - 無効化前の行が残ること。
    EXPECT_FALSE(testing::FileContains(path, "after disable")); // [確認_正常系] - 無効化後の行が追加されないこと。

    // Cleanup
    remove(path.c_str());
}

namespace
{

// デフォルト トレースファイルのパス (実行ファイルのディレクトリ + /log/<プロセス名>.log) を導出する
static std::string build_expected_default_path(void)
{
    char exe_path[PLATFORM_PATH_MAX];
    if (com_util_process_get_executable_path(exe_path, sizeof(exe_path)) != 0)
    {
        return std::string();
    }
    std::string full(exe_path);
    size_t sep_pos = full.rfind(PLATFORM_PATH_SEP_CHR);
    if (sep_pos == std::string::npos)
    {
        return std::string();
    }
    std::string dir = full.substr(0, sep_pos);
    std::string base = full.substr(sep_pos + 1);
    if (base.size() > 4 && base.compare(base.size() - 4, 4, ".exe") == 0)
    {
        base.resize(base.size() - 4); // Windows のプロセス名から .exe を除去する
    }
    return dir + PLATFORM_PATH_SEP "log" PLATFORM_PATH_SEP + base + ".log";
}

} // namespace

// set_file_level 未呼び出しでデフォルト パス (実行ファイルのディレクトリ + /log/<プロセス名>.log) へ
// 書き込まれることの確認。set_name はトレースファイル名に影響しないことも実証する。
TEST_F(traceFileIntegrationTest, test_default_path_writes_to_log_directory_next_to_executable)
{
    // Arrange
    std::string path = build_expected_default_path();
    ASSERT_FALSE(path.empty());
    remove(path.c_str());

    com_util_tracer *handle = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, handle);
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));
    int rtc_tracer_set_name = com_util_tracer_set_name(
        handle, "default_path_it", 0); // [手順] - インスタンス名を設定する (ファイル名には影響しない)。
    ASSERT_EQ(
        0,
        rtc_tracer_set_name); // [確認_正常系] - com_util_tracer_set_name の戻り値から、インスタンス名の設定が成功したと判断できること。

    // Pre-Assert

    // Act
    int rtc_tracer_start = com_util_tracer_start(handle); // [手順] - set_file_level なしで start する。
    ASSERT_EQ(
        0,
        rtc_tracer_start); // [確認_正常系] - set_file_level なしで start した com_util_tracer_start の戻り値が 0 であること。
    int rtc_tracer_write = _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                                  "default path message"); // [手順] - INFO 行を書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write); // [確認_正常系] - _com_util_tracer_write の戻り値として、INFO 行を書き込んだ結果が 0 であること。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path); // [確認_正常系] - プロセス名のトレースファイルが log/ に生成されること。
    EXPECT_FILE_CONTAINS(path, "default path message"); // [確認_正常系] - 書き込んだ行が含まれること。

    // Cleanup
    remove(path.c_str());
}

// 同一プロセス内の複数 tracer が占有モードのまま同一デフォルト パスへ出力できることの確認 (プロセス内調停)
TEST_F(traceFileIntegrationTest, test_two_tracers_share_default_path_in_single_process)
{
    // Arrange
    std::string path = build_expected_default_path();
    ASSERT_FALSE(path.empty());
    remove(path.c_str());

    com_util_tracer *first = com_util_tracer_create();
    com_util_tracer *second = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, first);
    ASSERT_NE((com_util_tracer *)NULL, second);
    ASSERT_EQ(0, com_util_tracer_set_os_level(first, COM_UTIL_TRACE_LEVEL_NONE));
    ASSERT_EQ(0, com_util_tracer_set_os_level(second, COM_UTIL_TRACE_LEVEL_NONE));

    // Pre-Assert

    // Act
    int rtc_tracer_start = com_util_tracer_start(first); // [手順] - 1 つ目の tracer を start する。
    ASSERT_EQ(
        0,
        rtc_tracer_start); // [確認_正常系] - 1 つ目の tracer を start した com_util_tracer_start の戻り値が 0 であること。
    int rtc_tracer_start_2 = com_util_tracer_start(second); // [手順] - 2 つ目も同一デフォルト パスで start する。
    ASSERT_EQ(
        0,
        rtc_tracer_start_2); // [確認_正常系] - 2 つ目も同一デフォルト パスで start した com_util_tracer_start の戻り値が 0 であること。
    int rtc_tracer_write = _com_util_tracer_write(first, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                  "from first tracer"); // [手順] - 1 つ目から書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write); // [確認_正常系] - _com_util_tracer_write の戻り値として、1 つ目から書き込んだ結果が 0 であること。
    int rtc_tracer_write_2 = _com_util_tracer_write(second, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                    "from second tracer"); // [手順] - 2 つ目から書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write_2); // [確認_正常系] - _com_util_tracer_write の戻り値として、2 つ目から書き込んだ結果が 0 であること。
    com_util_tracer_dispose(first);                             // [手順] - 1 つ目を解放する。
    int rtc_tracer_write_3 = _com_util_tracer_write(second, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                                    "after first dispose"); // [手順] - 解放後も 2 つ目から書き込む。
    EXPECT_EQ(
        0,
        rtc_tracer_write_3); // [確認_正常系] - _com_util_tracer_write の戻り値として、解放後も 2 つ目から書き込んだ結果が 0 であること。
    com_util_tracer_dispose(second);

    // Assert
    EXPECT_FILE_EXISTS(path); // [確認_正常系] - 占有モードでも 2 つ目の start が成功し同一ファイルを共有すること。
    EXPECT_FILE_CONTAINS(path, "from first tracer");   // [確認_正常系] - 1 つ目の行が含まれること。
    EXPECT_FILE_CONTAINS(path, "from second tracer");  // [確認_正常系] - 2 つ目の行が含まれること。
    EXPECT_FILE_CONTAINS(path, "after first dispose"); // [確認_正常系] - 参照が残る間は書き込みが継続できること。

    // Cleanup
    remove(path.c_str());
}
