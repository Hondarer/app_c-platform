#include <testfw.h>
#include <com_util/runtime/sym_loader.h>

#include <cstring>

class symLoaderResolveTest : public Test
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

// 実在するライブラリとシンボルが解決されることの確認
TEST_F(symLoaderResolveTest, resolves_existing_symbol)
{
    // Arrange
    set_names("libcom_util",
              "com_util_path_basename"); // [状態] - 実在するライブラリ名と関数名を設定したエントリを用意する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_NE(nullptr, func_ptr); // [確認_正常系] - com_util_sym_loader_resolve の戻り値が NULL でないこと。
    EXPECT_EQ(1, entry_.resolved); // [確認_正常系] - resolved が解決済みを示す 1 になること。
}

// 2 回目の呼び出しが解決済みの結果をそのまま返すことの確認
TEST_F(symLoaderResolveTest, second_call_returns_cached_result)
{
    // Arrange
    set_names("libcom_util", "com_util_path_basename");
    void *first = com_util_sym_loader_resolve(&entry_); // [状態] - 1 回目の解決を済ませておく。

    // Pre-Assert

    // Act
    void *second = com_util_sym_loader_resolve(&entry_); // [手順] - 同じエントリで 2 回目の解決を行う。

    // Assert
    EXPECT_EQ(first, second); // [確認_正常系] - 2 回目の戻り値が 1 回目と同じポインターであること。
    EXPECT_EQ(1, entry_.resolved); // [確認_正常系] - resolved が 1 のまま変化しないこと。
}

// lib_name と func_name がともに "default" の場合に明示的デフォルトとして扱われることの確認
TEST_F(symLoaderResolveTest, marks_explicit_default_when_both_names_are_default)
{
    // Arrange
    set_names("default", "default"); // [状態] - lib_name と func_name の双方に "default" を設定する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr); // [確認_正常系] - 明示的デフォルトのため戻り値が NULL であること。
    EXPECT_EQ(2, entry_.resolved); // [確認_正常系] - resolved が明示的デフォルトを示す 2 になること。
}

// lib_name が未設定の場合に定義なしとして扱われることの確認
TEST_F(symLoaderResolveTest, marks_undefined_when_lib_name_is_empty)
{
    // Arrange
    set_names("", "com_util_path_basename"); // [状態] - lib_name を空文字列にしたエントリを用意する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);   // [確認_異常系] - 戻り値が NULL であること。
    EXPECT_EQ(-1, entry_.resolved); // [確認_異常系] - resolved が定義なしを示す -1 になること。
}

// func_name が未設定の場合に定義なしとして扱われることの確認
TEST_F(symLoaderResolveTest, marks_undefined_when_func_name_is_empty)
{
    // Arrange
    set_names("libcom_util", ""); // [状態] - func_name を空文字列にしたエントリを用意する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);   // [確認_異常系] - 戻り値が NULL であること。
    EXPECT_EQ(-1, entry_.resolved); // [確認_異常系] - resolved が定義なしを示す -1 になること。
}

// 拡張子を加えた名称が上限を超える場合に名称長超過として扱われることの確認
TEST_F(symLoaderResolveTest, marks_name_too_long_when_extension_does_not_fit)
{
    // Arrange
    char long_name[COM_UTIL_SYM_LOADER_NAME_MAX];

    std::memset(long_name, 'a', sizeof(long_name) - 1u);
    long_name[sizeof(long_name) - 1u] = '\0';
    set_names(long_name,
              "com_util_path_basename"); // [状態] - 拡張子を加えると上限を超える長さの lib_name を設定する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);   // [確認_異常系] - 戻り値が NULL であること。
    EXPECT_EQ(-2, entry_.resolved); // [確認_異常系] - resolved が名称長超過を示す -2 になること。
}

// 実在しないライブラリのオープン失敗が記録されることの確認
TEST_F(symLoaderResolveTest, marks_open_error_when_library_is_missing)
{
    // Arrange
    set_names("libcom_util_missing_for_test",
              "com_util_path_basename"); // [状態] - 実在しないライブラリ名を設定したエントリを用意する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);   // [確認_異常系] - 戻り値が NULL であること。
    EXPECT_EQ(-3, entry_.resolved); // [確認_異常系] - resolved がライブラリ オープン エラーを示す -3 になること。
    EXPECT_EQ(nullptr, entry_.handle); // [確認_異常系] - handle が NULL のままであること。
}

// 実在しないシンボル名でハンドルが解放されることの確認
TEST_F(symLoaderResolveTest, releases_handle_when_symbol_is_missing)
{
    // Arrange
    set_names("libcom_util",
              "com_util_symbol_that_does_not_exist"); // [状態] - 実在するライブラリと実在しない関数名を設定する。

    // Pre-Assert

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);      // [確認_異常系] - 戻り値が NULL であること。
    EXPECT_EQ(1, entry_.resolved);     // [確認_異常系] - resolved が解決済みを示す 1 になること。
    EXPECT_EQ(nullptr, entry_.handle); // [確認_異常系] - シンボルが見つからないためハンドルが解放されること。
}
