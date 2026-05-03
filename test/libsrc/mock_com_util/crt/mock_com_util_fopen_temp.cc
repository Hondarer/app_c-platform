#include <testfw.h>
#include <mock_com_util.h>

FILE *delegate_real_com_util_fopen_temp(const char *prefix,
                                    const char *modes,
                                    char       *path_out,
                                    size_t      path_size,
                                    int        *errno_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_fopen_temp)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_fopen_temp"));

    return real_fn(prefix, modes, path_out, path_size, errno_out);
}

WEAK_ATR FILE *com_util_fopen_temp(const char *prefix,
                                    const char *modes,
                                    char       *path_out,
                                    size_t      path_size,
                                    int        *errno_out)
{
    FILE *fp = nullptr;

    if (_mock_com_util != nullptr)
    {
        fp = _mock_com_util->com_util_fopen_temp(prefix, modes, path_out, path_size, errno_out);
    }
    else
    {
        fp = delegate_real_com_util_fopen_temp(prefix, modes, path_out, path_size, errno_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, %s, 0x%p, %zu, 0x%p",
               __func__,
               (prefix != nullptr) ? prefix : "(null)",
               (modes != nullptr) ? modes : "(null)",
               (void *)path_out,
               path_size,
               (void *)errno_out);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)fp);
        }
        else
        {
            printf("\n");
        }
    }

    return fp;
}
