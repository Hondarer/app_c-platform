/**
 *******************************************************************************
 *  @file           syslog_test.h
 *  @brief          syslog テスト用ヘルパーヘッダー。
 *  @author         Tetsuo Honda
 *  @date           2026/04/05
 *  @version        1.0.0
 *
 *  テスト実行時に環境変数 `SYSLOG_TEST_FD` が設定されていれば、
 *  syslog 経路のメッセージをその FD に書き込みます。\n
 *  `shared_lib_lifecycle.h` を利用する外部モジュール、および `trace_syslog.c` から
 *  インクルードして使用します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef SYSLOG_TEST_H
#define SYSLOG_TEST_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_LINUX)

#include <stdlib.h>
#include <unistd.h>

/**
 *  @brief          SYSLOG_TEST_FD 環境変数が設定されていれば、その FD にバッファを書き込む。
 *  @details        テスト用パイプ FD への送信のみを行います。\n
 *                  呼び出し元は戻り値が 1 の場合、/dev/log への送信をスキップしてください。
 *  @param[in]      buf     送信するバッファ (呼び出し元で整形済みの 1 行分データ)。
 *  @param[in]      nbytes  送信バイト数。
 *  @return         1: テスト FD に書き込んだ (/dev/log への送信をスキップすること)。\n
 *                  0: 環境変数未設定 (通常の /dev/log 送信を続行すること)。
 */
static int syslog_test_fd_write__(const char *buf, size_t nbytes)
{
    const char *fd_str = getenv("SYSLOG_TEST_FD");
    int test_fd;

    if (fd_str == NULL)
    {
        return 0;
    }

    test_fd = atoi(fd_str);
    if (test_fd >= 0)
    {
        (void)write(test_fd, buf, nbytes);
    }
    return 1;
}

#endif /* PLATFORM_LINUX */

#endif /* SYSLOG_TEST_H */
