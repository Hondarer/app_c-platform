/**
 *******************************************************************************
 *  @file           random.h
 *  @brief          暗号論的に安全な乱数を取得する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/30
 *  @version        1.0.0
 *
 *  Linux では OpenSSL の `RAND_bytes`、Windows では CNG の `BCryptGenRandom` を
 *  使用して、OS が提供する暗号論的乱数源からバイト列を取得します。\n
 *  鍵、ノンス、セッション識別子など、推測されてはならない値の生成に使用します。
 *
 *  @warning        `rand()` は使用しないでください。glibc は 31 bit、MSVC は
 *                  15 bit しか返さず、いずれも予測可能です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_CRYPTO_RANDOM_H
#define COM_UTIL_CRYPTO_RANDOM_H

#include <com_util/base/result.h>
#include <com_util/com_util_export.h>
#include <stddef.h>

/**
 *  @ingroup        COM_UTIL_CRYPTO
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          暗号論的に安全な乱数でバッファーを満たします。
     *  @param[out]     buf   乱数の格納先。@p size が 0 の場合に限り NULL も指定できます。
     *  @param[in]      size  @p buf のバイト数。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  要求したバイト数をすべて満たした場合のみ @ref COM_UTIL_OK を返します。\n
     *  部分的に満たすことはありません。
     *
     *  @warning        戻り値を無視してはなりません。乱数源が利用できない場合に
     *                  @p buf の内容は不定であり、そのまま鍵やノンスとして使用すると
     *                  暗号強度が失われます。失敗時は処理を継続せず、呼び出し元へ
     *                  エラーを伝播してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持たず、OS の乱数源を直接使用します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_random_bytes(void *buf, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_CRYPTO_RANDOM_H */
