#include <testfw.h>

#include <mock_com_util.h>
#include <mock_stdio.h>
#include <mock_stdlib.h>

#include <com_util/trace/trace_file.h>
#include <com_util/trace/backends/file/trace_file_internal.h>

#include "trace_file.inject.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;

namespace
{

constexpr uint64_t kFileIndex = 100;

static void set_file_id(com_util_file_id *id_out, const uint64_t volume, const uint64_t index)
{
    id_out->volume = volume;
    id_out->index = index;
}

} // namespace

class traceFileCoverageTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_;

    void SetUp() override
    {
        ON_CALL(mock_, com_util_get_realtime(_))
            .WillByDefault(
                [](com_util_timespec *timestamp)
                {
                    timestamp->tv_sec = 1714100645LL;
                    timestamp->tv_nsec = 678000000;
                });
        ON_CALL(mock_, com_util_format_realtime_iso8601_local(_, _, _))
            .WillByDefault(
                [](char *buf, const size_t buf_size, const com_util_timespec *)
                {
                    snprintf(buf, buf_size, "%s", "2026-04-26T03:04:05.678+09:00");
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_, com_util_file_open(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_file_get_size(_, _, _))
            .WillByDefault(
                [](const com_util_file *, size_t *size_out, com_util_error *)
                {
                    *size_out = 0;
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_, com_util_file_write(_, _, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_file_close(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_remove(_, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_rename(_, _, _)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_file_get_id(_, _, _))
            .WillByDefault(
                [](const com_util_file *, com_util_file_id *id_out, com_util_error *)
                {
                    set_file_id(id_out, 1, kFileIndex);
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_, com_util_file_get_path_id(_, _, _))
            .WillByDefault(
                [](const char *, com_util_file_id *id_out, com_util_error *)
                {
                    set_file_id(id_out, 1, kFileIndex);
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_, com_util_interprocess_lock_open(_, _))
            .WillByDefault(
                [](const char *, com_util_interprocess_lock **lock)
                {
                    static int dummy_lock = 0;
                    *lock = (com_util_interprocess_lock *)&dummy_lock;
                    return COM_UTIL_OK;
                });
        ON_CALL(mock_, com_util_interprocess_lock_try_lock(_)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_interprocess_lock_unlock(_)).WillByDefault(Return(COM_UTIL_OK));
        ON_CALL(mock_, com_util_interprocess_lock_destroy(_)).WillByDefault(Return());
    }
};

// フル パス解決に失敗した場合に元のパスで sink を登録することの確認
TEST_F(traceFileCoverageTest, create_uses_original_path_when_full_path_resolution_fails)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_path_get_full(_, _, _, StrEq("relative.log")))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - フル パス解決を 1 回呼び出すこと。
                                             // [Pre-Assert手順] - フル パス解決から COM_UTIL_ERR_UNKNOWN を返却する。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("relative.log"), _, _))
        .WillOnce(Return(COM_UTIL_OK)); // [Pre-Assert確認_正常系] - 元のパスでファイルを開くこと。

    // Act
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "relative.log", 0, 0, 0); // [手順] - フル パス解決が失敗する相対パスで sink を生成する。

    // Assert
    ASSERT_NE((com_util_trace_file_sink *)NULL,
              handle); // [確認_正常系] - com_util_trace_file_sink_create の戻り値が NULL でないこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// レジストリの後方検索と容量再拡張を行うことの確認
TEST_F(traceFileCoverageTest, registry_expands_and_finds_later_sink)
{
    // Arrange
    std::vector<com_util_trace_file_sink *> handles;

    // Pre-Assert

    // Act
    for (int index = 0; index < 9; ++index)
    {
        std::string path = "registry-" + std::to_string(index) + ".log";
        handles.push_back(com_util_trace_file_sink_create(
            path.c_str(), 0, 0, 0)); // [手順] - 異なるパスで 9 個の sink を生成する。
    }
    com_util_trace_file_sink_dispose(
        handles.back()); // [手順] - レジストリの後方にある sink を先に破棄する。
    handles.pop_back();

    // Assert
    for (com_util_trace_file_sink *handle : handles)
    {
        ASSERT_NE((com_util_trace_file_sink *)NULL,
                  handle); // [確認_正常系] - 9 個の com_util_trace_file_sink_create の戻り値が NULL でないこと。
    }

    // Cleanup
    for (com_util_trace_file_sink *handle : handles)
    {
        com_util_trace_file_sink_dispose(handle);
    }
}

// 親ディレクトリ解決と初期サイズ取得に失敗しても sink を生成することの確認
TEST_F(traceFileCoverageTest, create_accepts_empty_path_and_size_query_failure)
{
    // Arrange

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 初期ファイル サイズを 1 回取得すること。
                                             // [Pre-Assert手順] - サイズ取得から COM_UTIL_ERR_UNKNOWN を返却する。

    // Act
    com_util_trace_file_sink *handle =
        com_util_trace_file_sink_create("", 0, 0, 0); // [手順] - 空パスで sink を生成する。

    // Assert
    ASSERT_NE((com_util_trace_file_sink *)NULL,
              handle); // [確認_正常系] - com_util_trace_file_sink_create の戻り値が NULL でないこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有ファイルの同一性を取得できない場合に書き込み前に開き直すことの確認
TEST_F(traceFileCoverageTest, shared_write_reopens_when_file_identity_is_unavailable)
{
    // Arrange
    EXPECT_CALL(mock_, com_util_file_get_id(_, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillOnce(DoDefault());
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "identity-unavailable.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle); // [状態] - ファイル同一性を保持しない共有 sink を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_close(_, _))
        .Times(2); // [Pre-Assert確認_異常系] - 開き直し前と sink 破棄時にファイルを閉じること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("identity-unavailable.log"), _, _))
        .WillOnce(Return(COM_UTIL_OK)); // [Pre-Assert確認_異常系] - ファイルを開き直すこと。

    // Act
    int result = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "message"); // [手順] - 同一性不明の共有 sink へ書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値が COM_UTIL_OK であること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有ファイルのパス同一性取得に失敗した場合に開き直すことの確認
TEST_F(traceFileCoverageTest, shared_write_reopens_when_path_identity_query_fails)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "path-identity-error.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("path-identity-error.log"), _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - パスのファイル同一性を取得すること。
                                             // [Pre-Assert手順] - 同一性取得から COM_UTIL_ERR_UNKNOWN を返却する。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("path-identity-error.log"), _, _))
        .WillOnce(Return(COM_UTIL_OK)); // [Pre-Assert確認_異常系] - ファイルを開き直すこと。

    // Act
    int result = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "message"); // [手順] - パス同一性を取得できない共有 sink へ書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値が COM_UTIL_OK であること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// volume が異なるファイルを別実体と判定することの確認
TEST_F(traceFileCoverageTest, shared_write_reopens_when_file_volume_changes)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "volume-changed.log", 0, 0, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_get_path_id(StrEq("volume-changed.log"), _, _))
        .WillOnce(
            [](const char *, com_util_file_id *id_out, com_util_error *)
            {
                set_file_id(id_out, 2, kFileIndex);
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_異常系] - volume の異なるファイル同一性を返却すること。
    EXPECT_CALL(mock_, com_util_file_open(_, StrEq("volume-changed.log"), _, _))
        .WillOnce(Return(COM_UTIL_OK)); // [Pre-Assert確認_異常系] - ファイルを開き直すこと。

    // Act
    int result = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "message"); // [手順] - volume が変化した共有 sink へ書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_trace_file_sink_write の戻り値が COM_UTIL_OK であること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// ローテーション中の rename 失敗時にカスケードを中止することの確認
TEST_F(traceFileCoverageTest, rotation_stops_after_rename_failure)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("rename-error.log", 1, 2, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_rename(_, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 最初の rename を 1 回呼び出すこと。
                                             // [Pre-Assert手順] - rename から COM_UTIL_ERR_UNKNOWN を返却する。

    // Act
    int result = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "message"); // [手順] - 1 byte 上限の sink へ書き込んでローテーションする。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - rename 失敗をベスト エフォートで扱い、com_util_trace_file_sink_write が COM_UTIL_OK を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 共有ローテーションのサイズ取得失敗とロック後の再確認を扱うことの確認
TEST_F(traceFileCoverageTest, shared_rotation_handles_size_query_outcomes)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "shared-size.log", 10, 2, COM_UTIL_TRACE_FILE_SINK_SHARED);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 10;
                return COM_UTIL_OK;
            })
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 10;
                return COM_UTIL_OK;
            })
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = 9;
                return COM_UTIL_OK;
            }); // [Pre-Assert確認_異常系] - 初回取得失敗、ロック後取得失敗、ロック後上限未満の順でサイズ取得を行うこと。
                // [Pre-Assert手順] - 各書き込みのサイズ取得結果を順番に返却する。

    // Act
    int initial_query_error = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "first"); // [手順] - 初回サイズ取得が失敗する条件で書き込む。
    int locked_query_error = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "second"); // [手順] - ロック後のサイズ再取得が失敗する条件で書き込む。
    int below_limit = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "third"); // [手順] - ロック後の再取得サイズが上限未満の条件で書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, initial_query_error); // [確認_異常系] - 初回サイズ取得失敗時も書き込みが成功扱いになること。
    EXPECT_EQ(COM_UTIL_OK, locked_query_error);  // [確認_異常系] - ロック後のサイズ取得失敗時も書き込みが成功扱いになること。
    EXPECT_EQ(COM_UTIL_OK, below_limit);         // [確認_正常系] - ロック後のサイズが上限未満ならローテーションを見送うこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// create のロック生成失敗とパス長上限を拒否することの確認
TEST_F(traceFileCoverageTest, create_rejects_lock_failure_and_long_path)
{
    // Arrange
    std::string long_path(PLATFORM_PATH_MAX, 'x');

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_local_lock_create(_))
        .WillOnce(DoDefault())
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - レジストリ用ロックに続く sink 用ロックを生成すること。
                                                 // [Pre-Assert手順] - sink 用ロック生成から COM_UTIL_ERR_UNKNOWN を返却する。

    // Act
    com_util_trace_file_sink *lock_failure = com_util_trace_file_sink_create(
        "lock-create-error.log", 0, 0, 0); // [手順] - ロック生成が失敗する条件で sink を生成する。
    com_util_trace_file_sink *too_long = com_util_trace_file_sink_create(
        long_path.c_str(), 0, 0, 0); // [手順] - PLATFORM_PATH_MAX バイトのパスで sink を生成する。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              lock_failure); // [確認_異常系] - ロック生成失敗時の com_util_trace_file_sink_create の戻り値が NULL であること。
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              too_long); // [確認_異常系] - 長すぎるパス指定時の com_util_trace_file_sink_create の戻り値が NULL であること。
}

// write が不正引数と依存処理の失敗を返すことの確認
TEST_F(traceFileCoverageTest, write_handles_invalid_arguments_and_dependency_failures)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("write-errors.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_get_realtime(_))
        .WillOnce(
            [](com_util_timespec *timestamp)
            {
                timestamp->tv_sec = 1;
                timestamp->tv_nsec = -1;
            })
        .WillOnce(
            [](com_util_timespec *timestamp)
            {
                timestamp->tv_sec = 1;
                timestamp->tv_nsec = 0;
            }); // [Pre-Assert確認_異常系] - 時刻解決を不正値と正常値の順で呼び出すこと。
                // [Pre-Assert手順] - 初回は不正なナノ秒、2 回目は正常な時刻を返却する。
    EXPECT_CALL(mock_, com_util_format_realtime_iso8601_local(_, _, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN)); // [Pre-Assert確認_異常系] - 正常な時刻の書式化を 1 回呼び出すこと。
                                                 // [Pre-Assert手順] - 書式化から COM_UTIL_ERR_UNKNOWN を返却する。

    // Act
    int null_handle = com_util_trace_file_sink_write(
        NULL, COM_UTIL_TRACE_LEVEL_INFO, NULL, "message"); // [手順] - NULL handle で書き込む。
    int null_message = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, NULL); // [手順] - NULL message で書き込む。

    int timestamp_error = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "message"); // [手順] - 現在時刻の解決に失敗する条件で書き込む。
    int format_error = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "message"); // [手順] - タイムスタンプ書式化に失敗する条件で書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, null_handle);              // [確認_異常系] - NULL handle の com_util_trace_file_sink_write の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, null_message);             // [確認_異常系] - NULL message の com_util_trace_file_sink_write の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, timestamp_error); // [確認_異常系] - 時刻解決失敗時の com_util_trace_file_sink_write の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, format_error);    // [確認_異常系] - 時刻書式化失敗時の com_util_trace_file_sink_write の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// write の書式化失敗、切り詰め、ロック失敗を扱うことの確認
TEST_F(traceFileCoverageTest, write_handles_format_truncation_and_lock_failure)
{
    // Arrange
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create("write-format.log", 0, 0, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle);
    std::string long_message(3000, 'x');

    // Pre-Assert
    NiceMock<Mock_stdio> mock_stdio;
    EXPECT_CALL(mock_stdio, snprintf(_, _, _, _, _, _))
        .WillOnce(Return(-1))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - トレース行の書式化を 3 回呼び出すこと。
                                      // [Pre-Assert手順] - 初回は -1 を返し、以降は本物へ委譲する。
    EXPECT_CALL(mock_, com_util_local_lock_lock(_, COM_UTIL_SYNC_WAIT_FOREVER)).WillOnce(DoDefault());
    EXPECT_CALL(mock_, com_util_local_lock_lock(_, 100))
        .WillOnce(Return(COM_UTIL_OK))
        .WillOnce(Return(COM_UTIL_ERR_TIMEOUT)); // [Pre-Assert確認_異常系] - 書式化成功後の書き込みロックを 2 回取得すること。
                                                 // [Pre-Assert手順] - 初回は成功し、2 回目は COM_UTIL_ERR_TIMEOUT を返却する。

    // Act
    int snprintf_error = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "message"); // [手順] - トレース行の書式化が失敗する条件で書き込む。
    int truncated = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        long_message.c_str()); // [手順] - 行バッファーを超えるメッセージを書き込む。
    int lock_error = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL, "message"); // [手順] - 書き込みロックを取得できない条件で書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, snprintf_error); // [確認_異常系] - 行書式化失敗時の com_util_trace_file_sink_write の戻り値が COM_UTIL_ERR_UNKNOWN であること。
    EXPECT_EQ(COM_UTIL_OK, truncated);               // [確認_正常系] - 長大メッセージを切り詰めた com_util_trace_file_sink_write の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN, lock_error);     // [確認_異常系] - ロック取得失敗時の com_util_trace_file_sink_write の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// サイズ加算がオーバーフローする場合に加算しないことの確認
TEST_F(traceFileCoverageTest, write_avoids_current_size_overflow)
{
    // Arrange
    EXPECT_CALL(mock_, com_util_file_get_size(_, _, _))
        .WillOnce(
            [](const com_util_file *, size_t *size_out, com_util_error *)
            {
                *size_out = std::numeric_limits<size_t>::max();
                return COM_UTIL_OK;
            });
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "size-overflow.log", std::numeric_limits<size_t>::max(), 1, 0);
    ASSERT_NE((com_util_trace_file_sink *)NULL, handle); // [状態] - current_bytes が SIZE_MAX の sink を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_, com_util_file_write(_, _, _, _))
        .WillOnce(Return(COM_UTIL_OK)); // [Pre-Assert確認_正常系] - ファイル書き込みを 1 回呼び出すこと。

    // Act
    int result = com_util_trace_file_sink_write(
        handle, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "message"); // [手順] - current_bytes に行長を加算するとオーバーフローする条件で書き込む。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - サイズ加算を行わずに com_util_trace_file_sink_write が COM_UTIL_OK を返すこと。

    // Cleanup
    com_util_trace_file_sink_dispose(handle);
}

// 登録済みと未登録の sink を通常と shutdown 経路で破棄できることの確認
TEST_F(traceFileCoverageTest, dispose_handles_registered_and_unregistered_sinks)
{
    // Arrange
    com_util_trace_file_sink *first = com_util_trace_file_sink_create("shutdown-shared.log", 0, 0, 0);
    com_util_trace_file_sink *second = com_util_trace_file_sink_create("shutdown-shared.log", 0, 0, 0);
    com_util_trace_file_sink *normal_unregistered =
        test_trace_file_sink_create_unregistered("normal-unregistered.log", 0, 0, 0);
    com_util_trace_file_sink *shutdown_unregistered =
        test_trace_file_sink_create_unregistered("shutdown-unregistered.log", 0, 0, 0);
    ASSERT_EQ(first, second); // [状態] - 同一 sink の参照カウントが 2 の状態とする。

    // Pre-Assert

    // Act
    com_util_trace_file_sink_dispose_on_shutdown(NULL); // [手順] - NULL sink を shutdown 経路で破棄する。
    com_util_trace_file_sink_dispose_on_shutdown(first); // [手順] - 2 参照の sink を 1 回 shutdown 経路で破棄する。
    int write_result = com_util_trace_file_sink_write(
        second, COM_UTIL_TRACE_LEVEL_INFO, NULL,
        "still alive"); // [手順] - 残る 1 参照で書き込む。
    com_util_trace_file_sink_dispose_on_shutdown(second); // [手順] - 最後の参照を shutdown 経路で破棄する。
    com_util_trace_file_sink_dispose(normal_unregistered); // [手順] - 未登録 sink を通常経路で破棄する。
    com_util_trace_file_sink_dispose_on_shutdown(
        shutdown_unregistered); // [手順] - 未登録 sink を shutdown 経路で破棄する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              write_result); // [確認_正常系] - 参照カウントが残る sink への com_util_trace_file_sink_write が COM_UTIL_OK を返すこと。
}

// 共有 sink の lock-path 確保に失敗した場合に生成を中止することの確認
TEST_F(traceFileCoverageTest, create_shared_returns_null_when_lock_path_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(DoDefault())
        .WillOnce(DoDefault())
        .WillOnce(DoDefault())
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - 4 回目の malloc が lock-path 確保のために呼び出されること。
                                      // [Pre-Assert手順] - 4 回目の malloc から NULL を返却する。

    // Act
    com_util_trace_file_sink *handle = com_util_trace_file_sink_create(
        "lock-path-allocation.log", 0, 0,
        COM_UTIL_TRACE_FILE_SINK_SHARED); // [手順] - lock-path 確保が失敗する条件で共有 sink を生成する。

    // Assert
    EXPECT_EQ((com_util_trace_file_sink *)NULL,
              handle); // [確認_異常系] - com_util_trace_file_sink_create の戻り値が NULL であること。
}
