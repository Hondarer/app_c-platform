#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vsnprintf(char *dest, size_t dest_size, const char *format, va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vsnprintf)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vsnprintf"));

    return real_fn(dest, dest_size, format, args);
}

MOCK_WEAK_IMPL(int, com_util_vsnprintf, char *dest, size_t dest_size, const char *format, va_list args)
{
    int rtc = -1;

    char buf[1024];
    {
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(buf, sizeof(buf), format, args_copy);
        va_end(args_copy);
    }

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_vsnprintf(dest, dest_size, buf);
    }
    else
    {
        rtc = delegate_real_com_util_vsnprintf(dest, dest_size, format, args);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, buf);
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
