#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

HMODULE delegate_real_LoadLibraryU(const char *utf8_file_name)
{
    static auto real_fn =
        reinterpret_cast<decltype(&LoadLibraryU)>(resolveSharedSymbolOrExit(kLibComUtilName, "LoadLibraryU"));

    return real_fn(utf8_file_name);
}

MOCK_WEAK_IMPL(HMODULE, LoadLibraryU, const char *utf8_file_name)
{
    HMODULE rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->LoadLibraryU(utf8_file_name);
    }
    else
    {
        rtc = delegate_real_LoadLibraryU(utf8_file_name);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s", __func__, (utf8_file_name != nullptr) ? utf8_file_name : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
