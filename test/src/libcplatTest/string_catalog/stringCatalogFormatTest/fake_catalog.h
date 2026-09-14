/**
 *******************************************************************************
 *  @file           fake_catalog.h
 *  @brief          テストが内容を差し替えられるカタログを宣言します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/10
 *  @version        1.0.0
 *
 *  製品のカタログは正しい内容だけを持つため、定義が壊れた場合の経路へ到達できません。\n
 *  本ファイルは、テストから内容を書き換えられるカタログを提供します。
 *
 *  カタログは利用者が定義する部分であり、文字列 ID の列挙も利用者が名付けます。\n
 *  ここではライブラリの接頭辞に依存しない名前を用い、注入方式が名前に依存しないことを示します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef FAKE_CATALOG_PRIVATE_H
#define FAKE_CATALOG_PRIVATE_H

#include <cplat/string_catalog/catalog.h>

/** 引数を取らない文字列のインデックスです。 */
#define FAKE_CATALOG_INDEX_NO_ARGUMENT 0

/** 引数を 2 個取る文字列のインデックスです。 */
#define FAKE_CATALOG_INDEX_TWO_ARGUMENTS 1

/** 引数を 1 個取る文字列のインデックスです。 */
#define FAKE_CATALOG_INDEX_ONE_ARGUMENT 2

/** 偽のカタログが保持する文字列の件数です。 */
#define FAKE_CATALOG_ENTRY_COUNT 3

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          偽のカタログの文字列 ID です。
     */
    typedef enum fake_catalog_id
    {
        FAKE_CATALOG_ID_NO_ARGUMENT = 1,   /**< 引数を取らない文字列です。 */
        FAKE_CATALOG_ID_TWO_ARGUMENTS = 2, /**< 引数を 2 個取る文字列です。 */
        FAKE_CATALOG_ID_ONE_ARGUMENT = 4,  /**< 引数を 1 個取る文字列です。 */
        FAKE_CATALOG_ID_UNKNOWN = 99       /**< カタログに登録しない文字列です。 */
    } fake_catalog_id;

    /**
     *  @brief          偽のカタログを既定の内容へ戻します。
     */
    void fake_catalog_reset(void);

    /**
     *  @brief          偽のカタログのカタログ識別オブジェクトを返します。
     *  @return         カタログ識別オブジェクトです。NULL は返しません。
     *
     *  インデックス表を持たないため、カタログの検索は線形探索の経路を通ります。
     */
    const cplat_string_catalog *fake_catalog(void);

    /**
     *  @brief          指定したカタログ要素の文字列 ID を書き換えます。
     *  @param[in]      index      書き換える文字列のインデックス。
     *  @param[in]      string_id 設定する文字列 ID。
     */
    void fake_catalog_set_id(int index, int string_id);

    /**
     *  @brief          指定したカタログ要素の処理用キーを書き換えます。
     *  @param[in]      index 書き換える文字列のインデックス。
     *  @param[in]      key   設定する処理用キー。NULL を指定すると未設定の状態になります。
     */
    void fake_catalog_set_key(int index, const char *key);

    /**
     *  @brief          指定した文字列の分類値を書き換えます。
     *  @param[in]      index    書き換える文字列のインデックス。
     *  @param[in]      category 設定する分類値。
     */
    void fake_catalog_set_category(int index, int category);

    /**
     *  @brief          指定した文字列の引数個数を書き換えます。
     *  @param[in]      index 書き換える文字列のインデックス。
     *  @param[in]      count 設定する引数個数。
     */
    void fake_catalog_set_argument_count(int index, int count);

    /**
     *  @brief          指定した文字列の引数種別を書き換えます。
     *  @param[in]      index          書き換える文字列のインデックス。
     *  @param[in]      argument_index 書き換える引数のインデックス。
     *  @param[in]      kind           設定する引数種別。
     */
    void fake_catalog_set_argument_kind(int index, int argument_index, cplat_string_catalog_argument_kind kind);

    /**
     *  @brief          指定した文字列の短い説明を書き換えます。
     *  @param[in]      index 書き換える文字列のインデックス。
     *  @param[in]      brief 設定する短い説明。NULL を指定すると未設定の状態になります。
     */
    void fake_catalog_set_brief(int index, const char *brief);

    /**
     *  @brief          指定した文字列の詳細説明を書き換えます。
     *  @param[in]      index   書き換える文字列のインデックス。
     *  @param[in]      details 設定する詳細説明。NULL を指定すると未設定の状態になります。
     */
    void fake_catalog_set_details(int index, const char *details);

    /**
     *  @brief          指定した文字列の引数定義配列を書き換えます。
     *  @param[in]      index     書き換える文字列のインデックス。
     *  @param[in]      arguments 設定する引数定義配列。NULL を指定できます。
     */
    void fake_catalog_set_arguments(int index, const cplat_string_catalog_argument *arguments);

    /**
     *  @brief          指定した引数の名前を書き換えます。
     *  @param[in]      index          書き換える文字列のインデックス。
     *  @param[in]      argument_index 書き換える引数のインデックス。
     *  @param[in]      name           設定する引数名。NULL を指定すると未設定の状態になります。
     */
    void fake_catalog_set_argument_name(int index, int argument_index, const char *name);

    /**
     *  @brief          指定した引数の説明を書き換えます。
     *  @param[in]      index          書き換える文字列のインデックス。
     *  @param[in]      argument_index 書き換える引数のインデックス。
     *  @param[in]      description    設定する引数説明。NULL を指定すると未設定の状態になります。
     */
    void fake_catalog_set_argument_description(int index, int argument_index, const char *description);

    /**
     *  @brief          指定した文字列の言語別の備考を書き換えます。
     *  @param[in]      index    書き換える文字列のインデックス。
     *  @param[in]      language 書き換える言語。
     *  @param[in]      note     設定する備考。NULL を指定するとリソースが未設定の状態になります。
     */
    void fake_catalog_set_note(int index, cplat_string_catalog_language language, const char *note);

    /**
     *  @brief          指定した文字列の言語別書式を書き換えます。
     *  @param[in]      index    書き換える文字列のインデックス。
     *  @param[in]      language 書き換える言語。
     *  @param[in]      text     設定する書式。NULL を指定するとリソースが未設定の状態になります。
     */
    void fake_catalog_set_text(int index, cplat_string_catalog_language language, const char *text);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FAKE_CATALOG_PRIVATE_H */
