/**
 *******************************************************************************
 *  @file           regex.cc
 *  @brief          UTF-8 文字列に対する正規表現の照合 API を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/08/04
 *  @version        1.0.0
 *
 *  照合エンジンには C++ 標準ライブラリの `std::basic_regex` を使用し、
 *  文字型を `wchar_t`、文字特性を本ファイル内の `regex_traits` に固定します。\n
 *  標準の `std::regex_traits` はロケール (`ctype` / `collate` ファセット) に
 *  依存し、環境設定によって照合結果が変わるため使用しません。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#include <cerrno>
#include <cstddef>
#include <locale>
#include <new>
#include <regex>
#include <string>
#include <vector>

#include <com_util/base/compiler.h>
#include <com_util/base/error.h>
#include <com_util/base/error_internal.h>
#include <com_util/base/result.h>
#include <com_util/regex/regex_utf8.h>

/*
 * 本ファイルは -fvisibility=hidden でコンパイルする (同ディレクトリの makepart.mk)。
 * C++ 標準ライブラリのテンプレートが本ファイルで実体化されると、既定では共有
 * ライブラリの動的シンボル表へ現れてしまい、com_util の公開 ABI (C 関数のみ) を
 * 逸脱するためである。
 * 公開 API の宣言だけは既定可視性に戻す必要があるため、本ヘッダーの取り込みを
 * visibility push(default) で囲む。定義側の可視性は宣言から引き継がれる。
 * see: https://gcc.gnu.org/onlinedocs/gcc-8.5.0/gcc/Visibility-Pragmas.html
 */
#if defined(COMPILER_GCC)
    #pragma GCC visibility push(default)
#endif /* COMPILER_GCC */

#include <com_util/regex/regex.h>

#if defined(COMPILER_GCC)
    #pragma GCC visibility pop
#endif /* COMPILER_GCC */

namespace
{

using com_util::regex_detail::index_of_offset;
using com_util::regex_detail::offset_of_begin;
using com_util::regex_detail::offset_of_end;
using com_util::regex_detail::utf8_decode;
using com_util::regex_detail::utf8_encode;

/** 文字クラスを表すビット値。ASCII 範囲の定義のみを扱う。 */
enum char_class_bit : unsigned int
{
    CLASS_ALNUM = 0x0001U,
    CLASS_ALPHA = 0x0002U,
    CLASS_BLANK = 0x0004U,
    CLASS_CNTRL = 0x0008U,
    CLASS_DIGIT = 0x0010U,
    CLASS_GRAPH = 0x0020U,
    CLASS_LOWER = 0x0040U,
    CLASS_PRINT = 0x0080U,
    CLASS_PUNCT = 0x0100U,
    CLASS_SPACE = 0x0200U,
    CLASS_UPPER = 0x0400U,
    CLASS_XDIGIT = 0x0800U,
    CLASS_WORD = 0x1000U
};

/**
 *  ロケール非依存かつ ASCII 定義の文字特性クラス。
 *
 *  `getloc()` が返す classic ロケールは、libstdc++ の内部実装がパターン走査時に
 *  `narrow()` と `is(digit)` を呼ぶためだけに使用される。
 *  文字の分類と大小の畳み込みは本クラスが行うため、照合結果はロケールに依存しない。
 */
class regex_traits
{
  public:
    typedef wchar_t char_type;
    typedef std::wstring string_type;
    typedef std::size_t size_type;
    typedef std::locale locale_type;
    typedef unsigned int char_class_type;

    static size_type length(const char_type *text)
    {
        return std::char_traits<wchar_t>::length(text);
    }

    char_type translate(char_type target) const
    {
        return target;
    }

/*
 * libstdc++ の _Compiler::_M_insert_bracket_matcher() は、未初期化の
 * _BracketState を宣言してから set() で値を設定する構造になっている
 * (/usr/include/c++/8/bits/regex_compiler.tcc:429)。
 * GCC 8 の最適化器は、本関数をそこへインライン展開した際に、set() 前の経路が
 * あり得ると誤判定して -Wmaybe-uninitialized を報告する。
 * 未初期化値が実際に読まれることはなく、標準ヘッダー側の構造に起因するため、
 * 本関数に限って警告を抑制する。
 * see: https://gcc.gnu.org/onlinedocs/gcc-8.5.0/gcc/Warning-Options.html#index-Wmaybe-uninitialized
 */
#if defined(COMPILER_GCC)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif /* COMPILER_GCC */
    char_type translate_nocase(char_type target) const
    {
        if ((target >= L'A') && (target <= L'Z'))
        {
            return static_cast<char_type>(target + (L'a' - L'A'));
        }
        return target;
    }
#if defined(COMPILER_GCC)
    #pragma GCC diagnostic pop
#endif /* COMPILER_GCC */

    template <typename ForwardIt> string_type transform(ForwardIt first, ForwardIt last) const
    {
        return string_type(first, last);
    }

    template <typename ForwardIt> string_type transform_primary(ForwardIt first, ForwardIt last) const
    {
        string_type result(first, last);
        for (string_type::iterator it = result.begin(); it != result.end(); ++it)
        {
            *it = translate_nocase(*it);
        }
        return result;
    }

    template <typename ForwardIt> string_type lookup_collatename(ForwardIt, ForwardIt) const
    {
        /* 照合要素 [[.x.]] はサポートしない。 */
        return string_type();
    }

    template <typename ForwardIt>
    char_class_type lookup_classname(ForwardIt first, ForwardIt last, bool icase = false) const
    {
        std::string name;
        for (ForwardIt it = first; it != last; ++it)
        {
            const wchar_t target = *it;
            if (target < 128)
            {
                char narrowed = static_cast<char>(target);
                if ((narrowed >= 'A') && (narrowed <= 'Z'))
                {
                    narrowed = static_cast<char>(narrowed + ('a' - 'A'));
                }
                name.push_back(narrowed);
            }
        }

        char_class_type mask = 0;
        if (name == "alnum")
        {
            mask = CLASS_ALNUM;
        }
        else if (name == "alpha")
        {
            mask = CLASS_ALPHA;
        }
        else if (name == "blank")
        {
            mask = CLASS_BLANK;
        }
        else if (name == "cntrl")
        {
            mask = CLASS_CNTRL;
        }
        else if ((name == "digit") || (name == "d"))
        {
            mask = CLASS_DIGIT;
        }
        else if (name == "graph")
        {
            mask = CLASS_GRAPH;
        }
        else if (name == "lower")
        {
            mask = CLASS_LOWER;
        }
        else if (name == "print")
        {
            mask = CLASS_PRINT;
        }
        else if (name == "punct")
        {
            mask = CLASS_PUNCT;
        }
        else if ((name == "space") || (name == "s"))
        {
            mask = CLASS_SPACE;
        }
        else if (name == "upper")
        {
            mask = CLASS_UPPER;
        }
        else if (name == "xdigit")
        {
            mask = CLASS_XDIGIT;
        }
        else if (name == "w")
        {
            mask = CLASS_WORD;
        }
        else
        {
            mask = 0;
        }

        if (icase && ((mask == CLASS_LOWER) || (mask == CLASS_UPPER)))
        {
            mask = CLASS_ALPHA;
        }
        return mask;
    }

    bool isctype(char_type target, char_class_type mask) const
    {
        if (target >= 128)
        {
            /* 文字クラスは ASCII 定義に限る。 */
            return false;
        }

        const bool is_upper = (target >= L'A') && (target <= L'Z');
        const bool is_lower = (target >= L'a') && (target <= L'z');
        const bool is_digit = (target >= L'0') && (target <= L'9');
        const bool is_space = (target == L' ') || ((target >= 0x09) && (target <= 0x0D));
        const bool is_blank = (target == L' ') || (target == L'\t');
        const bool is_cntrl = (target < 0x20) || (target == 0x7F);
        const bool is_alpha = is_upper || is_lower;
        const bool is_alnum = is_alpha || is_digit;
        const bool is_graph = (target > 0x20) && (target < 0x7F);
        const bool is_print = (target >= 0x20) && (target < 0x7F);
        const bool is_punct = is_graph && !is_alnum;
        const bool is_xdigit =
            is_digit || ((target >= L'a') && (target <= L'f')) || ((target >= L'A') && (target <= L'F'));
        const bool is_word = is_alnum || (target == L'_');

        unsigned int actual = 0;
        if (is_upper)
        {
            actual |= CLASS_UPPER;
        }
        if (is_lower)
        {
            actual |= CLASS_LOWER;
        }
        if (is_digit)
        {
            actual |= CLASS_DIGIT;
        }
        if (is_space)
        {
            actual |= CLASS_SPACE;
        }
        if (is_blank)
        {
            actual |= CLASS_BLANK;
        }
        if (is_cntrl)
        {
            actual |= CLASS_CNTRL;
        }
        if (is_alpha)
        {
            actual |= CLASS_ALPHA;
        }
        if (is_alnum)
        {
            actual |= CLASS_ALNUM;
        }
        if (is_graph)
        {
            actual |= CLASS_GRAPH;
        }
        if (is_print)
        {
            actual |= CLASS_PRINT;
        }
        if (is_punct)
        {
            actual |= CLASS_PUNCT;
        }
        if (is_xdigit)
        {
            actual |= CLASS_XDIGIT;
        }
        if (is_word)
        {
            actual |= CLASS_WORD;
        }

        return (actual & mask) != 0;
    }

    int value(char_type target, int radix) const
    {
        int digit = -1;
        if ((target >= L'0') && (target <= L'9'))
        {
            digit = static_cast<int>(target - L'0');
        }
        else if ((target >= L'a') && (target <= L'f'))
        {
            digit = static_cast<int>(target - L'a') + 10;
        }
        else if ((target >= L'A') && (target <= L'F'))
        {
            digit = static_cast<int>(target - L'A') + 10;
        }
        else
        {
            digit = -1;
        }

        if ((digit < 0) || (digit >= radix))
        {
            return -1;
        }
        return digit;
    }

    locale_type imbue(locale_type target)
    {
        return target;
    }

    locale_type getloc() const
    {
        return std::locale::classic();
    }
};

typedef std::basic_regex<wchar_t, regex_traits> engine_type;
typedef std::match_results<std::wstring::const_iterator> match_type;

/** コンパイル フラグとして受け付けるビットの和。 */
const unsigned int COMPILE_FLAG_MASK = COM_UTIL_REGEX_EXTENDED | COM_UTIL_REGEX_BASIC | COM_UTIL_REGEX_ICASE |
                                       COM_UTIL_REGEX_NOSUB | COM_UTIL_REGEX_OPTIMIZE;

/** 照合フラグとして受け付けるビットの和。 */
const unsigned int MATCH_FLAG_MASK = COM_UTIL_REGEX_MATCH_NOTBOL | COM_UTIL_REGEX_MATCH_NOTEOL |
                                     COM_UTIL_REGEX_MATCH_NOTEMPTY | COM_UTIL_REGEX_MATCH_ANCHORED;

/** 置換フラグとして受け付けるビットの和。 */
const unsigned int REPLACE_FLAG_MASK =
    COM_UTIL_REGEX_REPLACE_FIRST_ONLY | COM_UTIL_REGEX_REPLACE_NO_COPY | COM_UTIL_REGEX_REPLACE_SED;

/* OS 由来ではない失敗を記録し、結果コードを返す。 */
int report_plain(com_util_error *detail_out, int result)
{
    com_util_error_clear(detail_out);
    com_util_error_clear_last();
    return result;
}

/* 送出された例外を共通結果コードへ変換する。 */
int translate_exception(com_util_error *detail_out)
{
    int result = COM_UTIL_ERR_UNKNOWN;

    try
    {
        throw;
    }
    catch (const std::regex_error &error)
    {
        switch (error.code())
        {
        case std::regex_constants::error_complexity:
        case std::regex_constants::error_stack:
            result = report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
            break;
        case std::regex_constants::error_space:
            result = com_util_error_report_errno_as(detail_out, ENOMEM, COM_UTIL_ERR_OUT_OF_MEMORY);
            break;
        case std::regex_constants::error_collate:
        case std::regex_constants::error_ctype:
        case std::regex_constants::error_escape:
        case std::regex_constants::error_backref:
        case std::regex_constants::error_brack:
        case std::regex_constants::error_paren:
        case std::regex_constants::error_brace:
        case std::regex_constants::error_badbrace:
        case std::regex_constants::error_range:
        case std::regex_constants::error_badrepeat:
            result = report_plain(detail_out, COM_UTIL_ERR_INVALID_PATTERN);
            break;
        default:
            result = report_plain(detail_out, COM_UTIL_ERR_INVALID_PATTERN);
            break;
        }
    }
    catch (const std::bad_alloc &)
    {
        result = com_util_error_report_errno_as(detail_out, ENOMEM, COM_UTIL_ERR_OUT_OF_MEMORY);
    }
    catch (...)
    {
        result = report_plain(detail_out, COM_UTIL_ERR_UNKNOWN);
    }

    return result;
}

/* コンパイル フラグを std::regex_constants の値へ変換する。 */
bool to_syntax_option(unsigned int flags, std::regex_constants::syntax_option_type &option_out)
{
    if ((flags & COM_UTIL_REGEX_EXTENDED) != 0 && (flags & COM_UTIL_REGEX_BASIC) != 0)
    {
        return false;
    }

    std::regex_constants::syntax_option_type option = std::regex_constants::ECMAScript;
    if ((flags & COM_UTIL_REGEX_EXTENDED) != 0)
    {
        option = std::regex_constants::extended;
    }
    else if ((flags & COM_UTIL_REGEX_BASIC) != 0)
    {
        option = std::regex_constants::basic;
    }
    else
    {
        option = std::regex_constants::ECMAScript;
    }

    if ((flags & COM_UTIL_REGEX_ICASE) != 0)
    {
        option |= std::regex_constants::icase;
    }
    if ((flags & COM_UTIL_REGEX_NOSUB) != 0)
    {
        option |= std::regex_constants::nosubs;
    }
    if ((flags & COM_UTIL_REGEX_OPTIMIZE) != 0)
    {
        option |= std::regex_constants::optimize;
    }

    option_out = option;
    return true;
}

/* 照合フラグを std::regex_constants の値へ変換する。 */
std::regex_constants::match_flag_type to_match_flag(unsigned int match_flags)
{
    std::regex_constants::match_flag_type flag = std::regex_constants::match_default;

    if ((match_flags & COM_UTIL_REGEX_MATCH_NOTBOL) != 0)
    {
        flag |= std::regex_constants::match_not_bol;
    }
    if ((match_flags & COM_UTIL_REGEX_MATCH_NOTEOL) != 0)
    {
        flag |= std::regex_constants::match_not_eol;
    }
    if ((match_flags & COM_UTIL_REGEX_MATCH_NOTEMPTY) != 0)
    {
        flag |= std::regex_constants::match_not_null;
    }
    if ((match_flags & COM_UTIL_REGEX_MATCH_ANCHORED) != 0)
    {
        flag |= std::regex_constants::match_continuous;
    }

    return flag;
}

/* 照合結果を呼び出し側のマッチ範囲配列へ書き出す。 */
void store_matches(const match_type &result, const std::wstring &units, const std::vector<std::size_t> &offsets,
                   com_util_regex_match *matches_out, std::size_t matches_capacity)
{
    if (matches_out == nullptr)
    {
        return;
    }

    for (std::size_t group = 0; group < matches_capacity; group++)
    {
        if ((group >= result.size()) || !result[group].matched)
        {
            matches_out[group].begin = COM_UTIL_REGEX_NPOS;
            matches_out[group].end = COM_UTIL_REGEX_NPOS;
            continue;
        }

        const std::size_t begin_index = static_cast<std::size_t>(std::distance(units.cbegin(), result[group].first));
        const std::size_t end_index = static_cast<std::size_t>(std::distance(units.cbegin(), result[group].second));

        matches_out[group].begin = offset_of_begin(units, offsets, begin_index);
        matches_out[group].end = offset_of_end(units, offsets, end_index);
    }
}

} /* namespace */

/**
 *  コンパイル済みパターンのハンドルの実体。
 *
 *  照合エンジンと、@ref com_util_regex_get_group_count() が返す要素数を保持します。
 */
struct com_util_regex
{
    engine_type engine;      /**< 照合エンジン。 */
    std::size_t group_count; /**< 捕捉グループ数に全体マッチの 1 を加えた値。 */
};

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_create(const char *pattern, const unsigned int flags, com_util_regex **regex_out,
                          com_util_error *detail_out)
{
    if ((pattern == nullptr) || (regex_out == nullptr))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *regex_out = nullptr;

    if ((flags & ~COMPILE_FLAG_MASK) != 0)
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    std::regex_constants::syntax_option_type option = std::regex_constants::ECMAScript;
    if (!to_syntax_option(flags, option))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    int result = COM_UTIL_OK;

    try
    {
        const std::size_t pattern_len = std::char_traits<char>::length(pattern);
        if (pattern_len > COM_UTIL_REGEX_MAX_LENGTH)
        {
            return report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
        }

        std::wstring units;
        std::vector<std::size_t> offsets;
        if (!utf8_decode(pattern, pattern_len, units, offsets))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ENCODING);
        }

        com_util_regex *created = new com_util_regex{engine_type(units, option), 0};
        created->group_count = created->engine.mark_count() + 1;

        *regex_out = created;
        result = com_util_error_report_success(detail_out);
    }
    catch (...)
    {
        result = translate_exception(detail_out);
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_regex_dispose(com_util_regex *regex)
{
    if (regex == nullptr)
    {
        return;
    }

    try
    {
        delete regex;
    }
    catch (...)
    {
        /* 破棄処理から例外を伝播させない。 */
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

size_t com_util_regex_get_group_count(const com_util_regex *regex)
{
    if (regex == nullptr)
    {
        return 0;
    }
    return regex->group_count;
}

namespace
{

/* search と matches の共通処理を行う。 */
int execute(const com_util_regex *regex, const char *text, std::size_t text_len, std::size_t start_offset,
            unsigned int match_flags, bool whole_match, com_util_regex_match *matches_out, std::size_t matches_capacity,
            int *matched_out, com_util_error *detail_out)
{
    if ((regex == nullptr) || (text == nullptr) || (matched_out == nullptr))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *matched_out = 0;

    if ((match_flags & ~MATCH_FLAG_MASK) != 0)
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if (start_offset > text_len)
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if (text_len > COM_UTIL_REGEX_MAX_LENGTH)
    {
        return report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
    }

    int result = COM_UTIL_OK;

    try
    {
        std::wstring units;
        std::vector<std::size_t> offsets;
        if (!utf8_decode(text, text_len, units, offsets))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ENCODING);
        }

        std::size_t start_index = 0;
        if (!index_of_offset(offsets, start_offset, start_index))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
        }

        std::regex_constants::match_flag_type flag = to_match_flag(match_flags);
        if (start_index > 0)
        {
            /* 開始位置より前に文字が存在することをエンジンへ伝え、^ と \b を正しく評価させる。 */
            flag |= std::regex_constants::match_prev_avail;
        }

        match_type matched;
        bool found = false;
        if (whole_match)
        {
            found = std::regex_match(units.cbegin() + static_cast<std::ptrdiff_t>(start_index), units.cend(), matched,
                                     regex->engine, flag);
        }
        else
        {
            found = std::regex_search(units.cbegin() + static_cast<std::ptrdiff_t>(start_index), units.cend(), matched,
                                      regex->engine, flag);
        }

        if (found)
        {
            *matched_out = 1;
            store_matches(matched, units, offsets, matches_out, matches_capacity);
        }
        else if (matches_out != nullptr)
        {
            for (std::size_t group = 0; group < matches_capacity; group++)
            {
                matches_out[group].begin = COM_UTIL_REGEX_NPOS;
                matches_out[group].end = COM_UTIL_REGEX_NPOS;
            }
        }
        else
        {
            /* 格納先が無いため、何もしない。 */
        }

        result = com_util_error_report_success(detail_out);
    }
    catch (...)
    {
        result = translate_exception(detail_out);
    }

    return result;
}

} /* namespace */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_search(const com_util_regex *regex, const char *text, const size_t text_len,
                          const size_t start_offset, const unsigned int match_flags, com_util_regex_match *matches_out,
                          const size_t matches_capacity, int *matched_out, com_util_error *detail_out)
{
    return execute(regex, text, text_len, start_offset, match_flags, false, matches_out, matches_capacity, matched_out,
                   detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_matches(const com_util_regex *regex, const char *text, const size_t text_len,
                           const unsigned int match_flags, com_util_regex_match *matches_out,
                           const size_t matches_capacity, int *matched_out, com_util_error *detail_out)
{
    return execute(regex, text, text_len, 0, match_flags, true, matches_out, matches_capacity, matched_out, detail_out);
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_replace(const com_util_regex *regex, const char *text, const size_t text_len,
                           const char *replacement, const unsigned int flags, char *result_out,
                           const size_t result_size, size_t *required_size_out, com_util_error *detail_out)
{
    if ((regex == nullptr) || (text == nullptr) || (replacement == nullptr))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if ((result_out == nullptr) && (result_size != 0))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if ((flags & ~(MATCH_FLAG_MASK | REPLACE_FLAG_MASK)) != 0)
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if (text_len > COM_UTIL_REGEX_MAX_LENGTH)
    {
        return report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
    }

    int result = COM_UTIL_OK;

    try
    {
        std::wstring text_units;
        std::vector<std::size_t> text_offsets;
        if (!utf8_decode(text, text_len, text_units, text_offsets))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ENCODING);
        }

        const std::size_t replacement_len = std::char_traits<char>::length(replacement);
        if (replacement_len > COM_UTIL_REGEX_MAX_LENGTH)
        {
            return report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
        }

        std::wstring replacement_units;
        std::vector<std::size_t> replacement_offsets;
        if (!utf8_decode(replacement, replacement_len, replacement_units, replacement_offsets))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ENCODING);
        }

        std::regex_constants::match_flag_type flag = to_match_flag(flags);
        if ((flags & COM_UTIL_REGEX_REPLACE_FIRST_ONLY) != 0)
        {
            flag |= std::regex_constants::format_first_only;
        }
        if ((flags & COM_UTIL_REGEX_REPLACE_NO_COPY) != 0)
        {
            flag |= std::regex_constants::format_no_copy;
        }
        if ((flags & COM_UTIL_REGEX_REPLACE_SED) != 0)
        {
            flag |= std::regex_constants::format_sed;
        }

        const std::wstring replaced = std::regex_replace(text_units, regex->engine, replacement_units, flag);

        std::string encoded;
        if (!utf8_encode(replaced, encoded))
        {
            return report_plain(detail_out, COM_UTIL_ERR_UNKNOWN);
        }

        const std::size_t required_size = encoded.size() + 1;
        if (required_size_out != nullptr)
        {
            *required_size_out = required_size;
        }

        if (result_out == nullptr)
        {
            /* 必要サイズの問い合わせのため、置換結果は書き込まない。 */
            return com_util_error_report_success(detail_out);
        }
        if (result_size < required_size)
        {
            return report_plain(detail_out, COM_UTIL_ERR_BUFFER_TOO_SMALL);
        }

        std::char_traits<char>::copy(result_out, encoded.data(), encoded.size());
        result_out[encoded.size()] = '\0';

        result = com_util_error_report_success(detail_out);
    }
    catch (...)
    {
        result = translate_exception(detail_out);
    }

    return result;
}

/**
 *  一致箇所を順に列挙するイテレーターの実体。
 *
 *  入力はコード単位列として複製して保持するため、生成元の文字列より
 *  長く存在できます。
 */
struct com_util_regex_iter
{
    const com_util_regex *regex;      /**< 参照するコンパイル済みパターン。 */
    std::wstring units;               /**< 入力のコード単位列。 */
    std::vector<std::size_t> offsets; /**< コード単位索引から UTF-8 バイト オフセットへの写像表。 */
    std::size_t position;             /**< 次に照合を開始するコード単位索引。 */
    unsigned int match_flags;         /**< 照合フラグ。 */
    /* 末尾に詰め物が入らないよう、bool ではなく match_flags と同じ幅の型で保持する。 */
    unsigned int finished; /**< 列挙が終わった場合に 1、それ以外は 0。 */
};

namespace
{

/* 指定位置から次の一致箇所を探し、見つかった範囲をコード単位索引で返す。 */
bool find_next(const com_util_regex *regex, const std::wstring &units, unsigned int match_flags, std::size_t position,
               match_type &matched_out, std::size_t &begin_out, std::size_t &end_out)
{
    if (position > units.size())
    {
        return false;
    }

    std::regex_constants::match_flag_type flag = to_match_flag(match_flags);
    if (position > 0)
    {
        flag |= std::regex_constants::match_prev_avail;
    }

    const bool found = std::regex_search(units.cbegin() + static_cast<std::ptrdiff_t>(position), units.cend(),
                                         matched_out, regex->engine, flag);
    if (!found)
    {
        return false;
    }

    begin_out = position + static_cast<std::size_t>(matched_out.position(0));
    end_out = begin_out + static_cast<std::size_t>(matched_out.length(0));
    return true;
}

/* 空一致の後に次の照合を開始する位置を求める。サロゲート ペアの内側では停止しない。 */
std::size_t advance_position(const std::wstring &units, std::size_t position)
{
    std::size_t next = position + 1;
    while ((next < units.size()) && ((static_cast<unsigned int>(units[next]) & 0xFC00U) == 0xDC00U))
    {
        next++;
    }
    return next;
}

} /* namespace */

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_iter_create(const com_util_regex *regex, const char *text, const size_t text_len,
                               const unsigned int match_flags, com_util_regex_iter **iter_out,
                               com_util_error *detail_out)
{
    if ((regex == nullptr) || (text == nullptr) || (iter_out == nullptr))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *iter_out = nullptr;

    if ((match_flags & ~MATCH_FLAG_MASK) != 0)
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if (text_len > COM_UTIL_REGEX_MAX_LENGTH)
    {
        return report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
    }

    int result = COM_UTIL_OK;

    try
    {
        std::wstring units;
        std::vector<std::size_t> offsets;
        if (!utf8_decode(text, text_len, units, offsets))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ENCODING);
        }

        com_util_regex_iter *created = new com_util_regex_iter{regex, units, offsets, 0, match_flags, 0};

        *iter_out = created;
        result = com_util_error_report_success(detail_out);
    }
    catch (...)
    {
        result = translate_exception(detail_out);
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_iter_next(com_util_regex_iter *iter, com_util_regex_match *matches_out,
                             const size_t matches_capacity, int *has_match_out, com_util_error *detail_out)
{
    if ((iter == nullptr) || (has_match_out == nullptr))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }

    *has_match_out = 0;

    int result = COM_UTIL_OK;

    try
    {
        if (iter->finished != 0)
        {
            return com_util_error_report_success(detail_out);
        }

        match_type matched;
        std::size_t begin_index = 0;
        std::size_t end_index = 0;
        const bool found =
            find_next(iter->regex, iter->units, iter->match_flags, iter->position, matched, begin_index, end_index);
        if (!found)
        {
            iter->finished = 1;
            return com_util_error_report_success(detail_out);
        }

        *has_match_out = 1;
        store_matches(matched, iter->units, iter->offsets, matches_out, matches_capacity);

        if (end_index == begin_index)
        {
            /* 空一致では位置が進まないため、1 文字進めて無限ループを避ける。 */
            iter->position = advance_position(iter->units, end_index);
        }
        else
        {
            iter->position = end_index;
        }

        if (iter->position > iter->units.size())
        {
            iter->finished = 1;
        }

        result = com_util_error_report_success(detail_out);
    }
    catch (...)
    {
        result = translate_exception(detail_out);
    }

    return result;
}

/* Doxygen コメントは、ヘッダーに記載 */

void com_util_regex_iter_dispose(com_util_regex_iter *iter)
{
    if (iter == nullptr)
    {
        return;
    }

    try
    {
        delete iter;
    }
    catch (...)
    {
        /* 破棄処理から例外を伝播させない。 */
    }
}

/* Doxygen コメントは、ヘッダーに記載 */

int com_util_regex_split(const com_util_regex *regex, const char *text, const size_t text_len, const size_t max_parts,
                         const unsigned int match_flags, com_util_regex_match *parts_out, const size_t parts_capacity,
                         size_t *part_count_out, com_util_error *detail_out)
{
    if ((regex == nullptr) || (text == nullptr) || (part_count_out == nullptr))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if ((parts_out == nullptr) && (parts_capacity != 0))
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if ((match_flags & ~MATCH_FLAG_MASK) != 0)
    {
        return report_plain(detail_out, COM_UTIL_ERR_INVALID_ARGUMENT);
    }
    if (text_len > COM_UTIL_REGEX_MAX_LENGTH)
    {
        return report_plain(detail_out, COM_UTIL_ERR_LIMIT_EXCEEDED);
    }

    *part_count_out = 0;

    int result = COM_UTIL_OK;

    try
    {
        std::wstring units;
        std::vector<std::size_t> offsets;
        if (!utf8_decode(text, text_len, units, offsets))
        {
            return report_plain(detail_out, COM_UTIL_ERR_INVALID_ENCODING);
        }

        std::vector<com_util_regex_match> parts;
        std::size_t part_begin = 0;
        std::size_t position = 0;
        bool running = true;

        while (running)
        {
            if ((max_parts != 0) && ((parts.size() + 1) >= max_parts))
            {
                /* 上限に達したため、残りをすべて最後の要素とする。 */
                break;
            }

            match_type matched;
            std::size_t begin_index = 0;
            std::size_t end_index = 0;
            const bool found = find_next(regex, units, match_flags, position, matched, begin_index, end_index);
            if (!found)
            {
                break;
            }

            if (end_index == begin_index)
            {
                /* 空一致では区切りを作らず、位置だけを進める。 */
                const std::size_t next = advance_position(units, end_index);
                if (next > units.size())
                {
                    running = false;
                }
                else
                {
                    position = next;
                }
                continue;
            }

            com_util_regex_match part;
            part.begin = offset_of_begin(units, offsets, part_begin);
            part.end = offset_of_end(units, offsets, begin_index);
            parts.push_back(part);

            part_begin = end_index;
            position = end_index;
        }

        com_util_regex_match last_part;
        last_part.begin = offset_of_begin(units, offsets, part_begin);
        last_part.end = offset_of_end(units, offsets, units.size());
        parts.push_back(last_part);

        *part_count_out = parts.size();

        if (parts_out == nullptr)
        {
            /* 必要件数の問い合わせのため、分割結果は書き込まない。 */
            return com_util_error_report_success(detail_out);
        }

        const std::size_t copy_count = std::min(parts_capacity, parts.size());
        for (std::size_t index = 0; index < copy_count; index++)
        {
            parts_out[index] = parts[index];
        }

        if (parts_capacity < parts.size())
        {
            return report_plain(detail_out, COM_UTIL_ERR_BUFFER_TOO_SMALL);
        }

        result = com_util_error_report_success(detail_out);
    }
    catch (...)
    {
        result = translate_exception(detail_out);
    }

    return result;
}
