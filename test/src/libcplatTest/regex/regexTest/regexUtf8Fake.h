#ifndef REGEX_UTF8_FAKE_H
#define REGEX_UTF8_FAKE_H

enum regex_utf8_fake_mode
{
    REGEX_UTF8_FAKE_REAL = 0,
    REGEX_UTF8_FAKE_RETURN_FALSE,
    REGEX_UTF8_FAKE_THROW_BAD_ALLOC,
    REGEX_UTF8_FAKE_THROW_REGEX_ERROR,
    REGEX_UTF8_FAKE_THROW_INT
};

void test_regex_utf8_set_decode_mode(int mode);
void test_regex_utf8_set_encode_mode(int mode);

#endif /* REGEX_UTF8_FAKE_H */
