#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <testfw.h>
#include <mock_com_util.h>

std::vector<char> mock_com_util_expand_format(const char *format, va_list args)
{
    char *text;
    std::vector<char> buf;

    if (format == nullptr)
    {
        return std::vector<char>(1u, '\0');
    }

    /* 必要長の算出と確保は testfw の allocvprintf に委ねる (mock_libc の書式系モックと同じ方式) */
    text = allocvprintf(format, args);
    if (text == nullptr)
    {
        return std::vector<char>(1u, '\0');
    }

    buf.assign(text, text + strlen(text) + 1u);
    free(text);

    return buf;
}
