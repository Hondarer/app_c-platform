/**
 *******************************************************************************
 *  @file           socket.h
 *  @brief          IPv4 ソケットの生成、接続、送受信、待機を行う API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/11
 *  @version        1.0.0
 *
 *  プラットフォームごとに異なるソケット API を共通インターフェースで抽象化します。
 *
 *  | OS      | 使用ライブラリ                    | 待機 API   | 非ブロッキング設定       |
 *  | ------- | --------------------------------- | ---------- | ------------------------ |
 *  | Linux   | BSD ソケット (libc)               | `poll`     | `fcntl` (`O_NONBLOCK`)   |
 *  | Windows | Winsock2 (`ws2_32.lib`)           | `WSAPoll`  | `ioctlsocket` (`FIONBIO`)|
 *
 *  Winsock の初期化と終了は本モジュールの内部で行うため、利用側は初期化順序を
 *  意識せずに任意のタイミングで API を呼び出せます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_NET_SOCKET_H
#define COM_UTIL_NET_SOCKET_H

#include <com_util/base/error.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>
#include <com_util/net/endpoint.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/** 無効なソケットを表す値。 */
#define COM_UTIL_INVALID_SOCKET ((com_util_socket)-1)

/** タイムアウトなしで待機することを表すタイムアウト値。 */
#define COM_UTIL_SOCKET_WAIT_FOREVER (-1)

/** 即時リターンすることを表すタイムアウト値。 */
#define COM_UTIL_SOCKET_NO_WAIT 0

/** @ref com_util_socket_listen へ既定の待ち受けキュー長を指定する値。 */
#define COM_UTIL_SOCKET_BACKLOG_DEFAULT 0

/** @ref com_util_socket_wait_readable_multi へ一度に指定できるソケットの最大数。 */
#define COM_UTIL_SOCKET_WAIT_MAX 16U

/** 1 回の送受信で扱えるバイト数の上限。 */
#define COM_UTIL_SOCKET_MAX_TRANSFER ((size_t)INT_MAX)

/**
 *  @ingroup        COM_UTIL_NET
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          ソケット ハンドルを表します。
     *
     *  Linux のファイル記述子 (`int`) と Windows の `SOCKET` (`UINT_PTR`) の双方を
     *  可逆に格納できる幅を持ちます。\n
     *  無効値の判定には @ref COM_UTIL_INVALID_SOCKET を使用し、数値リテラルと比較しません。
     */
    typedef intptr_t com_util_socket;

    /**
     *  @brief          生成するソケットの種別を表します。
     */
    typedef enum com_util_socket_kind
    {
        COM_UTIL_SOCKET_TCP = 0, /**< 接続指向のストリーム ソケット。 */
        COM_UTIL_SOCKET_UDP = 1  /**< データグラム ソケット。 */
    } com_util_socket_kind;

    /**
     *  @brief          IPv4 ソケットを生成します。
     *  @param[in]      kind     生成するソケットの種別。
     *  @param[out]     sock_out 生成したソケットの格納先。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  失敗した場合、@p sock_out へ @ref COM_UTIL_INVALID_SOCKET を格納します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_open(com_util_socket_kind kind, com_util_socket *sock_out,
                                                          com_util_error *detail_out);

    /**
     *  @brief          ソケットを閉じます。
     *  @param[in]      sock 閉じるソケット。@ref COM_UTIL_INVALID_SOCKET を指定した場合は何もしません。
     *
     *  本関数は直前エラー (@ref com_util_error_get_last) を保存し、復元してから返ります。\n
     *  解放経路で呼び出しても、呼び出し前に記録された診断情報が失われません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一のソケットを複数のスレッドから同時に閉じないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_socket_close(com_util_socket sock);

    /**
     *  @brief          ソケットの送受信を両方向とも停止します。
     *  @param[in]      sock 対象のソケット。@ref COM_UTIL_INVALID_SOCKET を指定した場合は何もしません。
     *
     *  待機中のスレッドを解除する目的で使用します。ソケットは閉じません。\n
     *  失敗しても通知しません。停止できない場合でも呼び出し側に取るべき手段がないためです。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_socket_shutdown(com_util_socket sock);

    /**
     *  @brief          ソケットへローカルの端点を割り当てます。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      endpoint   割り当てる端点。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_bind(com_util_socket sock,
                                                          const com_util_ipv4_endpoint *endpoint,
                                                          com_util_error *detail_out);

    /**
     *  @brief          ソケットを接続待ち受け状態にします。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      backlog    待ち受けキューの長さ。@ref COM_UTIL_SOCKET_BACKLOG_DEFAULT を
     *                             指定した場合、OS の既定の最大値を使用します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_listen(com_util_socket sock, int backlog,
                                                            com_util_error *detail_out);

    /**
     *  @brief          待ち受け中のソケットで接続を受け付けます。
     *  @param[in]      sock       待ち受け中のソケット。
     *  @param[out]     peer_out   接続元の端点の格納先。NULL 可。
     *  @param[out]     sock_out   受け付けたソケットの格納先。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  失敗した場合、@p sock_out へ @ref COM_UTIL_INVALID_SOCKET を格納します。\n
     *  非ブロッキング モードで接続待ちがない場合は、@p detail_out の要因が
     *  @ref COM_UTIL_CAUSE_WOULD_BLOCK になります。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_accept(com_util_socket sock, com_util_ipv4_endpoint *peer_out,
                                                            com_util_socket *sock_out, com_util_error *detail_out);

    /**
     *  @brief          相手の端点へ接続します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      endpoint   接続先の端点。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  非ブロッキング モードで接続が完了しなかった場合は、@p detail_out の要因が
     *  @ref COM_UTIL_CAUSE_IN_PROGRESS になります。\n
     *  この場合は @ref com_util_socket_wait_writable で完了を待ち、
     *  @ref com_util_socket_get_pending_error で結果を確認します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_connect(com_util_socket sock,
                                                             const com_util_ipv4_endpoint *endpoint,
                                                             com_util_error *detail_out);

    /**
     *  @brief          ソケットに保留されているエラーを取得します。
     *  @param[in]      sock       対象のソケット。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         保留エラーがない場合は @ref COM_UTIL_OK 、
     *                  保留エラーがある場合はそれに対応する @ref COM_UTIL_ERR_UNKNOWN 等を返します。
     *
     *  非ブロッキングの接続が完了したかどうかの判定に使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_get_pending_error(com_util_socket sock,
                                                                       com_util_error *detail_out);

    /**
     *  @brief          ソケットの非ブロッキング モードを設定します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      enable     非ブロッキングにする場合は 0 以外、ブロッキングにする場合は 0。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_set_nonblocking(com_util_socket sock, int enable,
                                                                     com_util_error *detail_out);

    /**
     *  @brief          アドレスの再利用を許可するかどうかを設定します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      enable     許可する場合は 0 以外、許可しない場合は 0。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_set_reuse_address(com_util_socket sock, int enable,
                                                                       com_util_error *detail_out);

    /**
     *  @brief          ブロードキャスト送信を許可するかどうかを設定します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      enable     許可する場合は 0 以外、許可しない場合は 0。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_set_broadcast(com_util_socket sock, int enable,
                                                                   com_util_error *detail_out);

    /**
     *  @brief          マルチキャスト送信に使用するローカル インターフェースを設定します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      interface_address ローカル インターフェースのアドレス
     *                             (ネットワーク バイト オーダー)。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_set_multicast_interface(com_util_socket sock,
                                                                             uint32_t interface_address,
                                                                             com_util_error *detail_out);

    /**
     *  @brief          マルチキャスト グループへ参加します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      group_address マルチキャスト グループのアドレス
     *                             (ネットワーク バイト オーダー)。
     *  @param[in]      interface_address 受信に使用するローカル インターフェースのアドレス
     *                             (ネットワーク バイト オーダー)。
     *                             @ref COM_UTIL_IPV4_ADDR_ANY を指定した場合は OS が選択します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_join_multicast_group(com_util_socket sock, uint32_t group_address,
                                                                          uint32_t interface_address,
                                                                          com_util_error *detail_out);

    /**
     *  @brief          接続済みソケットへ送信します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      buf        送信するデータ。
     *  @param[in]      len        @p buf のバイト数。@ref COM_UTIL_SOCKET_MAX_TRANSFER 以下を指定します。
     *  @param[out]     sent_out   送信できたバイト数の格納先。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  1 回の呼び出しで @p len 全体を送信するとは限りません。\n
     *  全体の送信を保証する場合は @ref com_util_socket_send_all を使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一のソケットへ複数のスレッドから同時に送信しないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_send(com_util_socket sock, const void *buf, size_t len,
                                                          size_t *sent_out, com_util_error *detail_out);

    /**
     *  @brief          接続済みソケットから受信します。
     *  @param[in]      sock       対象のソケット。
     *  @param[out]     buf        受信データの格納先。
     *  @param[in]      len        @p buf のバイト数。@ref COM_UTIL_SOCKET_MAX_TRANSFER 以下を指定します。
     *  @param[out]     received_out 受信したバイト数の格納先。0 は相手が送信を終了したことを表します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一のソケットから複数のスレッドで同時に受信しないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_recv(com_util_socket sock, void *buf, size_t len,
                                                          size_t *received_out, com_util_error *detail_out);

    /**
     *  @brief          指定した端点へ送信します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      buf        送信するデータ。
     *  @param[in]      len        @p buf のバイト数。@ref COM_UTIL_SOCKET_MAX_TRANSFER 以下を指定します。
     *  @param[in]      endpoint   送信先の端点。
     *  @param[out]     sent_out   送信できたバイト数の格納先。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_sendto(com_util_socket sock, const void *buf, size_t len,
                                                            const com_util_ipv4_endpoint *endpoint, size_t *sent_out,
                                                            com_util_error *detail_out);

    /**
     *  @brief          任意の端点から受信します。
     *  @param[in]      sock       対象のソケット。
     *  @param[out]     buf        受信データの格納先。
     *  @param[in]      len        @p buf のバイト数。@ref COM_UTIL_SOCKET_MAX_TRANSFER 以下を指定します。
     *  @param[out]     peer_out   送信元の端点の格納先。NULL 可。
     *  @param[out]     received_out 受信したバイト数の格納先。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_recvfrom(com_util_socket sock, void *buf, size_t len,
                                                              com_util_ipv4_endpoint *peer_out, size_t *received_out,
                                                              com_util_error *detail_out);

    /**
     *  @brief          指定したバイト数をすべて送信します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      buf        送信するデータ。
     *  @param[in]      len        @p buf のバイト数。@ref COM_UTIL_SOCKET_MAX_TRANSFER 以下を指定します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @p len 全体を送信するまで繰り返します。途中で送信できなくなった場合は
     *  @ref COM_UTIL_ERR_UNKNOWN を返します。\n
     *  ブロッキング モードのソケットで使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一のソケットへ複数のスレッドから同時に送信しないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_send_all(com_util_socket sock, const void *buf, size_t len,
                                                              com_util_error *detail_out);

    /**
     *  @brief          指定したバイト数をすべて受信します。
     *  @param[in]      sock       対象のソケット。
     *  @param[out]     buf        受信データの格納先。
     *  @param[in]      len        受信するバイト数。@ref COM_UTIL_SOCKET_MAX_TRANSFER 以下を指定します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_EOF 、@ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @p len 全体を受信するまで繰り返します。\n
     *  受信し終える前に相手が送信を終了した場合は @ref COM_UTIL_ERR_EOF を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一のソケットから複数のスレッドで同時に受信しないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_recv_all(com_util_socket sock, void *buf, size_t len,
                                                              com_util_error *detail_out);

    /**
     *  @brief          ソケットが受信可能になるまで待機します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      timeout_ms タイムアウト時間 (ミリ秒)。
     *                             @ref COM_UTIL_SOCKET_WAIT_FOREVER または
     *                             @ref COM_UTIL_SOCKET_NO_WAIT も指定できます。
     *  @param[out]     ready_out  受信可能な場合に 1、タイムアウトした場合に 0 を格納します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  待機がシグナルで中断された場合は、タイムアウトと同じく @p ready_out へ 0 を格納し、
     *  @ref COM_UTIL_OK を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_wait_readable(com_util_socket sock, int timeout_ms, int *ready_out,
                                                                   com_util_error *detail_out);

    /**
     *  @brief          ソケットが送信可能になるまで待機します。
     *  @param[in]      sock       対象のソケット。
     *  @param[in]      timeout_ms タイムアウト時間 (ミリ秒)。
     *                             @ref COM_UTIL_SOCKET_WAIT_FOREVER または
     *                             @ref COM_UTIL_SOCKET_NO_WAIT も指定できます。
     *  @param[out]     ready_out  送信可能な場合に 1、タイムアウトした場合に 0 を格納します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  待機がシグナルで中断された場合は、タイムアウトと同じく @p ready_out へ 0 を格納し、
     *  @ref COM_UTIL_OK を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_wait_writable(com_util_socket sock, int timeout_ms, int *ready_out,
                                                                   com_util_error *detail_out);

    /**
     *  @brief          複数のソケットのいずれかが受信可能になるまで待機します。
     *  @param[in]      socks      対象のソケットの配列。@ref COM_UTIL_INVALID_SOCKET を含めても構いません。
     *  @param[in]      count      @p socks の要素数。1 以上 @ref COM_UTIL_SOCKET_WAIT_MAX 以下を指定します。
     *  @param[in]      timeout_ms タイムアウト時間 (ミリ秒)。
     *                             @ref COM_UTIL_SOCKET_WAIT_FOREVER または
     *                             @ref COM_UTIL_SOCKET_NO_WAIT も指定できます。
     *  @param[out]     ready_out  各ソケットが受信可能かどうかを格納する配列。
     *                             @p count 以上の要素数が必要です。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @p socks の全要素が @ref COM_UTIL_INVALID_SOCKET の場合、@p timeout_ms だけ待機してから
     *  @ref COM_UTIL_OK を返します。呼び出し側のポーリング ループが待機なしで回り続けることを
     *  避けるためです。\n
     *  待機がシグナルで中断された場合も、@p ready_out をすべて 0 にして @ref COM_UTIL_OK を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_wait_readable_multi(const com_util_socket *socks, size_t count,
                                                                         int timeout_ms, unsigned char *ready_out,
                                                                         com_util_error *detail_out);

    /**
     *  @brief          ソケットの受信方向を停止します。
     *  @param[in,out]  sock_inout 対象のソケット。Windows ではソケットを閉じ、
     *                             @ref COM_UTIL_INVALID_SOCKET を書き戻します。
     *  @param[out]     detail_out エラー詳細の格納先。NULL 可。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  受信で待機しているスレッドを解除する目的で使用します。
     *
     *  @warning        本関数はプラットフォームで意味論が異なります。\n
     *                  Linux は受信方向のみを停止してソケットを保持しますが、
     *                  Windows は受信の半クローズで待機を解除できないためソケットを閉じます。\n
     *                  呼び出し側がこの差を意識しなくて済むように、呼び出し後は
     *                  @p sock_inout の値を必ず参照し、@ref COM_UTIL_INVALID_SOCKET
     *                  でなくなっているかどうかで以後の利用可否を判断してください。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一のソケットに対して複数のスレッドから同時に呼び出さないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_socket_shutdown_receive(com_util_socket *sock_inout,
                                                                      com_util_error *detail_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_NET_SOCKET_H */
