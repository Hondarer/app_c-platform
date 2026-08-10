#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/crt/stdio.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

class fgetsTest : public Test
{
  protected:
    std::string make_path(const char *name)
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt/stdioTest/results";

        std::filesystem::create_directories(dir);
        return (dir / name).generic_string();
    }

    // 指定した内容のファイルを作成し、読み取り用に開いたストリームを返す
    FILE *open_with_content(const std::string &path, const char *content)
    {
        FILE *fp;

        com_util_remove(path.c_str(), NULL);

        fp = com_util_fopen(path.c_str(), "wb", NULL);
        if (fp == nullptr)
        {
            return nullptr;
        }
        std::fwrite(content, 1u, std::strlen(content), fp);
        com_util_fclose(fp, NULL);

        return com_util_fopen(path.c_str(), "rb", NULL);
    }
};

// LF で終わる行が改行を除去して取得されることの確認
TEST_F(fgetsTest, reads_line_terminated_by_lf)
{
    // Arrange
    std::string path = make_path("lf.txt");
    FILE *fp = open_with_content(path, "abc\ndef\n"); // [状態] - LF 区切りの 2 行を持つファイルを開く。
    char buf[16];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 1 行目を com_util_fgets で読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("abc", buf);    // [確認_正常系] - 末尾の LF を除いた "abc" が格納されること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// CRLF で終わる行が CR と LF の双方を除去して取得されることの確認
TEST_F(fgetsTest, strips_crlf)
{
    // Arrange
    std::string path = make_path("crlf.txt");
    FILE *fp = open_with_content(path, "abc\r\n"); // [状態] - CRLF で終わる 1 行を持つファイルを開く。
    char buf[16];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - CRLF で終わる行を com_util_fgets で読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("abc", buf);    // [確認_正常系] - CR と LF を除いた "abc" が格納されること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// 改行で終わらない最終行が取得できることの確認
TEST_F(fgetsTest, reads_last_line_without_newline)
{
    // Arrange
    std::string path = make_path("no_newline.txt");
    FILE *fp = open_with_content(path, "abc"); // [状態] - 改行を持たない 1 行だけのファイルを開く。
    char buf[16];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 改行のない最終行を com_util_fgets で読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("abc", buf);    // [確認_正常系] - "abc" が格納されること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// 空行が空文字列として取得されることの確認
TEST_F(fgetsTest, reads_empty_line)
{
    // Arrange
    std::string path = make_path("empty_line.txt");
    FILE *fp = open_with_content(path, "\nabc\n"); // [状態] - 1 行目が空行のファイルを開く。
    char buf[16];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 空行である 1 行目を com_util_fgets で読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret); // [確認_正常系] - com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("", buf);       // [確認_正常系] - 空文字列が格納されること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// 読み取る行がない場合に EOF を返すことの確認
TEST_F(fgetsTest, returns_eof_at_end_of_stream)
{
    // Arrange
    std::string path = make_path("eof.txt");
    FILE *fp = open_with_content(path, "abc\n"); // [状態] - 1 行だけを持つファイルを開く。
    char buf[16];
    int first_ret;
    int second_ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    first_ret = com_util_fgets(buf, sizeof(buf), fp, NULL);  // [手順] - 1 回目の呼び出しで唯一の行を読み取る。
    second_ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 2 回目の呼び出しを行末尾の状態で実行する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, first_ret); // [確認_正常系] - 1 回目の com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERR_EOF,
              second_ret); // [確認_正常系] - 2 回目の com_util_fgets の戻り値が COM_UTIL_ERR_EOF であること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// 行がバッファーに収まらない場合にバッファー不足を返すことの確認
TEST_F(fgetsTest, returns_buffer_too_small_for_long_line)
{
    // Arrange
    std::string path = make_path("long_line.txt");
    FILE *fp = open_with_content(path, "abcdefgh\n"); // [状態] - 8 文字の行を持つファイルを開く。
    char buf[4];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 4 バイトのバッファーで 8 文字の行を読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              ret);        // [確認_異常系] - com_util_fgets の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_STREQ("", buf); // [確認_異常系] - 途中までの内容を残さずバッファーが空文字列になること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// バッファーに収まらない行の残りが次の呼び出しで取得されることの確認
TEST_F(fgetsTest, continues_reading_remainder_after_buffer_too_small)
{
    // Arrange
    std::string path = make_path("remainder.txt");
    FILE *fp = open_with_content(path, "abcdef\n"); // [状態] - 6 文字の行を持つファイルを開く。
    char buf[5];
    int first_ret;
    int second_ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    first_ret = com_util_fgets(buf, sizeof(buf), fp, NULL);  // [手順] - 1 回目の呼び出しで先頭 4 文字分を読み取らせる。
    second_ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 2 回目の呼び出しで行の残りを読み取る。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        first_ret); // [確認_異常系] - 1 回目の com_util_fgets の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ(COM_UTIL_OK,
              second_ret);   // [確認_正常系] - 2 回目の com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("ef", buf); // [確認_正常系] - 2 回目の呼び出しで行の残り "ef" が格納されること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// バッファー サイズが 1 の場合にバッファー不足を返すことの確認
TEST_F(fgetsTest, buffer_size_one_returns_buffer_too_small)
{
    // Arrange
    std::string path = make_path("size_one.txt");
    FILE *fp = open_with_content(path, "abc\n"); // [状態] - 1 行だけを持つファイルを開く。
    char buf[1];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, NULL); // [手順] - 終端の 1 バイトしか置けないバッファーで読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              ret);        // [確認_異常系] - com_util_fgets の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_STREQ("", buf); // [確認_異常系] - バッファーが空文字列になること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// 格納先が NULL の場合に引数エラーになることの確認
TEST_F(fgetsTest, null_dest_returns_invalid_argument)
{
    // Arrange
    std::string path = make_path("null_dest.txt");
    FILE *fp = open_with_content(path, "abc\n"); // [状態] - 1 行だけを持つファイルを開く。
    com_util_error detail = {};
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp); // [Pre-Assert確認] - 読み取り用ストリームが開けていること。

    // Act
    ret = com_util_fgets(NULL, 16u, fp, &detail); // [手順] - 格納先に NULL を渡して com_util_fgets を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_fgets の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(EINVAL, com_util_error_get_errno(&detail)); // [確認_異常系] - 詳細エラーへ EINVAL が記録されること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}

// ストリームが NULL の場合に引数エラーになることの確認
TEST_F(fgetsTest, null_stream_returns_invalid_argument)
{
    // Arrange
    char buf[16]; // [状態] - 16 バイトの格納先バッファーを用意する。
    int ret;

    // Pre-Assert

    // Act
    ret =
        com_util_fgets(buf, sizeof(buf), NULL, NULL); // [手順] - ストリームに NULL を渡して com_util_fgets を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              ret); // [確認_異常系] - com_util_fgets の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 成功時に詳細エラーがクリアされることの確認
TEST_F(fgetsTest, clears_detail_on_success)
{
    // Arrange
    std::string path = make_path("detail.txt");
    FILE *fp = open_with_content(path, "abc\n"); // [状態] - 1 行だけを持つファイルを開く。
    com_util_error detail = {COM_UTIL_ERROR_DOMAIN_ERRNO, COM_UTIL_ERR_UNKNOWN,
                             ENOENT}; // [状態] - 詳細エラーへあらかじめ ENOENT を設定する。
    char buf[16];
    int ret;

    // Pre-Assert
    ASSERT_NE((FILE *)NULL, fp);                  // [Pre-Assert確認] - 読み取り用ストリームが開けていること。
    ASSERT_NE(0, com_util_error_is_set(&detail)); // [Pre-Assert確認] - 呼び出し前の詳細エラーが設定済みであること。

    // Act
    ret = com_util_fgets(buf, sizeof(buf), fp, &detail); // [手順] - 詳細エラー出力を指定して 1 行を読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, ret);                  // [確認_正常系] - com_util_fgets の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, com_util_error_is_set(&detail)); // [確認_正常系] - 成功時に詳細エラーがクリアされること。

    // Cleanup
    com_util_fclose(fp, NULL);
    com_util_remove(path.c_str(), NULL);
}
