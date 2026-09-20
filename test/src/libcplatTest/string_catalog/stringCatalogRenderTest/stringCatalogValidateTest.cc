#include <testfw.h>

#include <cplat/base/result.h>

#include "string_catalog.h"

class stringCatalogValidateTest : public Test
{
};

// 正しい書式が受理されることの確認
TEST_F(stringCatalogValidateTest, valid_format)
{
    // Arrange
    int actual_ret;

    // Pre-Assert

    // Act
    actual_ret = string_catalog_validate_text("value={{ {0} }} / {1} / {0}", NULL,
                                              2); // [手順] - エスケープ、並べ替え、繰り返しを含む書式を確認する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret); // [確認_正常系] - 戻り値が CPLAT_OK であること。
}

// 引数を取らない書式が受理されることの確認
TEST_F(stringCatalogValidateTest, no_placeholder)
{
    // Arrange
    int actual_ret;

    // Pre-Assert

    // Act
    actual_ret = string_catalog_validate_text("開始しました。", NULL, 0); // [手順] - 位置指定を含まない書式を確認する。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret); // [確認_正常系] - 戻り値が CPLAT_OK であること。
}

// 構文が不正な書式が拒否されることの確認
TEST_F(stringCatalogValidateTest, invalid_format)
{
    // Arrange
    int actual_ret;

    // Pre-Assert

    // Act / Assert
    actual_ret = string_catalog_validate_text("}", NULL, 0); // [手順] - 対を成さない } だけの書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。

    actual_ret = string_catalog_validate_text("{0}", NULL, 0); // [手順] - 引数を取らない定義に位置指定がある書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。

    actual_ret = string_catalog_validate_text("{8}", NULL, 8); // [手順] - 上限を超えるインデックスを持つ書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
}

// 値を受け取らない引数を参照する書式が拒否されることの確認
TEST_F(stringCatalogValidateTest, unused_argument_reference)
{
    // Arrange
    cplat_string_catalog_argument arguments[2] = {}; // [状態] - 2 個の引数定義を用意する。
    int actual_ret;

    arguments[0].kind = CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING;
    arguments[1].kind = CPLAT_STRING_CATALOG_ARGUMENT_KIND_UNUSED;

    // Pre-Assert

    // Act / Assert
    actual_ret = string_catalog_validate_text("{0}", arguments,
                                              2); // [手順] - 値を受け取る引数だけを参照する書式を確認する。
    EXPECT_EQ(CPLAT_OK, actual_ret);              // [確認_正常系] - 戻り値が CPLAT_OK であること。

    actual_ret = string_catalog_validate_text("{1}", arguments,
                                              2); // [手順] - 値を受け取らない引数を参照する書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
}
