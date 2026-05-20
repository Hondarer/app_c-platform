/**
 *******************************************************************************
 *  @file           sym_loader_info.c
 *  @brief          com_util_sym_loader_entry_t ポインタ配列の内容を標準出力に表示します。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  各エントリを表示し、未解決のエントリがあれば解決を試みます。\n
 *  1 つでも解決失敗した場合は -1 を返します。\n
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/runtime/sym_loader.h>
#include <stdio.h>

/* doxygen コメントは、ヘッダーに記載 */

COM_UTIL_EXPORT int COM_UTIL_API
    com_util_sym_loader_info(com_util_sym_loader_entry_t *const *fobj_array, const size_t fobj_length)
{
    int rtc = 0;
    size_t fobj_index;

    for (fobj_index = 0; fobj_index < fobj_length; fobj_index++)
    {
        com_util_sym_loader_entry_t *fobj = fobj_array[fobj_index];

        if (fobj->resolved == 0)
        {
            (void)com_util_sym_loader_resolve(fobj);
        }
        printf("- [%zu] %s\n", fobj_index, fobj->func_key);
        printf("    - resolved : %d\n", fobj->resolved);
        printf("    - lib_name : %s\n", fobj->lib_name);
        printf("    - func_name: %s\n", fobj->func_name);
        printf("    - handle   : %p\n", (void *)fobj->handle);
        printf("    - func_ptr : %p\n", fobj->func_ptr);

        if (fobj->resolved < 0)
        {
            rtc = -1;
        }
    }

    return rtc;
}
