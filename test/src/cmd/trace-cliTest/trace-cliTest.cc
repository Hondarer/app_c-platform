#include <testfw.h>
#include <mock_stdio.h>
#include <mock_com_util.h>
#include "trace-cli.h"

#include <vector>
#include <string>
#include <cerrno>
#include <cstring>
#include <cstdint>

using testing::_;
using testing::AtLeast;
using testing::HasSubstr;
using testing::NiceMock;
using testing::AnyNumber;
using testing::Return;
using testing::StrEq;

namespace
{

static char *copy_line(char *dst, int size, const char *src)
{
    size_t len = strlen(src);

    if (size <= 0)
    {
        return NULL;
    }

    if (len >= (size_t)size)
    {
        len = (size_t)size - 1U;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

static int emulate_com_util_strncpy(char *dest, size_t dest_size, const char *src, size_t count)
{
    size_t len;

    if (dest == NULL || dest_size == 0U || src == NULL)
    {
        return EINVAL;
    }

    len = strlen(src);
    if (len > count)
    {
        len = count;
    }
    if (len >= dest_size)
    {
        len = dest_size - 1U;
    }

    memcpy(dest, src, len);
    dest[len] = '\0';
    return 0;
}

} // namespace

class trace_cliTest : public Test
{
protected:
    NiceMock<Mock_stdio> mock_stdio_;
    NiceMock<Mock_com_util> mock_com_util_;
    trace_cli_session_t session_{};
    com_util_tracer_t *handle_ =
        reinterpret_cast<com_util_tracer_t *>(static_cast<uintptr_t>(0x1234));

    void SetUp() override
    {
        trace_cli_session_init(&session_);
        ON_CALL(mock_com_util_, com_util_strncpy(_, _, _, _))
            .WillByDefault(emulate_com_util_strncpy);
        ON_CALL(mock_com_util_, com_util_tracer_dispose(_))
            .WillByDefault(Return());
        ON_CALL(mock_stdio_, printf(_, _, _, _))
            .WillByDefault(Return(0));
        ON_CALL(mock_stdio_, fprintf(_, _, _, _, _))
            .WillByDefault(Return(0));
    }

    void TearDown() override
    {
        trace_cli_session_dispose(&session_);
    }
};

TEST_F(trace_cliTest, process_line_create_and_reject_second_create)
{
    // Arrange
    EXPECT_CALL(mock_com_util_, com_util_tracer_create())
        .WillOnce(Return(handle_)); // [Pre-Assert確認] - create で tracer handle が 1 回生成されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("handle=created\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - create 成功時の結果が出力されること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("既存の handle")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - 2 回目の create で操作エラーが出力されること。

    // Act
    int first = trace_cli_process_line(&session_, "create");
    int second = trace_cli_process_line(&session_, "create");

    // Assert
    EXPECT_EQ(0, first); // [確認] - 1 回目の create が継続扱いで完了すること。
    EXPECT_LT(second, 0); // [確認] - 2 回目の create がエラー扱いになること。
    EXPECT_EQ(handle_, session_.handle); // [確認] - session に生成済み handle が保持されること。
}

TEST_F(trace_cliTest, process_line_set_file_level_accepts_null_keyword)
{
    // Arrange
    session_.handle = handle_; // [状態] - 既存 handle を持つ session を用意する。
    EXPECT_CALL(mock_com_util_,
                com_util_tracer_set_file_level(handle_, nullptr, COM_UTIL_TRACE_LEVEL_INFO, 0U, 0))
        .WillOnce(Return(0)); // [Pre-Assert確認] - null keyword が NULL path として渡されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("rc=0\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - set-file-level の戻り値が表示されること。

    // Act
    int rc = trace_cli_process_line(&session_, "set-file-level null INFO");

    // Assert
    EXPECT_EQ(0, rc); // [確認] - set-file-level が正常に処理されること。
}

TEST_F(trace_cliTest, process_line_dispose_releases_handle)
{
    // Arrange
    session_.handle = handle_; // [状態] - 既存 handle を持つ session を用意する。
    EXPECT_CALL(mock_com_util_, com_util_tracer_dispose(_))
        .Times(AnyNumber())
        .WillRepeatedly(Return()); // [Pre-Assert確認] - 後処理で発生する NULL dispose は許容すること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_dispose(handle_))
        .WillOnce(Return()); // [Pre-Assert確認] - dispose が保持中 handle を解放すること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("handle=disposed\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - dispose 結果が出力されること。

    // Act
    int rc = trace_cli_process_line(&session_, "dispose");

    // Assert
    EXPECT_EQ(0, rc); // [確認] - dispose が正常に処理されること。
    EXPECT_EQ(nullptr, session_.handle); // [確認] - session から handle が外れること。
}

TEST_F(trace_cliTest, process_line_write_hex_parses_quoted_hex_and_label)
{
    // Arrange
    session_.handle = handle_; // [状態] - started 済み handle 相当の session を用意する。
    EXPECT_CALL(mock_com_util_,
                _com_util_tracer_write_hex(handle_, COM_UTIL_TRACE_LEVEL_INFO, nullptr, _, 3U, StrEq("payload bytes")))
        .WillOnce([](com_util_tracer_t *, com_util_trace_level_t, const com_util_realtime_timestamp_t *, const void *data,
                     size_t size, const char *) {
            const unsigned char *bytes = static_cast<const unsigned char *>(data);
            EXPECT_EQ((size_t)3, size); // [確認] - 変換後データ長が 3 byte であること。
            EXPECT_EQ((unsigned char)0x01, bytes[0]); // [確認] - 先頭 byte が 0x01 であること。
            EXPECT_EQ((unsigned char)0xAB, bytes[1]); // [確認] - 2 byte 目が 0xAB であること。
            EXPECT_EQ((unsigned char)0xFF, bytes[2]); // [確認] - 3 byte 目が 0xFF であること。
            return 0;
        }); // [Pre-Assert確認] - quoted hex と label が write-hex に渡されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("rc=0\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - write-hex の戻り値が表示されること。

    // Act
    int rc = trace_cli_process_line(&session_, "write-hex INFO \"01 AB FF\" payload bytes");

    // Assert
    EXPECT_EQ(0, rc); // [確認] - write-hex が正常に処理されること。
}

TEST_F(trace_cliTest, process_line_writef_uses_message_as_single_string)
{
    // Arrange
    session_.handle = handle_; // [状態] - started 済み handle 相当の session を用意する。
    EXPECT_CALL(mock_com_util_,
                _com_util_tracer_writef(handle_, COM_UTIL_TRACE_LEVEL_DEBUG, nullptr,
                                        StrEq("message with spaces")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - writef が行末までを 1 つの文字列として受け取ること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("rc=0\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - writef の戻り値が表示されること。

    // Act
    int rc = trace_cli_process_line(&session_, "writef DEBUG message with spaces");

    // Assert
    EXPECT_EQ(0, rc); // [確認] - writef が正常に処理されること。
}

TEST_F(trace_cliTest, process_line_get_os_level_calls_api_with_null_handle)
{
    // Arrange
    EXPECT_CALL(mock_com_util_, com_util_tracer_get_os_level(nullptr))
        .WillOnce(Return(COM_UTIL_TRACE_LEVEL_WARNING)); // [Pre-Assert確認] - NULL handle のまま getter が呼び出されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("level=WARNING(2)\n")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - getter 結果が文字列と enum 値で表示されること。

    // Act
    int rc = trace_cli_process_line(&session_, "get-os-level");

    // Assert
    EXPECT_EQ(0, rc); // [確認] - get-os-level が正常に処理されること。
}

TEST_F(trace_cliTest, process_line_help_prints_command_list)
{
    // Arrange
    EXPECT_CALL(mock_stdio_, printf(_, _, _, _))
        .Times(AnyNumber())
        .WillRepeatedly(Return(0)); // [Pre-Assert確認] - help の複数行 stdout 出力を許容すること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("trace-cli")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - help 見出しが stdout に出力されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("write-hexf")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - コマンド一覧が stdout に出力されること。

    // Act
    int rc = trace_cli_process_line(&session_, "help");

    // Assert
    EXPECT_EQ(0, rc); // [確認] - help が正常に処理されること。
}

TEST_F(trace_cliTest, process_line_quit_requests_exit)
{
    // Act
    int rc = trace_cli_process_line(&session_, "quit");

    // Assert
    EXPECT_EQ(1, rc); // [確認] - quit が終了要求として処理されること。
    EXPECT_EQ(1, session_.exit_requested); // [確認] - session に終了要求が保持されること。
}

TEST_F(trace_cliTest, main_rejects_extra_arguments)
{
    // Arrange
    int argc = 2;
    const char *argv[] = {"trace-cli", "--help"}; // [状態] - 想定外の起動引数を 1 個付与する。
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(Return()); // [Pre-Assert確認] - main() 起動時に console 初期化が呼ばれること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("使用方法: trace-cli")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - 使用方法が stderr に出力されること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("help")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - help への案内が stderr に出力されること。

    // Act
    int rc = __real_main(argc, (char **)&argv);

    // Assert
    EXPECT_NE(0, rc); // [確認] - 不正な起動引数で失敗終了すること。
}

TEST_F(trace_cliTest, main_runs_interactive_sequence_and_disposes_handle)
{
    // Arrange
    int argc = 1;
    const char *argv[] = {"trace-cli"}; // [状態] - 引数なしで対話モード起動する。
    std::vector<std::string> lines = {
        "create",
        "start",
        "write INFO hello world",
        "stop",
        "dispose",
        "exit",
    };
    size_t index = 0U;
    com_util_prompt_t *prompt_handle =
        reinterpret_cast<com_util_prompt_t *>(static_cast<uintptr_t>(0x5678));

    testing::Sequence io_seq;

    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(Return()); // [Pre-Assert確認] - main() 起動時に console 初期化が呼ばれること。
    EXPECT_CALL(mock_com_util_, com_util_prompt_create(_))
        .WillOnce(Return(prompt_handle)); // [Pre-Assert確認] - main() 起動時に prompt が生成されること。
    EXPECT_CALL(mock_com_util_, com_util_prompt_dispose(prompt_handle))
        .WillOnce(Return()); // [Pre-Assert確認] - main() 終了時に prompt が解放されること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_dispose(_))
        .Times(AnyNumber())
        .WillRepeatedly(Return()); // [Pre-Assert確認] - 後処理で発生する NULL dispose は許容すること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_create())
        .WillOnce(Return(handle_)); // [Pre-Assert確認] - create コマンドで tracer handle が生成されること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_get_state(handle_))
        .WillOnce(Return(COM_UTIL_TRACER_STATE_STOPPED))
        .WillOnce(Return(COM_UTIL_TRACER_STATE_STARTED))
        .WillOnce(Return(COM_UTIL_TRACER_STATE_STARTED))
        .WillOnce(Return(COM_UTIL_TRACER_STATE_STOPPED)); // [Pre-Assert確認] - prompt が状態遷移に応じた getter を参照すること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_start(handle_))
        .WillOnce(Return(0)); // [Pre-Assert確認] - start コマンドで tracer が開始されること。
    EXPECT_CALL(mock_com_util_, _com_util_tracer_write(handle_, COM_UTIL_TRACE_LEVEL_INFO, nullptr, StrEq("hello world")))
        .WillOnce(Return(0)); // [Pre-Assert確認] - write コマンドで message がそのまま渡されること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_stop(handle_))
        .WillOnce(Return(0)); // [Pre-Assert確認] - stop コマンドで tracer が停止されること。
    EXPECT_CALL(mock_com_util_, com_util_tracer_dispose(handle_))
        .WillOnce(Return()); // [Pre-Assert確認] - dispose コマンドで保持中 handle が解放されること。
    EXPECT_CALL(mock_com_util_, _com_util_prompt_readline_fmt(prompt_handle, _, _, _, _, _, _))
        .WillRepeatedly([&](com_util_prompt_t *, char *buf, size_t buf_size,
                            const char *, int, const char *, va_list) -> int {
            if (index >= lines.size())
            {
                return 0;
            }
            copy_line(buf, (int)buf_size, lines[index++].c_str());
            return 1;
        }); // [Pre-Assert確認] - REPL 入力が順に main() へ供給されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, _))
        .Times(AnyNumber())
        .WillRepeatedly(Return(0)); // [Pre-Assert確認] - help を含むその他の stdout 出力は許容すること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("handle=created\n")))
        .InSequence(io_seq)
        .WillOnce(Return(0)); // [Pre-Assert確認] - create 結果が出力されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("rc=0\n")))
        .InSequence(io_seq)
        .WillOnce(Return(0)); // [Pre-Assert確認] - start の戻り値が rc=0 として表示されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("rc=0\n")))
        .InSequence(io_seq)
        .WillOnce(Return(0)); // [Pre-Assert確認] - write の戻り値が rc=0 として表示されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("rc=0\n")))
        .InSequence(io_seq)
        .WillOnce(Return(0)); // [Pre-Assert確認] - stop の戻り値が rc=0 として表示されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, StrEq("handle=disposed\n")))
        .InSequence(io_seq)
        .WillOnce(Return(0)); // [Pre-Assert確認] - dispose 結果が出力されること。

    // Act
    int rc = __real_main(argc, (char **)&argv);

    // Assert
    EXPECT_EQ(0, rc); // [確認] - 対話シーケンスが正常終了すること。
}
