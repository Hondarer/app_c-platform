/* テスト対象ソース ファイルの注入用追加ソース
 * このソースはテスト対象ソースの末尾に結合されます
 * この static 関数へのアクセサーによって
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#include "regex.inject.h"

#ifdef _IN_TEST_SRC

namespace
{
unsigned int lookup_classname(const wchar_t *name, bool icase)
{
    regex_traits traits;
    const std::wstring value(name);
    return traits.lookup_classname(value.cbegin(), value.cend(), icase);
}

bool isctype(wchar_t target, unsigned int mask)
{
    regex_traits traits;
    return traits.isctype(target, mask);
}

int value(wchar_t target, int radix)
{
    regex_traits traits;
    return traits.value(target, radix);
}

wchar_t translate_nocase(wchar_t target)
{
    regex_traits traits;
    return traits.translate_nocase(target);
}

std::wstring transform(const wchar_t *text)
{
    regex_traits traits;
    const std::wstring value(text);
    return traits.transform(value.cbegin(), value.cend());
}

wchar_t translate(wchar_t target)
{
    regex_traits traits;
    return traits.translate(target);
}

std::wstring transform_primary(const wchar_t *text)
{
    regex_traits traits;
    const std::wstring value(text);
    return traits.transform_primary(value.cbegin(), value.cend());
}

std::wstring lookup_collatename(const wchar_t *text)
{
    regex_traits traits;
    const std::wstring value(text);
    return traits.lookup_collatename(value.cbegin(), value.cend());
}

std::string imbue(const wchar_t *text)
{
    (void)text;
    regex_traits traits;
    return traits.imbue(std::locale::classic()).name();
}

bool getloc_is_classic(void)
{
    regex_traits traits;
    return traits.getloc() == std::locale::classic();
}

    #if !defined(COM_UTIL_REGEX_NO_EXCEPTIONS)
int translate_error(std::regex_constants::error_type code)
{
    try
    {
        throw std::regex_error(code);
    }
    catch (...)
    {
        return translate_exception(nullptr);
    }
}

int translate_bad_alloc(void)
{
    try
    {
        throw std::bad_alloc();
    }
    catch (...)
    {
        return translate_exception(nullptr);
    }
}

int translate_unknown(void)
{
    try
    {
        throw 1;
    }
    catch (...)
    {
        return translate_exception(nullptr);
    }
}
    #endif /* !COM_UTIL_REGEX_NO_EXCEPTIONS */
} // namespace

unsigned int test_regex_lookup_classname(const wchar_t *name, bool icase)
{
    return lookup_classname(name, icase);
}

bool test_regex_isctype(wchar_t target, unsigned int mask)
{
    return isctype(target, mask);
}

int test_regex_value(wchar_t target, int radix)
{
    return value(target, radix);
}

wchar_t test_regex_translate_nocase(wchar_t target)
{
    return translate_nocase(target);
}

wchar_t test_regex_translate(wchar_t target)
{
    return translate(target);
}

std::wstring test_regex_transform(const wchar_t *text)
{
    return transform(text);
}

std::wstring test_regex_transform_primary(const wchar_t *text)
{
    return transform_primary(text);
}

std::wstring test_regex_lookup_collatename(const wchar_t *text)
{
    return lookup_collatename(text);
}

std::string test_regex_imbue(const wchar_t *text)
{
    return imbue(text);
}

bool test_regex_getloc_is_classic(void)
{
    return getloc_is_classic();
}

    #if !defined(COM_UTIL_REGEX_NO_EXCEPTIONS)
int test_regex_translate_error(std::regex_constants::error_type code)
{
    return translate_error(code);
}

int test_regex_translate_bad_alloc(void)
{
    return translate_bad_alloc();
}

int test_regex_translate_unknown(void)
{
    return translate_unknown();
}
    #endif /* !COM_UTIL_REGEX_NO_EXCEPTIONS */

void test_regex_iter_set_position(com_util_regex_iter *iter, std::size_t position)
{
    iter->position = position;
}

bool test_regex_find_next_rejects_position_past_end(const com_util_regex *regex)
{
    match_type matched;
    std::size_t begin_index = 0;
    std::size_t end_index = 0;
    const std::wstring units;

    return find_next(regex, units, 0U, 1U, matched, begin_index, end_index);
}

bool test_regex_to_syntax_option(unsigned int flags)
{
    std::regex_constants::syntax_option_type option = std::regex_constants::ECMAScript;

    return to_syntax_option(flags, option);
}

std::size_t test_regex_advance_position(const wchar_t *text, std::size_t position)
{
    const std::wstring units(text);

    return advance_position(units, position);
}

#endif /* _IN_TEST_SRC */
