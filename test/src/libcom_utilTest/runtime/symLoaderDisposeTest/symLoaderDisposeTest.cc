#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/runtime/sym_loader.h>

#include <cstring>

class symLoaderDisposeTest : public Test
{
  protected:
    /* lib_name / func_name は固定長配列のため、テストからは直接書き込む */
    static void set_names(com_util_sym_loader_entry *entry, const char *lib_name, const char *func_name)
    {
        std::strncpy(entry->lib_name, lib_name, sizeof(entry->lib_name) - 1u);
        std::strncpy(entry->func_name, func_name, sizeof(entry->func_name) - 1u);
    }
};

// 解決済みエントリのハンドルと関数ポインターが解放されることの確認
TEST_F(symLoaderDisposeTest, releases_handle_and_func_ptr_of_resolved_entry)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("test_key", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry};

    set_names(&entry, "libcom_util", "com_util_path_basename");
    ASSERT_NE(nullptr, com_util_sym_loader_resolve(&entry)); // [状態] - 解決済みのエントリを 1 件用意する。
    ASSERT_NE(nullptr, entry.handle);

    // Pre-Assert

    // Act
    com_util_sym_loader_dispose(entries, 1u); // [手順] - 解決済みエントリ 1 件を指定して解放する。

    // Assert
    EXPECT_EQ(nullptr, entry.handle);   // [確認_正常系] - handle が NULL になること。
    EXPECT_EQ(nullptr, entry.func_ptr); // [確認_正常系] - func_ptr が NULL になること。
}

// ハンドルを持たないエントリが読み飛ばされることの確認
TEST_F(symLoaderDisposeTest, skips_entry_without_handle)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("test_key", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 未解決のままハンドルを持たないエントリを用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_dispose(entries, 1u); // [手順] - ハンドルを持たないエントリを指定して解放する。

    // Assert
    EXPECT_EQ(nullptr, entry.handle); // [確認_正常系] - handle が NULL のままであること。
}

// 要素数 0 の指定で何も起こらないことの確認
TEST_F(symLoaderDisposeTest, accepts_zero_length)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("test_key", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry};

    set_names(&entry, "libcom_util", "com_util_path_basename");
    ASSERT_NE(nullptr, com_util_sym_loader_resolve(&entry)); // [状態] - 解決済みのエントリを 1 件用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_dispose(entries, 0u); // [手順] - 要素数に 0 を指定して解放する。

    // Assert
    EXPECT_NE(nullptr, entry.handle); // [確認_正常系] - 走査されないため handle が保持されたままであること。

    // Cleanup
    com_util_sym_loader_dispose(entries, 1u);
}

// 複数エントリがまとめて解放されることの確認
TEST_F(symLoaderDisposeTest, releases_multiple_entries)
{
    // Arrange
    com_util_sym_loader_entry first = COM_UTIL_SYM_LOADER_ENTRY_INIT("first", void (*)(void));
    com_util_sym_loader_entry second = COM_UTIL_SYM_LOADER_ENTRY_INIT("second", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&first, &second};

    set_names(&first, "libcom_util", "com_util_path_basename");
    set_names(&second, "libcom_util", "com_util_path_extension");
    ASSERT_NE(nullptr, com_util_sym_loader_resolve(&first));
    ASSERT_NE(nullptr, com_util_sym_loader_resolve(&second)); // [状態] - 解決済みのエントリを 2 件用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_dispose(entries, 2u); // [手順] - 解決済みエントリ 2 件を指定して解放する。

    // Assert
    EXPECT_EQ(nullptr, first.handle);  // [確認_正常系] - 1 件目の handle が NULL になること。
    EXPECT_EQ(nullptr, second.handle); // [確認_正常系] - 2 件目の handle が NULL になること。
}
