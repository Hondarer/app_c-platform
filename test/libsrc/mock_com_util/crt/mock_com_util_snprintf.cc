#include <stdarg.h>
#include <stdio.h>
#include <testfw.h>
#include <mock_com_util.h>

#include <vector>

int delegate_real_com_util_snprintf(char *buf, size_t buf_size, const char *format, ...)
{
    static auto real_fn =
        reinterpret_cast<decltype(&com_util_snprintf)>(resolveSharedSymbolOrExit(kLibComUtilName, "com_util_snprintf"));

    return real_fn(buf, buf_size, "%s", format);
}

MOCK_WEAK_IMPL(int, com_util_snprintf, char *buf, size_t buf_size, const char *format, ...)
{
    int rtc;
    va_list args;

    va_start(args, format);
    if (_mock_com_util != nullptr)
    {
        /* MOCK_METHOD へは整形済み文字列を渡す。固定長バッファーでは切り詰めにより */
        /* 戻り値が実際の必要文字数と食い違うため、必要長を求めてから確保する。      */
        std::vector<char> fmt_buf;
        va_list len_args;
        int needed;

        va_copy(len_args, args);
        needed = vsnprintf(nullptr, 0U, format, len_args);
        va_end(len_args);

        if (needed < 0)
        {
            va_end(args);
            return needed;
        }

        fmt_buf.resize(static_cast<size_t>(needed) + 1U);
        {
            va_list fmt_args;

            va_copy(fmt_args, args);
            vsnprintf(fmt_buf.data(), fmt_buf.size(), format, fmt_args);
            va_end(fmt_args);
        }

        rtc = _mock_com_util->com_util_snprintf(buf, buf_size, fmt_buf.data());
    }
    else
    {
        /* 未注入時は引数をそのまま転送し、元の snprintf の意味を保存する。 */
        rtc = delegate_real_com_util_vsnprintf(buf, buf_size, format, args);
    }
    va_end(args);

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)buf, buf_size, format);
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
