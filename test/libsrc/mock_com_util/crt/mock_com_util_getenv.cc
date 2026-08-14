#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_getenv(const char *name, char *buf, size_t buf_size, int *exists_out,
                                  com_util_error *detail_out)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_getenv)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_getenv"));

    return real_fn(name, buf, buf_size, exists_out, detail_out);
}

MOCK_WEAK_IMPL(int, com_util_getenv, const char *name, char *buf, size_t buf_size, int *exists_out,
               com_util_error *detail_out)
{
    int mock_ret = -1;

    if (_mock_com_util != nullptr)
    {
        mock_ret = _mock_com_util->com_util_getenv(name, buf, buf_size, exists_out, detail_out);
    }
    else
    {
        mock_ret = delegate_real_com_util_getenv(name, buf, buf_size, exists_out, detail_out);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s %s, 0x%p, %zu", __func__, name, (void *)buf, buf_size);
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
