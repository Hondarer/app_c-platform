/**
 *******************************************************************************
 *  @file           sym_loader_resolve.c
 *  @brief          拡張可能な関数の動的ロードを行い、関数アドレスを返却します。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  cplat_sym_loader_resolve_as はスレッド セーフです。
 *  内部で mutex (Linux) または SRW ロック (Windows) を使用して排他制御します。
 *
 *  @par            double-checked locking とメモリ順序
 *  fast path (ロックなし) の resolved 読み取りには acquire セマンティクスを使用します。
 *  これにより、weakly-ordered アーキテクチャー (ARM64 等) においても
 *  resolved != 0 が見えた時点で func_ptr の書き込みも可視であることが保証されます。\n
 *  GCC では __atomic_load_n / __atomic_store_n を使用します。\n
 *  MSVC (x86_64) では TSO により load が acquire-ordered のため変更不要です。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <cplat/runtime/sym_loader.h>
#include <cplat/crt/string.h>
#if defined(PLATFORM_LINUX)
    #include <sched.h>
#endif /* PLATFORM_LINUX */
#if defined(PLATFORM_WINDOWS)
    #include <cplat/win32/win32.h>
#endif /* PLATFORM_WINDOWS */
#include <string.h>

#if defined(PLATFORM_LINUX)
static void wait_for_entry_lock_initialization(cplat_sym_loader_entry *fobj)
{
    (void)fobj;
    sched_yield();
}

static void (*s_entry_lock_wait_hook)(cplat_sym_loader_entry *fobj) = wait_for_entry_lock_initialization;
#endif /* PLATFORM_LINUX */

static int ensure_entry_lock_initialized(cplat_sym_loader_entry *fobj)
{
#if defined(PLATFORM_LINUX)
    int32_t expected = 0;

    if (__atomic_compare_exchange_n(&fobj->lock_state, &expected, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    {
        if (cplat_local_lock_create(&fobj->lock) != CPLAT_OK)
        {
            __atomic_store_n(&fobj->lock_state, -1, __ATOMIC_RELEASE);
            return -1;
        }
        __atomic_store_n(&fobj->lock_state, 2, __ATOMIC_RELEASE);
        return 0;
    }

    while ((expected = __atomic_load_n(&fobj->lock_state, __ATOMIC_ACQUIRE)) == 1)
    {
        s_entry_lock_wait_hook(fobj);
    }

    if (expected == 2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
#elif defined(PLATFORM_WINDOWS)
    LONG expected;

    expected = InterlockedCompareExchange((volatile LONG *)&fobj->lock_state, 1, 0);
    if (expected == 0)
    {
        if (cplat_local_lock_create(&fobj->lock) != CPLAT_OK)
        {
            InterlockedExchange((volatile LONG *)&fobj->lock_state, -1);
            return -1;
        }
        InterlockedExchange((volatile LONG *)&fobj->lock_state, 2);
        return 0;
    }

    while ((expected = InterlockedCompareExchange((volatile LONG *)&fobj->lock_state, 2, 2)) == 1)
    {
        SwitchToThread();
    }

    if (expected == 2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
#endif /* PLATFORM_ */
}

static void *unlock_entry_and_return_func_ptr(cplat_sym_loader_entry *fobj)
{
    cplat_local_lock_unlock(fobj->lock);
    return fobj->func_ptr;
}

/* Doxygen コメントは、ヘッダーに記載 */

void *cplat_sym_loader_resolve(cplat_sym_loader_entry *fobj)
{
#if defined(PLATFORM_LINUX)
    const char *ext = ".so";
#elif defined(PLATFORM_WINDOWS)
    const char *ext = ".dll";
#endif /* PLATFORM_ */
    char lib_with_ext[CPLAT_SYM_LOADER_NAME_MAX];

    /* ロード完了後は resolved が 0 以外になる。早期リターンで判定する。
     * ARM64 など weakly-ordered アーキテクチャーでのメモリ可視性を保証するため
     * GCC では acquire ロードを使用します。
     * MSVC (x86_64/TSO) では plain load で acquire 相当の保証が得られる。 */
#if defined(COMPILER_GCC)
    if (__atomic_load_n(&fobj->resolved, __ATOMIC_ACQUIRE) != 0)
    {
        return __atomic_load_n(&fobj->func_ptr, __ATOMIC_RELAXED);
    }
#else  /* !COMPILER_GCC */
    if (fobj->resolved != 0)
    {
        return fobj->func_ptr;
    }
#endif /* COMPILER_GCC */

    if (ensure_entry_lock_initialized(fobj) != 0)
    {
        return NULL;
    }

    /* ロード処理を排他制御する。
     * ロック取得後に再度 resolved を確認し、他スレッドが先にロードを
     * 完了していた場合は処理をスキップする (double-checked locking)。 */
    if (cplat_local_lock_lock(fobj->lock, CPLAT_SYNC_WAIT_FOREVER) != CPLAT_OK)
    {
        return NULL;
    }

    if (fobj->resolved == 0)
    {
        if (strcmp(fobj->lib_name, "default") == 0 && strcmp(fobj->func_name, "default") == 0)
        {
            /* resolved=2: 明示的デフォルト。func_ptr は NULL のまま。
             * func_ptr への書き込みなし → release store のみ必要。 */
#if defined(COMPILER_GCC)
            __atomic_store_n(&fobj->resolved, 2, __ATOMIC_RELEASE);
#else  /* !COMPILER_GCC */
            fobj->resolved = 2;
#endif /* COMPILER_GCC */
            return unlock_entry_and_return_func_ptr(fobj);
        }
        if (fobj->lib_name[0] == '\0' || fobj->func_name[0] == '\0')
        {
#if defined(COMPILER_GCC)
            __atomic_store_n(&fobj->resolved, -1, __ATOMIC_RELEASE);
#else  /* !COMPILER_GCC */
            fobj->resolved = -1; /* resolved=-1: 定義なし (定義ファイル不存在、定義行が不存在) */
#endif /* COMPILER_GCC */
            return unlock_entry_and_return_func_ptr(fobj);
        }
        if (strlen(fobj->lib_name) + strlen(ext) >= CPLAT_SYM_LOADER_NAME_MAX)
        {
#if defined(COMPILER_GCC)
            __atomic_store_n(&fobj->resolved, -2, __ATOMIC_RELEASE);
#else  /* !COMPILER_GCC */
            fobj->resolved = -2; /* resolved=-2: 名称長さオーバー */
#endif /* COMPILER_GCC */
            return unlock_entry_and_return_func_ptr(fobj);
        }
        (void)cplat_strcpy(lib_with_ext, sizeof(lib_with_ext), fobj->lib_name);
        (void)cplat_strcat(lib_with_ext, sizeof(lib_with_ext), ext);

#if defined(PLATFORM_LINUX)
        fobj->handle = dlopen(lib_with_ext, RTLD_LAZY);
#elif defined(PLATFORM_WINDOWS)
        fobj->handle = LoadLibraryU(lib_with_ext);
#endif /* PLATFORM_ */
        if (fobj->handle == NULL)
        {
#if defined(COMPILER_GCC)
            __atomic_store_n(&fobj->resolved, -3, __ATOMIC_RELEASE);
#else  /* !COMPILER_GCC */
            fobj->resolved = -3; /* resolved=-3: ライブラリ オープン エラー */
#endif /* COMPILER_GCC */
            return unlock_entry_and_return_func_ptr(fobj);
        }

        /* func_ptr を書き込んでから resolved を release-store する。
         * fast path の acquire ロードと対になり、func_ptr の可視性を保証する。 */
#if defined(PLATFORM_LINUX)
        {
            void *ptr = dlsym(fobj->handle, fobj->func_name);
    #if defined(COMPILER_GCC)
            __atomic_store_n(&fobj->func_ptr, ptr, __ATOMIC_RELAXED);
    #else  /* !COMPILER_GCC */
            fobj->func_ptr = ptr;
    #endif /* COMPILER_GCC */
            if (ptr == NULL)
            {
                dlclose(fobj->handle);
                fobj->handle = NULL;
            }
        }
#elif defined(PLATFORM_WINDOWS)
        {
            void *ptr = (void *)GetProcAddress(fobj->handle, fobj->func_name);
            fobj->func_ptr = ptr; /* MSVC x86_64: TSO により release-ordered */
            if (ptr == NULL)
            {
                FreeLibrary(fobj->handle);
                fobj->handle = NULL;
            }
        }
#endif /* PLATFORM_ */

#if defined(COMPILER_GCC)
        __atomic_store_n(&fobj->resolved, 1, __ATOMIC_RELEASE); /* resolved=1: 解決済 */
#else                                                           /* !COMPILER_GCC */
        fobj->resolved = 1; /* resolved=1: 解決済 */
#endif                                                          /* COMPILER_GCC */
    }

    return unlock_entry_and_return_func_ptr(fobj);
}
