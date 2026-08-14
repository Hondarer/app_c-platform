#include <testfw.h>
#include <mock_com_util.h>
#include "traceSyncMock.h"
#include <com_util/trace/tracer.h>
#include <com_util/trace/tracer_internal.h>

#if defined(PLATFORM_LINUX)
    #include <syslog.h>
#endif /* PLATFORM_LINUX */

using testing::_;
using testing::NiceMock;
using testing::Return;

class traceAllocFailureTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util;

#if defined(PLATFORM_LINUX)
    com_util_syslog_sink *os_handle_ = reinterpret_cast<com_util_syslog_sink *>(static_cast<uintptr_t>(0x1100));
#elif defined(PLATFORM_WINDOWS)
    com_util_etw_provider *os_handle_ = reinterpret_cast<com_util_etw_provider *>(static_cast<uintptr_t>(0x1100));
#endif /* PLATFORM_ */

    void SetUp() override
    {
        set_trace_sync_mock_defaults(mock_com_util);
        ON_CALL(mock_com_util, com_util_shutdown_register(_, _)).WillByDefault(Return(COM_UTIL_OK));
#if defined(PLATFORM_LINUX)
        ON_CALL(mock_com_util, com_util_syslog_sink_create(_, _)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_com_util, com_util_syslog_sink_dispose(_)).WillByDefault(Return());
#elif defined(PLATFORM_WINDOWS)
        ON_CALL(mock_com_util, com_util_etw_provider_create(_)).WillByDefault(Return(os_handle_));
        ON_CALL(mock_com_util, com_util_etw_provider_dispose(_)).WillByDefault(Return());
#endif /* PLATFORM_ */
    }
};

// ハンドルの確保に失敗した場合に生成が失敗することの確認
TEST_F(traceAllocFailureTest, create_returns_null_when_handle_allocation_fails)
{
    // Arrange
    // Pre-Assert
    /* com_util_tracer は不透明型でサイズを指定できない。生成時の malloc はハンドル確保の 1 回だけである */
    EXPECT_CALL(mock_com_util, com_util_malloc(_))
        .WillOnce(Return(nullptr)); // [Pre-Assert確認_異常系] - com_util_malloc がハンドル確保のために 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_malloc から NULL を返却する。
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_dispose(os_handle_))
        .Times(1); // [Pre-Assert確認_異常系] - 確保済みの syslog sink が 1 回破棄されること。
#endif             /* PLATFORM_LINUX */

    // Act
    com_util_tracer *handle = com_util_tracer_create(
        COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED); // [手順] - com_util_tracer_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_tracer *)NULL,
              handle);                            // [確認_異常系] - com_util_tracer_create の戻り値が NULL であること。
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_異常系] - registry へ登録されないこと。
}

#if defined(PLATFORM_LINUX)

// インスタンス名の複製に失敗した場合に生成が失敗することの確認
// Windows の com_util_tracer_create は _strdup を使うため、この失敗経路は Linux のみに存在する
TEST_F(traceAllocFailureTest, create_returns_null_when_name_duplication_fails)
{
    // Arrange
    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_strdup(_))
        .WillOnce(Return(nullptr)); // [Pre-Assert確認_異常系] - com_util_strdup が 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_strdup から NULL を返却する。
    EXPECT_CALL(mock_com_util, com_util_syslog_sink_dispose(os_handle_))
        .Times(1); // [Pre-Assert確認_異常系] - 確保済みの syslog sink が 1 回破棄されること。

    // Act
    com_util_tracer *handle = com_util_tracer_create(
        COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED); // [手順] - com_util_tracer_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_tracer *)NULL,
              handle);                            // [確認_異常系] - com_util_tracer_create の戻り値が NULL であること。
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_異常系] - registry へ登録されないこと。
}

#endif /* PLATFORM_LINUX */

// レジストリの拡張に失敗した場合に生成が失敗することの確認
TEST_F(traceAllocFailureTest, create_returns_null_when_registry_expansion_fails)
{
    // Arrange
    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_realloc(_, _, _))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - com_util_realloc がレジストリの拡張のために 1 回呼び出されること。
                              // [Pre-Assert手順] - com_util_realloc から NULL を返却する。

    // Act
    com_util_tracer *handle = com_util_tracer_create(
        COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED); // [手順] - com_util_tracer_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_tracer *)NULL,
              handle);                            // [確認_異常系] - com_util_tracer_create の戻り値が NULL であること。
    EXPECT_EQ((size_t)0, trace_registry_count()); // [確認_異常系] - registry が空のままであること。
}

// インスタンス識別付きの名前組み立てで確保に失敗した場合に設定が失敗することの確認
TEST_F(traceAllocFailureTest, set_name_fails_when_effective_name_allocation_fails)
{
    // Arrange
    com_util_tracer *handle = com_util_tracer_create(COM_UTIL_TRACER_CONCURRENCY_TRACER_MANAGED); // [状態] - 生成済みのトレース ハンドルを用意する。

    ASSERT_NE((com_util_tracer *)NULL, handle); // [状態確認] - ハンドルが非 NULL であること。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_malloc(_))
        .WillOnce(Return(nullptr)); // [Pre-Assert確認_異常系] - com_util_malloc が名前組み立て用に 1 回呼び出されること。
                                    // [Pre-Assert手順] - com_util_malloc から NULL を返却する。

    // Act
    int actual_ret =
        com_util_tracer_set_name(handle, "sample",
                                 42); // [手順] - インスタンス識別 42 を指定して com_util_tracer_set_name を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              actual_ret); // [確認_異常系] - com_util_tracer_set_name の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。

    // Cleanup
    com_util_tracer_dispose(&handle);
}
