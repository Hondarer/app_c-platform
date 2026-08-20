#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

int delegate_real_com_util_utf8_to_wpath(wchar_t *wbuf, size_t wbuf_count, const char *utf8_path)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_utf8_to_wpath)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_utf8_to_wpath"));

    return real_fn(wbuf, wbuf_count, utf8_path);
}

MOCK_WEAK_IMPL(int, com_util_utf8_to_wpath, wchar_t *wbuf, size_t wbuf_count, const char *utf8_path)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_utf8_to_wpath(wbuf, wbuf_count, utf8_path);
    }
    else
    {
        return delegate_real_com_util_utf8_to_wpath(wbuf, wbuf_count, utf8_path);
    }
}

int delegate_real_com_util_wpath_to_utf8(char *dest, size_t dest_size, const wchar_t *wpath)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_wpath_to_utf8)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_wpath_to_utf8"));

    return real_fn(dest, dest_size, wpath);
}

MOCK_WEAK_IMPL(int, com_util_wpath_to_utf8, char *dest, size_t dest_size, const wchar_t *wpath)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_wpath_to_utf8(dest, dest_size, wpath);
    }
    else
    {
        return delegate_real_com_util_wpath_to_utf8(dest, dest_size, wpath);
    }
}

wchar_t *delegate_real_com_util_utf8_to_wstr_alloc(const char *utf8_text)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_utf8_to_wstr_alloc)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_utf8_to_wstr_alloc"));

    return real_fn(utf8_text);
}

MOCK_WEAK_IMPL(wchar_t *, com_util_utf8_to_wstr_alloc, const char *utf8_text)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_utf8_to_wstr_alloc(utf8_text);
    }
    else
    {
        return delegate_real_com_util_utf8_to_wstr_alloc(utf8_text);
    }
}

char *delegate_real_com_util_wstr_to_utf8_alloc(const wchar_t *wtext)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_wstr_to_utf8_alloc)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_wstr_to_utf8_alloc"));

    return real_fn(wtext);
}

MOCK_WEAK_IMPL(char *, com_util_wstr_to_utf8_alloc, const wchar_t *wtext)
{
    if (_mock_com_util != nullptr)
    {
        return _mock_com_util->com_util_wstr_to_utf8_alloc(wtext);
    }
    else
    {
        return delegate_real_com_util_wstr_to_utf8_alloc(wtext);
    }
}

#endif /* PLATFORM_WINDOWS */
