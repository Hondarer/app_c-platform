/**
 *******************************************************************************
 *  @file           filter_test_catalog.h
 *  @brief          フィルター スロットのテストが使用するカタログを宣言します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/30
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef FILTER_TEST_CATALOG_PRIVATE_H
#define FILTER_TEST_CATALOG_PRIVATE_H

#include <cplat/string_catalog/catalog.h>

/** 引数を 1 個取る文字列の文字列キーです。 */
#define FILTER_TEST_CATALOG_KEY_NUMBER 1

/** カタログに存在しない文字列キーです。 */
#define FILTER_TEST_CATALOG_KEY_MISSING 99

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief          テスト用のカタログを返します。
     *  @return         カタログ識別オブジェクト。
     */
    const cplat_string_catalog *filter_test_catalog(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FILTER_TEST_CATALOG_PRIVATE_H */
