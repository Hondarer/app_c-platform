#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_mutex_destroy(com_util_mutex_t *mtx)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_mutex_destroy)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_mutex_destroy"));

    return real_fn(mtx);
}

MOCK_WEAK_IMPL(int, com_util_mutex_destroy, com_util_mutex_t *mtx)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_mutex_destroy(mtx);
    }
    else
    {
        rtc = delegate_real_com_util_mutex_destroy(mtx);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)mtx);
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
