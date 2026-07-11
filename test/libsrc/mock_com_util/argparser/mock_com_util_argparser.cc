#include <testfw.h>
#include <mock_com_util.h>

#define DEFINE_ARGPARSER_RESULT(name, call_args, ...) \
    com_util_argparser_result_t delegate_real_##name(__VA_ARGS__) \
    { \
        static auto real_fn = reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibComUtilName, #name)); \
        return real_fn call_args; \
    } \
    MOCK_WEAK_IMPL(com_util_argparser_result_t, name, __VA_ARGS__) \
    { \
        com_util_argparser_result_t rtc; \
        if (_mock_com_util != nullptr) \
        { \
            rtc = _mock_com_util->name call_args; \
        } \
        else \
        { \
            rtc = delegate_real_##name call_args; \
        } \
        if (getTraceLevel() > TRACE_NONE) \
        { \
            printf("  > %s -> %d\n", __func__, (int)rtc); \
        } \
        return rtc; \
    }

#define DEFINE_ARGPARSER_VOID(name, call_args, ...) \
    void delegate_real_##name(__VA_ARGS__) \
    { \
        static auto real_fn = reinterpret_cast<decltype(&name)>(resolveSharedSymbolOrExit(kLibComUtilName, #name)); \
        real_fn call_args; \
    } \
    MOCK_WEAK_IMPL(void, name, __VA_ARGS__) \
    { \
        if (_mock_com_util != nullptr) \
        { \
            _mock_com_util->name call_args; \
        } \
        else \
        { \
            delegate_real_##name call_args; \
        } \
        if (getTraceLevel() > TRACE_NONE) \
        { \
            printf("  > %s\n", __func__); \
        } \
    }

DEFINE_ARGPARSER_RESULT(com_util_argparser_register_flag, (parser, short_name, long_name, description, storage),
                        com_util_argparser *parser, const char *short_name, const char *long_name,
                        const char *description, int *storage)
DEFINE_ARGPARSER_RESULT(com_util_argparser_register_option_int,
                        (parser, short_name, long_name, value_name, description, flags, storage),
                        com_util_argparser *parser, const char *short_name, const char *long_name,
                        const char *value_name, const char *description, unsigned int flags, int *storage)
DEFINE_ARGPARSER_RESULT(com_util_argparser_register_option_string,
                        (parser, short_name, long_name, value_name, description, flags, storage),
                        com_util_argparser *parser, const char *short_name, const char *long_name,
                        const char *value_name, const char *description, unsigned int flags, const char **storage)
DEFINE_ARGPARSER_RESULT(com_util_argparser_register_option_int_array,
                        (parser, short_name, long_name, value_name, description, flags, storage, capacity, count),
                        com_util_argparser *parser, const char *short_name, const char *long_name,
                        const char *value_name, const char *description, unsigned int flags, int *storage,
                        size_t capacity, size_t *count)
DEFINE_ARGPARSER_RESULT(com_util_argparser_register_option_string_array,
                        (parser, short_name, long_name, value_name, description, flags, storage, capacity, count),
                        com_util_argparser *parser, const char *short_name, const char *long_name,
                        const char *value_name, const char *description, unsigned int flags, const char **storage,
                        size_t capacity, size_t *count)
DEFINE_ARGPARSER_RESULT(com_util_argparser_register_positional_int, (parser, name, description, flags, storage),
                        com_util_argparser *parser, const char *name, const char *description, unsigned int flags,
                        int *storage)
DEFINE_ARGPARSER_RESULT(com_util_argparser_register_positional_string, (parser, name, description, flags, storage),
                        com_util_argparser *parser, const char *name, const char *description, unsigned int flags,
                        const char **storage)
DEFINE_ARGPARSER_RESULT(com_util_argparser_parse, (parser, argc, argv), com_util_argparser *parser, int argc,
                        char *const *argv)
DEFINE_ARGPARSER_RESULT(com_util_argparser_get_error_message, (parser, buffer, buffer_size),
                        const com_util_argparser *parser, char *buffer, size_t buffer_size)
DEFINE_ARGPARSER_RESULT(com_util_argparser_get_usage, (parser, buffer, buffer_size, required_size),
                        const com_util_argparser *parser, char *buffer, size_t buffer_size, size_t *required_size)
DEFINE_ARGPARSER_RESULT(com_util_argparser_print_usage, (parser, stream), const com_util_argparser *parser,
                        FILE *stream)
DEFINE_ARGPARSER_RESULT(com_util_argparser_print_error_messages, (parser, stream), const com_util_argparser *parser,
                        FILE *stream)

DEFINE_ARGPARSER_VOID(com_util_argparser_dispose, (parser), com_util_argparser *parser)

com_util_argparser *delegate_real_com_util_argparser_create(const com_util_argparser_options *options)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_argparser_create)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_argparser_create"));

    return real_fn(options);
}

MOCK_WEAK_IMPL(com_util_argparser *, com_util_argparser_create, const com_util_argparser_options *options)
{
    com_util_argparser *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_argparser_create(options);
    }
    else
    {
        rtc = delegate_real_com_util_argparser_create(options);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)options);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

com_util_argparser *delegate_real_com_util_argparser_default(const com_util_argparser_options *options)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_argparser_default)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_argparser_default"));

    return real_fn(options);
}

MOCK_WEAK_IMPL(com_util_argparser *, com_util_argparser_default, const com_util_argparser_options *options)
{
    com_util_argparser *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_argparser_default(options);
    }
    else
    {
        rtc = delegate_real_com_util_argparser_default(options);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s 0x%p", __func__, (const void *)options);
        if (getTraceLevel() >= TRACE_DETAIL)
        {
            printf(" -> 0x%p\n", (void *)rtc);
        }
        else
        {
            printf("\n");
        }
    }

    return rtc;
}

com_util_argparser_error_t delegate_real_com_util_argparser_get_error(const com_util_argparser *parser)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_argparser_get_error)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_argparser_get_error"));

    return real_fn(parser);
}

MOCK_WEAK_IMPL(com_util_argparser_error_t, com_util_argparser_get_error, const com_util_argparser *parser)
{
    com_util_argparser_error_t rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_argparser_get_error(parser);
    }
    else
    {
        rtc = delegate_real_com_util_argparser_get_error(parser);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s -> %d\n", __func__, (int)rtc);
    }

    return rtc;
}

const char *delegate_real_com_util_argparser_get_error_target(const com_util_argparser *parser)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_argparser_get_error_target)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_argparser_get_error_target"));

    return real_fn(parser);
}

MOCK_WEAK_IMPL(const char *, com_util_argparser_get_error_target, const com_util_argparser *parser)
{
    const char *rtc = nullptr;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_argparser_get_error_target(parser);
    }
    else
    {
        rtc = delegate_real_com_util_argparser_get_error_target(parser);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        const char *trace_result = rtc;
        if (trace_result == nullptr)
        {
            trace_result = "(null)";
        }
        printf("  > %s -> %s\n", __func__, trace_result);
    }

    return rtc;
}

int delegate_real_com_util_argparser_get_error_index(const com_util_argparser *parser)
{
    static auto real_fn = reinterpret_cast<decltype(&com_util_argparser_get_error_index)>(
        resolveSharedSymbolOrExit(kLibComUtilName, "com_util_argparser_get_error_index"));

    return real_fn(parser);
}

MOCK_WEAK_IMPL(int, com_util_argparser_get_error_index, const com_util_argparser *parser)
{
    int rtc;

    if (_mock_com_util != nullptr)
    {
        rtc = _mock_com_util->com_util_argparser_get_error_index(parser);
    }
    else
    {
        rtc = delegate_real_com_util_argparser_get_error_index(parser);
    }

    if (getTraceLevel() > TRACE_NONE)
    {
        printf("  > %s -> %d\n", __func__, rtc);
    }

    return rtc;
}
