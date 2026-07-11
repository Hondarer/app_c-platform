#include <testfw.h>
#include <mock_stdio.h>
#include <mock_com_util.h>

using testing::_;
using testing::HasSubstr;
using testing::NiceMock;

class argparser_sampleTest : public Test
{
  protected:
    NiceMock<Mock_stdio> mock_stdio_;
    NiceMock<Mock_com_util> mock_com_util_;
};

// 全種別の引数を指定した正常系で解析結果が表示されることの確認
TEST_F(argparser_sampleTest, main_parses_and_prints_all_kinds)
{
    // Arrange
    const char *argv[] = {"argparser-sample", "-v",     "-v",     "--count=3", "-n", "alice", "-i", "dir1",
                          "--include=dir2",   "in.txt", "out.txt"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Pre-Assert
    // [Pre-Assert確認_正常系] - 解析結果が標準出力に表示されること。
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("verbose: 2"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("count: 3"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("name: alice"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("includes: 2"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("include[0]: dir1"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("include[1]: dir2"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("input: in.txt"))).Times(1);
    EXPECT_CALL(mock_stdio_, printf(_, _, _, HasSubstr("output: out.txt"))).Times(1);

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 全種別の引数で main を呼び出す。

    // Assert
    EXPECT_EQ(0, rc); // [確認_正常系] - 正常終了すること。
}

// 未知のオプションで失敗終了することの確認
//
// エラー メッセージと usage は com_util_argparser_print_error_messages() /
// com_util_argparser_print_usage() (いずれも argparser.c 側、本テストでは ADD_SRCS として
// ビルドされ mock_stdio の override 対象外) が出力するため、本テストでは mock で検証できない。
// メッセージ内容の確認は argparserTest.cc の print_error_messages_writes_to_stream /
// error_message_formatting、usage の内容確認は同ファイルの print_usage_writes_to_stream /
// usage_formatting で行う。
TEST_F(argparser_sampleTest, main_reports_parse_error_with_usage)
{
    // Arrange
    const char *argv[] = {"argparser-sample", "--bogus", "in.txt"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 未知のオプションで main を呼び出す。

    // Assert
    EXPECT_NE(0, rc); // [確認_異常系] - 失敗終了すること。
}

// 必須引数の欠落で失敗終了することの確認 (メッセージ内容の確認は上記コメント参照)
TEST_F(argparser_sampleTest, main_reports_missing_required)
{
    // Arrange
    const char *argv[] = {"argparser-sample"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 引数なしで main を呼び出す。

    // Assert
    EXPECT_NE(0, rc); // [確認_異常系] - 失敗終了すること。
}

// --help 指定時に必須引数が省略されていても正常終了することの確認
//
// usage の内容確認は argparserTest.cc 側で行う (上記コメント参照)。
TEST_F(argparser_sampleTest, main_prints_usage_on_help)
{
    // Arrange
    const char *argv[] = {"argparser-sample", "--help"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - --help で main を呼び出す。

    // Assert
    EXPECT_EQ(0, rc); // [確認_正常系] - 正常終了すること。
}
