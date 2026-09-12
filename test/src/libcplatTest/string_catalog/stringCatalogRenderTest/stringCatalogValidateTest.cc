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
    actual_ret = string_catalog_validate_text("value={{ {0} }} / {1} / {0}",
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
    actual_ret = string_catalog_validate_text("開始しました。", 0); // [手順] - 位置指定を含まない書式を確認する。

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
    actual_ret = string_catalog_validate_text("}", 0); // [手順] - 対を成さない } だけの書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。

    actual_ret = string_catalog_validate_text("{0}", 0); // [手順] - 引数を取らない定義に位置指定がある書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。

    actual_ret = string_catalog_validate_text("{8}", 8); // [手順] - 上限を超えるインデックスを持つ書式を確認する。
    EXPECT_EQ(CPLAT_ERR_MALFORMED_DEFINITION,
              actual_ret); // [確認_異常系] - 戻り値が CPLAT_ERR_MALFORMED_DEFINITION であること。
}
