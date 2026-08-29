#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

HMODULE delegate_real_LoadLibraryU(const char *utf8_file_name)
{
    static auto real_fn =
        reinterpret_cast<decltype(&LoadLibraryU)>(resolveSharedSymbolOrExit(kLibCplatName, "LoadLibraryU"));

    return real_fn(utf8_file_name);
}

MOCK_WEAK_IMPL(HMODULE, LoadLibraryU, const char *utf8_file_name)
{
    HMODULE mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->LoadLibraryU(utf8_file_name);
    }
    else
    {
        mock_ret = delegate_real_LoadLibraryU(utf8_file_name);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, (utf8_file_name != nullptr) ? utf8_file_name : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}

#endif /* PLATFORM_WINDOWS */
