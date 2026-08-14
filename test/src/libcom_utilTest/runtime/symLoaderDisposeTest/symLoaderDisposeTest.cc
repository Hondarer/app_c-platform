#include <testfw.h>
#include <com_util/base/platform.h>
#include <com_util/runtime/sym_loader.h>

#if defined(PLATFORM_LINUX)
    #include <mock_dlfcn.h>
#endif /* PLATFORM_LINUX */

using testing::_;
using testing::NiceMock;
using testing::Return;

class symLoaderDisposeTest : public Test
{
};

namespace
{
COM_UTIL_MODULE_HANDLE const kFakeHandle = reinterpret_cast<COM_UTIL_MODULE_HANDLE>(static_cast<uintptr_t>(0x51));
void *const kFakeFunc = reinterpret_cast<void *>(static_cast<uintptr_t>(0x52));
} // namespace

// 解決済みエントリのハンドルと関数ポインターが解放されることの確認
TEST_F(symLoaderDisposeTest, releases_handle_and_func_ptr_of_resolved_entry)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("test_key", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry};
    entry.handle = kFakeHandle;
    entry.func_ptr = kFakeFunc; // [状態] - ハンドルと関数ポインターが入ったエントリを用意する。
#if defined(PLATFORM_LINUX)
    NiceMock<Mock_dlfcn> mock_dlfcn;
#endif /* PLATFORM_LINUX */

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_dlfcn, dlclose(_, _, _, kFakeHandle)).WillOnce(Return(0));
#endif /* PLATFORM_LINUX */
    // [Pre-Assert確認_正常系] - 解放 API がエントリのハンドルを 1 回閉じること。
    // [Pre-Assert手順] - 閉じる操作は成功を返却する。

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
#if defined(PLATFORM_LINUX)
    NiceMock<Mock_dlfcn> mock_dlfcn;
#endif /* PLATFORM_LINUX */

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_dlfcn, dlclose(_, _, _, _)).Times(0);
#endif /* PLATFORM_LINUX */
    // [Pre-Assert確認_正常系] - ハンドルが無いとき解放 API が呼び出されないこと。

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
    entry.handle = kFakeHandle;
    entry.func_ptr = kFakeFunc; // [状態] - ハンドル入りのエントリを用意する。
#if defined(PLATFORM_LINUX)
    NiceMock<Mock_dlfcn> mock_dlfcn;
#endif /* PLATFORM_LINUX */

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_dlfcn, dlclose(_, _, _, _)).Times(0);
#endif /* PLATFORM_LINUX */
    // [Pre-Assert確認_正常系] - 要素数 0 のとき解放 API が呼び出されないこと。

    // Act
    com_util_sym_loader_dispose(entries, 0u); // [手順] - 要素数に 0 を指定して解放する。

    // Assert
    EXPECT_EQ(kFakeHandle, entry.handle); // [確認_正常系] - 走査されないため handle が保持されたままであること。
}

// 複数エントリがまとめて解放されることの確認
TEST_F(symLoaderDisposeTest, releases_multiple_entries)
{
    // Arrange
    com_util_sym_loader_entry first = COM_UTIL_SYM_LOADER_ENTRY_INIT("first", void (*)(void));
    com_util_sym_loader_entry second = COM_UTIL_SYM_LOADER_ENTRY_INIT("second", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&first, &second};
    first.handle = kFakeHandle;
    first.func_ptr = kFakeFunc;
    second.handle = reinterpret_cast<COM_UTIL_MODULE_HANDLE>(static_cast<uintptr_t>(0x53));
    second.func_ptr =
        reinterpret_cast<void *>(static_cast<uintptr_t>(0x54)); // [状態] - ハンドル入りのエントリを 2 件用意する。
#if defined(PLATFORM_LINUX)
    NiceMock<Mock_dlfcn> mock_dlfcn;
#endif /* PLATFORM_LINUX */

    // Pre-Assert
#if defined(PLATFORM_LINUX)
    EXPECT_CALL(mock_dlfcn, dlclose(_, _, _, first.handle)).WillOnce(Return(0));
    EXPECT_CALL(mock_dlfcn, dlclose(_, _, _, second.handle)).WillOnce(Return(0));
#endif /* PLATFORM_LINUX */
    // [Pre-Assert確認_正常系] - 2 件のハンドルがそれぞれ閉じられること。
    // [Pre-Assert手順] - 閉じる操作は成功を返却する。

    // Act
    com_util_sym_loader_dispose(entries, 2u); // [手順] - 解決済みエントリ 2 件を指定して解放する。

    // Assert
    EXPECT_EQ(nullptr, first.handle);  // [確認_正常系] - 1 件目の handle が NULL になること。
    EXPECT_EQ(nullptr, second.handle); // [確認_正常系] - 2 件目の handle が NULL になること。
}
