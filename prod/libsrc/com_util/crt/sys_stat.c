/**
 *******************************************************************************
 *  @file           sys_stat.c
 *  @brief          ファイル情報の取得とディレクトリ生成を行う API を実装します。
 *
 *  UTF-8 パスに対応した stat、mkdir、再帰的ディレクトリ生成を提供します。
 *
 *******************************************************************************
 */

#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>

#include <com_util/crt/wchar_conv.h>

#include <stddef.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <sys/stat.h>
#elif defined(PLATFORM_WINDOWS)
    #include <direct.h>
#endif /* PLATFORM_ */

/**
 *  @brief  指定されたディレクトリが存在することを確認し、なければ生成します。
 *  @param[in]  dir  対象ディレクトリのパス (UTF-8)。
 *  @return     成功時は 0、失敗時は -1 を返します。
 *
 *  com_util_mkdir が競合生成で -1 を返す場合も com_util_stat で再確認して
 *  ディレクトリが存在すれば成功とみなします。
 */
static int ensure_one_dir(const char *dir)
{
    com_util_file_stat_t st;

    /* 既に存在する場合は成功 */
    if (com_util_stat(&st, dir) == 0)
    {
        return 0;
    }

    /* 存在しないので生成する */
    if (com_util_mkdir(dir) == 0)
    {
        return 0;
    }

    /* mkdir 失敗: 競合生成の可能性があるため再確認する */
    if (com_util_stat(&st, dir) == 0)
    {
        return 0;
    }

    return -1;
}

#if defined(PLATFORM_WINDOWS)
static int is_ascii_alpha(const char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return 1;
    }
    if (ch >= 'a' && ch <= 'z')
    {
        return 1;
    }
    return 0;
}
#endif /* PLATFORM_WINDOWS */

static size_t path_root_prefix_len(const char *path)
{
#if defined(PLATFORM_WINDOWS)
    if (is_ascii_alpha(path[0]) && path[1] == ':' && path[2] == PLATFORM_PATH_SEP_CHR)
    {
        return 3u;
    }

    if (path[0] == PLATFORM_PATH_SEP_CHR && path[1] == PLATFORM_PATH_SEP_CHR)
    {
        size_t i = 2u;

        while (path[i] != '\0' && path[i] != PLATFORM_PATH_SEP_CHR)
        {
            i++;
        }
        if (path[i] == '\0')
        {
            return i;
        }
        while (path[i] == PLATFORM_PATH_SEP_CHR)
        {
            i++;
        }
        if (path[i] == '\0')
        {
            return i;
        }
        while (path[i] != '\0' && path[i] != PLATFORM_PATH_SEP_CHR)
        {
            i++;
        }
        return i;
    }
#endif /* PLATFORM_WINDOWS */

    if (path[0] == PLATFORM_PATH_SEP_CHR)
    {
        return 1u;
    }

    return 0u;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_makedirs(const char *path)
{
    char buf[PLATFORM_PATH_MAX];
    size_t path_len;
    size_t root_len;
    size_t i;

    if (path == NULL || path[0] == '\0')
    {
        return -1;
    }

    path_len = strlen(path);
    if (path_len >= (size_t)PLATFORM_PATH_MAX)
    {
        return -1;
    }

    /* パスをローカル バッファーに複製する */
    memcpy(buf, path, path_len + 1);

#if defined(PLATFORM_WINDOWS)
    com_util_normalize_path_sep(buf);
#endif /* PLATFORM_WINDOWS */

    root_len = path_root_prefix_len(buf);

    for (i = root_len; i < path_len; i++)
    {
        if (buf[i] == PLATFORM_PATH_SEP_CHR)
        {
            if (i > root_len && buf[i - 1u] != PLATFORM_PATH_SEP_CHR)
            {
                /* 中間ディレクトリを一時終端して生成する */
                buf[i] = '\0';
                if (ensure_one_dir(buf) != 0)
                {
                    return -1;
                }
                buf[i] = PLATFORM_PATH_SEP_CHR;
            }
        }
    }

    /* 末尾要素 (= パス全体) を生成する */
    return ensure_one_dir(buf);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_stat(com_util_file_stat_t *buf, const char *path)
{
    if (buf == NULL || path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return stat(path, buf);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        return _wstat64(wpath, buf);
    }
#endif /* PLATFORM_ */
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_mkdir(const char *path)
{
    if (path == NULL)
    {
        return -1;
    }

#if defined(PLATFORM_LINUX)
    return mkdir(path, 0755);
#elif defined(PLATFORM_WINDOWS)
    {
        wchar_t wpath[PLATFORM_PATH_MAX];

        if (com_util_utf8_to_wpath(wpath, sizeof(wpath) / sizeof(wpath[0]), path) < 0)
        {
            return -1;
        }

        return _wmkdir(wpath);
    }
#endif /* PLATFORM_ */
}
