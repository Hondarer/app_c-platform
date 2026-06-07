#include <com_util/crt/sys/stat.h>
#include <com_util/crt/path.h>

#include <com_util/crt/crt_internal.h>

#include <stddef.h>
#include <string.h>

#if defined(PLATFORM_LINUX)
    #include <sys/stat.h>
#elif defined(PLATFORM_WINDOWS)
    #include <direct.h>
#endif /* PLATFORM_ */

/**
 *  @brief  指定されたディレクトリが存在することを確認し、なければ生成する。
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_makedirs(const char *path)
{
    char buf[PLATFORM_PATH_MAX];
    size_t path_len;
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

    /* i=1 から開始して先頭 '/' (ルート) や空成分を誤処理しない */
    for (i = 1; i < path_len; i++)
    {
        if (buf[i] == PLATFORM_PATH_SEP_CHR)
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

    /* 末尾要素 (= パス全体) を生成する */
    return ensure_one_dir(buf);
}

COM_UTIL_EXPORT int COM_UTIL_API com_util_stat(com_util_file_stat_t *buf,
                                                const char       *path)
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

COM_UTIL_EXPORT int COM_UTIL_API com_util_mkdir(const char *path)
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
