/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 関数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef _IN_TEST_SRC
    #include "argparser.c"
#endif /* _IN_TEST_SRC */

#include "argparser.inject.h"

int test_argparser_is_valid_short_name(const char *name)
{
    return argparser_is_valid_short_name(name);
}

int test_argparser_is_valid_long_name(const char *name)
{
    return argparser_is_valid_long_name(name);
}

void test_argparser_default_dispose_on_shutdown(const com_util_shutdown_event *event, void *context)
{
    argparser_default_dispose_on_shutdown(event, context);
}

com_util_argparser *test_argparser_default_acquire(const com_util_argparser_options *options, int reset_existing)
{
    return argparser_default_acquire(options, reset_existing);
}

void test_argparser_reset_default(void)
{
    if (s_default_parser != NULL)
    {
        argparser_dispose_core(s_default_parser);
        s_default_parser = NULL;
    }
    if (s_default_lock != NULL)
    {
        com_util_local_lock_dispose(s_default_lock);
        s_default_lock = NULL;
    }
    s_default_initialize_once.state = 0;
}

char *test_argparser_replace_program_description(com_util_argparser *parser, char *description)
{
    char *previous = parser->program_description;
    parser->program_description = description;
    return previous;
}

void test_argparser_set_register_error_result(com_util_argparser *parser, size_t index, int result)
{
    parser->register_errors[index].result = result;
}

void test_argparser_set_last_error(com_util_argparser *parser, int result)
{
    parser->last_error = result;
    parser->last_error_target = NULL;
}

void test_argparser_clear_register_error_target(com_util_argparser *parser, size_t index)
{
    parser->register_errors[index].target = NULL;
}

void test_argparser_record_register_result(com_util_argparser *parser, int result)
{
    argparser_record_register_result(parser, result, NULL, NULL);
}

void test_argparser_replace_short_name(com_util_argparser *parser, size_t index, const char *name)
{
    free(parser->specs[index].short_name);
    parser->specs[index].short_name = argparser_strdup(name);
}
