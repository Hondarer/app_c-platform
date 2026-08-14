#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

BOOL delegate_real_ChangeServiceConfig2U(SC_HANDLE service, DWORD info_level, const char *utf8_text)
{
    static auto real_fn = reinterpret_cast<decltype(&ChangeServiceConfig2U)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "ChangeServiceConfig2U"));

    return real_fn(service, info_level, utf8_text);
}

MOCK_WEAK_IMPL(BOOL, ChangeServiceConfig2U, SC_HANDLE service, DWORD info_level, const char *utf8_text)
{
    BOOL mock_ret;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->ChangeServiceConfig2U(service, info_level, utf8_text);
    }
    else
    {
        mock_ret = delegate_real_ChangeServiceConfig2U(service, info_level, utf8_text);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %lu, %s", __func__, (void *)service, info_level,
               (utf8_text != nullptr) ? utf8_text : "(null)");
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> %d\n", mock_ret);
        }
        else
        {
            printf("\n");
        }
    }

    return mock_ret;
}

#endif /* PLATFORM_WINDOWS */
