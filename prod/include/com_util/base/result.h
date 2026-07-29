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
 *  必要な API は、結果コードとは別に errno_out などの出力引数で伝達します。
 *
 *  判定は @c != @ref COM_UTIL_OK の名前比較を正とします。
 *  全エラーが負値のため @c < 0 判定も等価ですが、名前比較を推奨します。
 *
 *  なお、CRT ラッパー (com_util_fopen など) と Windows API の UTF-8
 *  ラッパー (win32.h) は、元 API の戻り値規約を保存する層であるため、
 *  本結果コードの適用対象外です。
 *
 *  @attention      各コードの値は ABI として凍結します。既存の値の変更は
 *                  禁止し、コードの追加は末尾 (より小さい負値) への追記
 *                  のみとします。
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
 *  @ingroup        COM_UTIL_PUBLIC_API
 *
 *  com_util の公開 API が戻り値として使用する共通の結果コードです。\n
 *  成功は @ref COM_UTIL_OK (0) のみで、エラーは負値です。
 */

/**
 *  @ingroup        COM_UTIL_RESULT
 *  @{
 */
#define COM_UTIL_OK                     0     /**< 処理に成功しました。 */
#define COM_UTIL_ERR_UNKNOWN            (-1)  /**< -2 以下の分類済みコードに該当しない、その他のエラーです。 */
#define COM_UTIL_ERR_INVALID_ARGUMENT   (-2)  /**< API 引数が不正です (NULL、負値など)。 */
#define COM_UTIL_ERR_UNSUPPORTED        (-3)  /**< 現在のプラットフォームまたは状態では操作がサポートされていません。 */
#define COM_UTIL_ERR_OUT_OF_MEMORY      (-4)  /**< メモリを確保できません。 */
#define COM_UTIL_ERR_PERMISSION_DENIED  (-5)  /**< 権限が不足しています。 */
#define COM_UTIL_ERR_TIMEOUT            (-6)  /**< タイムアウトが発生しました。 */
#define COM_UTIL_ERR_BUSY               (-7)  /**< リソースがビジー状態です。 */
#define COM_UTIL_ERR_BUFFER_TOO_SMALL   (-8)  /**< 出力バッファーが不足しています。 */
#define COM_UTIL_ERR_LIMIT_EXCEEDED     (-9)  /**< リソースの上限を超過しました。 */
#define COM_UTIL_ERR_CORRUPT_DESCRIPTOR (-10) /**< ディスクリプタが破損しています。 */
#define COM_UTIL_ERR_DUPLICATE_DEFINITION (-11) /**< 同名の定義が登録済みです。 */
#define COM_UTIL_ERR_PARSE                (-12) /**< 解析エラーが発生しました。 */
#define COM_UTIL_ERR_EOF                  (-13) /**< 入力が EOF に達しました。 */
#define COM_UTIL_ERR_CANCELED             (-14) /**< ユーザー操作 (Ctrl+C など) により中断しました。 */
/** @} */

#endif /* COM_UTIL_RESULT_H */
