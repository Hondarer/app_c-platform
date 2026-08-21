/**
 *******************************************************************************
 *  @file           hashtable.h
 *  @brief          固定長スロットと遅延削除を持つハッシュ テーブル API を提供します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/18
 *  @version        1.0.0
 *
 *  キーと値を固定長バイト列として保持するハッシュ テーブルです。\n
 *  衝突はチェイン法で扱い、削除は寿命付きの加齢により空きへ戻します。\n
 *  lifetime が @ref COM_UTIL_HASHTABLE_LIFETIME_INFINITE のとき、
 *  削除済みは加齢の末に 255 で止まり、push では空へ戻りません。\n
 *  呼び出し側が用意した領域へ構築する、または構築済み領域を再接続できます。
 *
 *  レコード番号は 1 相対で、内部スロットと 1 対 1 です。\n
 *  1 から capacity までを `get_status` / `get_key_*` / `get_value_*` /
 *  `get_timestamp_*` で走査できます。\n
 *  テーブルは常に横断の変更時刻を持ち、最後にキーまたは値が変わった実時刻です。\n
 *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD のときは、各レコードも
 *  実時刻 (`com_util_timespec`) の変更時刻を持ち、status が 0 以外のときだけ有効です。
 *
 *  テーブルは、識別子・ヘッダー・バケット配列・エントリ配列からなる「管理領域」と、
 *  値配列からなる「データ領域」の 2 領域で構成されます。\n
 *  呼び出し側が両方省略すれば内部で 1 回の確保にまとめて構築し、解放も
 *  @ref com_util_hashtable_dispose の 1 回で両方が片付きます。\n
 *  両方を明示的に指定すれば、独立した 2 領域(データ領域はメモリマップド
 *  ファイルなど管理領域と連続しない領域を想定)へ構築・再接続できます。\n
 *  片方だけを指定することはできません。
 *
 *  永続化の入出力とエンディアン変換は本 API の対象外です。\n
 *  読み戻しは同一環境 (同一ビット幅・同一アラインメント規則) を前提とします。\n
 *  データ領域先頭アドレスは実行時のみ有効な値であり、管理領域を永続化しても
 *  意味を持ちません。@ref com_util_hashtable_attach は、呼び出し側が渡した
 *  データ領域アドレスで必ず上書きします。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_HASHTABLE_HASHTABLE_H
#define COM_UTIL_HASHTABLE_HASHTABLE_H

#include <stddef.h>
#include <stdint.h>

#include <com_util/base/result.h>
#include <com_util/clock/timespec.h>
#include <com_util/com_util_export.h>

/**
 *  @ingroup        COM_UTIL_HASHTABLE
 *  @{
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          キーの解釈方法です。
     */
    typedef enum com_util_hashtable_key_type
    {
        COM_UTIL_HASHTABLE_KEY_STRING = 0, /**< NULL 終端文字列。key_size バイト以内に NUL が必要。 */
        COM_UTIL_HASHTABLE_KEY_BINARY = 1  /**< key_size バイトのバイナリ。全バイト 0 も有効。 */
    } com_util_hashtable_key_type;

#define COM_UTIL_HASHTABLE_LIFETIME_INFINITE 255 /**< 削除済みを空へ戻さない寿命です。 */

    /**
     *  @brief          変更時刻をどの粒度で持つかです。
     *
     *  ゼロ値はテーブル横断のみです。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD はテーブル横断に加え、
     *  レコードごとの変更時刻も持ちます。
     */
    typedef enum com_util_hashtable_timestamp_scope
    {
        COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE = 0, /**< テーブル横断の変更時刻だけを持つ。 */
        COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD = 1 /**< テーブル横断に加え、レコードごとの変更時刻も持つ。 */
    } com_util_hashtable_timestamp_scope;

    /**
     *  @brief          ハッシュ テーブルの生成設定です。
     *
     *  暗黙パディングは pad2 として明示し、意味を持ちません。\n
     *  呼び出し側はゼロ初期化してから必要なフィールドだけを設定してください。\n
     *  @p timestamp_scope はエントリ配置と管理領域サイズを決めます。\n
     *  ゼロ値は @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE です。
     */
    typedef struct com_util_hashtable_config
    {
        size_t capacity;                                    /**< バケット数兼エントリ数。0 は不正。 */
        com_util_hashtable_key_type key_type;               /**< キーの解釈。 */
        com_util_hashtable_timestamp_scope timestamp_scope; /**< 変更時刻の粒度。レイアウト入力。 */
        size_t key_size;                                    /**< 1 キーのバイト数。0 は不正。 */
        size_t record_size;                                 /**< 1 値のバイト数。0 は不正。 */
        unsigned char lifetime;                             /**< 削除済みの寿命。2 から 254 は有限、255 は無限。 */
        unsigned char pad2[7];                              /**< lifetime から末尾までの予約。常に 0。 */
    } com_util_hashtable_config;

    /**
     *  @brief          ハッシュ テーブルの不透明ハンドルです。
     *
     *  メンバーへ直接アクセスしてはなりません。
     */
    typedef struct com_util_hashtable com_util_hashtable;

    /**
     *  @brief          外部バッファーで構築する場合に必要な最小バイト数を求めます。
     *  @param[in]      config         設定。NULL を渡してはなりません。
     *  @param[out]     mgmt_size_out  管理領域の必要バイト数の格納先。不要なら NULL を渡せます。
     *  @param[out]     data_size_out  データ領域の必要バイト数の格納先。不要なら NULL を渡せます。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @p mgmt_size_out と @p data_size_out の両方に NULL を渡してはなりません。\n
     *  @ref com_util_hashtable_create と同じ基準で @p config を検証します。\n
     *  @p timestamp_scope は管理領域サイズに使います。\n
     *  @p key_type と @p lifetime はレイアウトには使いませんが、検証は行います。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_required_size(const com_util_hashtable_config *config,
                                                                      size_t *mgmt_size_out, size_t *data_size_out);

    /**
     *  @brief          ハッシュ テーブルを構築します。
     *  @param[in]      config         設定。NULL を渡してはなりません。
     *  @param[in]      buf_mgmt       管理領域。NULL のときは内部確保します。
     *  @param[in]      buf_mgmt_size  @p buf_mgmt のバイト数。@p buf_mgmt が NULL のときは無視します。
     *  @param[in]      buf_data       データ領域。NULL のときは内部確保します。
     *  @param[in]      buf_data_size  @p buf_data のバイト数。@p buf_data が NULL のときは無視します。
     *  @param[out]     ht_out         ハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_BUFFER_TOO_SMALL 、@ref COM_UTIL_ERR_OUT_OF_MEMORY 。
     *
     *  失敗時は *@p ht_out を NULL にします。\n
     *  @p buf_mgmt と @p buf_data は、ともに NULL かともに非 NULL である必要があります。\n
     *  片方だけ NULL は @ref COM_UTIL_ERR_INVALID_ARGUMENT です。\n
     *  ともに NULL のときは、内部で 1 回の確保にまとめて構築し、
     *  解放も @ref com_util_hashtable_dispose の 1 回で両方が片付きます。\n
     *  ともに非 NULL のとき、不足やアラインメント不正なら領域へ触れずに失敗します。\n
     *  @p timestamp_scope は @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE または
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD である必要があります。\n
     *  @p buf_mgmt は struct のアラインメント境界が必要ですが、
     *  @p buf_data にアラインメント要件はありません
     *  (メモリマップド ファイルなど管理領域と連続しない領域を渡せます)。\n
     *  外部領域の所有権は呼び出し側に残ります。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  呼び出しごとに独立したテーブルを構築します。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_create(const com_util_hashtable_config *config, void *buf_mgmt,
                                                               size_t buf_mgmt_size, void *buf_data,
                                                               size_t buf_data_size, com_util_hashtable **ht_out);

    /**
     *  @brief          構築済み領域へ、既存内容を保ったまま再接続します。
     *  @param[in,out]  buf_mgmt       初期化済み管理領域。NULL を渡してはなりません。
     *  @param[in]      buf_mgmt_size  @p buf_mgmt のバイト数。
     *  @param[in]      buf_data       データ領域。NULL を渡してはなりません。
     *  @param[in]      buf_data_size  @p buf_data のバイト数。
     *  @param[out]     ht_out         ハンドルの格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_BUFFER_TOO_SMALL 。
     *
     *  マジックと版番号、ヘッダー範囲、保存されている timestamp_scope を検証します。\n
     *  チェインの整合性は検証しないため、必要なら直後に
     *  @ref com_util_hashtable_validate を呼んでください。\n
     *  @p buf_mgmt に残っているデータ領域アドレスは信用せず、常に @p buf_data で
     *  上書きします (プロセスをまたいだ再接続や再マップに対応するためです)。\n
     *  成功時は所有権を呼び出し側のままにします。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一 @p buf_mgmt を他スレッドが使っていないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_attach(void *buf_mgmt, size_t buf_mgmt_size, void *buf_data,
                                                               size_t buf_data_size, com_util_hashtable **ht_out);

    /**
     *  @brief          内部整合性を検証します。
     *  @param[in]      ht  対象。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_MEMORY 、@ref COM_UTIL_ERR_CORRUPT_DESCRIPTOR 。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_validate(const com_util_hashtable *ht);

    /**
     *  @brief          設定への参照を返します。
     *  @param[in]      ht          対象。NULL を渡してはなりません。
     *  @param[out]     config_out  参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  返るポインターは @p ht の寿命に依存します。書き換えたり解放してはなりません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_config_ref(const com_util_hashtable *ht,
                                                                       const com_util_hashtable_config **config_out);

    /**
     *  @brief          設定を呼び出し側へ複製します。
     *  @param[in]      ht          対象。NULL を渡してはなりません。
     *  @param[out]     config_out  複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_config_val(const com_util_hashtable *ht,
                                                                       com_util_hashtable_config *config_out);

    /**
     *  @brief          テーブルが占める管理領域とデータ領域のバイト数を返します。
     *  @param[in]      ht             対象。NULL を渡してはなりません。
     *  @param[out]     mgmt_size_out  管理領域のバイト数の格納先。不要なら NULL を渡せます。
     *  @param[out]     data_size_out  データ領域のバイト数の格納先。不要なら NULL を渡せます。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @p mgmt_size_out と @p data_size_out の両方に NULL を渡してはなりません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_buffer_size(const com_util_hashtable *ht, size_t *mgmt_size_out,
                                                                    size_t *data_size_out);

    /**
     *  @brief          テーブルが現在管理している管理領域とデータ領域の先頭を返します。
     *  @param[in]      ht        対象。NULL を渡してはなりません。
     *  @param[out]     mgmt_out  管理領域の先頭の格納先。不要なら NULL を渡せます。
     *  @param[out]     data_out  データ領域の先頭の格納先。不要なら NULL を渡せます。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @p mgmt_out と @p data_out の両方に NULL を渡してはなりません。\n
     *  内部確保と外部指定のどちらで構築したかに関わらず、現在管理している領域を返します。\n
     *  各領域のバイト数は @ref com_util_hashtable_buffer_size で求めてください。\n
     *  返るポインターは @p ht の寿命に依存します。書き換えたり解放してはなりません。
     *
     *  内部確保したテーブルを永続化する用途を想定しています。\n
     *  管理領域とデータ領域は連続とは限らないため、それぞれの先頭とバイト数で個別に
     *  書き出してください。\n
     *  読み戻すときは、呼び出し側が用意した 2 領域へ読み込み、
     *  @ref com_util_hashtable_attach で再接続します。\n
     *  管理領域が持つデータ領域の先頭アドレスと所有フラグは
     *  @ref com_util_hashtable_attach が上書きするため、永続化しても差し支えありません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_buffer_ref(const com_util_hashtable *ht, const void **mgmt_out,
                                                                   const void **data_out);

    /**
     *  @brief          キーを新規追加します。
     *  @param[in,out]  ht     対象。NULL を渡してはなりません。
     *  @param[in]      key    キー。NULL を渡してはなりません。
     *  @param[in]      value  record_size バイト以上の値。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE 、@ref COM_UTIL_ERR_DUPLICATE_DEFINITION 、
     *                  @ref COM_UTIL_ERR_LIMIT_EXCEEDED 。
     *
     *  文字列キーは key_size バイト以内に NUL が無いと
     *  @ref COM_UTIL_ERR_OUT_OF_RANGE です。\n
     *  実装中の同一キーは @ref COM_UTIL_ERR_DUPLICATE_DEFINITION です。\n
     *  削除済みの同一キーは同じレコードを再利用します。\n
     *  成功時はテーブルの変更時刻を @ref com_util_get_realtime で刻みます。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD のときはレコードにも刻みます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_add(com_util_hashtable *ht, const void *key, const void *value);

    /**
     *  @brief          レコード番号を指定してキーと値と変更時刻を直接書き込みます。
     *  @param[in,out]  ht      対象。NULL を渡してはなりません。
     *  @param[in]      record  1 相対のレコード番号。
     *  @param[in]      key     キー。NULL を渡してはなりません。
     *  @param[in]      status  書き込む実装状況。1 は実装中、2 以上は削除済みの加齢です。
     *  @param[in]      value   record_size バイト以上の値。NULL を渡してはなりません。
     *  @param[in]      timestamp  書き込む変更時刻。粒度と対応する要否があります。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_SKIPPED 、
     *                  @ref COM_UTIL_ERR_INVALID_ARGUMENT 、@ref COM_UTIL_ERR_OUT_OF_RANGE 、
     *                  @ref COM_UTIL_ERR_DUPLICATE_DEFINITION 。
     *
     *  マイグレーションや再構築で、番号を保ったままレコードを置くための入口です。\n
     *  レコード番号が 0 または capacity を超える場合は
     *  @ref COM_UTIL_ERR_INVALID_ARGUMENT です。\n
     *  `status >= 2` かつ先の lifetime が有限で `status` が lifetime 以上なら
     *  @ref COM_UTIL_SKIPPED を返し、テーブルは変更しません。\n
     *  先の lifetime が @ref COM_UTIL_HASHTABLE_LIFETIME_INFINITE なら
     *  status 255 も書き込めます。\n
     *  先のスロットが空でない、または同一キーが既にある場合は
     *  @ref COM_UTIL_ERR_DUPLICATE_DEFINITION です。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD では @p timestamp は必須で、
     *  渡した時刻がテーブルの変更時刻より新しいときだけテーブル時刻を更新します。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE では @p timestamp に
     *  NULL 以外を渡すと @ref COM_UTIL_ERR_INVALID_ARGUMENT です。\n
     *  この経路ではテーブル時刻は進めません。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_insert_direct(com_util_hashtable *ht, uint64_t record,
                                                                      const void *key, int status, const void *value,
                                                                      const com_util_timespec *timestamp);

    /**
     *  @brief          実装中の既存キーの値を書き換えます。
     *  @param[in,out]  ht     対象。NULL を渡してはなりません。
     *  @param[in]      key    キー。NULL を渡してはなりません。
     *  @param[in]      value  record_size バイト以上の値。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  成功時はテーブルの変更時刻を @ref com_util_get_realtime で刻みます。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD のときはレコードにも刻みます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_update(com_util_hashtable *ht, const void *key,
                                                               const void *value);

    /**
     *  @brief          レコード番号で値を書き換えます。
     *  @param[in,out]  ht      対象。NULL を渡してはなりません。
     *  @param[in]      record  1 相対のレコード番号。
     *  @param[in]      value   record_size バイト以上の値。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  成功時はテーブルの変更時刻を @ref com_util_get_realtime で刻みます。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD のときはレコードにも刻みます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_update_rec(com_util_hashtable *ht, uint64_t record,
                                                                   const void *value);

    /**
     *  @brief          キーで値への参照を取得します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[in]      key        キー。NULL を渡してはなりません。
     *  @param[out]     value_out  参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  削除済みは見つからない扱いです。返るポインターは解放してはなりません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_find_value_ref(const com_util_hashtable *ht, const void *key,
                                                                       const void **value_out);

    /**
     *  @brief          キーで値を呼び出し側バッファーへ複製します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[in]      key        キー。NULL を渡してはなりません。
     *  @param[out]     value_out  record_size バイト以上の複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_find_value_val(const com_util_hashtable *ht, const void *key,
                                                                       void *value_out);

    /**
     *  @brief          キーからレコード番号を取得します。
     *  @param[in]      ht          対象。NULL を渡してはなりません。
     *  @param[in]      key         キー。NULL を渡してはなりません。
     *  @param[out]     record_out  1 相対番号の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_find_recno(const com_util_hashtable *ht, const void *key,
                                                                   uint64_t *record_out);

    /**
     *  @brief          キーで変更時刻への参照を取得します。
     *  @param[in]      ht              対象。NULL を渡してはなりません。
     *  @param[in]      key             キー。NULL を渡してはなりません。
     *  @param[out]     timestamp_out   参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNSUPPORTED 、@ref COM_UTIL_ERR_OUT_OF_RANGE 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE では
     *  @ref COM_UTIL_ERR_UNSUPPORTED です。\n
     *  削除済みは見つからない扱いです。返るポインターは解放してはなりません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_find_timestamp_ref(const com_util_hashtable *ht,
                                                                           const void *key,
                                                                           const com_util_timespec **timestamp_out);

    /**
     *  @brief          キーで変更時刻を呼び出し側へ複製します。
     *  @param[in]      ht              対象。NULL を渡してはなりません。
     *  @param[in]      key             キー。NULL を渡してはなりません。
     *  @param[out]     timestamp_out   複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNSUPPORTED 、@ref COM_UTIL_ERR_OUT_OF_RANGE 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_find_timestamp_val(const com_util_hashtable *ht,
                                                                           const void *key,
                                                                           com_util_timespec *timestamp_out);

    /**
     *  @brief          レコード番号からキーへの参照を取得します。
     *  @param[in]      ht       対象。NULL を渡してはなりません。
     *  @param[in]      record   1 相対のレコード番号。
     *  @param[out]     key_out  参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  空は失敗、削除済みはキーを返します。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_key_ref(const com_util_hashtable *ht, uint64_t record,
                                                                    const void **key_out);

    /**
     *  @brief          レコード番号からキーを呼び出し側バッファーへ複製します。
     *  @param[in]      ht       対象。NULL を渡してはなりません。
     *  @param[in]      record   1 相対のレコード番号。
     *  @param[out]     key_out  key_size バイト以上の複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_key_val(const com_util_hashtable *ht, uint64_t record,
                                                                    void *key_out);

    /**
     *  @brief          レコード番号から値への参照を取得します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[in]      record     1 相対のレコード番号。
     *  @param[out]     value_out  参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  空は失敗、削除済みは削除直前の値を返します。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_value_ref(const com_util_hashtable *ht, uint64_t record,
                                                                      const void **value_out);

    /**
     *  @brief          レコード番号から値を呼び出し側バッファーへ複製します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[in]      record     1 相対のレコード番号。
     *  @param[out]     value_out  record_size バイト以上の複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_value_val(const com_util_hashtable *ht, uint64_t record,
                                                                      void *value_out);

    /**
     *  @brief          レコード番号の実装状況を取得します。
     *  @param[in]      ht          対象。NULL を渡してはなりません。
     *  @param[in]      record      1 相対のレコード番号。
     *  @param[out]     status_out  状態の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  0 は空、1 は実装中、2 以上は削除済みの加齢カウンタです。\n
     *  255 は lifetime が @ref COM_UTIL_HASHTABLE_LIFETIME_INFINITE のときの終端です。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_status(const com_util_hashtable *ht, uint64_t record,
                                                                   int *status_out);

    /**
     *  @brief          レコード番号から変更時刻への参照を取得します。
     *  @param[in]      ht              対象。NULL を渡してはなりません。
     *  @param[in]      record          1 相対のレコード番号。
     *  @param[out]     timestamp_out   参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNSUPPORTED 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_TABLE では
     *  @ref COM_UTIL_ERR_UNSUPPORTED です。\n
     *  空は失敗、削除済みは削除時の時刻を返します。返るポインターは解放してはなりません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_timestamp_ref(const com_util_hashtable *ht, uint64_t record,
                                                                          const com_util_timespec **timestamp_out);

    /**
     *  @brief          レコード番号から変更時刻を呼び出し側へ複製します。
     *  @param[in]      ht              対象。NULL を渡してはなりません。
     *  @param[in]      record          1 相対のレコード番号。
     *  @param[out]     timestamp_out   複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_UNSUPPORTED 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_timestamp_val(const com_util_hashtable *ht, uint64_t record,
                                                                          com_util_timespec *timestamp_out);

    /**
     *  @brief          テーブル横断の変更時刻への参照を取得します。
     *  @param[in]      ht              対象。NULL を渡してはなりません。
     *  @param[out]     timestamp_out   参照の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  最後にキーまたは値が変わった実時刻です。粒度に関わらず常に有効です。\n
     *  構築直後は 0 です。空テーブルでも成功します。\n
     *  返るポインターは解放してはなりません。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API
    com_util_hashtable_get_table_timestamp_ref(const com_util_hashtable *ht, const com_util_timespec **timestamp_out);

    /**
     *  @brief          テーブル横断の変更時刻を呼び出し側へ複製します。
     *  @param[in]      ht              対象。NULL を渡してはなりません。
     *  @param[out]     timestamp_out   複製先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_get_table_timestamp_val(const com_util_hashtable *ht,
                                                                                com_util_timespec *timestamp_out);

    /**
     *  @brief          実装中・削除済み・空の件数を 1 回の走査で求めます。
     *  @param[in]      ht           対象。NULL を渡してはなりません。
     *  @param[out]     in_use_out   実装中件数。不要なら NULL を渡せます。
     *  @param[out]     deleted_out  削除済み件数。不要なら NULL を渡せます。
     *  @param[out]     empty_out    空件数。不要なら NULL を渡せます。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  3 値の合計は capacity と一致します。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_count_status(const com_util_hashtable *ht, size_t *in_use_out,
                                                                     size_t *deleted_out, size_t *empty_out);

    /**
     *  @brief          実装中の件数を返します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[out]     count_out  件数の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  物理満杯は @ref com_util_hashtable_empty_count が 0 であることで判定します。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_count(const com_util_hashtable *ht, size_t *count_out);

    /**
     *  @brief          削除済みの件数を返します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[out]     count_out  件数の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_deleted_count(const com_util_hashtable *ht, size_t *count_out);

    /**
     *  @brief          空の件数を返します。
     *  @param[in]      ht         対象。NULL を渡してはなりません。
     *  @param[out]     count_out  件数の格納先。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  @par            スレッド セーフ
     *  条件付きスレッド セーフです。\n
     *  異なるテーブルへの同時呼び出しはできます。\n
     *  同一テーブルへの書き込みと同時に呼んではなりません。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_empty_count(const com_util_hashtable *ht, size_t *count_out);

    /**
     *  @brief          キーでレコードを削除します。
     *  @param[in,out]  ht   対象。NULL を渡してはなりません。
     *  @param[in]      key  キー。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_OUT_OF_RANGE 、@ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  通常は削除済み (状態 2) にします。lifetime が 2 のときは直ちに空へ戻します。\n
     *  テーブルの変更時刻は刻みます。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD では、削除済みとして残す場合に
     *  レコードの変更時刻も刻み、直ちに空へ戻す場合はレコードの時刻を 0 埋めします。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_delete(com_util_hashtable *ht, const void *key);

    /**
     *  @brief          レコード番号で削除します。加齢ルールは
     *                  @ref com_util_hashtable_delete と同じです。
     *  @param[in,out]  ht      対象。NULL を渡してはなりません。
     *  @param[in]      record  1 相対のレコード番号。
     *  @return         @ref COM_UTIL_OK 、@ref COM_UTIL_ERR_INVALID_ARGUMENT 、
     *                  @ref COM_UTIL_ERR_NOT_FOUND 。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_delete_rec(com_util_hashtable *ht, uint64_t record);

    /**
     *  @brief          削除済みレコードを 1 段階加齢し、寿命到達分を空へ戻します。
     *  @param[in,out]  ht  対象。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  lifetime が @ref COM_UTIL_HASHTABLE_LIFETIME_INFINITE のとき、
     *  status 254 の次は 255 になり、255 はそれ以上増えず空へ戻りません。\n
     *  レコードとテーブルの変更時刻は更新しません。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD では、空へ戻したスロットの時刻を 0 埋めします。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_push_deleted(com_util_hashtable *ht);

    /**
     *  @brief          削除済みレコードを加齢せずすべて空へ戻します。
     *  @param[in,out]  ht  対象。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  残したレコードとテーブルの時刻は変えません。\n
     *  @ref COM_UTIL_HASHTABLE_TIMESTAMP_SCOPE_RECORD では、空へ戻したスロットの時刻を 0 埋めします。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_purge_deleted(com_util_hashtable *ht);

    /**
     *  @brief          実装中と削除済みを含めてテーブルを空にします。
     *  @param[in,out]  ht  対象。NULL を渡してはなりません。
     *  @return         @ref COM_UTIL_OK 、または @ref COM_UTIL_ERR_INVALID_ARGUMENT 。
     *
     *  設定と所有権は変えません。\n
     *  空にしたスロットは 0 埋めします。\n
     *  テーブルの変更時刻は @ref com_util_get_realtime で刻みます。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一テーブルへの同時呼び出しは、呼び出し側で直列化してください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_hashtable_clear(com_util_hashtable *ht);

    /**
     *  @brief          内部確保した領域を解放します。
     *  @param[in]      ht  対象。NULL のときは何もしません。
     *
     *  外部バッファーで構築または再接続した場合は何もしません。\n
     *  外部指定時は、@p buf_mgmt と @p buf_data それぞれの解放
     *  (メモリマップド ファイルであれば @c com_util_mmap_detach 等) が
     *  呼び出し側の責務です。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  破棄対象を他スレッドが使っていないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_hashtable_dispose(com_util_hashtable *ht);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* COM_UTIL_HASHTABLE_HASHTABLE_H */
