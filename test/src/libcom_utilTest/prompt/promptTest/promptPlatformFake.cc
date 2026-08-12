// promptTest 専用のプラットフォーム層 fake
// prompt.c が呼ぶ prompt_platform_* を差し替え、実機の端末なしにキー入力を再現します
#include <com_util/prompt/prompt_internal.h>

#include <cstddef>
#include <string>
#include <vector>

#include "promptPlatformFake.h"

namespace
{

std::vector<int> g_input;
size_t g_pos = 0u;
int g_enter_raw = 0;
int g_leave_raw = 0;

int next_byte()
{
    if (g_pos >= g_input.size())
    {
        return -1;
    }

    return g_input[g_pos++];
}

} // namespace

void promptFakeSetInput(const std::string &bytes)
{
    g_input.clear();
    for (char byte : bytes)
    {
        g_input.push_back((int)(unsigned char)byte);
    }
    g_pos = 0u;
}

void promptFakeSetResults(const std::vector<int> &results)
{
    g_input = results;
    g_pos = 0u;
}

int promptFakeEnterRawCount(void)
{
    return g_enter_raw;
}

int promptFakeLeaveRawCount(void)
{
    return g_leave_raw;
}

void promptFakeReset(void)
{
    g_input.clear();
    g_pos = 0u;
    g_enter_raw = 0;
    g_leave_raw = 0;
}

extern "C"
{

    void prompt_platform_enter_raw(com_util_prompt *p)
    {
        (void)p;
        g_enter_raw++;
    }

    void prompt_platform_leave_raw(com_util_prompt *p)
    {
        (void)p;
        g_leave_raw++;
    }

    int prompt_platform_read_char(com_util_prompt *p)
    {
        (void)p;
        return next_byte();
    }

    int prompt_platform_read_char_nb(com_util_prompt *p)
    {
        (void)p;
        return next_byte();
    }
}
