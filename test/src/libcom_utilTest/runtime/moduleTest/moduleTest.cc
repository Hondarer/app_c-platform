#include <testfw.h>
#include <mock_com_util.h>
#include <com_util/base/result.h>
#include <com_util/crt/path.h>
#include <com_util/runtime/module.h>

#include <cstring>
#include <string>

#include "module.inject.h"

class moduleTest : public Test
{
  protected:
    /* テスト バイナリ内の関数アドレス。所属モジュールの特定に使用する。
       共有ライブラリ側の関数は ADD_SRCS でテスト バイナリへ取り込まれるため、
       所属モジュールとしては常にテスト バイナリが解決される。 */
    static const void *self_func_addr()
    {
        return reinterpret_cast<const void *>(&self_func_addr);
    }
};

/*
 * com_util_module_get_path
 */

// 関数アドレスから所属モジュールの絶対パスが取得できることの確認
TEST_F(moduleTest, get_path_returns_absolute_path_of_owning_module)
{
    // Arrange
    char path[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    std::memset(path, 0, sizeof(path));

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_path(path, sizeof(path),
                                       self_func_addr()); // [手順] - テスト バイナリ内の関数アドレスを指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_module_get_path の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ('/', path[0]);     // [確認_正常系] - 絶対パスが返ること。
    EXPECT_NE(nullptr,
              std::strstr(path, "moduleTest")); // [確認_正常系] - パスに所属モジュール名 moduleTest が含まれること。
}

// out_path に NULL を渡した場合に拒否されることの確認
TEST_F(moduleTest, get_path_rejects_null_out_path)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_path(NULL, 16u,
                                       self_func_addr()); // [手順] - out_path に NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_module_get_path の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// out_path_sz に 0 を渡した場合に拒否されることの確認
TEST_F(moduleTest, get_path_rejects_zero_size)
{
    // Arrange
    char path[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_path(path, 0u,
                                       self_func_addr()); // [手順] - out_path_sz に 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_module_get_path の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// func_addr に NULL を渡した場合に拒否されることの確認
TEST_F(moduleTest, get_path_rejects_null_func_addr)
{
    // Arrange
    char path[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_path(path, sizeof(path), NULL); // [手順] - func_addr に NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_module_get_path の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// 出力バッファーが不足する場合に拒否されることの確認
TEST_F(moduleTest, get_path_returns_buffer_too_small)
{
    // Arrange
    char path[4]; // [状態] - モジュール パスが収まらない 4 byte の出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_path(path, sizeof(path),
                                       self_func_addr()); // [手順] - 不足するバッファーを指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              rtc); // [確認_異常系] - com_util_module_get_path の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
}

/*
 * com_util_module_get_basename
 */

// 所属モジュールの拡張子を除いた名前が取得できることの確認
TEST_F(moduleTest, get_basename_returns_module_name_without_extension)
{
    // Arrange
    char basename[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    std::memset(basename, 0, sizeof(basename));

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_basename(
        basename, sizeof(basename), self_func_addr()); // [手順] - テスト バイナリ内の関数アドレスを指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc);        // [確認_正常系] - com_util_module_get_basename の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("moduleTest", basename); // [確認_正常系] - 拡張子を持たないモジュール名 "moduleTest" が返ること。
}

// out_basename に NULL を渡した場合に拒否されることの確認
TEST_F(moduleTest, get_basename_rejects_null_out_basename)
{
    // Arrange

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_basename(NULL, 16u,
                                           self_func_addr()); // [手順] - out_basename に NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - com_util_module_get_basename の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// out_basename_sz に 0 を渡した場合に拒否されることの確認
TEST_F(moduleTest, get_basename_rejects_zero_size)
{
    // Arrange
    char basename[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_basename(basename, 0u,
                                           self_func_addr()); // [手順] - out_basename_sz に 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - com_util_module_get_basename の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// パス取得に失敗した場合にその結果がそのまま返ることの確認
TEST_F(moduleTest, get_basename_propagates_get_path_failure)
{
    // Arrange
    char basename[PLATFORM_PATH_MAX]; // [状態] - 出力バッファーを用意する。

    std::memset(basename, 'X', sizeof(basename));

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_basename(basename, sizeof(basename),
                                           NULL); // [手順] - func_addr に NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc); // [確認_異常系] - com_util_module_get_path が返した COM_UTIL_ERR_INVALID_ARGUMENT がそのまま返ること。
    EXPECT_STREQ("", basename); // [確認_異常系] - 出力バッファーが空文字列に初期化されること。
}

// 拡張子を除いた名前が収まらない場合に拒否されることの確認
TEST_F(moduleTest, get_basename_returns_buffer_too_small)
{
    // Arrange
    char basename[4]; // [状態] - "moduleTest" が収まらない 4 byte の出力バッファーを用意する。

    // Pre-Assert

    // Act
    int rtc = com_util_module_get_basename(basename, sizeof(basename),
                                           self_func_addr()); // [手順] - 不足するバッファーを指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_BUFFER_TOO_SMALL,
        rtc); // [確認_異常系] - com_util_module_get_basename の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_STREQ("", basename); // [確認_異常系] - 出力バッファーが空文字列に初期化されること。
}

/*
 * find_shared_lib_extension_cut (inject 経由)
 */

#if defined(PLATFORM_LINUX)

// バージョン付き .so の切り出し位置が返ることの確認
TEST_F(moduleTest, extension_cut_finds_versioned_so)
{
    // Arrange
    const char name[] = "libsample.so.1.2.3"; // [状態] - バージョン付き共有ライブラリ名を用意する。

    // Pre-Assert

    // Act
    const char *cut = test_find_shared_lib_extension_cut(name); // [手順] - 切り出し位置を取得する。

    // Assert
    EXPECT_EQ(name + 9, cut); // [確認_正常系] - ".so.1.2.3" の先頭を指すポインターが返ること。
}

// 末尾が .so の場合の切り出し位置が返ることの確認
TEST_F(moduleTest, extension_cut_finds_plain_so)
{
    // Arrange
    const char name[] = "libsample.so"; // [状態] - 末尾が .so の共有ライブラリ名を用意する。

    // Pre-Assert

    // Act
    const char *cut = test_find_shared_lib_extension_cut(name); // [手順] - 切り出し位置を取得する。

    // Assert
    EXPECT_EQ(name + 9, cut); // [確認_正常系] - ".so" の先頭を指すポインターが返ること。
}

// 末尾が .dylib の場合の切り出し位置が返ることの確認
TEST_F(moduleTest, extension_cut_finds_dylib)
{
    // Arrange
    const char name[] = "libsample.dylib"; // [状態] - 末尾が .dylib の共有ライブラリ名を用意する。

    // Pre-Assert

    // Act
    const char *cut = test_find_shared_lib_extension_cut(name); // [手順] - 切り出し位置を取得する。

    // Assert
    EXPECT_EQ(name + 9, cut); // [確認_正常系] - ".dylib" の先頭を指すポインターが返ること。
}

// 共有ライブラリの拡張子でない場合に NULL が返ることの確認
TEST_F(moduleTest, extension_cut_returns_null_for_other_extension)
{
    // Arrange
    const char name[] = "sample.txt"; // [状態] - 共有ライブラリでない拡張子の名前を用意する。

    // Pre-Assert

    // Act
    const char *cut = test_find_shared_lib_extension_cut(name); // [手順] - 切り出し位置を取得する。

    // Assert
    EXPECT_EQ(nullptr, cut); // [確認_正常系] - 汎用の拡張子除去へ委譲するため NULL が返ること。
}

// 拡張子より短い名前で NULL が返ることの確認
TEST_F(moduleTest, extension_cut_returns_null_for_short_name)
{
    // Arrange
    const char name[] = "ab"; // [状態] - ".so" より短い名前を用意する。

    // Pre-Assert

    // Act
    const char *cut = test_find_shared_lib_extension_cut(name); // [手順] - 切り出し位置を取得する。

    // Assert
    EXPECT_EQ(nullptr, cut); // [確認_正常系] - 比較対象に満たないため NULL が返ること。
}

#endif /* PLATFORM_LINUX */
