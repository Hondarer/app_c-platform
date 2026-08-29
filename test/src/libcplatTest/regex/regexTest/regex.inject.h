/* テスト対象ソース ファイルの注入用追加ヘッダー
 * このヘッダーをテスト プログラムが参照することで
 * テスト プログラムからテスト対象ソースの static 関数にアクセスできます
 */
#ifndef CPLAT_REGEX_TEST_INJECT_H
#define CPLAT_REGEX_TEST_INJECT_H

#include <cstddef>
#include <regex>
#include <string>

#include <cplat/regex/regex.h>

unsigned int test_regex_lookup_classname(const wchar_t *name, bool icase);
bool test_regex_isctype(wchar_t target, unsigned int mask);
int test_regex_value(wchar_t target, int radix);
wchar_t test_regex_translate_nocase(wchar_t target);
wchar_t test_regex_translate(wchar_t target);
std::wstring test_regex_transform(const wchar_t *text);
std::wstring test_regex_transform_primary(const wchar_t *text);
std::wstring test_regex_lookup_collatename(const wchar_t *text);
std::string test_regex_imbue(const wchar_t *text);
bool test_regex_getloc_is_classic(void);
#if !defined(CPLAT_REGEX_NO_EXCEPTIONS)
int test_regex_translate_error(std::regex_constants::error_type code);
int test_regex_translate_bad_alloc(void);
int test_regex_translate_unknown(void);
#endif /* !CPLAT_REGEX_NO_EXCEPTIONS */
void test_regex_iter_set_position(cplat_regex_iter *iter, std::size_t position);
bool test_regex_find_next_rejects_position_past_end(const cplat_regex *regex);
bool test_regex_to_syntax_option(unsigned int flags);
std::size_t test_regex_advance_position(const wchar_t *text, std::size_t position);

#endif /* CPLAT_REGEX_TEST_INJECT_H */
