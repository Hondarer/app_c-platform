/**
 *******************************************************************************
 *  @file           byteorder.h
 *  @brief          ホスト バイト オーダーとネットワーク バイト オーダーを相互変換する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/11
 *  @version        1.0.0
 *
 *  変換はシフト演算とバイト列の再構成だけで行い、`htons` などの OS API を使用しません。\n
 *  ホストのバイト オーダーを判定せずに正しく変換できるため、プラットフォーム分岐も持ちません。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_NET_BYTEORDER_H
#define COM_UTIL_NET_BYTEORDER_H

#include <stdint.h>
#include <string.h>

/**
 *  @ingroup        COM_UTIL_NET
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          16 bit 値をホスト バイト オーダーからネットワーク バイト オーダーへ変換します。
     *  @param[in]      value 変換元の値 (ホスト バイト オーダー)。
     *  @return         ネットワーク バイト オーダーの値を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    static inline uint16_t com_util_hton16(const uint16_t value)
    {
        uint8_t bytes[2];
        uint16_t result;

        bytes[0] = (uint8_t)(((uint32_t)value >> 8U) & UINT32_C(0xFF));
        bytes[1] = (uint8_t)((uint32_t)value & UINT32_C(0xFF));
        memcpy(&result, bytes, sizeof(result));

        return result;
    }

    /**
     *  @brief          16 bit 値をネットワーク バイト オーダーからホスト バイト オーダーへ変換します。
     *  @param[in]      value 変換元の値 (ネットワーク バイト オーダー)。
     *  @return         ホスト バイト オーダーの値を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    static inline uint16_t com_util_ntoh16(const uint16_t value)
    {
        uint8_t bytes[2];

        memcpy(bytes, &value, sizeof(value));

        return (uint16_t)(((uint16_t)bytes[0] << 8) | (uint16_t)bytes[1]);
    }

    /**
     *  @brief          32 bit 値をホスト バイト オーダーからネットワーク バイト オーダーへ変換します。
     *  @param[in]      value 変換元の値 (ホスト バイト オーダー)。
     *  @return         ネットワーク バイト オーダーの値を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    static inline uint32_t com_util_hton32(const uint32_t value)
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

    /**
     *  @brief          32 bit 値をネットワーク バイト オーダーからホスト バイト オーダーへ変換します。
     *  @param[in]      value 変換元の値 (ネットワーク バイト オーダー)。
     *  @return         ホスト バイト オーダーの値を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    static inline uint32_t com_util_ntoh32(const uint32_t value)
    {
        uint8_t bytes[4];

        memcpy(bytes, &value, sizeof(value));

        return (((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) |
                (uint32_t)bytes[3]);
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_NET_BYTEORDER_H */
