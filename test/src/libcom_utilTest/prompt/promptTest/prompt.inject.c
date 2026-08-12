/* prompt.c の static 関数へテスト用アクセサーを追加します。 */
#ifndef _IN_TEST_SRC
    #include "prompt.c"
#endif /* _IN_TEST_SRC */

#include "prompt.inject.h"

void test_prompt_history_add(com_util_prompt *prompt, com_util_prompt_ctx *context, const char *line)
{
    history_add(prompt, context, line);
}

void test_prompt_history_prev(com_util_prompt *prompt, com_util_prompt_ctx *context, const char *prompt_string)
{
    history_browse_prev(prompt, context, prompt_string);
}

void test_prompt_history_next(com_util_prompt *prompt, com_util_prompt_ctx *context, const char *prompt_string)
{
    history_browse_next(prompt, context, prompt_string);
}

com_util_prompt_ctx *test_prompt_find_or_create_context(com_util_prompt *prompt, const char *file, int line)
{
    return find_or_create_ctx(prompt, file, line);
}
