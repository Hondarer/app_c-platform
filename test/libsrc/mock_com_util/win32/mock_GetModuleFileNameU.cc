#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

DWORD delegate_real_GetModuleFileNameU(HMODULE module, char *utf8_buf, DWORD size)
{
    static auto real_fn = reinterpret_cast<decltype(&GetModuleFileNameU)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "GetModuleFileNameU"));

    return real_fn(module, utf8_buf, size);
}

MOCK_WEAK_IMPL(DWORD, GetModuleFileNameU, HMODULE module, char *utf8_buf, DWORD size)
{
    DWORD rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->GetModuleFileNameU(module, utf8_buf, size);
    }
    else
    {
        rtc = delegate_real_GetModuleFileNameU(module, utf8_buf, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, 0x%p, %lu", __func__, (void *)module, (void *)utf8_buf, size);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %lu\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
