#include <testfw.h>
#include <mock_com_util.h>

#if defined(PLATFORM_WINDOWS)

HANDLE delegate_real_CreateNamedPipeU(const char *utf8_name, DWORD open_mode, DWORD pipe_mode, DWORD max_instances,
                                      DWORD out_buffer_size, DWORD in_buffer_size, DWORD default_timeout,
                                      LPSECURITY_ATTRIBUTES security_attributes)
{
    static auto real_fn =
        reinterpret_cast<decltype(&CreateNamedPipeU)>(resolveSharedSymbolOrExit(kLibComUtilName, "CreateNamedPipeU"));

    return real_fn(utf8_name, open_mode, pipe_mode, max_instances, out_buffer_size, in_buffer_size, default_timeout,
                   security_attributes);
}

MOCK_WEAK_IMPL(HANDLE, CreateNamedPipeU, const char *utf8_name, DWORD open_mode, DWORD pipe_mode, DWORD max_instances,
               DWORD out_buffer_size, DWORD in_buffer_size, DWORD default_timeout,
               LPSECURITY_ATTRIBUTES security_attributes)
{
    HANDLE rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->CreateNamedPipeU(utf8_name, open_mode, pipe_mode, max_instances, out_buffer_size,
                                               in_buffer_size, default_timeout, security_attributes);
    }
    else
    {
        rtc = delegate_real_CreateNamedPipeU(utf8_name, open_mode, pipe_mode, max_instances, out_buffer_size,
                                             in_buffer_size, default_timeout, security_attributes);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %lu, %lu, %lu, %lu, %lu, %lu, 0x%p", __func__,
               (utf8_name != nullptr) ? utf8_name : "(null)", open_mode, pipe_mode, max_instances, out_buffer_size,
               in_buffer_size, default_timeout, (void *)security_attributes);
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
