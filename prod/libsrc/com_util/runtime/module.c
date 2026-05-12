/**
 *******************************************************************************
 *  @file           module.c
 *  @brief          共有ライブラリ自身 (.so/.dll) の絶対パスと basename
 *                  (パスなし・拡張子なし) を取得します。
 *  @author         c-modenization-kit sample team
 *  @date           2026/02/23
 *  @version        1.0.0
 *
 *  Linux(gcc) では dladdr() で共有オブジェクトを特定し、realpath() で可能な限り正規化 (絶対化・symlink 解決) します。
 *  Windows(MSVC) では GetModuleHandleEx() で DLL の HMODULE を得て、GetModuleFileNameW() でパスを取得します。
 *
 *  @note           - 「絶対パス」は OS/ローダの情報とファイルシステム状態に依存します。
 *                    Linux でロード後にファイルが移動/削除される等により realpath()
 *                    が失敗する場合、可能な範囲で絶対化した文字列を返します。
 *                  - Windows は基本的にフルパスが得られますが、古い環境では MAX_PATH 制約が残る場合があります。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <com_util/runtime/module.h>
#include <com_util/crt/crt_internal.h>
#include <com_util/crt/path.h>
#include <com_util/crt/string.h>
#include <errno.h>
#if defined(PLATFORM_LINUX)
    #include <dlfcn.h>
    #include <stddef.h>
    #include <stdint.h>
    #include <string.h>
#elif defined(PLATFORM_WINDOWS)
    #include <stddef.h>
    #include <stdint.h>
    #include <string.h>
#endif /* PLATFORM_ */

/**
 *******************************************************************************
 *  @brief          内部関数の戻り値(ステータス)。
 *******************************************************************************
 */
typedef enum get_lib_info_status_t
{
    /** 成功 */
    MYLIB_OK = 0,
    /** 引数不正 (NULL、サイズ0など) */
    MYLIB_EINVAL = -1,
    /** バッファ不足 (出力が収まらない) */
    MYLIB_ENOBUFS = -2,
    /** その他の失敗 (取得不能、OS API 失敗など) */
    MYLIB_EFAIL = -3
} get_lib_info_status_t;

/**
 *******************************************************************************
 *  @brief          文字列を安全にコピーします (UTF-8 想定だが単なる byte 列として扱う)。
 *
 *  @param[out]     dst    出力バッファ。
 *  @param[in]      dst_sz 出力バッファサイズ[byte]。
 *  @param[in]      src    入力文字列 (NULL 終端)。
 *  @return         get_lib_info_status_t
 *******************************************************************************
 */
static get_lib_info_status_t copy_str(char *dst, size_t dst_sz, const char *src)
{
    size_t n;

    if (!dst || dst_sz == 0 || !src)
        return MYLIB_EINVAL;
    n = strlen(src);
    if (n + 1 > dst_sz)
    {
        if (dst_sz > 0)
            dst[0] = '\0';
        return MYLIB_ENOBUFS;
    }
    memcpy(dst, src, n + 1);
    return MYLIB_OK;
}

/**
 *******************************************************************************
 *  @brief          パス文字列からファイル名部分 (最後の区切り文字以降) を返します。
 *
 *  @param[in]      path パス。
 *  @return         const char* ファイル名部分へのポインタ (元の文字列内)。
 *******************************************************************************
 */
static const char *get_ilename_part(const char *path)
{
    const char *p;
    const char *last = path;

    if (!path)
        return "";
    for (p = path; *p; ++p)
    {
        /* コンパイル時 __FILE__ (MSVC は '\\') や外部由来パスを扱う汎用関数のため両セパレータに対応する */
        if (*p == '/' || *p == '\\')
            last = p + 1;
    }
    return last;
}

/**
 *******************************************************************************
 *  @brief          拡張子を取り除きます (その場で書き換え)。
 *
 *  @details        - Linux: ".so." が含まれる場合はそこから先を削除 (例: libx.so.1.2.3 -> libx)
 *                  - Linux: 末尾が ".so" の場合は削除 (例: libx.so -> libx)
 *                  - その他: 最後の '.' 以降を削除 (一般的な拡張子扱い)
 *
 *  @param[in,out]  s 対象文字列 (NULL 終端)。
 *******************************************************************************
 */
static void strip_extension_inplace(char *s)
{
    char *dot;

    if (!s)
        return;

#if defined(PLATFORM_LINUX)
    {
        char *so_ver = strstr(s, ".so.");
        if (so_ver)
        {
            *so_ver = '\0';
            return;
        }

        {
            size_t len = strlen(s);
            if (len >= 3 && strcmp(s + (len - 3), ".so") == 0)
            {
                s[len - 3] = '\0';
                return;
            }
        }

        {
            size_t len = strlen(s);
            if (len >= 6 && strcmp(s + (len - 6), ".dylib") == 0)
            {
                s[len - 6] = '\0';
                return;
            }
        }
    }
#endif /* PLATFORM_LINUX */

    dot = strrchr(s, '.');
    if (dot && dot != s)
    {
        *dot = '\0';
    }
}

#if defined(PLATFORM_LINUX)

/**
 *******************************************************************************
 *  @brief          .so 自身の絶対パスを取得します (Linux/Unix)。
 *
 *  dladdr() に指定された関数アドレスを渡して所属共有オブジェクトを取得し、\n
 *  realpath() で可能な限り絶対化・正規化します。
 *
 *  @param[out]     out_path    出力 (UTF-8、NULL 終端)。
 *  @param[in]      out_path_sz 出力バッファサイズ[byte]。
 *  @param[in]      func_addr   所属モジュールを特定するための関数アドレス。
 *  @return         get_lib_info_status_t
 *******************************************************************************
 */
static get_lib_info_status_t get_self_path_posix(char *out_path, size_t out_path_sz, const void *func_addr)
{
    Dl_info info;
    const char *p;
    int err = 0;

    if (!out_path || out_path_sz == 0 || !func_addr)
        return MYLIB_EINVAL;

    memset(&info, 0, sizeof(info));
    if (dladdr(func_addr, &info) == 0)
    {
        return MYLIB_EFAIL;
    }

    if (info.dli_fname)
    {
        p = info.dli_fname;
    }
    else
    {
        p = "";
    }
    if (p[0] == '\0')
        return MYLIB_EFAIL;

    if (com_util_path_get_full(out_path, out_path_sz, &err, p) == 0)
    {
        return MYLIB_OK;
    }

    if (err == ENAMETOOLONG)
    {
        return MYLIB_ENOBUFS;
    }
    else
    {
        return MYLIB_EFAIL;
    }
}

#elif defined(PLATFORM_WINDOWS)

/**
 *******************************************************************************
 *  @brief          DLL 自身の絶対パス (ワイド文字列) を取得します。
 *
 *  @param[out]     out_w     出力 (UTF-16、NULL 終端)。
 *  @param[in]      out_w_cap 出力バッファサイズ[wchar_t 個数]。
 *  @param[in]      func_addr 所属モジュールを特定するための関数アドレス。
 *  @return         get_lib_info_status_t
 *******************************************************************************
 */
static get_lib_info_status_t get_self_path_w(wchar_t *out_w, size_t out_w_cap, const void *func_addr)
{
    HMODULE hm = NULL;
    wchar_t buf[PLATFORM_PATH_MAX];
    DWORD n;

    if (!out_w || out_w_cap == 0 || !func_addr)
        return MYLIB_EINVAL;

    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCWSTR)func_addr, &hm))
    {
        return MYLIB_EFAIL;
    }

    n = GetModuleFileNameW(hm, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if (n == 0 || n >= (DWORD)(sizeof(buf) / sizeof(buf[0])))
    {
        return MYLIB_EFAIL;
    }
    buf[n] = L'\0';
    if (wcslen(buf) + 1 > out_w_cap)
        return MYLIB_ENOBUFS;
    (void)com_util_wcscpy(out_w, out_w_cap, buf);
    return MYLIB_OK;
}

#endif /* PLATFORM_ */

/* doxygen コメントは、ヘッダーに記載 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_module_get_path(char *out_path, const size_t out_path_sz, const void *func_addr)
{
#if defined(PLATFORM_LINUX)
    return (int)get_self_path_posix(out_path, out_path_sz, func_addr);
#elif defined(PLATFORM_WINDOWS)
    wchar_t wpath[PLATFORM_PATH_MAX];
    char utf8_path[PLATFORM_PATH_MAX];
    int err = 0;
    get_lib_info_status_t st = get_self_path_w(wpath, (size_t)(sizeof(wpath) / sizeof(wpath[0])), func_addr);
    if (st != MYLIB_OK)
    {
        if (out_path && out_path_sz)
            out_path[0] = '\0';
        return st;
    }
    if (com_util_wpath_to_utf8(utf8_path, sizeof(utf8_path), wpath) < 0)
    {
        if (out_path && out_path_sz)
            out_path[0] = '\0';
        return MYLIB_ENOBUFS;
    }
    if (com_util_path_get_full(out_path, out_path_sz, &err, utf8_path) != 0)
    {
        if (out_path && out_path_sz)
            out_path[0] = '\0';
        if (err == ENAMETOOLONG)
        {
            return MYLIB_ENOBUFS;
        }
        else
        {
            return MYLIB_EFAIL;
        }
    }
    return MYLIB_OK;
#endif /* PLATFORM_ */
}

/* doxygen コメントは、ヘッダーに記載 */
COM_UTIL_EXPORT int COM_UTIL_API com_util_module_get_basename(char *out_basename, const size_t out_basename_sz, const void *func_addr)
{
    get_lib_info_status_t st;
    char path_buf[4096];
    const char *fname;
    char tmp[1024];

    if (!out_basename || out_basename_sz == 0)
        return MYLIB_EINVAL;

    st = com_util_module_get_path(path_buf, sizeof(path_buf), func_addr);
    if (st != MYLIB_OK)
    {
        out_basename[0] = '\0';
        return st;
    }

    fname = get_ilename_part(path_buf);
    if (!fname || fname[0] == '\0')
    {
        out_basename[0] = '\0';
        return MYLIB_EFAIL;
    }

    /* ファイル名のみをコピーして拡張子を除く */
    st = copy_str(tmp, sizeof(tmp), fname);
    if (st != MYLIB_OK)
    {
        out_basename[0] = '\0';
        return st;
    }

    strip_extension_inplace(tmp);

    return (int)copy_str(out_basename, out_basename_sz, tmp);
}
