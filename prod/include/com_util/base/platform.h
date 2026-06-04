/**
 *******************************************************************************
 *  @file           platform.h
 *  @brief          プラットフォームおよびアーキテクチャ検出マクロのヘッダーファイル。
 *  @author         Tetsuo Honda
 *  @date           2025/11/22
 *
 *  ビルド対象の OS とプロセッサアーキテクチャを検出し、統一的なマクロを定義します。
 *
 *  @section        platform_detection プラットフォーム検出マクロ
 *
 *  検出されたプラットフォームに応じて、以下のマクロを定義します。
 *
 *  | プラットフォーム | 識別マクロ           | PLATFORM_NAME       |
 *  | ---------------- | -------------------- | ------------------- |
 *  | Windows          | PLATFORM_WINDOWS     | "Windows"           |
 *  | Linux            | PLATFORM_LINUX       | "Linux"             |
 *  | その他           | PLATFORM_UNKNOWN     | "Unknown"           |
 *
 *  @section        arch_detection アーキテクチャ検出マクロ
 *
 *  検出されたアーキテクチャに応じて、以下のマクロを定義します。
 *
 *  | アーキテクチャ | 識別マクロ   | ARCH_NAME |
 *  | -------------- | ------------ | --------- |
 *  | x86_64         | ARCH_X64     | "x64"     |
 *  | x86 (32bit)    | ARCH_X86     | "x86"     |
 *  | その他         | ARCH_UNKNOWN | "Unknown" |
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2025-2026. All rights reserved.
 *
 *
 *  @hideincludedbygraph
 *
 *******************************************************************************
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include "compiler.h"

/**
 *  @ingroup        COM_UTIL_PUBLIC_API
 *  @{
 */

#ifdef DOXYGEN
    #define PLATFORM_WINDOWS        /**< Windows の場合に定義されます。 */
    #define PLATFORM_LINUX          /**< Linux の場合に定義されます。 */
    #define PLATFORM_UNKNOWN        /**< 未知のプラットフォームの場合に定義されます。 */
    #define PLATFORM_NAME    "name" /**< プラットフォーム名の文字列 ("Windows", "Linux", "Unknown")。 */
#else                               /* !DOXYGEN */
    #if defined(_WIN32) || defined(_WIN64)
        #define PLATFORM_WINDOWS
        #define PLATFORM_NAME "Windows"
    #elif defined(__linux__)
        #define PLATFORM_LINUX
        #define PLATFORM_NAME "Linux"
    #else
        #define PLATFORM_UNKNOWN
        #define PLATFORM_NAME "Unknown"
    #endif
#endif /* DOXYGEN */

#ifdef DOXYGEN
    #define ARCH_X64            /**< x86_64 アーキテクチャの場合に定義されます。 */
    #define ARCH_X86            /**< x86 (32bit) アーキテクチャの場合に定義されます。 */
    #define ARCH_UNKNOWN        /**< 未知のアーキテクチャの場合に定義されます。 */
    #define ARCH_NAME    "name" /**< アーキテクチャ名の文字列 ("x64", "x86", "Unknown")。 */
#else                           /* !DOXYGEN */
    #if defined(__x86_64__) || defined(_M_X64)
        #define ARCH_X64
        #define ARCH_NAME "x64"
    #elif defined(__i386__) || defined(_M_IX86)
        #define ARCH_X86
        #define ARCH_NAME "x86"
    #else
        #define ARCH_UNKNOWN
        #define ARCH_NAME "Unknown"
    #endif
#endif /* DOXYGEN */

/** @} */

#endif /* PLATFORM_H */
