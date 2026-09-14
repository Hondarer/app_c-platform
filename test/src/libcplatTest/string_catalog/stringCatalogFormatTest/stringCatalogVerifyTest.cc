#include <testfw.h>

#include "fake_catalog.h"

#include <cplat/base/result.h>
#include <cplat/string_catalog/string_catalog.h>

class stringCatalogVerifyTest : public Test
{
  protected:
    /** 不正を検出した文字列 ID の受け取り先です。 */
    int string_id;

    /** 不正を検出した言語の受け取り先です。 */
    cplat_string_catalog_language language;

    void SetUp() override
    {
        string_id = FAKE_CATALOG_ID_UNKNOWN;
        language = CPLAT_STRING_CATALOG_LANGUAGE_ENGLISH;
        fake_catalog_reset();
    }
};

// 整合したカタログが受理されることの確認
TEST_F(stringCatalogVerifyTest, valid_catalog)
{
    // Arrange
    int actual_ret;

    // Pre-Assert

    // Act
    actual_ret =
        cplat_string_catalog_verify(fake_catalog(), &string_id, &language); // [手順] - 既定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret); // [確認_正常系] - 戻り値が CPLAT_OK であること。
}

// 必須の項目メタデータが未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_entry_metadata)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_brief(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 短い説明が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 必須メタデータの欠落を定義エラーとして通知すること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
}

// 詳細説明が未設定でもカタログが受理されることの確認
TEST_F(stringCatalogVerifyTest, missing_details_is_allowed)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_details(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 詳細説明が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret); // [確認_正常系] - 詳細説明の欠落を許容すること。
    EXPECT_EQ(FAKE_CATALOG_ID_UNKNOWN,
              string_id); // [確認_正常系] - 不正がないため文字列 ID を変更しないこと。
}

// 処理用キーが未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_key)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_key(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 処理用キーが未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 処理用キーの欠落を定義エラーとして通知すること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
}

// 引数定義のメタデータが未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_argument_metadata)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_argument_name(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, 0, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 引数名が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 引数メタデータの欠落を定義エラーとして通知すること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
}

// 引数説明が未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_argument_description)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_argument_description(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, 0, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 引数説明が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 引数説明の欠落を定義エラーとして通知すること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
}

// 引数定義配列が未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_argument_array)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_arguments(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 引数定義配列が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 引数定義配列の欠落を定義エラーとして通知すること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
}

// 書式の構文が不正なカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, invalid_format)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_text(FAKE_CATALOG_INDEX_ONE_ARGUMENT, CPLAT_STRING_CATALOG_LANGUAGE_ENGLISH, "limit {2}");

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 引数個数を超える位置指定を持つカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
    EXPECT_EQ(FAKE_CATALOG_ID_ONE_ARGUMENT,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_LANGUAGE_ENGLISH, language); // [確認_異常系] - 不正を検出した言語を報告すること。
}

// ニュートラル言語以外のリソースが未定義であっても受理されることの確認
TEST_F(stringCatalogVerifyTest, missing_localized_text_is_allowed)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_text(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 日本語のリソースが未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_OK,
              actual_ret); // [確認_正常系] - ニュートラル言語へフォールバックされるため、不正としないこと。
}

// ニュートラル言語の書式が未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_neutral_text)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_text(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - ニュートラル言語の書式が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL, language); // [確認_異常系] - 不正を検出した言語を報告すること。
}

// ニュートラル言語の備考が未設定のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, missing_neutral_note)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_note(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL, NULL);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - ニュートラル言語の備考が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL, language); // [確認_異常系] - 不正を検出した言語を報告すること。
}

// 引数個数が上限を超えたカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, argument_count_over_max)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_argument_count(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, CPLAT_STRING_CATALOG_ARGUMENT_MAX + 1);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 引数個数が上限を超えるカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_LANGUAGE_COUNT,
              language); // [確認_異常系] - 言語に依らない不正として、言語ではない値を報告すること。
}

// 文字列 ID が重複したカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, duplicated_string_id)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_id(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, FAKE_CATALOG_ID_NO_ARGUMENT);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 文字列 ID が重複したカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 検索が自分自身へ到達しないため、定義エラーを返すこと。
    EXPECT_EQ(FAKE_CATALOG_ID_NO_ARGUMENT, string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_LANGUAGE_COUNT,
              language); // [確認_異常系] - 言語に依らない不正として、言語ではない値を報告すること。
}

// 処理用キーが重複したカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, duplicated_key)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_key(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, "FAKE_ID_0001");

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 処理用キーが重複したカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 処理用キーの重複を定義エラーとして通知すること。
    EXPECT_EQ(FAKE_CATALOG_ID_TWO_ARGUMENTS,
              string_id); // [確認_異常系] - 不正を検出した文字列 ID を報告すること。
    EXPECT_EQ(CPLAT_STRING_CATALOG_LANGUAGE_COUNT,
              language); // [確認_異常系] - 言語に依らない不正として、言語ではない値を報告すること。
}

// 引数個数が負のカタログが拒否されることの確認
TEST_F(stringCatalogVerifyTest, argument_count_negative)
{
    // Arrange
    int actual_ret;
    fake_catalog_set_argument_count(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, -1);

    // Pre-Assert

    // Act
    actual_ret = cplat_string_catalog_verify(fake_catalog(), &string_id,
                                             &language); // [手順] - 引数個数が負のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
}

// 出力引数を省略しても結果コードを返すことの確認
TEST_F(stringCatalogVerifyTest, omitted_output_arguments)
{
    // Arrange
    int actual_ret_count;
    int actual_ret_text;

    // Pre-Assert

    // Act
    fake_catalog_set_argument_count(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, CPLAT_STRING_CATALOG_ARGUMENT_MAX + 1);
    actual_ret_count =
        cplat_string_catalog_verify(fake_catalog(), NULL,
                                    NULL); // [手順] - 出力引数を省略して、引数個数が不正なカタログを確認する。

    fake_catalog_reset();
    fake_catalog_set_text(FAKE_CATALOG_INDEX_TWO_ARGUMENTS, CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL, NULL);
    actual_ret_text = cplat_string_catalog_verify(
        fake_catalog(), NULL, NULL); // [手順] - 出力引数を省略して、ニュートラル言語の書式が未設定のカタログを確認する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret_count); // [確認_異常系] - 引数個数の不正を報告すること。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret_text); // [確認_異常系] - 書式の欠落を報告すること。
}
