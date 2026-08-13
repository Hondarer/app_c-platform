#include "regexUtf8Fake.h"

#include <new>
#include <regex>
#include <stdexcept>

namespace
{
int s_decode_mode = REGEX_UTF8_FAKE_REAL;
int s_encode_mode = REGEX_UTF8_FAKE_REAL;
} // namespace

#define utf8_decode utf8_decode_real
#define utf8_encode utf8_encode_real
#include "../../../../../prod/libsrc/com_util/regex/regex_utf8.cc"
#undef utf8_decode
#undef utf8_encode

void test_regex_utf8_set_decode_mode(const int mode)
{
    s_decode_mode = mode;
}

void test_regex_utf8_set_encode_mode(const int mode)
{
    s_encode_mode = mode;
}

namespace
{

void apply_fake_mode(const int mode)
{
    if (mode == REGEX_UTF8_FAKE_THROW_BAD_ALLOC)
    {
        throw std::bad_alloc();
    }
    if (mode == REGEX_UTF8_FAKE_THROW_REGEX_ERROR)
    {
        throw std::regex_error(std::regex_constants::error_stack);
    }
    if (mode == REGEX_UTF8_FAKE_THROW_INT)
    {
        throw 1;
    }
}

} // namespace

namespace com_util
{
namespace regex_detail
{

bool utf8_decode(const char *text, std::size_t text_len, std::wstring &units_out, std::vector<std::size_t> &offsets_out)
{
    apply_fake_mode(s_decode_mode);
    if (s_decode_mode == REGEX_UTF8_FAKE_RETURN_FALSE)
    {
        return false;
    }
    return utf8_decode_real(text, text_len, units_out, offsets_out);
}

bool utf8_encode(const std::wstring &units, std::string &text_out)
{
    apply_fake_mode(s_encode_mode);
    if (s_encode_mode == REGEX_UTF8_FAKE_RETURN_FALSE)
    {
        return false;
    }
    return utf8_encode_real(units, text_out);
}

} /* namespace regex_detail */
} /* namespace com_util */
