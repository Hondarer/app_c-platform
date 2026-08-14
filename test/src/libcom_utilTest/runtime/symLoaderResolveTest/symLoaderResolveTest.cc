#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/runtime/sym_loader.h>

#include <cstring>

#include "sym_loader_resolve.inject.h"

namespace
{
void complete_entry_lock_initialization(com_util_sym_loader_entry *entry)
{
    entry->lock_state = 2;
}
} // namespace

class symLoaderResolveTest : public Test
{
  protected:
    com_util_sym_loader_entry entry_ = COM_UTIL_SYM_LOADER_ENTRY_INIT("test_key", void (*)(void));

    /* lib_name / func_name は固定長配列のため、テストからは直接書き込む */
    void set_names(const char *lib_name, const char *func_name)
    {
        ASSERT_EQ(COM_UTIL_OK, com_util_strcpy(entry_.lib_name, sizeof(entry_.lib_name), lib_name));
        ASSERT_EQ(COM_UTIL_OK, com_util_strcpy(entry_.func_name, sizeof(entry_.func_name), func_name));
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
    EXPECT_NE(nullptr, func_ptr);  // [確認_正常系] - com_util_sym_loader_resolve の戻り値が NULL でないこと。
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
    EXPECT_EQ(first, second);      // [確認_正常系] - 2 回目の戻り値が 1 回目と同じポインターであること。
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
    EXPECT_EQ(nullptr, func_ptr);  // [確認_正常系] - 明示的デフォルトのため戻り値が NULL であること。
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
    EXPECT_EQ(nullptr, func_ptr);      // [確認_異常系] - 戻り値が NULL であること。
    EXPECT_EQ(-3, entry_.resolved);    // [確認_異常系] - resolved がライブラリ オープン エラーを示す -3 になること。
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

#if defined(PLATFORM_LINUX)

// ロックの生成に失敗した場合に解決が失敗することの確認
// Windows は InterlockedCompareExchange を使うが、ロック生成の失敗経路は共通である
TEST_F(symLoaderResolveTest, returns_null_when_lock_creation_fails)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;

    set_names("libcom_util", "com_util_path_basename"); // [状態] - 実在するライブラリ名と関数名を設定する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_local_lock_create(_))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - com_util_local_lock_create が 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は COM_UTIL_ERR_UNKNOWN を返却し、以降は本物へ委譲する。

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);  // [確認_異常系] - com_util_sym_loader_resolve の戻り値が NULL であること。
    EXPECT_EQ(0, entry_.resolved); // [確認_異常系] - 解決状態が未解決のままであること。
}

// ロックの取得に失敗した場合に解決が失敗することの確認
TEST_F(symLoaderResolveTest, returns_null_when_lock_acquisition_fails)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;

    set_names("libcom_util", "com_util_path_basename"); // [状態] - 実在するライブラリ名と関数名を設定する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_local_lock_lock(_, _))
        .WillOnce(Return(COM_UTIL_ERR_UNKNOWN))
        .WillRepeatedly(
            DoDefault()); // [Pre-Assert確認_異常系] - com_util_local_lock_lock が 1 回目に呼び出されること。
                          // [Pre-Assert手順] - 1 回目は COM_UTIL_ERR_UNKNOWN を返却し、以降は本物へ委譲する。

    // Act
    void *func_ptr = com_util_sym_loader_resolve(&entry_); // [手順] - com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, func_ptr);  // [確認_異常系] - com_util_sym_loader_resolve の戻り値が NULL であること。
    EXPECT_EQ(0, entry_.resolved); // [確認_異常系] - 解決状態が未解決のままであること。
}

// ほかのスレッドがロック初期化を完了した状態を再利用することの確認
TEST_F(symLoaderResolveTest, reuses_lock_initialized_by_another_thread)
{
    // Arrange
    ASSERT_EQ(COM_UTIL_OK, com_util_local_lock_create(&entry_.lock)); // [状態] - エントリのロックを生成する。
                                                                    // [状態確認] - com_util_local_lock_create の戻り値が COM_UTIL_OK であること。
    entry_.lock_state = 1;
    test_sym_loader_set_entry_lock_wait_hook(complete_entry_lock_initialization);

    // Pre-Assert

    // Act
    int result = test_sym_loader_ensure_entry_lock_initialized(
        &entry_); // [手順] - 他スレッドの初期化中を表すエントリでロック初期化確認を実行する。

    // Assert
    EXPECT_EQ(0, result); // [確認_正常系] - test_sym_loader_ensure_entry_lock_initialized の戻り値が 0 であること。
    EXPECT_EQ(2, entry_.lock_state); // [確認_正常系] - ロック初期化状態が完了を示す 2 であること。

    // Cleanup
    test_sym_loader_set_entry_lock_wait_hook(NULL);
}

// ロック初期化待機の既定処理が呼び出し元へ制御を戻すことの確認
TEST_F(symLoaderResolveTest, default_lock_wait_yields_execution)
{
    // Arrange

    // Pre-Assert

    // Act
    test_sym_loader_default_entry_lock_wait(&entry_); // [手順] - ロック初期化待機の既定処理を 1 回実行する。

    // Assert
    SUCCEED(); // [確認_正常系] - test_sym_loader_default_entry_lock_wait が呼び出し元へ制御を戻すこと。
}

// ほかのスレッドによるロック初期化失敗を通知することの確認
TEST_F(symLoaderResolveTest, reports_lock_initialization_failure_from_another_thread)
{
    // Arrange
    entry_.lock_state = -1;

    // Pre-Assert

    // Act
    int result = test_sym_loader_ensure_entry_lock_initialized(
        &entry_); // [手順] - 他スレッドの初期化失敗を表すエントリでロック初期化確認を実行する。

    // Assert
    EXPECT_EQ(-1, result); // [確認_異常系] - test_sym_loader_ensure_entry_lock_initialized の戻り値が -1 であること。
}

// ロック取得中に解決済みとなった結果を再利用することの確認
TEST_F(symLoaderResolveTest, returns_result_resolved_while_waiting_for_lock)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    set_names("default", "not_default");

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_local_lock_lock(_, _))
        .WillOnce(Invoke(
            [this](com_util_local_lock *, int)
            {
                entry_.resolved = 1;
                entry_.func_ptr = reinterpret_cast<void *>(1);
                return COM_UTIL_OK;
            })); // [Pre-Assert確認_正常系] - com_util_local_lock_lock が 1 回呼び出されること。
                 // [Pre-Assert手順] - resolved と func_ptr を設定し、COM_UTIL_OK を返却する。

    // Act
    void *result = com_util_sym_loader_resolve(
        &entry_); // [手順] - ロック待機中に解決済みとなるエントリで com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(reinterpret_cast<void *>(1),
              result); // [確認_正常系] - com_util_sym_loader_resolve の戻り値が別スレッドの解決結果であること。
}

// ライブラリ名だけが default の場合に通常のライブラリ名として扱うことの確認
TEST_F(symLoaderResolveTest, does_not_mark_default_when_only_library_name_is_default)
{
    // Arrange
    set_names("default", "not_default");

    // Pre-Assert

    // Act
    void *result = com_util_sym_loader_resolve(
        &entry_); // [手順] - ライブラリ名だけが default のエントリで com_util_sym_loader_resolve を呼び出す。

    // Assert
    EXPECT_EQ(nullptr, result);     // [確認_異常系] - com_util_sym_loader_resolve の戻り値が NULL であること。
    EXPECT_EQ(-3, entry_.resolved); // [確認_異常系] - 実在しない default ライブラリの解決結果が -3 であること。
}

#endif /* PLATFORM_LINUX */
