/**
 *******************************************************************************
 *  @file           random_windows.c
 *  @brief          CNG を用いて暗号論的乱数を取得する Windows 向け機能を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/30
 *  @version        1.0.0
 *
 *  CNG の `BCryptGenRandom` を `BCRYPT_USE_SYSTEM_PREFERRED_RNG` 指定で使用し、
 *  アルゴリズム プロバイダーを開かずに暗号論的乱数源からバイト列を取得します。\n
 *  本ファイルは Windows ビルドでのみコンパイルされます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>

    #include <bcrypt.h>
    #pragma comment(lib, "Bcrypt.lib")

    #include <com_util/base/result.h>
    #include <com_util/crypto/random.h>

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_random_bytes(void *buf, const size_t size)
{
    NTSTATUS status;

    if (size == 0U)
    {
        return COM_UTIL_OK;
    }
    if (buf == NULL)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }
    /* クロスプラットフォームの最大値を超える要求は、OS API へ渡す前に引数不正として扱う */
    if (size > COM_UTIL_CRYPTO_RANDOM_MAX_BYTES)
    {
        return COM_UTIL_ERR_INVALID_ARGUMENT;
    }

    /* BCRYPT_USE_SYSTEM_PREFERRED_RNG を指定すると、アルゴリズム プロバイダーの */
    /* ハンドルを開かずにシステム既定の乱数源を使用できる。                       */
    /* see: https://learn.microsoft.com/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom */
    status = BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status))
    {
        return COM_UTIL_ERR_UNKNOWN;
    }

    return COM_UTIL_OK;
}

#endif /* PLATFORM_WINDOWS */
