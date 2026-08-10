#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

BOOL delegate_real_GetVolumePathNameU(const char *utf8_path, char *utf8_volume_root, DWORD size)
{
    static auto real_fn = reinterpret_cast<decltype(&GetVolumePathNameU)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "GetVolumePathNameU"));

    return real_fn(utf8_path, utf8_volume_root, size);
}

MOCK_WEAK_IMPL(BOOL, GetVolumePathNameU, const char *utf8_path, char *utf8_volume_root, DWORD size)
{
    BOOL rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->GetVolumePathNameU(utf8_path, utf8_volume_root, size);
    }
    else
    {
        rtc = delegate_real_GetVolumePathNameU(utf8_path, utf8_volume_root, size);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, %lu", __func__, (utf8_path != nullptr) ? utf8_path : "(null)",
               (void *)utf8_volume_root, size);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

#endif /* PLATFORM_WINDOWS */
