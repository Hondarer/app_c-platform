/**
 *******************************************************************************
 *  @file           byteorder.c
 *  @brief          ホスト バイト オーダーとネットワーク バイト オーダーを相互変換します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/15
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/net/byteorder.h>
#include <string.h>

/* Doxygen コメントは、ヘッダーに記載 */

uint16_t com_util_hton16(const uint16_t value)
{
    uint8_t bytes[2];
    uint16_t result;

    bytes[0] = (uint8_t)(((uint32_t)value >> 8U) & UINT32_C(0xFF));
    bytes[1] = (uint8_t)((uint32_t)value & UINT32_C(0xFF));
    memcpy(&result, bytes, sizeof(result));

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

uint16_t com_util_ntoh16(const uint16_t value)
{
    uint8_t bytes[2];

    memcpy(bytes, &value, sizeof(value));

    return (uint16_t)(((uint16_t)bytes[0] << 8) | (uint16_t)bytes[1]);
}

/* Doxygen コメントは、ヘッダーに記載 */

uint32_t com_util_hton32(const uint32_t value)
{
    uint8_t bytes[4];
    uint32_t result;

    bytes[0] = (uint8_t)((value >> 24) & 0xFFU);
    bytes[1] = (uint8_t)((value >> 16) & 0xFFU);
    bytes[2] = (uint8_t)((value >> 8) & 0xFFU);
    bytes[3] = (uint8_t)(value & 0xFFU);
    memcpy(&result, bytes, sizeof(result));

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

uint32_t com_util_ntoh32(const uint32_t value)
{
    uint8_t bytes[4];

    memcpy(bytes, &value, sizeof(value));

    return (((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3]);
}
