#include <testfw.h>
#include <mock_cplat.h>

#if defined(PLATFORM_WINDOWS)

BOOL delegate_real_WriteConsoleU(HANDLE console, const char *utf8_text, DWORD utf8_length, DWORD *written_length,
                                 void *reserved)
{
    static auto real_fn =
        reinterpret_cast<decltype(&WriteConsoleU)>(resolveSharedSymbolOrExit(kLibCplatName, "WriteConsoleU"));

    return real_fn(console, utf8_text, utf8_length, written_length, reserved);
}

MOCK_WEAK_IMPL(BOOL, WriteConsoleU, HANDLE console, const char *utf8_text, DWORD utf8_length, DWORD *written_length,
               void *reserved)
{
    BOOL mock_ret;

    if (_mock_cplat != nullptr)
    {
        mock_ret = _mock_cplat->WriteConsoleU(console, utf8_text, utf8_length, written_length, reserved);
    }
    else
    {
        mock_ret = delegate_real_WriteConsoleU(console, utf8_text, utf8_length, written_length, reserved);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %s, %lu, 0x%p, 0x%p", __func__, (void *)console,
               (utf8_text != nullptr) ? utf8_text : "(null)", utf8_length, (void *)written_length, reserved);
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
