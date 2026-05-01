/**
 *******************************************************************************
 *  @file           com_util_export.h
 *  @brief          com_util の Windows DLL エクスポート/呼び出し規約マクロ。
 *  @author         Tetsuo Honda
 *  @date           2026/04/21
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_EXPORT_H
#define COM_UTIL_EXPORT_H

#ifdef DOXYGEN

    /**
     *  @brief          DLL エクスポート/インポート制御マクロ。
     *  @details        ビルド条件に応じて以下の値を取ります。
     *
     *  | 条件                                               | 値                      |
     *  | -------------------------------------------------- | ----------------------- |
     *  | Linux (非 Windows)                                 | (空)                    |
     *  | Windows / `__INTELLISENSE__` 定義時                | (空)                    |
     *  | Windows / `COM_UTIL_STATIC` 定義時 (静的リンク)    | (空)                    |
     *  | Windows / `COM_UTIL_EXPORTS` 定義時 (DLL ビルド)   | `__declspec(dllexport)` |
     *  | Windows / `COM_UTIL_EXPORTS` 未定義時 (DLL 利用側) | `__declspec(dllimport)` |
     */
    #define COM_UTIL_EXPORT

    /**
     *  @brief          呼び出し規約マクロ。
     *  @details        Windows 環境では `__stdcall` 呼び出し規約を指定します。\n
     *                  Linux (非 Windows) 環境では空に展開されます。
     */
    #define COM_UTIL_API

#else /* !DOXYGEN */

    #ifndef COM_UTIL_STATIC
        #define COM_UTIL_STATIC 0
    #endif /* COM_UTIL_STATIC */
    #ifndef COM_UTIL_EXPORTS
        #define COM_UTIL_EXPORTS 0
    #endif /* COM_UTIL_EXPORTS */
    #include <com_util/base/dll_exports.h>
    #define COM_UTIL_EXPORT COM_UTIL_DLL_EXPORT(COM_UTIL)
    #define COM_UTIL_API    COM_UTIL_DLL_API(COM_UTIL)

#endif /* DOXYGEN */

#endif /* COM_UTIL_EXPORT_H */
