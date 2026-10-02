/**
 *******************************************************************************
 *  @file           filter_test_context.h
 *  @brief          トレースのコンテキスト引数として付加する値の取得関数を宣言します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/26
 *  @version        0.1.0
 *
 *  カタログ設定 `catalog_settings.jsonc` の `context` が、本ヘッダーの関数を取得式として参照します。\n
 *  cplat はこの引数の意味を知りません。条件式フィルターが、cplat の文脈引数と同じく引数名で照合できることを確かめるためのものです。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#ifndef FILTER_TEST_CONTEXT_PRIVATE_H
#define FILTER_TEST_CONTEXT_PRIVATE_H

#include <stdint.h>

/** ラウンド トリップ ID の上限値です。この値の次は 1 へ戻ります。巡回を短時間で観察できるよう小さくしています。 */
#define FILTER_TEST_SEQUENCE_NUMBER_MAX 99

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          トレースへ付与するラウンド トリップ ID を 1 つ進めて返します。
     *  @return         1 から @ref FILTER_TEST_SEQUENCE_NUMBER_MAX までの値です。
     *
     *  トレースの出力要求ごとに 1 回評価され、位置指定 `{46}` の値 `sequence_number` になります。\n
     *  上限に達した次の呼び出しで 1 へ戻るため、番号は巡回します。
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。
     */
    int32_t filter_test_next_sequence_number(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FILTER_TEST_CONTEXT_PRIVATE_H */
