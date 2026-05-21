#ifndef COM_UTIL_SYSLOG_H
#define COM_UTIL_SYSLOG_H

#include <com_util/base/platform.h>
#include <com_util/clock/clock.h>
#include <com_util/com_util_export.h>

/**
 *  @file           syslog.h
 *  @brief          syslog ヘルパーライブラリ。
 *
 *  Linux syslog (RFC5424 系実装) のラッパー関数群を提供します。\n
 *  Linux 専用ライブラリです。呼び出し元は @c \#if defined(PLATFORM_LINUX) の
 *  中でのみ使用してください。
 */

#if defined(PLATFORM_LINUX)

/* ===== 不透明ハンドル型 ===== */

/** syslog プロバイダハンドル (不透明型)。 */
typedef struct com_util_syslog_sink com_util_syslog_sink_t;

    /* ===== API 関数 ===== */

    #ifdef __cplusplus
extern "C"
{
    #endif /* __cplusplus */

    /**
     *  @brief          syslog プロバイダを初期化する。
     *
     *  @param[in]      ident     syslog メッセージに付与される識別子文字列。
     *  @param[in]      facility  syslog facility 値 (例: LOG_USER, LOG_LOCAL0〜LOG_LOCAL7)。
     *  @return         成功時: ハンドル。失敗時: NULL。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_syslog_sink_t *COM_UTIL_API com_util_syslog_sink_create(const char *ident, int facility);

    /**
     *  @brief          syslog へ UTF-8 メッセージを書き込む。
     *
     *  @param[in]      handle   com_util_syslog_sink_create の戻り値。NULL は無視。
     *  @param[in]      level    syslog severity 値。
     *  @param[in]      timestamp  デバッグ用 FD 出力に付与する実時刻。NULL の場合は時刻を付与しない。
     *                             不正な明示タイムスタンプが渡された場合は現在時刻へ代替し、
     *                             出力は継続しつつ戻り値は -1 を返します。
     *  @param[in]      message  null 終端 UTF-8 文字列。NULL は無視。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の reconnect_lock で保護されており、同一 @p handle に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_syslog_sink_write(com_util_syslog_sink_t *handle, int level,
                                                                const com_util_realtime_timestamp_t *timestamp,
                                                                const char *message);

    /**
     *  @brief          syslog プロバイダの識別子を変更する。
     *
     *  @param[in]      handle     com_util_syslog_sink_create の戻り値。NULL は -1 を返す。
     *  @param[in]      new_ident  新しい識別子文字列。NULL は -1 を返す。
     *  @return         成功 0 / 失敗 -1。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部の reconnect_lock で保護されており、同一 @p handle に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_syslog_sink_rename(com_util_syslog_sink_t *handle, const char *new_ident);

    /**
     *  @brief          syslog プロバイダを終了する。
     *
     *  @param[in]      handle   com_util_syslog_sink_create の戻り値。NULL は無視。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p handle を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_syslog_sink_dispose(com_util_syslog_sink_t *handle);

    #ifdef __cplusplus
}
    #endif /* __cplusplus */

#endif /* PLATFORM_LINUX */

#endif /* COM_UTIL_SYSLOG_H */
