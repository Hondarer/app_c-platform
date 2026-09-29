#include <testfw.h>
#include <mock_cplat.h>
/* テスト対象が使用する標準ライブラリ関数の mock。Windows で実装オブジェクトを取り込むため、ヘッダーを取り込む */
#include <mock_stdio.h>
#include <mock_stdlib.h>
#include <mock_string.h>

#include "filter_test_catalog.h"

#include <cplat/base/result.h>
#include <cplat/string_catalog/filter.h>
#include <cplat/string_catalog/string_catalog.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

using testing::_;
using testing::NiceMock;
using testing::Return;

namespace
{
/** テストで使用する行数の上限です。 */
const size_t kLineCapacity = 4U;

/** テストで使用する行幅です。 */
const size_t kLineWidth = 64U;

/**
 *  可変長引数を組み立てて、cplat_string_catalog_filter_slot_vformat へ中継します。
 *
 *  `va_start` の直前の名前付き引数を持たせるため、メンバー関数ではなく通常の関数とします。
 */
int call_slot_vformat(cplat_string_catalog_filter_slot *slot, char *dest, size_t dest_size, int *matched_out,
                      int string_key, ...)
{
    va_list args;
    int ret;

    va_start(args, string_key);
    ret = cplat_string_catalog_filter_slot_vformat(slot, dest, dest_size, matched_out, string_key, args);
    va_end(args);

    return ret;
}
} // namespace

class stringCatalogFilterSlotTest : public Test
{
  protected:
    NiceMock<Mock_cplat> mock_cplat;

    /** 組み立てた文字列の格納先です。 */
    char dest[64];

    /** 作成したスロットです。 */
    cplat_string_catalog_filter_slot *slot = nullptr;

    void SetUp() override
    {
        memset(dest, 0, sizeof(dest));
        cplat_string_catalog_set_language(CPLAT_STRING_CATALOG_LANGUAGE_NEUTRAL);
    }

    void TearDown() override
    {
        cplat_string_catalog_filter_slot_dispose(&slot);
    }
};

// 読み書きロックの作成に失敗した場合、作成関数の結果コードを返し、スロットを返さないことの確認
TEST_F(stringCatalogFilterSlotTest, create_returns_rwlock_create_result)
{
    // Arrange
    int actual_ret;
    slot = reinterpret_cast<cplat_string_catalog_filter_slot *>(
        static_cast<uintptr_t>(0x1)); // [状態] - 格納先を非 NULL の値で初期化する。

    // Pre-Assert
    EXPECT_CALL(mock_cplat, cplat_local_rwlock_create(_))
        .WillOnce(
            Return(CPLAT_ERR_UNKNOWN)); // [Pre-Assert手順] - 読み書きロックの作成で CPLAT_ERR_UNKNOWN を返却する。
    EXPECT_CALL(mock_cplat, cplat_local_lock_create(_))
        .Times(0); // [Pre-Assert確認_異常系] - ミューテックスの作成へ進まないこと。

    // Act
    actual_ret = cplat_string_catalog_filter_slot_create(filter_test_catalog(), NULL, 0U, kLineCapacity, kLineWidth,
                                                         &slot); // [手順] - スロットを作成する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_UNKNOWN, actual_ret); // [確認_異常系] - 作成関数の結果コードを返すこと。
    EXPECT_EQ(nullptr, slot);                 // [確認_異常系] - 格納先へ NULL を格納すること。
}

// ミューテックスの作成に失敗した場合、作成関数の結果コードを返し、スロットを返さないことの確認
TEST_F(stringCatalogFilterSlotTest, create_returns_lock_create_result)
{
    // Arrange
    int actual_ret;

    // Pre-Assert
    EXPECT_CALL(mock_cplat, cplat_local_lock_create(_))
        .WillOnce(Return(
            CPLAT_ERR_PERMISSION_DENIED)); // [Pre-Assert手順] - ミューテックスの作成で CPLAT_ERR_PERMISSION_DENIED を返却する。
    EXPECT_CALL(mock_cplat, cplat_local_rwlock_dispose(_))
        .Times(1); // [Pre-Assert確認_異常系] - 作成済みの読み書きロックを破棄すること。

    // Act
    actual_ret = cplat_string_catalog_filter_slot_create(filter_test_catalog(), NULL, 0U, kLineCapacity, kLineWidth,
                                                         &slot); // [手順] - スロットを作成する。

    // Assert
    EXPECT_EQ(CPLAT_ERR_PERMISSION_DENIED, actual_ret); // [確認_異常系] - 作成関数の結果コードを返すこと。
    EXPECT_EQ(nullptr, slot);                           // [確認_異常系] - 格納先へ NULL を格納すること。
}

// 判定のための読み書きロックを取得できない場合、結果コードを返し、文字列を組み立てないことの確認
TEST_F(stringCatalogFilterSlotTest, vformat_returns_lock_result_without_formatting)
{
    // Arrange
    int actual_ret;
    int actual_matched = 1;
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_create(filter_test_catalog(), NULL, 0U, kLineCapacity,
                                                                kLineWidth, &slot)); // [状態] - スロットを作成する。
    strcpy(dest, "previous"); // [状態] - 格納先へ以前の内容を書き込む。

    // Pre-Assert
    EXPECT_CALL(mock_cplat, cplat_local_rwlock_lock_shared(_, _))
        .WillOnce(Return(CPLAT_ERR_UNKNOWN)); // [Pre-Assert手順] - 共有ロックの取得で CPLAT_ERR_UNKNOWN を返却する。

    // Act
    actual_ret = call_slot_vformat(slot, dest, sizeof(dest), &actual_matched, FILTER_TEST_CATALOG_KEY_NUMBER,
                                   (int32_t)7); // [手順] - 判定と書式展開を行う。

    // Assert
    EXPECT_EQ(CPLAT_ERR_UNKNOWN, actual_ret); // [確認_異常系] - 共有ロックの取得の結果コードを返すこと。
    EXPECT_EQ(0, actual_matched);             // [確認_異常系] - 一致結果へ 0 を格納すること。
    EXPECT_STREQ("", dest);                   // [確認_異常系] - 格納先を空文字列にし、文字列を組み立てないこと。
}

// 判定のための読み書きロックを取得できた場合、文字列を組み立てることの確認
TEST_F(stringCatalogFilterSlotTest, vformat_formats_when_lock_succeeds)
{
    // Arrange
    int actual_ret;
    int actual_matched = 1;
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_create(filter_test_catalog(), NULL, 0U, kLineCapacity,
                                                                kLineWidth, &slot)); // [状態] - スロットを作成する。

    // Pre-Assert
    EXPECT_CALL(mock_cplat, cplat_local_rwlock_lock_shared(_, _))
        .Times(1); // [Pre-Assert確認_正常系] - 共有ロックを 1 回取得すること。

    // Act
    actual_ret = call_slot_vformat(slot, dest, sizeof(dest), &actual_matched, FILTER_TEST_CATALOG_KEY_NUMBER,
                                   (int32_t)7); // [手順] - 条件のないスロットで判定と書式展開を行う。

    // Assert
    EXPECT_EQ(CPLAT_OK, actual_ret); // [確認_正常系] - 戻り値が CPLAT_OK であること。
    EXPECT_EQ(0, actual_matched);    // [確認_正常系] - 条件がないため不一致であること。
    EXPECT_STREQ("number 7", dest);  // [確認_正常系] - 不一致でも文字列を組み立てること。
}

// カタログに存在しない文字列キーは判定せず、文字列の組み立てへ進むことの確認
TEST_F(stringCatalogFilterSlotTest, vformat_skips_lock_for_missing_key)
{
    // Arrange
    int actual_ret;
    int actual_matched = 1;
    ASSERT_EQ(CPLAT_OK, cplat_string_catalog_filter_slot_create(filter_test_catalog(), NULL, 0U, kLineCapacity,
                                                                kLineWidth, &slot)); // [状態] - スロットを作成する。

    // Pre-Assert
    EXPECT_CALL(mock_cplat, cplat_local_rwlock_lock_shared(_, _))
        .Times(0); // [Pre-Assert確認_異常系] - 共有ロックを取得しないこと。

    // Act
    actual_ret =
        call_slot_vformat(slot, dest, sizeof(dest), &actual_matched,
                          FILTER_TEST_CATALOG_KEY_MISSING); // [手順] - 存在しない文字列キーで判定と書式展開を行う。

    // Assert
    EXPECT_EQ(CPLAT_ERR_NOT_FOUND, actual_ret); // [確認_異常系] - 書式展開の結果コードを返すこと。
    EXPECT_EQ(0, actual_matched);               // [確認_異常系] - 一致結果へ 0 を格納すること。
}
