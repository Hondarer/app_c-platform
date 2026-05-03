#include <testfw.h>
#include <mock_com_util.h>

void delegate_real_com_util_call_once(com_util_once_flag_t *flag, com_util_once_func_t func)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_call_once)>(
            resolveSharedSymbolOrExit(kLibComUtilName, "com_util_call_once"));

    real_fn(flag, func);
}

WEAK_ATR void com_util_call_once(com_util_once_flag_t *flag, com_util_once_func_t func)
{
    if (_mock_com_util != nullptr)
    {
        _mock_com_util->com_util_call_once(flag, func);
    }
    else
    {
        delegate_real_com_util_call_once(flag, func);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (void *)func);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            const int32_t state = (flag != nullptr) ? flag->state : -1;
            printf(" -> %d\n", state);
        }
        else
        {
            printf("\n");
        }
    }
}
