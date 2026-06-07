/**
 *******************************************************************************
 *  @file           crypto_windows.c
 *  @brief          Windows 向け暗号化・復号モジュール (CNG AES-256-GCM)。
 *  @author         Tetsuo Honda
 *  @date           2026/03/12
 *  @version        1.0.0
 *
 *  Windows CNG (Cryptography Next Generation) の BCrypt API を使用して
 *  AES-256-GCM 暗号化・復号を実装します。\n
 *  Linux 実装 (OpenSSL) と同一の wire フォーマット ([暗号文][GCM タグ 16B]) を使用するため、
 *  クロスプラットフォーム通信に対応します。\n
 *  本ファイルは Windows ビルドでのみコンパイルされます (_WIN32 定義時)。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #include <com_util/base/windows_sdk.h>
    #include <bcrypt.h>
    #pragma comment(lib, "bcrypt.lib")
    #include <string.h>

    #include <com_util/crypto/crypto.h>

/* BCrypt を使用した AES-256-GCM 暗号化の共通実装。
   is_encrypt: TRUE = 暗号化、FALSE = 復号 (タグ検証含む)。
   src/src_len: 暗号化時は平文、復号時は暗号文 (タグを除く)。
   tag: 暗号化時は出力バッファー、復号時は検証用入力バッファー。 */
static int bcrypt_aes_gcm(const BOOL is_encrypt, uint8_t *dst, size_t *dst_len, const uint8_t *src,
                          const size_t src_len, const uint8_t *key, const uint8_t *nonce, const uint8_t *aad,
                          const size_t aad_len, uint8_t *tag, const ULONG tag_len)
{
    BCRYPT_ALG_HANDLE h_alg = NULL;
    BCRYPT_KEY_HANDLE h_key = NULL;
    NTSTATUS status;
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    ULONG out_len = 0;

    status = BCryptOpenAlgorithmProvider(&h_alg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    status =
        BCryptSetProperty(h_alg, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(h_alg, 0);
        return -1;
    }

    status = BCryptGenerateSymmetricKey(h_alg, &h_key, NULL, 0, (PUCHAR)key, (ULONG)COM_UTIL_CRYPTO_KEY_SIZE, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(h_alg, 0);
        return -1;
    }

    BCRYPT_INIT_AUTH_MODE_INFO(auth_info);
    auth_info.pbNonce = (PUCHAR)nonce;
    auth_info.cbNonce = (ULONG)COM_UTIL_CRYPTO_NONCE_SIZE;
    auth_info.pbAuthData = (PUCHAR)aad;
    if (aad != NULL)
    {
        auth_info.cbAuthData = (ULONG)aad_len;
    }
    else
    {
        auth_info.cbAuthData = 0U;
    }
    auth_info.pbTag = (PUCHAR)tag;
    auth_info.cbTag = tag_len;

    if (is_encrypt)
    {
        status = BCryptEncrypt(h_key, (PUCHAR)src, (ULONG)src_len, &auth_info, NULL, 0, (PUCHAR)dst, (ULONG)*dst_len,
                               &out_len, 0);
    }
    else
    {
        status = BCryptDecrypt(h_key, (PUCHAR)src, (ULONG)src_len, &auth_info, NULL, 0, (PUCHAR)dst, (ULONG)*dst_len,
                               &out_len, 0);
    }

    BCryptDestroyKey(h_key);
    BCryptCloseAlgorithmProvider(h_alg, 0);

    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    *dst_len = (size_t)out_len;
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_encrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, const size_t src_len, const uint8_t *key,
                     const uint8_t *nonce, const uint8_t *aad, const size_t aad_len)
{
    uint8_t tag[COM_UTIL_CRYPTO_TAG_SIZE];
    size_t enc_len;

    if (dst == NULL || dst_len == NULL || (src == NULL && src_len > 0) || key == NULL || nonce == NULL ||
        *dst_len < src_len + COM_UTIL_CRYPTO_TAG_SIZE)
    {
        return -1;
    }

    enc_len = src_len;

    {
        /* BCrypt は src=NULL を受け付けないため、空バッファーを用意する */
        static const uint8_t empty_src[1] = {0};
        const uint8_t *actual_src;
        if (src != NULL)
        {
            actual_src = src;
        }
        else
        {
            actual_src = empty_src;
        }

        if (bcrypt_aes_gcm(TRUE, dst, &enc_len, actual_src, src_len, key, nonce, aad, aad_len, tag,
                           (ULONG)COM_UTIL_CRYPTO_TAG_SIZE) != 0)
        {
            return -1;
        }
    }

    /* タグ (16B) を暗号文の直後に付加する */
    memcpy(dst + enc_len, tag, COM_UTIL_CRYPTO_TAG_SIZE);
    *dst_len = enc_len + COM_UTIL_CRYPTO_TAG_SIZE;
    return 0;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_decrypt(uint8_t *dst, size_t *dst_len, const uint8_t *src, const size_t src_len, const uint8_t *key,
                     const uint8_t *nonce, const uint8_t *aad, const size_t aad_len)
{
    size_t plain_len;

    if (dst == NULL || dst_len == NULL || src == NULL || src_len < COM_UTIL_CRYPTO_TAG_SIZE || key == NULL ||
        nonce == NULL)
    {
        return -1;
    }

    plain_len = src_len - COM_UTIL_CRYPTO_TAG_SIZE;

    if (plain_len > 0 && *dst_len < plain_len)
    {
        return -1;
    }

    /* タグは暗号文の末尾 COM_UTIL_CRYPTO_TAG_SIZE バイト。BCrypt がタグ検証を行う。
       STATUS_AUTH_TAG_MISMATCH 時は bcrypt_aes_gcm が -1 を返す。 */
    return bcrypt_aes_gcm(FALSE, dst, dst_len, src, plain_len, key, nonce, aad, aad_len, (uint8_t *)(src + plain_len),
                          (ULONG)COM_UTIL_CRYPTO_TAG_SIZE);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_passphrase_to_key(uint8_t *key, const uint8_t *passphrase, const size_t passphrase_len)
{
    BCRYPT_ALG_HANDLE h_alg = NULL;
    BCRYPT_HASH_HANDLE h_hash = NULL;
    NTSTATUS status;
    static const uint8_t empty[1] = {0};
    const uint8_t *data;
    ULONG data_len;
    if (passphrase != NULL)
    {
        data = passphrase;
    }
    else
    {
        data = empty;
    }
    if (passphrase != NULL)
    {
        data_len = (ULONG)passphrase_len;
    }
    else
    {
        data_len = 0UL;
    }

    if (key == NULL || (passphrase == NULL && passphrase_len > 0))
    {
        return -1;
    }

    status = BCryptOpenAlgorithmProvider(&h_alg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        return -1;
    }

    status = BCryptCreateHash(h_alg, &h_hash, NULL, 0, NULL, 0, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(h_alg, 0);
        return -1;
    }

    status = BCryptHashData(h_hash, (PUCHAR)data, data_len, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptDestroyHash(h_hash);
        BCryptCloseAlgorithmProvider(h_alg, 0);
        return -1;
    }

    status = BCryptFinishHash(h_hash, (PUCHAR)key, (ULONG)COM_UTIL_CRYPTO_KEY_SIZE, 0);

    BCryptDestroyHash(h_hash);
    BCryptCloseAlgorithmProvider(h_alg, 0);

    if (BCRYPT_SUCCESS(status))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

#endif /* PLATFORM_WINDOWS */
