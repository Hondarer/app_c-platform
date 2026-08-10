#include <testfw.h>
#include <mock_com_util.h>
#include <mock_stdlib.h>
#include <com_util/argparser/argparser.h>
#include <com_util/base/result.h>

#include <cstdio>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

class argparserAllocFailureTest : public Test
{
  protected:
    NiceMock<Mock_com_util> mock_com_util_;
    com_util_argparser *parser_ = NULL;
    const char *storage_ = NULL;

    void SetUp() override
    {
        parser_ = _com_util_argparser_create(NULL);
        ASSERT_NE((com_util_argparser *)NULL, parser_);
    }

    void TearDown() override
    {
        _com_util_argparser_dispose(parser_);
        parser_ = NULL;
    }
};

// 登録項目配列の拡張に失敗した場合に登録が失敗することの確認
TEST_F(argparserAllocFailureTest, register_fails_when_spec_array_expansion_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    /* 名前の複製など他の確保は本物へ委譲する。
       gMock は後から宣言した期待値を優先するため、汎用の期待値を先に宣言する */
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_正常系] - malloc が任意の回数呼び出されること。
                                      // [Pre-Assert手順] - 本物の malloc へ委譲する。
    EXPECT_CALL(mock_stdlib, realloc(_, _, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - realloc が登録項目配列の拡張のために 1 回呼び出されること。
                          // [Pre-Assert手順] - 1 回目は NULL を返却し、以降 (エラー記録用の確保) は本物へ委譲する。

    // Act
    int rtc = _com_util_argparser_register_option_string(
        parser_, "-a", "--alpha", "VALUE", "説明", 0u,
        &storage_); // [手順] - 文字列オプション --alpha を登録する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        rtc); // [確認_異常系] - _com_util_argparser_register_option_string の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
}

// 登録項目の名前複製に失敗した場合に登録が失敗することの確認
TEST_F(argparserAllocFailureTest, register_fails_when_name_duplication_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - malloc が名前の複製のために 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = _com_util_argparser_register_option_string(
        parser_, "-a", "--alpha", "VALUE", "説明", 0u,
        &storage_); // [手順] - 文字列オプション --alpha を登録する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        rtc); // [確認_異常系] - _com_util_argparser_register_option_string の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
}

// 使用方法の出力バッファー確保に失敗した場合に出力が失敗することの確認
TEST_F(argparserAllocFailureTest, print_usage_fails_when_buffer_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;

    ASSERT_EQ(COM_UTIL_OK,
              _com_util_argparser_register_option_string(parser_, "-a", "--alpha", "VALUE", "説明", 0u,
                                                         &storage_)); // [状態] - オプションを 1 件登録しておく。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - malloc が使用方法の組み立てバッファーのために 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = _com_util_argparser_print_usage(parser_,
                                              stdout); // [手順] - _com_util_argparser_print_usage を呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_OUT_OF_MEMORY,
        rtc); // [確認_異常系] - _com_util_argparser_print_usage の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
}
