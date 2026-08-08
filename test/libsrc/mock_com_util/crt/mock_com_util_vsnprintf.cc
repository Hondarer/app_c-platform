#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <testfw.h>
#include <mock_com_util.h>

int delegate_real_com_util_vsnprintf(char *dest, size_t dest_size, const char *format, va_list args)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_vsnprintf)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_vsnprintf"));

    return real_fn(dest, dest_size, format, args);
}

// 書式を展開した文字列を返す。
// 固定長バッファーで展開すると長い出力が切り詰められ、被テスト側の切り詰め判定が
// 実関数と食い違うため、必要な長さを求めてから確保する。
std::vector<char> mock_com_util_expand_format(const char *format, va_list args)
{
    va_list args_len;
    int needed;

    va_copy(args_len, args);
    needed = vsnprintf(nullptr, 0u, format, args_len);
    va_end(args_len);

    if (needed < 0)
    {
        return std::vector<char>(1u, '\0');
    }

    std::vector<char> buf((size_t)needed + 1u, '\0');
    {
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(buf.data(), buf.size(), format, args_copy);
        va_end(args_copy);
    }

    return buf;
}

MOCK_WEAK_IMPL(int, com_util_vsnprintf, char *dest, size_t dest_size, const char *format, va_list args)
{
    int rtc = -1;

    if (_mock_com_util != nullptr)
    {
        std::vector<char> buf = mock_com_util_expand_format(format, args);

        rtc = _mock_com_util->com_util_vsnprintf(dest, dest_size, buf.data());

        if (getTraceLevel() > TRACE_NONE)
        {
            printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, buf.data());
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

    rtc = delegate_real_com_util_vsnprintf(dest, dest_size, format, args);

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p, %zu, %s", __func__, (void *)dest, dest_size, format);
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
