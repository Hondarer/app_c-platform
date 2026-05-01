/**
 *******************************************************************************
 *  @file           libcom_util_lifecycle.c
 *  @brief          libcom_util.so / com_util.dll のロード・アンロード時処理。
 *  @author         Tetsuo Honda
 *  @date           2026/04/03
 *  @version        1.0.0
 *
 *  プラットフォームごとのフック (Linux constructor/destructor, Windows DllMain)
 *  は shared_lib_lifecycle が提供します。
 *  このファイルは libcom_util における onLoad / onUnload の実装を定義します。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/base/shared_lib_lifecycle.h>

#include <com_util/console/console_internal.h>
#include <com_util/trace/tracer_internal.h>

/* doxygen コメントは、ヘッダに記載 */
static void onLoad(void)
{
}

/* doxygen コメントは、ヘッダに記載 */
static void onUnload(int process_terminating)
{
    com_util_console_dispose_on_unload(process_terminating);
    trace_registry_dispose_all_on_unload(process_terminating);
}
