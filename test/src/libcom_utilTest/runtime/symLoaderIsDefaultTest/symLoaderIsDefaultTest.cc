#include <testfw.h>
#include <com_util/runtime/sym_loader.h>

#include <cstring>

class symLoaderIsDefaultTest : public Test
{
  protected:
    com_util_sym_loader_entry entry_ = COM_UTIL_SYM_LOADER_ENTRY_INIT("test_key", void (*)(void));

    /* lib_name / func_name は固定長配列のため、テストからは直接書き込む */
    void set_names(const char *lib_name, const char *func_name)
    {
        std::strncpy(entry_.lib_name, lib_name, sizeof(entry_.lib_name) - 1u);
        std::strncpy(entry_.func_name, func_name, sizeof(entry_.func_name) - 1u);
    }

    void TearDown() override
    {
        com_util_sym_loader_entry *entries[] = {&entry_};

        com_util_sym_loader_dispose(entries, 1u);
    }
};

// 未解決のエントリが解決されたうえで明示的デフォルトと判定されることの確認
TEST_F(symLoaderIsDefaultTest, resolves_and_reports_explicit_default)
{
    // Arrange
    set_names("default", "default"); // [状態] - 未解決かつ双方の名称が "default" のエントリを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_sym_loader_is_default(&entry_); // [手順] - com_util_sym_loader_is_default を呼び出す。

    // Assert
    EXPECT_EQ(1, rtc);             // [確認_正常系] - com_util_sym_loader_is_default の戻り値が 1 であること。
    EXPECT_EQ(2, entry_.resolved); // [確認_正常系] - 呼び出しの中で解決が行われ resolved が 2 になること。
}

// 解決済みのエントリが明示的デフォルトでないと判定されることの確認
TEST_F(symLoaderIsDefaultTest, reports_not_default_for_resolved_symbol)
{
    // Arrange
    set_names("libcom_util", "com_util_path_basename");
    ASSERT_NE(nullptr, com_util_sym_loader_resolve(&entry_)); // [状態] - 解決済みのエントリを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_sym_loader_is_default(&entry_); // [手順] - com_util_sym_loader_is_default を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);             // [確認_正常系] - com_util_sym_loader_is_default の戻り値が 0 であること。
    EXPECT_EQ(1, entry_.resolved); // [確認_正常系] - resolved が 1 のまま変化しないこと。
}

// 解決に失敗したエントリが明示的デフォルトでないと判定されることの確認
TEST_F(symLoaderIsDefaultTest, reports_not_default_for_unresolved_entry)
{
    // Arrange
    set_names("", ""); // [状態] - lib_name と func_name がともに未設定のエントリを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_sym_loader_is_default(&entry_); // [手順] - com_util_sym_loader_is_default を呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);              // [確認_異常系] - com_util_sym_loader_is_default の戻り値が 0 であること。
    EXPECT_EQ(-1, entry_.resolved); // [確認_異常系] - resolved が定義なしを示す -1 になること。
}
