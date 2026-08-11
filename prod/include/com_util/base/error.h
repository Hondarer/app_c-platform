/**
 *******************************************************************************
 *  @file           error.h
 *  @brief          OS 由来の詳細エラーをドメイン付きで保持する API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/31
 *  @version        1.0.0
 *
 *  errno と Win32 エラー コードを取り違えずに保持し、直前の com_util API
 *  が記録した詳細エラーの取得と、プラットフォーム共通の要因判定を提供します。
 *
 *  @section        error_last_contract 直前値の契約
 *
 *  `detail_out` を引数に持つ API は、失敗時に出力引数と現在のスレッドの直前値へ
 *  同じ詳細を記録し、成功時に両方をクリアします。\n
 *  出力引数へ NULL を指定した場合も、スレッドの直前値は更新されます。\n
 *  したがって com_util_error_get_last() は、直前に呼び出した対応 API の結果を
 *  成功と失敗のどちらであっても反映します。
 *
 *  直前値は次の対応 API の呼び出しで上書きされるため、以下は保証しません。
 *
 *  - スレッドをまたいだ参照 (直前値はスレッドごとに独立しています)
 *  - 対応 API 以外の関数を挟んだ後の参照
 *
 *  詳細を確実に保持する必要がある場合は `detail_out` を使用するか、
 *  com_util_error_get_last() の値を直ちにコピーしてください。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_BASE_ERROR_H
#define COM_UTIL_BASE_ERROR_H

#include <com_util/base/platform.h>
#include <com_util/base/result.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_BASE
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          詳細エラー値の由来を表します。
     */
    typedef enum com_util_error_domain
    {
        COM_UTIL_ERROR_DOMAIN_NONE = 0,         /**< 詳細エラーが設定されていません。 */
        COM_UTIL_ERROR_DOMAIN_ERRNO = 1,        /**< errno の値を保持しています。 */
        COM_UTIL_ERROR_DOMAIN_WINDOWS = 2,      /**< Win32 エラー コードを保持しています。 */
        COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO = 3, /**< ソケット操作が設定した errno の値を保持しています。 */
        COM_UTIL_ERROR_DOMAIN_WINSOCK = 4,      /**< Winsock エラー コード (WSAGetLastError) を保持しています。 */
        COM_UTIL_ERROR_DOMAIN_GAI = 5           /**< getaddrinfo のエラー コード (EAI_*) を保持しています。 */
    } com_util_error_domain;

    /**
     *  @brief          OS エラーをプラットフォーム共通で判定する要因を表します。
     *
     *  値は ABI として固定し、新しい要因は末尾へ追加します。
     */
    typedef enum com_util_error_cause
    {
        COM_UTIL_CAUSE_NONE = 0,
        COM_UTIL_CAUSE_OTHER = 1,
        COM_UTIL_CAUSE_NOT_FOUND = 2,
        COM_UTIL_CAUSE_ALREADY_EXISTS = 3,
        COM_UTIL_CAUSE_ACCESS_DENIED = 4,
        COM_UTIL_CAUSE_SHARING_VIOLATION = 5,
        COM_UTIL_CAUSE_NOT_A_DIRECTORY = 6,
        COM_UTIL_CAUSE_IS_A_DIRECTORY = 7,
        COM_UTIL_CAUSE_DIRECTORY_NOT_EMPTY = 8,
        COM_UTIL_CAUSE_NAME_TOO_LONG = 9,
        COM_UTIL_CAUSE_INVALID_ARGUMENT = 10,
        COM_UTIL_CAUSE_OUT_OF_MEMORY = 11,
        COM_UTIL_CAUSE_DISK_FULL = 12,
        COM_UTIL_CAUSE_BUSY = 13,
        COM_UTIL_CAUSE_TIMEOUT = 14,
        COM_UTIL_CAUSE_INTERRUPTED = 15,
        COM_UTIL_CAUSE_BROKEN_PIPE = 16,
        COM_UTIL_CAUSE_TOO_MANY_OPEN_FILES = 17,
        COM_UTIL_CAUSE_READ_ONLY = 18,
        COM_UTIL_CAUSE_BUFFER_TOO_SMALL = 19,
        COM_UTIL_CAUSE_UNSUPPORTED = 20,
        COM_UTIL_CAUSE_IO_ERROR = 21,
        COM_UTIL_CAUSE_WOULD_BLOCK = 22,
        COM_UTIL_CAUSE_IN_PROGRESS = 23,
        COM_UTIL_CAUSE_CONNECTION_REFUSED = 24,
        COM_UTIL_CAUSE_CONNECTION_RESET = 25,
        COM_UTIL_CAUSE_CONNECTION_ABORTED = 26,
        COM_UTIL_CAUSE_NOT_CONNECTED = 27,
        COM_UTIL_CAUSE_ALREADY_CONNECTED = 28,
        COM_UTIL_CAUSE_ADDRESS_IN_USE = 29,
        COM_UTIL_CAUSE_ADDRESS_NOT_AVAILABLE = 30,
        COM_UTIL_CAUSE_NETWORK_DOWN = 31,
        COM_UTIL_CAUSE_NETWORK_UNREACHABLE = 32,
        COM_UTIL_CAUSE_HOST_UNREACHABLE = 33,
        COM_UTIL_CAUSE_MESSAGE_SIZE = 34,
        COM_UTIL_CAUSE_SHUTDOWN = 35,
        COM_UTIL_CAUSE_NOT_INITIALIZED = 36
    } com_util_error_cause;

    /**
     *  @brief          OS 由来の詳細エラーをドメイン付きで保持します。
     */
    typedef struct com_util_error
    {
        com_util_error_domain domain; /**< code の由来。 */
        int result;                   /**< 対応する共通結果コード (COM_UTIL_OK または COM_UTIL_ERR_*)。 */
        unsigned long code;           /**< ドメイン固有の生のエラー値。 */
    } com_util_error;

    /**
     *  @brief          詳細エラーを空の値へ初期化します。
     *  @param[out]     error 初期化する値。NULL 可。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側が指定した値だけを書き換え、共有状態を持ちません。\n
     *  同一の @p error を複数のスレッドから同時に書き換えないことは呼び出し側の責務です。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_clear(com_util_error *error);

    /**
     *  @brief          errno の値を詳細エラーへ取り込みます。
     *  @param[out]     error       格納先。NULL 可。
     *  @param[in]      errno_value errno の値。0 の場合は空の値を格納します。
     *
     *  自前で呼び出した OS API の結果を、com_util と同じ枠組みで扱う場合に使用します。
     *
     *  @attention      Windows の `GetLastError()` の値を渡してはなりません。
     *                  Win32 の値には com_util_error_capture_windows_error() を使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側が指定した値だけを書き換え、スレッドの直前値は更新しません。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_capture_errno(com_util_error *error, int errno_value);

    /**
     *  @brief          現在の errno を詳細エラーへ取り込みます。
     *  @param[out]     error 格納先。NULL 可。
     *
     *  OS API の失敗を検出した直後に使用します。本関数は errno を読み取る前に、
     *  errno を変更する可能性がある処理を行いません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側が指定した値だけを書き換え、スレッドの直前値は更新しません。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_capture_current_errno(com_util_error *error);

#if defined(PLATFORM_WINDOWS)
    /**
     *  @brief          Win32 エラー コードを詳細エラーへ取り込みます。
     *  @param[out]     error      格納先。NULL 可。
     *  @param[in]      error_code GetLastError() が返した値。ERROR_SUCCESS の場合は空の値を格納します。
     *
     *  @attention      errno の値を渡してはなりません。
     *                  errno には com_util_error_capture_errno() を使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側が指定した値だけを書き換え、スレッドの直前値は更新しません。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_capture_windows_error(com_util_error *error,
                                                                           unsigned long error_code);

    /**
     *  @brief          現在の Win32 エラー コードを詳細エラーへ取り込みます。
     *  @param[out]     error 格納先。NULL 可。
     *
     *  Win32 API の失敗を検出した直後に使用します。本関数は GetLastError() を呼び出す前に、
     *  Win32 エラー コードを変更する可能性がある処理を行いません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出し側が指定した値だけを書き換え、スレッドの直前値は更新しません。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_capture_current_windows_error(com_util_error *error);
#endif

    /**
     *  @brief          現在のスレッドで直前に記録された詳細エラーを取得します。
     *  @param[out]     error_out 格納先。NULL 可。
     *
     *  返された値は呼び出し側の記憶域へコピーされ、後続 API の呼び出し後も保持できます。\n
     *  記録の契約と保証しない範囲は @ref error_last_contract を参照してください。\n
     *  直前値が記録されていないスレッドでは、空の値 (@ref COM_UTIL_ERROR_DOMAIN_NONE) を格納します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  直前値はスレッド ローカルであり、他スレッドの記録と干渉しません。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_get_last(com_util_error *error_out);

    /**
     *  @brief          現在のスレッドの直前値を、保存済みの詳細エラーで更新します。
     *  @param[in]      error 設定する値。NULL の場合は直前値をクリアします。
     *
     *  OS API の失敗後に保存した詳細を、後処理で直前値が変化した後に復元する場合に使用します。
     *  指定した値は現在のスレッドの記憶域へコピーされ、呼び出し元の値は変更しません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  更新する対象は呼び出したスレッドの直前値だけです。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_set_last(const com_util_error *error);

    /**
     *  @brief          現在のスレッドに記録された詳細エラーをクリアします。
     *
     *  対応 API は成功時に直前値をクリアするため、通常は呼び出す必要がありません。\n
     *  対応 API 以外の処理を挟む前に、古い値が残らないようにする場合に使用します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  クリアする対象は呼び出したスレッドの直前値だけです。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_error_clear_last(void);

    /**
     *  @brief          詳細エラーが設定されているかを返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @return         設定されている場合は 1、それ以外は 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_error_is_set(const com_util_error *error);

    /**
     *  @brief          詳細エラーのドメインを返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @return         ドメインを返します。NULL または不正な値の場合は COM_UTIL_ERROR_DOMAIN_NONE を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT com_util_error_domain COM_UTIL_API com_util_error_get_domain(const com_util_error *error);

    /**
     *  @brief          errno の値を返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @return         errno ドメインの場合は保持値、それ以外は 0 を返します。
     *
     *  Win32 ドメインの値を errno として取り出すことはありません。\n
     *  ドメインが一致しない場合に 0 を返すことで、体系の取り違えを防ぎます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_error_get_errno(const com_util_error *error);

#if defined(PLATFORM_WINDOWS)
    /**
     *  @brief          Win32 エラー コードを返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @return         Win32 ドメインの場合は保持値、それ以外は ERROR_SUCCESS を返します。
     *
     *  errno ドメインの値を Win32 エラー コードとして取り出すことはありません。\n
     *  ドメインが一致しない場合に ERROR_SUCCESS を返すことで、体系の取り違えを防ぎます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT unsigned long COM_UTIL_API com_util_error_get_windows_error(const com_util_error *error);
#endif

    /**
     *  @brief          詳細エラーに対応する共通結果コードを返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @return         対応する共通結果コードを返します。空の値の場合は @ref COM_UTIL_OK 、
     *                  NULL または不正なドメインの場合は @ref COM_UTIL_ERR_INVALID_ARGUMENT を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_error_to_result(const com_util_error *error);

    /**
     *  @brief          詳細エラーに対応するプラットフォーム共通の要因を返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @return         対応する要因を返します。未設定または NULL の場合は COM_UTIL_CAUSE_NONE、
     *                  対応がない場合は COM_UTIL_CAUSE_OTHER を返します。
     *
     *  1 つのエラー値に対応する要因は高々 1 つです。\n
     *  分岐が複数に及ぶ場合は、本関数の戻り値に対する `switch` で記述できます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT com_util_error_cause COM_UTIL_API com_util_error_get_cause(const com_util_error *error);

    /**
     *  @brief          詳細エラーが指定した要因に一致するかを返します。
     *  @param[in]      error 確認する値。NULL 可。
     *  @param[in]      cause 判定する要因。
     *  @return         一致する場合は 1、それ以外は 0 を返します。
     *
     *  単発の判定に使用します。com_util_error_get_cause() の結果との比較と同じ意味です。
     *
     *  @attention      @p error が NULL の場合は、@p cause の値にかかわらず常に 0 を返します。
     *                  com_util_error_get_cause() は NULL に対して @ref COM_UTIL_CAUSE_NONE を
     *                  返すため、`com_util_error_is(NULL, COM_UTIL_CAUSE_NONE)` は
     *                  両者を組み合わせた判定とは結果が異なります。
     *                  範囲外の @p cause を指定した場合も 0 を返します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  引数の値だけを参照し、共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_error_is(const com_util_error *error, com_util_error_cause cause);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_BASE_ERROR_H */
