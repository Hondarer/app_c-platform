/**
 *******************************************************************************
 *  @file           mmap.h
 *  @brief          メモリ マップド ファイルを抽象化する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/26
 *  @version        1.0.0
 *
 *  Linux の `mmap` と Windows の `CreateFileMapping` + `MapViewOfFile` を
 *  共通のインターフェースで扱い、ファイルをプロセスのアドレス空間へマップした
 *  アドレスを公開します。\n
 *  同一ファイルへのプロセス横断リーダーライター ロックは、
 *  @ref com_util_mmap_get_rwlock() の初回呼び出し時に自動で用意され、
 *  @ref com_util_interprocess_rwlock_lock_shared() 等 (`sync.h`) を
 *  そのまま使用します。\n
 *  ロックを参照しない限り生成しないため、排他が不要な用途では
 *  ロックの生成コストを負担しません。
 *
 *  @warning        本 API はローカル ファイル システム上のファイルを対象とする設計です。\n
 *                  NFS などのネットワーク ファイル システムでは、以下の理由により
 *                  プロセス間 (特にホストをまたぐ場合) の一貫性が保証されません。
 *                  - アドバイザリ ロック (`flock` / `LockFileEx` 相当) がネットワーク ファイル
 *                    システム上では信頼できないことがある。
 *                  - @ref com_util_mmap_flush() が反映するのはサーバーへの書き込みまでであり、
 *                    他クライアントのキャッシュを無効化するものではない。
 *                  - 他クライアントが既にマップ済みのページは、ロックの取得/解放だけでは
 *                    最新化されない (close-to-open セマンティクスに依存する)。\n
 *                  ネットワーク ファイル システム越しの共有が必要な場合は、本 API ではなく
 *                  別方式を検討してください。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_MMAP_MMAP_H
#define COM_UTIL_MMAP_MMAP_H

#include <stddef.h>

#include <com_util/base/result.h>
#include <com_util/com_util_export.h>
#include <com_util/sync/sync.h>

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /** @brief マップ時のアクセス モード。 */
    typedef enum
    {
        COM_UTIL_MMAP_ACCESS_READ_ONLY = 0, /**< 読み取り専用でマップする。既存ファイルが必要。 */
        COM_UTIL_MMAP_ACCESS_READ_WRITE = 1 /**< 読み書き両用でマップする。存在しなければ新規作成する。 */
    } com_util_mmap_access_t;

    /** @brief メモリ マップド ファイルのハンドル (不透明構造体)。 */
    typedef struct com_util_mmap com_util_mmap;

    /**
     *  @brief          ファイルをアタッチし、プロセスのアドレス空間へマップします。
     *  @param[in]      path         マップするファイルのパス (UTF-8)。NULL を渡してはなりません。
     *  @param[in]      access       アクセス モード (@ref com_util_mmap_access_t)。
     *  @param[in]      create_size  新規作成時のファイル サイズ (バイト)。\n
     *                               @p access が @ref COM_UTIL_MMAP_ACCESS_READ_WRITE で、
     *                               @p path のファイルが存在しない場合にのみ使用します。
     *                               0 を渡した場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。\n
     *                               既存ファイルを開く場合は無視され、現在のファイル サイズが
     *                               マップ サイズになります。
     *  @param[out]     map          生成したハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNKNOWN のいずれかを返します。
     *
     *  @p access が @ref COM_UTIL_MMAP_ACCESS_READ_ONLY のとき、@p path が存在しない場合は
     *  新規作成せず @ref COM_UTIL_ERR_UNKNOWN を返します。\n
     *  マップ結果のサイズが 0 になる場合 (空の既存ファイルを開いた場合など) は、
     *  `mmap`/`MapViewOfFile` がサイズ 0 を扱えないため @ref COM_UTIL_ERR_UNKNOWN を返します。\n
     *  成功すると、同一 @p path がプロセス横断リーダーライター ロックの識別子として
     *  ハンドル内に保持されます。\n
     *  ロック自体は本関数では開かず、@ref com_util_mmap_get_rwlock() の
     *  初回呼び出し時に開きます。
     *
     *  @warning        @p path はローカル ファイル システム上のファイルを指定してください。
     *                  ネットワーク ファイル システム上のファイルを指定した場合、
     *                  プロセス間の一貫性は保証されません (本ファイル冒頭の @warning を参照)。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出しごとに独立したハンドルを生成し、内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_mmap_attach(const char *path, com_util_mmap_access_t access,
                                                          size_t create_size, com_util_mmap **map);

    /**
     *  @brief          マップ済みアドレスを取得します。
     *  @param[in]      map  対象のハンドル。NULL を渡してはなりません。
     *  @return         マップ済みアドレス。@p map が NULL の場合は NULL を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT void *COM_UTIL_API com_util_mmap_get_address(const com_util_mmap *map);

    /**
     *  @brief          マップ サイズを取得します。
     *  @param[in]      map  対象のハンドル。NULL を渡してはなりません。
     *  @return         マップ サイズ (バイト)。@p map が NULL の場合は 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT size_t COM_UTIL_API com_util_mmap_get_size(const com_util_mmap *map);

    /**
     *  @brief          ハンドルに内包されたプロセス横断リーダーライター ロックを取得します。
     *  @param[in]      map  対象のハンドル。NULL を渡してはなりません。
     *  @return         ロックへの非所有参照。@p map が NULL の場合、および
     *                  ロックのオープンに失敗した場合は NULL を返します。
     *
     *  ロックは本関数の初回呼び出し時に、@ref com_util_mmap_attach() へ渡したパスを
     *  識別子として開きます。\n
     *  2 回目以降の呼び出しでは、生成済みのロックをそのまま返します。\n
     *  戻り値は @p map の寿命に紐づく非所有参照です。\n
     *  `sync.h` の `com_util_interprocess_rwlock_lock_shared()` /
     *  `com_util_interprocess_rwlock_lock_exclusive()` / `com_util_interprocess_rwlock_unlock()`
     *  をそのまま使用してください。\n
     *  呼び出し側が `com_util_interprocess_rwlock_destroy()` を呼び出してはなりません
     *  (@ref com_util_mmap_detach() が内部で破棄します)。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のプロセス内ミューテックスにより、初回のオープンを直列化します。
     *
     *  @note           初回の呼び出しのみロックのオープンに伴うコストが発生します。\n
     *                  排他が不要な用途では本関数を呼び出さないことで、
     *                  このコストを回避できます。
     *
     *  @note           初回呼び出しでロックを内部にキャッシュしますが、
     *                  マップ済みアドレスやマップ サイズなど呼び出し側から見える状態は
     *                  変化しないため、@p map は const です。
     */
    COM_UTIL_EXPORT com_util_interprocess_rwlock *COM_UTIL_API com_util_mmap_get_rwlock(const com_util_mmap *map);

    /**
     *  @brief          マップした内容をディスクへ反映します。
     *  @param[in]      map      対象のハンドル。NULL を渡してはなりません。
     *  @param[in]      address  反映対象の先頭アドレス。NULL の場合はマップ全体を対象にします。
     *  @param[in]      length   反映対象のサイズ (バイト)。@p address が NULL の場合は無視されます。
     *  @return         @ref COM_UTIL_OK または @ref COM_UTIL_ERR_UNKNOWN を返します。
     *
     *  Linux では `msync(MS_SYNC)`、Windows では `FlushViewOfFile` に続けて
     *  `FlushFileBuffers` を呼び出します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  同一 @p map に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_mmap_flush(com_util_mmap *map, void *address, size_t length);

    /**
     *  @brief          マッピングを解除し、ハンドルを破棄します。
     *  @param[in]      map  破棄するハンドル。NULL の場合は何もしません。
     *
     *  内包するロックの破棄 → アンマップ → ファイル クローズの順で行います。\n
     *  @ref com_util_mmap_get_rwlock() を一度も呼び出していない場合、
     *  ロックは生成されていないため破棄も行いません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象の @p map を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_mmap_detach(com_util_mmap *map);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_MMAP_MMAP_H */
