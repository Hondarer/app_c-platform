/**
 *******************************************************************************
 *  @file           socket_internal.h
 *  @brief          net モジュール内部で共有するソケット初期化 API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/11
 *  @version        1.0.0
 *
 *  Winsock の初期化と終了は公開 API にせず、net モジュール内部で完結させます。\n
 *  Linux では初期化が不要なため、本ヘッダーの API は Windows 専用です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef CPLAT_NET_SOCKET_INTERNAL_H
#define CPLAT_NET_SOCKET_INTERNAL_H

#include <cplat/base/error.h>
#include <cplat/base/platform.h>

#if defined(PLATFORM_WINDOWS)

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    /**
     *  @brief          Winsock を初期化します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref CPLAT_OK 、@ref CPLAT_ERR_UNKNOWN のいずれかを返します。
     *
     *  初回の呼び出しでだけ WSAStartup() を実行し、以降は記録した結果を返します。\n
     *  ソケットを扱う公開 API は、OS API を呼び出す前に本関数を呼び出します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  初期化の実行は cplat_call_once() で 1 回に限定されます。
     */
    int cplat_internal_socket_startup(cplat_error *detail_out);

    /**
     *  @brief          Winsock を終了します。
     *
     *  共有ライブラリのアンロード時に呼び出します。初期化していない場合は何もしません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  共有ライブラリのアンロード経路からのみ呼び出します。
     */
    void cplat_internal_socket_cleanup(void);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_WINDOWS */

#endif /* CPLAT_NET_SOCKET_INTERNAL_H */
