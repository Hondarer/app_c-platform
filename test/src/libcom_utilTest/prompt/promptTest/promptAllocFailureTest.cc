#include <testfw.h>
#include <mock_com_util.h>
#include <mock_stdlib.h>
#include <com_util/base/result.h>
#include <com_util/prompt/prompt.h>
#include <com_util/prompt/prompt_internal.h>

#include <cstring>
#include <string>

#include "promptPlatformFake.h"

class promptAllocFailureTest : public Test
{
  protected:
    com_util_prompt *prompt_ = NULL;

    void SetUp() override
    {
        promptFakeReset();
        prompt_ = com_util_prompt_create(NULL);
        ASSERT_NE((com_util_prompt *)NULL, prompt_);
        /* 端末に接続していない実行環境でも対話パスを通すため、TTY 状態を直接立てる */
        prompt_->is_tty = 1;
    }

    void TearDown() override
    {
        com_util_prompt_dispose(prompt_);
        prompt_ = NULL;
    }
};

// ハンドルの確保に失敗した場合に生成が失敗することの確認
TEST_F(promptAllocFailureTest, create_returns_null_when_handle_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, calloc(_, _, _, 1u, _))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - calloc が要素数 1 を指定してハンドル確保のために 1 回呼び出されること。
                              // [Pre-Assert手順] - calloc から NULL を返却する。

    // Act
    com_util_prompt *handle = com_util_prompt_create(NULL); // [手順] - com_util_prompt_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_prompt *)NULL,
              handle); // [確認_異常系] - com_util_prompt_create の戻り値が NULL であること。
}

// 編集バッファーの確保に失敗した場合に生成が失敗することの確認
TEST_F(promptAllocFailureTest, create_returns_null_when_edit_buffer_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - malloc が編集バッファー確保のために 1 回呼び出されること。
                              // [Pre-Assert手順] - malloc から NULL を返却する。

    // Act
    com_util_prompt *handle = com_util_prompt_create(NULL); // [手順] - com_util_prompt_create を呼び出す。

    // Assert
    EXPECT_EQ((com_util_prompt *)NULL,
              handle); // [確認_異常系] - com_util_prompt_create の戻り値が NULL であること。
}

// コンテキスト配列の拡張に失敗した場合に fgets へフォールバックすることの確認
TEST_F(promptAllocFailureTest, readline_falls_back_when_context_expansion_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    char buf[32]; // [状態] - 出力バッファーを用意する。

    promptFakeSetInput("abc\r");

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - realloc がコンテキスト配列の拡張のために 1 回呼び出されること。
                              // [Pre-Assert手順] - realloc から NULL を返却する。

    // Act
    int rtc = com_util_prompt_readline_at(prompt_, buf, sizeof(buf), ">> ", "promptAllocFailureTest.cc",
                                          1); // [手順] - 1 行読み取る。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_EOF,
        rtc); // [確認_異常系] - コンテキストを取得できず fgets へフォールバックし、標準入力が EOF のため COM_UTIL_ERR_EOF が返ること。
    EXPECT_EQ(0, promptFakeEnterRawCount()); // [確認_異常系] - raw モードへ移行しないこと。
}

// 履歴の退避バッファーの確保に失敗した場合に fgets へフォールバックすることの確認
TEST_F(promptAllocFailureTest, readline_falls_back_when_saved_line_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    char buf[32]; // [状態] - 出力バッファーを用意する。

    promptFakeSetInput("abc\r");

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - malloc が退避バッファー確保のために 1 回呼び出されること。
                              // [Pre-Assert手順] - malloc から NULL を返却する。

    // Act
    int rtc = com_util_prompt_readline_at(prompt_, buf, sizeof(buf), ">> ", "promptAllocFailureTest.cc",
                                          2); // [手順] - 1 行読み取る。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_EOF,
        rtc); // [確認_異常系] - コンテキストを取得できず fgets へフォールバックし、標準入力が EOF のため COM_UTIL_ERR_EOF が返ること。
    EXPECT_EQ(0, promptFakeEnterRawCount()); // [確認_異常系] - raw モードへ移行しないこと。
}

// 履歴エントリの確保に失敗しても行の確定が成功することの確認
TEST_F(promptAllocFailureTest, readline_succeeds_when_history_entry_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    char buf[32]; // [状態] - 出力バッファーを用意する。

    promptFakeSetInput("abc\r");
    /* 1 回目はコンテキスト生成時の退避バッファー、2 回目が履歴エントリの確保になる */

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(DoDefault())
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - malloc が 2 回呼び出されること。
                              // [Pre-Assert手順] - 1 回目は本物へ委譲し、2 回目は NULL を返却する。

    // Act
    int rtc = com_util_prompt_readline_at(prompt_, buf, sizeof(buf), ">> ", "promptAllocFailureTest.cc",
                                          3); // [手順] - "abc" と Enter を入力して 1 行読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - 履歴へ残せなくても com_util_prompt_readline_at は COM_UTIL_OK を返すこと。
    EXPECT_STREQ("abc", buf);    // [確認_正常系] - 入力した "abc" が返ること。
}

// 書式バッファーの確保に失敗した場合に空のプロンプトで継続することの確認
TEST_F(promptAllocFailureTest, readline_fmt_continues_with_empty_prompt_when_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    char buf[32]; // [状態] - 出力バッファーを用意する。

    promptFakeSetInput("abc\r");

    // Pre-Assert
    /* 書式バッファー以外の確保 (コンテキストの退避バッファーなど) は本物へ委譲する。
       gMock は後から宣言した期待値を優先するため、汎用の期待値を先に宣言する */
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_正常系] - 書式バッファー以外の malloc が任意の回数呼び出されること。
                                      // [Pre-Assert手順] - 本物の malloc へ委譲する。
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, 256u))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - malloc が書式バッファーの初期容量 256 を指定して 1 回呼び出されること。
                              // [Pre-Assert手順] - malloc から NULL を返却する。

    // Act
    int rtc = com_util_prompt_readline_fmt_at(prompt_, buf, sizeof(buf), "promptAllocFailureTest.cc", 4, "[%d] ",
                                              7); // [手順] - 書式付きプロンプトで 1 行読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - 空のプロンプトで継続し COM_UTIL_OK が返ること。
    EXPECT_STREQ("abc", buf);    // [確認_正常系] - 入力した "abc" が返ること。
}

// 書式バッファーの再確保に失敗した場合に切り捨てて継続することの確認
TEST_F(promptAllocFailureTest, readline_fmt_truncates_prompt_when_reallocation_fails)
{
    // Arrange
    char buf[32];
    std::string long_prompt(512u, 'p'); // [状態] - 初期容量 256 を超える長さのプロンプト文字列を用意する。

    /* 同じ呼び出し位置で 1 度読み取り、コンテキストと書式バッファーを確保済みにする。
       これにより Act 中の realloc は書式バッファーの拡張だけになる */
    promptFakeSetInput("x\r");
    ASSERT_EQ(COM_UTIL_OK, com_util_prompt_readline_fmt_at(prompt_, buf, sizeof(buf), "promptAllocFailureTest.cc", 5,
                                                           "%s", "short"));

    NiceMock<Mock_stdlib> mock_stdlib;

    promptFakeSetInput("abc\r");

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _))
        .WillOnce(
            Return(nullptr)); // [Pre-Assert確認_異常系] - realloc が書式バッファーの拡張のために 1 回呼び出されること。
                              // [Pre-Assert手順] - realloc から NULL を返却する。

    // Act
    int rtc = com_util_prompt_readline_fmt_at(prompt_, buf, sizeof(buf), "promptAllocFailureTest.cc", 5, "%s",
                                              long_prompt.c_str()); // [手順] - 長いプロンプトで 1 行読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - プロンプトを切り捨てて継続し COM_UTIL_OK が返ること。
    EXPECT_STREQ("abc", buf);    // [確認_正常系] - 入力した "abc" が返ること。
}
