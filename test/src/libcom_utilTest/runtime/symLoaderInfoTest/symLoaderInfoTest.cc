#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/base/result.h>
#include <com_util/runtime/sym_loader.h>

// シンボル情報 API が空の配列を受け付けることの確認
TEST(symLoaderInfoTest, sym_loader_info_accepts_empty_array)
{
    // Arrange

    // Pre-Assert

    // Act
    int result = com_util_sym_loader_info(NULL, 0); // [手順] - 要素数 0 と NULL の配列を渡してシンボル情報を表示する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_sym_loader_info の戻り値が COM_UTIL_OK であること。
}

// シンボル情報 API が不正な配列を拒否することの確認
TEST(symLoaderInfoTest, sym_loader_info_rejects_invalid_arrays)
{
    // Arrange
    com_util_sym_loader_entry *entries[] = {NULL};

    // Pre-Assert

    // Act
    int null_array_result =
        com_util_sym_loader_info(NULL, 1); // [手順] - 要素数 1 と NULL の配列を渡してシンボル情報を表示する。
    int null_entry_result =
        com_util_sym_loader_info(entries, 1); // [手順] - NULL 要素を含む配列を渡してシンボル情報を表示する。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_array_result); // [確認_異常系] - 配列が NULL の呼び出しの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        null_entry_result); // [確認_異常系] - 配列要素が NULL の呼び出しの戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// シンボル情報 API が解決状態を結果コードへ反映することの確認
TEST(symLoaderInfoTest, sym_loader_info_reports_resolution_state)
{
    // Arrange
    com_util_sym_loader_entry resolved = COM_UTIL_SYM_LOADER_ENTRY_INIT("resolved", void (*)(void));
    com_util_sym_loader_entry unresolved = COM_UTIL_SYM_LOADER_ENTRY_INIT("unresolved", void (*)(void));
    com_util_sym_loader_entry *resolved_entries[] = {&resolved};
    com_util_sym_loader_entry *unresolved_entries[] = {&unresolved};
    resolved.resolved = 1;    // [状態] - 1 個のエントリを解決済みとする。
    unresolved.resolved = -1; // [状態] - 1 個のエントリを解決失敗済みとする。

    // Pre-Assert

    // Act
    int resolved_result = com_util_sym_loader_info(resolved_entries, 1); // [手順] - 解決済みのエントリ情報を表示する。
    int unresolved_result =
        com_util_sym_loader_info(unresolved_entries, 1); // [手順] - 解決失敗済みのエントリ情報を表示する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              resolved_result); // [確認_正常系] - 解決済みエントリに対する戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        unresolved_result); // [確認_異常系] - 解決失敗済みエントリに対する戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 未解決エントリを情報表示時に解決してから状態を表示することの確認
TEST(symLoaderInfoTest, sym_loader_info_resolves_unresolved_entry)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("default_entry", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry};
    ASSERT_EQ(COM_UTIL_OK, com_util_strcpy(entry.lib_name, sizeof(entry.lib_name),
                                           "default")); // [状態] - 明示的デフォルトとして解決できるエントリを用意する。
    ASSERT_EQ(COM_UTIL_OK, com_util_strcpy(entry.func_name, sizeof(entry.func_name), "default"));

    // Pre-Assert

    // Act
    int result = com_util_sym_loader_info(entries, 1u); // [手順] - 未解決エントリの情報を表示する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result);            // [確認_正常系] - com_util_sym_loader_info の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(2, entry.resolved); // [確認_正常系] - 情報表示前の解決により resolved が 2 になること。
}
