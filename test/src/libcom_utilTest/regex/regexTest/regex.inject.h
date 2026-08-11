#ifndef COM_UTIL_REGEX_TEST_INJECT_H
#define COM_UTIL_REGEX_TEST_INJECT_H

#include <regex>
#include <string>

unsigned int test_regex_lookup_classname(const wchar_t *name, bool icase);
bool test_regex_isctype(wchar_t target, unsigned int mask);
int test_regex_value(wchar_t target, int radix);
wchar_t test_regex_translate_nocase(wchar_t target);
std::wstring test_regex_transform(const wchar_t *text);
std::wstring test_regex_transform_primary(const wchar_t *text);
std::wstring test_regex_lookup_collatename(const wchar_t *text);
std::string test_regex_imbue(const wchar_t *text);
bool test_regex_getloc_is_classic(void);
int test_regex_translate_error(std::regex_constants::error_type code);
int test_regex_translate_bad_alloc(void);
int test_regex_translate_unknown(void);

#endif /* COM_UTIL_REGEX_TEST_INJECT_H */
