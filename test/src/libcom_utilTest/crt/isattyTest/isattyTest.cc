#include <testfw.h>
#include <com_util/crt/unistd.h>

class isattyTest : public Test
{
};

TEST_F(isattyTest, invalid_stream_returns_zero)
{
    /* 定義外の enum 値は 0 を返すこと */
    int ret = com_util_isatty((com_util_stream_t)99);

    EXPECT_EQ(0, ret);
}

TEST_F(isattyTest, stdin_returns_int)
{
    /* テスト ハーネス下では stdin はリダイレクトされているため
     * 戻り値は 0 か 1 のいずれかであること (クラッシュしないこと) */
    int ret = com_util_isatty(COM_UTIL_STREAM_STDIN);

    EXPECT_TRUE(ret == 0 || ret == 1);
}

TEST_F(isattyTest, stdout_returns_int)
{
    int ret = com_util_isatty(COM_UTIL_STREAM_STDOUT);

    EXPECT_TRUE(ret == 0 || ret == 1);
}

TEST_F(isattyTest, stderr_returns_int)
{
    int ret = com_util_isatty(COM_UTIL_STREAM_STDERR);

    EXPECT_TRUE(ret == 0 || ret == 1);
}
