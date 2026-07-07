/**
 *******************************************************************************
 *  @file           sync_descriptor.c
 *  @brief          プロセス間同期ディスクリプターの直列化機能 (内部共有) を実装します。
 *
 *  ワイヤ フォーマットの詳細は sync_descriptor.h を参照してください。
 *  プラットフォーム非依存であり、Linux と Windows の双方から使用します。
 *******************************************************************************
 */

#include <stdlib.h>
#include <string.h>

#include <com_util/sync/sync_descriptor.h>

/** ディスクリプターの先頭に置くマジック文字列。 */
static const char DESCRIPTOR_MAGIC[4] = {'C', 'U', 'L', 'K'};

/* Doxygen コメントは、ヘッダーに記載 */

com_util_sync_result_t interprocess_sync_descriptor_export(const char *identity, const uint8_t kind,
                                                           const uint8_t backend, void *descriptor,
                                                           size_t *descriptor_size)
{
    uint8_t *out;
    size_t identity_len;
    size_t required;

    if (identity == NULL || descriptor_size == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    identity_len = strlen(identity);
    required = INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE + identity_len;
    if (descriptor == NULL || *descriptor_size < required)
    {
        *descriptor_size = required;
        return COM_UTIL_SYNC_BUFFER_TOO_SMALL;
    }

    out = (uint8_t *)descriptor;
    memcpy(out, DESCRIPTOR_MAGIC, sizeof(DESCRIPTOR_MAGIC));
    out[4] = INTERPROCESS_SYNC_DESCRIPTOR_VERSION;
    out[5] = kind;
    out[6] = backend;
    out[7] = 0U;
    out[8] = (uint8_t)(identity_len & 0xffU);
    out[9] = (uint8_t)((identity_len >> 8) & 0xffU);
    out[10] = (uint8_t)((identity_len >> 16) & 0xffU);
    out[11] = (uint8_t)((identity_len >> 24) & 0xffU);
    memset(out + 12, 0, 8);
    memcpy(out + INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE, identity, identity_len);
    *descriptor_size = required;
    return COM_UTIL_SYNC_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

com_util_sync_result_t interprocess_sync_descriptor_import(const void *descriptor, const size_t descriptor_size,
                                                           const uint8_t kind, const uint8_t backend,
                                                           char **identity_out)
{
    const uint8_t *in = (const uint8_t *)descriptor;
    uint32_t identity_len;
    char *identity;

    if (descriptor == NULL || identity_out == NULL)
    {
        return COM_UTIL_SYNC_INVALID_ARGUMENT;
    }
    if (descriptor_size < INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE ||
        memcmp(in, DESCRIPTOR_MAGIC, sizeof(DESCRIPTOR_MAGIC)) != 0 || in[4] != INTERPROCESS_SYNC_DESCRIPTOR_VERSION ||
        in[5] != kind || in[6] != backend)
    {
        return COM_UTIL_SYNC_CORRUPT_DESCRIPTOR;
    }
    identity_len = (uint32_t)in[8] | ((uint32_t)in[9] << 8) | ((uint32_t)in[10] << 16) | ((uint32_t)in[11] << 24);
    if (identity_len == 0 || descriptor_size != INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE + (size_t)identity_len)
    {
        return COM_UTIL_SYNC_CORRUPT_DESCRIPTOR;
    }
    identity = (char *)malloc((size_t)identity_len + 1U);
    if (identity == NULL)
    {
        return COM_UTIL_SYNC_SYSTEM_ERROR;
    }
    memcpy(identity, in + INTERPROCESS_SYNC_DESCRIPTOR_HEADER_SIZE, identity_len);
    identity[identity_len] = '\0';
    *identity_out = identity;
    return COM_UTIL_SYNC_OK;
}
