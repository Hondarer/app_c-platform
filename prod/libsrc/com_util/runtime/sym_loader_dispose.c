/**
 *******************************************************************************
 *  @file           sym_loader_dispose.c
 *  @brief          com_util_sym_loader_entry_t ポインタ配列を解放します。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/runtime/sym_loader.h>

/* doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT void COM_UTIL_API
    com_util_sym_loader_dispose(com_util_sym_loader_entry_t *const *fobj_array, const size_t fobj_length)
{
    size_t fobj_index;

    /* DllMain / destructor コンテキストから呼ばれるため、
     * ローダーロック保持中にミューテックスを取得すると
     * デッドロックを引き起こす恐れがある。
     * このコンテキストではシングルスレッド動作が保証されるため、
     * ロックなしで解放する。 */

    for (fobj_index = 0; fobj_index < fobj_length; fobj_index++)
    {
        com_util_sym_loader_entry_t *cache = fobj_array[fobj_index];

        if (cache->handle == NULL)
        {
            continue;
        }

#if defined(PLATFORM_LINUX)
        dlclose(cache->handle);
#elif defined(PLATFORM_WINDOWS)
        FreeLibrary(cache->handle);
#endif /* PLATFORM_ */

        cache->handle = NULL;
        cache->func_ptr = NULL;
    }
}
