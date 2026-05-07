/**
 *******************************************************************************
 *  @file           windows_sdk.h
 *  @brief          Windows SDK 共通取り込みヘッダー。
 *  @author         Tetsuo Honda
 *  @date           2026/04/20
 *  @version        1.0.0
 *
 *  @details
 *  com_util の公開ヘッダーから Windows SDK を参照する際の正本です。
 *  winsock2.h / ws2tcpip.h を windows.h より先に取り込むことで、
 *  winsock.h との衝突を防ぎます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_WINDOWS_SDK_H
#define COM_UTIL_WINDOWS_SDK_H

#include <com_util/base/platform.h>

#if defined(PLATFORM_WINDOWS)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif /* WIN32_LEAN_AND_MEAN */
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "Advapi32.lib")
    #ifdef byte
        #undef byte /* C++17 std::byte と Windows SDK byte typedef の競合を解消 */
    #endif /* byte */
#endif /* PLATFORM_WINDOWS */

#endif /* COM_UTIL_WINDOWS_SDK_H */
