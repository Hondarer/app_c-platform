/**
 *******************************************************************************
 *  @file           sym_loader_info.c
 *  @brief          cplat_sym_loader_entry ポインター配列の内容を標準出力に表示します。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  各エントリを表示し、未解決のエントリがあれば解決を試みます。\n
 *  1 つでも解決失敗した場合は @ref CPLAT_ERR_UNKNOWN を返します。\n
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <cplat/runtime/sym_loader.h>
#include <stdio.h>

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_sym_loader_info(cplat_sym_loader_entry *const *fobj_array, const size_t fobj_length)
{
    int result = CPLAT_OK;
    size_t fobj_index;

    if (fobj_length > 0 && fobj_array == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    for (fobj_index = 0; fobj_index < fobj_length; fobj_index++)
    {
        if (fobj_array[fobj_index] == NULL)
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
    }

    for (fobj_index = 0; fobj_index < fobj_length; fobj_index++)
    {
        cplat_sym_loader_entry *fobj = fobj_array[fobj_index];

        if (fobj->resolved == 0)
        {
            (void)cplat_sym_loader_resolve(fobj);
        }
        printf("- [%zu] %s\n", fobj_index, fobj->func_key);
        printf("    - resolved : %d\n", fobj->resolved);
        printf("    - lib_name : %s\n", fobj->lib_name);
        printf("    - func_name: %s\n", fobj->func_name);
        printf("    - handle   : %p\n", (void *)fobj->handle);
        printf("    - func_ptr : %p\n", fobj->func_ptr);

        if (fobj->resolved < 0)
        {
            result = CPLAT_ERR_UNKNOWN;
        }
    }

    return result;
}
