#include <testfw.h>
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

    // Act
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                0)); // [手順] - file trace を有効化する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                        "file error message")); // [手順] - ERROR 行を書き込む。
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
                                        "file info message")); // [手順] - INFO 行を書き込む。
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

    // Act
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_ERROR, 0, 0,
                                                0)); // [手順] - file level を ERROR に設定する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                        "should be in file")); // [手順] - ERROR 行を書き込む。
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_WARNING, NULL,
                                        "should not be in file")); // [手順] - WARNING 行を書き込む。
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

    // Act
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_DEBUG, 0, 0,
                                                0)); // [手順] - file level を DEBUG に設定する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_VERBOSE, NULL,
                                        "verbose in debug file")); // [手順] - VERBOSE 行を書き込む。
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_DEBUG, NULL,
                                        "debug in debug file")); // [手順] - DEBUG 行を書き込む。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path);                               // [確認_正常系] - 実ファイルが生成されること。
    EXPECT_FILE_CONTAINS(path, " V verbose in debug file"); // [確認_正常系] - VERBOSE が V marker で出力されること。
    EXPECT_FILE_CONTAINS(path, " D debug in debug file");   // [確認_正常系] - DEBUG が D marker で出力されること。

    // Cleanup
    remove(path.c_str());
}

// NULL path 指定で file trace を無効化できることの確認
TEST_F(traceFileIntegrationTest, test_null_path_disables_file_trace)
{
    // Arrange
    std::string ws = findWorkspaceRoot();
    std::string path =
        ws + "/app/com_util/test/src/integration/trace/traceFileIntegrationTest/results/trace_disable.log";
    remove(path.c_str());

    com_util_tracer *handle = com_util_tracer_create();
    ASSERT_NE((com_util_tracer *)NULL, handle);
    ASSERT_EQ(0, com_util_tracer_set_os_level(handle, COM_UTIL_TRACE_LEVEL_NONE));

    // Act
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, path.c_str(), COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                0)); // [手順] - file trace を有効化する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                        "before disable")); // [手順] - 無効化前に 1 行書き込む。
    ASSERT_EQ(0, com_util_tracer_stop(handle));
    ASSERT_EQ(0, com_util_tracer_set_file_level(handle, NULL, COM_UTIL_TRACE_LEVEL_INFO, 0, 0,
                                                0)); // [手順] - NULL path で file trace を無効化する。
    ASSERT_EQ(0, com_util_tracer_start(handle));
    EXPECT_EQ(0, _com_util_tracer_write(handle, COM_UTIL_TRACE_LEVEL_ERROR, NULL,
                                        "after disable")); // [手順] - 無効化後に 1 行書き込む。
    com_util_tracer_dispose(handle);

    // Assert
    EXPECT_FILE_EXISTS(path);                                   // [確認_正常系] - 最初の実ファイルが残ること。
    EXPECT_FILE_CONTAINS(path, "before disable");               // [確認_正常系] - 無効化前の行が残ること。
    EXPECT_FALSE(testing::FileContains(path, "after disable")); // [確認_正常系] - 無効化後の行が追加されないこと。

    // Cleanup
    remove(path.c_str());
}
