#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <TraceLoggingProvider.h>
    #include <testfw.h>
    #include <com_util/trace/etw.h>

COM_UTIL_ETW_DEFINE_PROVIDER(s_test_provider, "TraceEtwTest",
                             (0x62ab1ccc, 0x5fc6, 0x4e1e, 0x82, 0x60, 0x9e, 0xa2, 0x77, 0x2a, 0xfe, 0x5e));

class trace_etwTest : public Test
{
};

// プロバイダを登録し、有効なハンドルが返されることの確認
TEST_F(trace_etwTest, test_init_and_dispose)
{
    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider); // [手順] - ETW provider を登録する。
    EXPECT_NE((com_util_etw_provider *)NULL, handle); // [確認_正常系] - ハンドルが NULL でないこと。
    com_util_etw_provider_dispose(handle);
}

// INFO レベルで書き込みが成功することの確認
TEST_F(trace_etwTest, test_write_returns_zero)
{
    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    int result = com_util_etw_provider_write(handle, 4, NULL, "test message"); // [手順] - INFO レベルで書き込む。

    EXPECT_EQ(0, result); // [確認_正常系] - 戻り値が 0 であること。
    com_util_etw_provider_dispose(handle);
}

// 全レベルで書き込みが成功することの確認
TEST_F(trace_etwTest, test_write_all_levels)
{
    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);
    ASSERT_NE((com_util_etw_provider *)NULL, handle);

    EXPECT_EQ(0, com_util_etw_provider_write(handle, 1, NULL,
                                             "critical")); // [確認_正常系] - CRITICAL レベルで書き込めること。
    EXPECT_EQ(0,
              com_util_etw_provider_write(handle, 2, NULL, "error")); // [確認_正常系] - ERROR レベルで書き込めること。
    EXPECT_EQ(
        0, com_util_etw_provider_write(handle, 3, NULL, "warning")); // [確認_正常系] - WARNING レベルで書き込めること。
    EXPECT_EQ(0, com_util_etw_provider_write(handle, 4, NULL, "info")); // [確認_正常系] - INFO レベルで書き込めること。
    EXPECT_EQ(
        0, com_util_etw_provider_write(handle, 5, NULL, "verbose")); // [確認_正常系] - VERBOSE レベルで書き込めること。

    com_util_etw_provider_dispose(handle);
}

// NULL 引数が安全に無視されることの確認
TEST_F(trace_etwTest, test_null_arguments_are_safe)
{
    com_util_etw_provider *handle = com_util_etw_provider_create(s_test_provider);

    EXPECT_EQ((com_util_etw_provider *)NULL,
              com_util_etw_provider_create(NULL)); // [確認_異常系] - NULL provider_ref で create が失敗すること。
    EXPECT_EQ(0, com_util_etw_provider_write(NULL, 4, NULL,
                                             "test message"));        // [確認_異常系] - NULL ハンドルが安全であること。
    EXPECT_EQ(0, com_util_etw_provider_write(handle, 4, NULL, NULL)); // [確認_異常系] - NULL message が安全であること。

    com_util_etw_provider_dispose(handle);
    com_util_etw_provider_dispose(NULL); // [手順] - NULL ハンドルで dispose を呼ぶ。
}

#elif defined(PLATFORM_LINUX)

    #include <testfw.h>

TEST(trace_etwTest, not_supported)
{
    GTEST_SKIP() << "ETW is not supported on this platform";
}

#endif /* PLATFORM_ */
