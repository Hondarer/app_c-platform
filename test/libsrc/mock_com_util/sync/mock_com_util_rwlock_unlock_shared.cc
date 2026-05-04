#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_rwlock_unlock_shared(com_util_rwlock_t *rwlock)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_rwlock_unlock_shared)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_rwlock_unlock_shared"));

    return real_fn(rwlock);
}

MOCK_WEAK_IMPL(int, com_util_rwlock_unlock_shared, com_util_rwlock_t *rwlock)
{
    int rtc = 0;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_rwlock_unlock_shared(rwlock);
    }
    else
    {
        rtc = delegate_real_com_util_rwlock_unlock_shared(rwlock);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)rwlock);
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
