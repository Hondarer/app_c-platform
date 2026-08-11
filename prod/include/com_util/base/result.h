/**
 *******************************************************************************
 *  @file           result.h
 *  @brief          com_util ライブラリ共通の結果コードを定義します。
 *  @author         Tetsuo Honda
 *  @date           2026/07/27
 *  @version        1.0.0
 *
 *  com_util の公開 API が戻り値として使用する共通の結果コードです。\n
 *  成功は @ref COM_UTIL_OK (0) のみとし、非 0 はすべて
 *  「要求した操作が完遂されなかった」ことを表します。エラーは負値です。
 *
 *  @ref COM_UTIL_ERR_UNKNOWN (-1) は、-2 以下の分類済みコードに該当しない
 *  その他のエラーを表します。OS 由来の詳細 (errno、GetLastError() の値) が
 *  必要な API は、結果コードとは別に com_util_error の出力引数または
 *  com_util_error_get_last() で伝達します。
 *
 *  判定は @c != @ref COM_UTIL_OK の名前比較を正とします。
 *  全エラーが負値のため @c < 0 判定も等価ですが、名前比較を推奨します。
 *
 *  なお、com_util_fopen など元 API の戻り値規約を保存する CRT ラッパーと
 *  Windows API の UTF-8 ラッパー (win32.h) は、本結果コードの適用対象外です。
 *  CRT の機能を使用する API でも、戻り値に共通結果コードを使用すると明記した
 *  関数には本結果コードを適用します。
 *
 *  本ヘッダーは、粗い分類 (@ref COM_UTIL_ERR_INVALID_ARGUMENT など) と
 *  細かい分類 (@ref COM_UTIL_ERR_UNKNOWN_OPTION など) の両方を含む単一の
 *  体系です。モジュール固有のコード体系を別に設けず、必要な分類は本ヘッダー
 *  へ追加します。
 *
 *  定義は課題別の帯 (引数・状態・権限、リソース・バッファー、入力解析、制御)
 *  に分けて並べ、各帯に将来の追加用の余白を設けています。
 *
 *  @warning        帯は定義の整理と追加位置を示すためのものであり、範囲判定に
 *                  よる分類は本 API の契約に含めません。
 *                  @c rc @c <= @c -20 のような範囲比較で種別を判定せず、
 *                  個々のコード名との比較を使用してください。
 *
 *  @attention      各コードの値は ABI として凍結します。既存の値の変更は
 *                  禁止し、コードの追加は該当する帯の余白への追記のみと
 *                  します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_RESULT_H
#define COM_UTIL_RESULT_H

/**
 *  @defgroup       COM_UTIL_RESULT 共通結果コード
 *  @ingroup        COM_UTIL_BASE
 *
 *  com_util の公開 API が戻り値として使用する共通の結果コードです。\n
 *  成功は @ref COM_UTIL_OK (0) のみで、エラーは負値です。
 */

/**
 *  @ingroup        COM_UTIL_RESULT
 *  @{
 */
#define COM_UTIL_OK 0 /**< 処理に成功しました。 */

/* 分類不能 */
#define COM_UTIL_ERR_UNKNOWN (-1) /**< -2 以下の分類済みコードに該当しない、その他のエラーです。 */

/* 引数・状態・権限: -2 〜 -9 */
#define COM_UTIL_ERR_INVALID_ARGUMENT  (-2) /**< API 引数が不正です (NULL、負値など)。 */
#define COM_UTIL_ERR_UNSUPPORTED       (-3) /**< 現在のプラットフォームまたは状態では操作がサポートされていません。 */
#define COM_UTIL_ERR_PERMISSION_DENIED (-4) /**< 権限が不足しています。 */
#define COM_UTIL_ERR_DUPLICATE_DEFINITION (-5) /**< 同名の定義が登録済みです。 */
#define COM_UTIL_ERR_NOT_FOUND            (-6) /**< 対象が存在しません (ホスト名を解決できない場合など)。 */

/* リソース・バッファー: -10 〜 -19 */
#define COM_UTIL_ERR_OUT_OF_MEMORY      (-10) /**< メモリを確保できません。 */
#define COM_UTIL_ERR_BUSY               (-11) /**< リソースがビジー状態です。 */
#define COM_UTIL_ERR_TIMEOUT            (-12) /**< タイムアウトが発生しました。 */
#define COM_UTIL_ERR_LIMIT_EXCEEDED     (-13) /**< リソースの上限を超過しました。 */
#define COM_UTIL_ERR_BUFFER_TOO_SMALL   (-14) /**< 出力バッファーが不足しています。 */
#define COM_UTIL_ERR_CORRUPT_DESCRIPTOR (-15) /**< ディスクリプタが破損しています。 */

/* 入力解析: -20 〜 -39 */
#define COM_UTIL_ERR_UNKNOWN_OPTION       (-20) /**< 未登録のオプションが指定されました。 */
#define COM_UTIL_ERR_MISSING_VALUE        (-21) /**< 値を要する項目に値が指定されていません。 */
#define COM_UTIL_ERR_UNEXPECTED_VALUE     (-22) /**< 値を取らない項目に値が指定されました。 */
#define COM_UTIL_ERR_INVALID_INTEGER      (-23) /**< 整数値として解釈できません。 */
#define COM_UTIL_ERR_OUT_OF_RANGE         (-24) /**< 値が表現可能な範囲を超えています。 */
#define COM_UTIL_ERR_MISSING_REQUIRED     (-25) /**< 必須の項目が指定されていません。 */
#define COM_UTIL_ERR_DUPLICATE_OPTION     (-26) /**< 単数指定の項目が複数回指定されました。 */
#define COM_UTIL_ERR_TOO_MANY_ARGUMENTS   (-27) /**< 引数の個数が受入数を超えています。 */
#define COM_UTIL_ERR_TOO_MANY_OCCURRENCES (-28) /**< 同一項目の出現回数が容量を超えています。 */
#define COM_UTIL_ERR_INVALID_PATTERN      (-29) /**< 正規表現パターンの構文が不正です。 */
#define COM_UTIL_ERR_INVALID_ENCODING     (-30) /**< 文字列が UTF-8 として不正です。 */

/* 制御: -40 〜 */
#define COM_UTIL_ERR_EOF      (-40) /**< 入力が EOF に達しました。 */
#define COM_UTIL_ERR_CANCELED (-41) /**< ユーザー操作 (Ctrl+C など) により中断しました。 */
/** @} */

#endif /* COM_UTIL_RESULT_H */
