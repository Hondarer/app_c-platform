#include <testfw.h>
#include <com_util/base/platform.h>
#include <com_util/crt/stdio.h>

#include <cstdarg>
#include <cstdio>

#if defined(PLATFORM_LINUX)
    #include <unistd.h>
#elif defined(PLATFORM_WINDOWS)
    #include <io.h>
#endif /* PLATFORM_ */

static int call_com_util_vscanf(const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = com_util_vscanf(format, args);
    va_end(args);
    return result;
}

static int call_com_util_vfscanf(FILE *stream, const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = com_util_vfscanf(stream, format, args);
    va_end(args);
    return result;
}

static FILE *create_input_stream(const char *text)
{
    FILE *stream = nullptr;

#if defined(PLATFORM_LINUX)
    stream = tmpfile();
#elif defined(PLATFORM_WINDOWS)
    if (tmpfile_s(&stream) != 0)
    {
        return nullptr;
    }
#endif /* PLATFORM_ */

    if (stream == nullptr)
    {
        return nullptr;
    }
    if (fputs(text, stream) == EOF || fflush(stream) != 0 || fseek(stream, 0, SEEK_SET) != 0)
    {
        fclose(stream);
        return nullptr;
    }

    return stream;
}

class stdin_redirector
{
  public:
    bool redirect_to(FILE *input)
    {
        int input_fd;
        int stdin_fd;

        if (input == nullptr)
        {
            return false;
        }

#if defined(PLATFORM_LINUX)
        input_fd = fileno(input);
        stdin_fd = fileno(stdin);
        saved_stdin_fd_ = dup(stdin_fd);
#elif defined(PLATFORM_WINDOWS)
        input_fd = _fileno(input);
        stdin_fd = _fileno(stdin);
        saved_stdin_fd_ = _dup(stdin_fd);
#endif /* PLATFORM_ */
        if (input_fd < 0 || stdin_fd < 0 || saved_stdin_fd_ < 0)
        {
            return false;
        }

#if defined(PLATFORM_LINUX)
        if (dup2(input_fd, stdin_fd) < 0)
#elif defined(PLATFORM_WINDOWS)
        if (_dup2(input_fd, stdin_fd) != 0)
#endif /* PLATFORM_ */
        {
            close_saved_stdin();
            return false;
        }

        stdin_fd_ = stdin_fd;
        clearerr(stdin);
        return true;
    }

    ~stdin_redirector()
    {
        if (saved_stdin_fd_ >= 0)
        {
#if defined(PLATFORM_LINUX)
            (void)dup2(saved_stdin_fd_, stdin_fd_);
#elif defined(PLATFORM_WINDOWS)
            (void)_dup2(saved_stdin_fd_, stdin_fd_);
#endif /* PLATFORM_ */
            clearerr(stdin);
        }
        close_saved_stdin();
    }

  private:
    void close_saved_stdin()
    {
        if (saved_stdin_fd_ >= 0)
        {
#if defined(PLATFORM_LINUX)
            (void)close(saved_stdin_fd_);
#elif defined(PLATFORM_WINDOWS)
            (void)_close(saved_stdin_fd_);
#endif /* PLATFORM_ */
            saved_stdin_fd_ = -1;
        }
    }

    int saved_stdin_fd_ = -1;
    int stdin_fd_ = -1;
};

class scanfTest : public Test
{
};

// scanf が標準入力から幅指定した文字列と数値を読み取ることの確認
TEST_F(scanfTest, scanf_reads_width_limited_token_and_integer)
{
    // Arrange
    FILE *input = create_input_stream("alpha 42");
    stdin_redirector redirector;
    char token[8];
    int value = 0; // [状態] - "alpha 42" を標準入力として設定し、文字列と数値の格納先を用意する。

    ASSERT_NE(nullptr, input);                  // [状態] - 標準入力へ転送する一時ストリームを作成できること。
    ASSERT_TRUE(redirector.redirect_to(input)); // [状態] - 一時ストリームを標準入力へ転送できること。

    // Pre-Assert

    // Act
    int count = com_util_scanf("%7s %d", token, &value); // [手順] - 幅 7 の文字列と整数を com_util_scanf で読み取る。

    // Assert
    EXPECT_EQ(2, count);          // [確認_正常系] - com_util_scanf の戻り値が 2 であること。
    EXPECT_STREQ("alpha", token); // [確認_正常系] - 文字列が "alpha" であること。
    EXPECT_EQ(42, value);         // [確認_正常系] - 数値が 42 であること。

    // Cleanup
    fclose(input);
}

// vscanf が標準入力から幅指定した文字列と数値を読み取ることの確認
TEST_F(scanfTest, vscanf_reads_width_limited_token_and_integer)
{
    // Arrange
    FILE *input = create_input_stream("bravo 24");
    stdin_redirector redirector;
    char token[8];
    int value = 0; // [状態] - "bravo 24" を標準入力として設定し、文字列と数値の格納先を用意する。

    ASSERT_NE(nullptr, input);                  // [状態] - 標準入力へ転送する一時ストリームを作成できること。
    ASSERT_TRUE(redirector.redirect_to(input)); // [状態] - 一時ストリームを標準入力へ転送できること。

    // Pre-Assert

    // Act
    int count =
        call_com_util_vscanf("%7s %d", token, &value); // [手順] - 幅 7 の文字列と整数を com_util_vscanf で読み取る。

    // Assert
    EXPECT_EQ(2, count);          // [確認_正常系] - com_util_vscanf の戻り値が 2 であること。
    EXPECT_STREQ("bravo", token); // [確認_正常系] - 文字列が "bravo" であること。
    EXPECT_EQ(24, value);         // [確認_正常系] - 数値が 24 であること。

    // Cleanup
    fclose(input);
}

// fscanf がストリームから幅指定した文字列と数値を読み取ることの確認
TEST_F(scanfTest, fscanf_reads_width_limited_token_and_integer)
{
    // Arrange
    FILE *input = create_input_stream("charlie 17");
    char token[8];
    int value = 0; // [状態] - "charlie 17" を含む一時ストリームと、文字列と数値の格納先を用意する。

    ASSERT_NE(nullptr, input); // [状態] - 読み取り元の一時ストリームを作成できること。

    // Pre-Assert

    // Act
    int count =
        com_util_fscanf(input, "%7s %d", token, &value); // [手順] - 幅 7 の文字列と整数を com_util_fscanf で読み取る。

    // Assert
    EXPECT_EQ(2, count);            // [確認_正常系] - com_util_fscanf の戻り値が 2 であること。
    EXPECT_STREQ("charlie", token); // [確認_正常系] - 文字列が "charlie" であること。
    EXPECT_EQ(17, value);           // [確認_正常系] - 数値が 17 であること。

    // Cleanup
    fclose(input);
}

// vfscanf がストリームから幅指定した文字列と数値を読み取ることの確認
TEST_F(scanfTest, vfscanf_reads_width_limited_token_and_integer)
{
    // Arrange
    FILE *input = create_input_stream("delta 71");
    char token[8];
    int value = 0; // [状態] - "delta 71" を含む一時ストリームと、文字列と数値の格納先を用意する。

    ASSERT_NE(nullptr, input); // [状態] - 読み取り元の一時ストリームを作成できること。

    // Pre-Assert

    // Act
    int count = call_com_util_vfscanf(input, "%7s %d", token,
                                      &value); // [手順] - 幅 7 の文字列と整数を com_util_vfscanf で読み取る。

    // Assert
    EXPECT_EQ(2, count);          // [確認_正常系] - com_util_vfscanf の戻り値が 2 であること。
    EXPECT_STREQ("delta", token); // [確認_正常系] - 文字列が "delta" であること。
    EXPECT_EQ(71, value);         // [確認_正常系] - 数値が 71 であること。

    // Cleanup
    fclose(input);
}
