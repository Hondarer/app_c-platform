/**
 *******************************************************************************
 *  @file           filter_slot.c
 *  @brief          フィルター スロット (適用、事前計算、判定、差し替え) を実装します。
 *  @author         Tetsuo Honda
 *  @date           2026/09/26
 *  @version        0.1.0
 *
 *  スロットは、判定が参照する内容を 2 面 (plane) で持ちます。\n
 *  適用は使用していない面へ複製、名前解決、事前計算を行い、排他モードのロックの下で参照する面を切り替えます。\n
 *  判定は共有モードのロックの下で、参照中の面だけを読みます。
 *
 *  面を書き換えるのは適用だけであり、適用は専用のロックで 1 つずつ処理します。\n
 *  そのため、適用は参照中の面をロックなしで読み、未変更の行の結果を再利用できます。
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *******************************************************************************
 */

#include "filter.h"

#include <cplat/base/result.h>
#include <cplat/sync/sync.h>

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 *  事前計算の領域の要素数の最大値です。[行][項目][引数参照] の要素数で確保します。
 *
 *  項目数は int の最大値まで受け付けるため、行数と引数参照の上限との積になります。
 */
#define PLANE_ELEMENT_COUNT_MAX \
    ((uint64_t)CPLAT_STRING_CATALOG_FILTER_LINE_MAX * (uint64_t)INT_MAX * \
     (uint64_t)CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX)

/* 32 ビットの size_t では成り立たないため、本実装は 64 ビットの size_t を前提とする */
_Static_assert(PLANE_ELEMENT_COUNT_MAX < (uint64_t)SIZE_MAX, "filter plane element count must fit in size_t");

/** 3 値の論理値です。事前計算では、引数の値に依存する判定要素を不定とします。 */
typedef enum truth_value
{
    TRUTH_VALUE_FALSE = 0,
    TRUTH_VALUE_TRUE = 1,
    TRUTH_VALUE_UNKNOWN = 2
} truth_value;

/** 数値の比較結果のうち、NaN を含むため順序が定まらないことを表す値です。 */
#define ORDER_UNORDERED 2

/** 判定が参照する内容の 1 面分です。 */
typedef struct filter_plane
{
    unsigned char *image;                           /**< フィルター オブジェクトの複製。 */
    uint8_t *entry_states;                          /**< [項目] の cplat_string_catalog_filter_state。 */
    uint64_t *entry_dependent_lines;                /**< [項目] の、引数値に依存する行の集合。 */
    uint8_t *line_states;                           /**< [行][項目] の truth_value。 */
    int8_t *argument_maps;                          /**< [行][項目][引数参照] の引数インデックス。未定義は -1。 */
    int64_t *identifier_values;                     /**< [行][識別子] の文字列キー。 */
    cplat_string_catalog_filter_error *line_errors; /**< [行] の無効にした原因。 */
    uint64_t enabled_lines;                         /**< 適用で有効になった行の集合。 */
    uint32_t line_count;                            /**< 格納している条件式の数。 */
    uint32_t pad;                                   /**< 明示的アラインメントです。 */
} filter_plane;

struct cplat_string_catalog_filter_slot
{
    const cplat_string_catalog *catalog;
    const cplat_string_catalog_filter_key_name *key_names;
    size_t key_name_count;
    size_t entry_count;
    size_t image_size;
    uint32_t line_capacity;
    uint32_t line_width;
    uint32_t record_size;
    int active_plane; /**< 判定が参照する面。plane_lock の下で読み書きします。 */
    const cplat_string_catalog_filter_category_names
        *category_names; /**< 自然文での表現に使う分類値の名前。未設定は NULL。 */
    cplat_local_rwlock *plane_lock;
    cplat_local_lock *apply_lock;
    filter_plane planes[2];
};

/** 判定に使用する引数の値です。比較の区分ごとに正規化して保持します。 */
typedef struct argument_value
{
    cplat_string_catalog_argument_kind kind;
    unsigned int pad;
    union argument_storage
    {
        const char *string_value;
        int64_t signed_value;
        uint64_t unsigned_value;
        double real_value;
    } value;
} argument_value;

/** 比較に使用する数値です。整数は符号と絶対値で表し、数学的な大小で比較します。 */
typedef struct number
{
    uint64_t magnitude;
    double real;
    bool is_real;
    bool is_negative;
    uint8_t pad[6]; /**< 明示的アラインメントです。 */
} number;

/** 引数の比較上の区分です。 */
typedef enum argument_class
{
    ARGUMENT_CLASS_NONE = 0,
    ARGUMENT_CLASS_SIGNED = 1,
    ARGUMENT_CLASS_UNSIGNED = 2,
    ARGUMENT_CLASS_REAL = 3,
    ARGUMENT_CLASS_STRING = 4,
    ARGUMENT_CLASS_POINTER = 5
} argument_class;

/** 1 行を 1 項目について評価するための情報です。 */
typedef struct evaluation_context
{
    const cplat_string_catalog_entry *entry;
    const unsigned char *record;
    const unsigned char *constants;
    uint32_t constant_size;
    uint32_t pad;
    const int8_t *argument_map;       /**< [引数参照] */
    const int64_t *identifier_values; /**< [識別子] */
    const argument_value *values;     /**< 引数の値。事前計算では NULL。 */
} evaluation_context;

/* ===== 引数と数値 ===== */

static argument_class class_of_argument(const cplat_string_catalog_argument_kind kind)
{
    switch (kind)
    {
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_CHAR:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT8:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT16:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT32:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT64:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_SSIZE:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_ERROR_CODE:
        return ARGUMENT_CLASS_SIGNED;
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT8:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT16:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT32:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT64:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX8:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX16:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX32:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX64:
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_SIZE:
        return ARGUMENT_CLASS_UNSIGNED;
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_DOUBLE:
        return ARGUMENT_CLASS_REAL;
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING:
        return ARGUMENT_CLASS_STRING;
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_POINTER:
        return ARGUMENT_CLASS_POINTER;
    case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UNUSED:
    default:
        return ARGUMENT_CLASS_NONE;
    }
}

/**
 *  @brief          引数スキーマに従って、可変長引数を値の配列へ取り出します。
 *
 *  cplat の cplat_internal_string_catalog_collect_arguments() と同じ取り出し型を使用します。\n
 *  既定引数拡張と一致しない va_arg は未定義動作となるため、8 ビットと 16 ビットの種別は int として取り出します。
 *
 *  @return         すべての引数を取り出せた場合は true、未知の種別があった場合は false。
 */
static bool collect_arguments(const cplat_string_catalog_entry *entry, va_list args, argument_value *values)
{
    for (int index = 0; index < entry->argument_count; index++)
    {
        const cplat_string_catalog_argument_kind kind = entry->arguments[index].kind;
        argument_value *value = &values[index];

        value->kind = kind;
        value->pad = 0U;
        value->value.unsigned_value = 0U;

        switch (kind)
        {
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UNUSED:
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_STRING:
            value->value.string_value = va_arg(args, const char *);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_CHAR:
            value->value.signed_value = (char)va_arg(args, int);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT8:
            value->value.signed_value = (int8_t)va_arg(args, int);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT8:
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX8:
            value->value.unsigned_value = (uint8_t)va_arg(args, int);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT16:
            value->value.signed_value = (int16_t)va_arg(args, int);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT16:
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX16:
            value->value.unsigned_value = (uint16_t)va_arg(args, int);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT32:
            value->value.signed_value = va_arg(args, int32_t);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT32:
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX32:
            value->value.unsigned_value = va_arg(args, uint32_t);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_INT64:
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_SSIZE:
            value->value.signed_value = va_arg(args, int64_t);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_UINT64:
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_HEX64:
            value->value.unsigned_value = va_arg(args, uint64_t);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_SIZE:
            value->value.unsigned_value = va_arg(args, size_t);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_POINTER:
            value->value.unsigned_value = (uint64_t)(uintptr_t)va_arg(args, const void *);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_DOUBLE:
            value->value.real_value = va_arg(args, double);
            break;
        case CPLAT_STRING_CATALOG_ARGUMENT_KIND_ERROR_CODE:
            value->value.signed_value = va_arg(args, int);
            break;
        default:
            return false;
        }
    }
    return true;
}

static number number_from_signed(const int64_t value)
{
    number result;

    memset(&result, 0, sizeof(result));
    if (value < 0)
    {
        result.is_negative = true;
        /* INT64_MIN でも回り込まないよう、1 を足してから符号を反転する */
        result.magnitude = (uint64_t)(-(value + 1)) + 1U;
    }
    else
    {
        result.magnitude = (uint64_t)value;
    }
    return result;
}

static number number_from_unsigned(const uint64_t value)
{
    number result;

    memset(&result, 0, sizeof(result));
    result.magnitude = value;
    return result;
}

static number number_from_real(const double value)
{
    number result;

    memset(&result, 0, sizeof(result));
    result.is_real = true;
    result.real = value;
    return result;
}

static double real_of_number(const number *value)
{
    if (value->is_real)
    {
        return value->real;
    }
    if (value->is_negative)
    {
        return -(double)value->magnitude;
    }
    return (double)value->magnitude;
}

/**
 *  @brief          2 つの数値を数学的な大小で比較します。
 *  @return         -1、0、1、または NaN を含む場合は ORDER_UNORDERED。
 */
static int compare_numbers(const number *left, const number *right)
{
    if (left->is_real || right->is_real)
    {
        const double left_real = real_of_number(left);
        const double right_real = real_of_number(right);

        if (isnan(left_real) || isnan(right_real))
        {
            return ORDER_UNORDERED;
        }
        if (left_real < right_real)
        {
            return -1;
        }
        if (left_real > right_real)
        {
            return 1;
        }
        return 0;
    }

    if (left->is_negative != right->is_negative)
    {
        if (left->is_negative)
        {
            return -1;
        }
        return 1;
    }
    if (left->magnitude == right->magnitude)
    {
        return 0;
    }
    /* 負どうしでは、絶対値の大きい方が小さい */
    if ((left->magnitude < right->magnitude) != left->is_negative)
    {
        return -1;
    }
    return 1;
}

static bool is_order_accepted(const uint8_t operator_kind, const int order)
{
    switch (operator_kind)
    {
    case STRING_CATALOG_FILTER_OPERATOR_EQUAL:
    case STRING_CATALOG_FILTER_OPERATOR_IN:
        return order == 0;
    case STRING_CATALOG_FILTER_OPERATOR_NOT_EQUAL:
        return order != 0;
    case STRING_CATALOG_FILTER_OPERATOR_LESS:
        return order == -1;
    case STRING_CATALOG_FILTER_OPERATOR_LESS_EQUAL:
        return (order == -1) || (order == 0);
    case STRING_CATALOG_FILTER_OPERATOR_GREATER:
        return order == 1;
    case STRING_CATALOG_FILTER_OPERATOR_GREATER_EQUAL:
        return (order == 1) || (order == 0);
    default:
        return false;
    }
}

/** 数値の定数を比較用の数値へ変換します。 */
static number number_from_constant(const evaluation_context *context, const string_catalog_filter_constant *constant)
{
    switch (constant->header.kind)
    {
    case STRING_CATALOG_FILTER_CONSTANT_KIND_FLOAT:
        return number_from_real(constant->real);
    case STRING_CATALOG_FILTER_CONSTANT_KIND_IDENTIFIER:
        return number_from_signed(context->identifier_values[constant->header.slot]);
    default:
    {
        number result = number_from_unsigned(constant->magnitude);
        result.is_negative = ((constant->header.flags & STRING_CATALOG_FILTER_CONSTANT_FLAG_NEGATIVE) != 0U);
        return result;
    }
    }
}

/* ===== 文字列の比較 ===== */

static unsigned char to_lower_ascii(const unsigned char character)
{
    if ((character >= (unsigned char)'A') && (character <= (unsigned char)'Z'))
    {
        return (unsigned char)(character + ('a' - 'A'));
    }
    return character;
}

static bool equals_bytes(const char *left, const char *right, const size_t length, const bool ignores_case)
{
    if (!ignores_case)
    {
        return memcmp(left, right, length) == 0;
    }
    for (size_t index = 0; index < length; index++)
    {
        if (to_lower_ascii((unsigned char)left[index]) != to_lower_ascii((unsigned char)right[index]))
        {
            return false;
        }
    }
    return true;
}

static bool match_string(const uint8_t operator_kind, const char *subject, const char *pattern,
                         const size_t pattern_length)
{
    const size_t subject_length = strlen(subject);
    const bool ignores_case = (operator_kind >= (uint8_t)STRING_CATALOG_FILTER_OPERATOR_STARTS_WITH_I);

    switch (operator_kind)
    {
    case STRING_CATALOG_FILTER_OPERATOR_EQUAL:
    case STRING_CATALOG_FILTER_OPERATOR_IN:
        return (subject_length == pattern_length) && equals_bytes(subject, pattern, pattern_length, false);
    case STRING_CATALOG_FILTER_OPERATOR_NOT_EQUAL:
        return (subject_length != pattern_length) || !equals_bytes(subject, pattern, pattern_length, false);
    case STRING_CATALOG_FILTER_OPERATOR_STARTS_WITH:
    case STRING_CATALOG_FILTER_OPERATOR_STARTS_WITH_I:
        return (subject_length >= pattern_length) && equals_bytes(subject, pattern, pattern_length, ignores_case);
    case STRING_CATALOG_FILTER_OPERATOR_ENDS_WITH:
    case STRING_CATALOG_FILTER_OPERATOR_ENDS_WITH_I:
        return (subject_length >= pattern_length) &&
               equals_bytes(subject + (subject_length - pattern_length), pattern, pattern_length, ignores_case);
    case STRING_CATALOG_FILTER_OPERATOR_CONTAINS:
    case STRING_CATALOG_FILTER_OPERATOR_CONTAINS_I:
        if (pattern_length > subject_length)
        {
            return false;
        }
        for (size_t start = 0; start <= (subject_length - pattern_length); start++)
        {
            if (equals_bytes(subject + start, pattern, pattern_length, ignores_case))
            {
                return true;
            }
        }
        return false;
    default:
        return false;
    }
}

/* ===== 判定要素と命令列の評価 ===== */

/** 文字列を対象とする判定要素を評価します。対象は NULL の場合があります。 */
static truth_value evaluate_string(const evaluation_context *context,
                                   const string_catalog_filter_instruction *instruction, uint32_t offset,
                                   const char *subject)
{
    string_catalog_filter_constant constant;

    for (uint16_t index = 0; index < instruction->operand_count; index++)
    {
        (void)string_catalog_filter_read_constant(context->constants, context->constant_size, offset, &constant);
        offset = constant.next_offset;

        if (constant.header.kind == (uint8_t)STRING_CATALOG_FILTER_CONSTANT_KIND_NULL)
        {
            /* null との比較は == と != だけが成立し得る */
            if ((subject == NULL) == (instruction->operator_kind == (uint8_t)STRING_CATALOG_FILTER_OPERATOR_EQUAL))
            {
                return TRUTH_VALUE_TRUE;
            }
            return TRUTH_VALUE_FALSE;
        }

        if (subject == NULL)
        {
            /* NULL は、どの文字列とも等しくない。文字列の判定演算はすべて偽とする */
            if (instruction->operator_kind == (uint8_t)STRING_CATALOG_FILTER_OPERATOR_NOT_EQUAL)
            {
                return TRUTH_VALUE_TRUE;
            }
            return TRUTH_VALUE_FALSE;
        }

        if (match_string(instruction->operator_kind, subject, constant.text, constant.header.length))
        {
            return TRUTH_VALUE_TRUE;
        }
    }
    return TRUTH_VALUE_FALSE;
}

/** 数値を対象とする判定要素を評価します。 */
static truth_value evaluate_number(const evaluation_context *context,
                                   const string_catalog_filter_instruction *instruction, uint32_t offset,
                                   const number *subject)
{
    string_catalog_filter_constant constant;

    if (instruction->operator_kind == (uint8_t)STRING_CATALOG_FILTER_OPERATOR_BETWEEN)
    {
        number lower;
        number upper;
        int lower_order;
        int upper_order;

        (void)string_catalog_filter_read_constant(context->constants, context->constant_size, offset, &constant);
        lower = number_from_constant(context, &constant);
        (void)string_catalog_filter_read_constant(context->constants, context->constant_size, constant.next_offset,
                                                  &constant);
        upper = number_from_constant(context, &constant);

        lower_order = compare_numbers(subject, &lower);
        upper_order = compare_numbers(subject, &upper);
        if (((lower_order == 0) || (lower_order == 1)) && ((upper_order == 0) || (upper_order == -1)))
        {
            return TRUTH_VALUE_TRUE;
        }
        return TRUTH_VALUE_FALSE;
    }

    for (uint16_t index = 0; index < instruction->operand_count; index++)
    {
        number literal;

        (void)string_catalog_filter_read_constant(context->constants, context->constant_size, offset, &constant);
        offset = constant.next_offset;
        literal = number_from_constant(context, &constant);

        if (is_order_accepted(instruction->operator_kind, compare_numbers(subject, &literal)))
        {
            return TRUTH_VALUE_TRUE;
        }
    }
    return TRUTH_VALUE_FALSE;
}

/**
 *  @brief          引数の型区分と、比較対象の定数の種類が組み合わせとして成立するかを判定します。
 *
 *  成立しない組み合わせは、その項目では偽とします (設計資料「引数のデータ型と比較規則」)。
 */
static bool is_argument_comparable(const argument_class kind_class, const uint8_t constant_kind)
{
    switch (constant_kind)
    {
    case STRING_CATALOG_FILTER_CONSTANT_KIND_STRING:
        return kind_class == ARGUMENT_CLASS_STRING;
    case STRING_CATALOG_FILTER_CONSTANT_KIND_NULL:
        return (kind_class == ARGUMENT_CLASS_STRING) || (kind_class == ARGUMENT_CLASS_POINTER);
    default:
        return kind_class != ARGUMENT_CLASS_STRING;
    }
}

static truth_value evaluate_predicate(const evaluation_context *context,
                                      const string_catalog_filter_instruction *instruction)
{
    const cplat_string_catalog_entry *entry = context->entry;
    string_catalog_filter_constant constant;
    uint32_t offset = instruction->operand;
    int argument_index = -1;
    argument_class kind_class;

    switch (instruction->field)
    {
    case STRING_CATALOG_FILTER_FIELD_KEY:
    {
        const number subject = number_from_signed(entry->key);
        return evaluate_number(context, instruction, offset, &subject);
    }
    case STRING_CATALOG_FILTER_FIELD_CATEGORY:
    {
        const number subject = number_from_signed(entry->category);
        return evaluate_number(context, instruction, offset, &subject);
    }
    case STRING_CATALOG_FILTER_FIELD_ID:
        return evaluate_string(context, instruction, offset, entry->id);

    case STRING_CATALOG_FILTER_FIELD_ARGUMENT_NAME:
        argument_index = context->argument_map[instruction->argument];
        /* 比較対象は引数名の定数の直後から並ぶ */
        (void)string_catalog_filter_read_constant(context->constants, context->constant_size, offset, &constant);
        offset = constant.next_offset;
        break;

    case STRING_CATALOG_FILTER_FIELD_ARGUMENT_INDEX:
        if (((int)instruction->argument < entry->argument_count) &&
            (entry->arguments[instruction->argument].kind != CPLAT_STRING_CATALOG_ARGUMENT_KIND_UNUSED))
        {
            argument_index = instruction->argument;
        }
        break;

    default:
        return TRUTH_VALUE_FALSE;
    }

    if (instruction->operator_kind == (uint8_t)STRING_CATALOG_FILTER_OPERATOR_HAS)
    {
        if (argument_index >= 0)
        {
            return TRUTH_VALUE_TRUE;
        }
        return TRUTH_VALUE_FALSE;
    }

    /* 項目に対応する引数がない場合、その引数を参照する比較は偽 */
    if (argument_index < 0)
    {
        return TRUTH_VALUE_FALSE;
    }

    kind_class = class_of_argument(entry->arguments[argument_index].kind);
    (void)string_catalog_filter_read_constant(context->constants, context->constant_size, offset, &constant);
    if (!is_argument_comparable(kind_class, constant.header.kind))
    {
        return TRUTH_VALUE_FALSE;
    }

    /* 事前計算では、引数の値に依存する判定要素を不定とする */
    if (context->values == NULL)
    {
        return TRUTH_VALUE_UNKNOWN;
    }

    switch (kind_class)
    {
    case ARGUMENT_CLASS_STRING:
        return evaluate_string(context, instruction, offset, context->values[argument_index].value.string_value);

    case ARGUMENT_CLASS_POINTER:
        if (constant.header.kind == (uint8_t)STRING_CATALOG_FILTER_CONSTANT_KIND_NULL)
        {
            const bool is_null = (context->values[argument_index].value.unsigned_value == 0U);

            if (is_null == (instruction->operator_kind == (uint8_t)STRING_CATALOG_FILTER_OPERATOR_EQUAL))
            {
                return TRUTH_VALUE_TRUE;
            }
            return TRUTH_VALUE_FALSE;
        }
        {
            const number subject = number_from_unsigned(context->values[argument_index].value.unsigned_value);
            return evaluate_number(context, instruction, offset, &subject);
        }

    case ARGUMENT_CLASS_SIGNED:
    {
        const number subject = number_from_signed(context->values[argument_index].value.signed_value);
        return evaluate_number(context, instruction, offset, &subject);
    }
    case ARGUMENT_CLASS_UNSIGNED:
    {
        const number subject = number_from_unsigned(context->values[argument_index].value.unsigned_value);
        return evaluate_number(context, instruction, offset, &subject);
    }
    case ARGUMENT_CLASS_REAL:
    {
        const number subject = number_from_real(context->values[argument_index].value.real_value);
        return evaluate_number(context, instruction, offset, &subject);
    }
    case ARGUMENT_CLASS_NONE:
    default:
        return TRUTH_VALUE_FALSE;
    }
}

static truth_value truth_not(const truth_value value)
{
    if (value == TRUTH_VALUE_TRUE)
    {
        return TRUTH_VALUE_FALSE;
    }
    if (value == TRUTH_VALUE_FALSE)
    {
        return TRUTH_VALUE_TRUE;
    }
    return TRUTH_VALUE_UNKNOWN;
}

static truth_value truth_and(const truth_value left, const truth_value right)
{
    if ((left == TRUTH_VALUE_FALSE) || (right == TRUTH_VALUE_FALSE))
    {
        return TRUTH_VALUE_FALSE;
    }
    if ((left == TRUTH_VALUE_TRUE) && (right == TRUTH_VALUE_TRUE))
    {
        return TRUTH_VALUE_TRUE;
    }
    return TRUTH_VALUE_UNKNOWN;
}

static truth_value truth_or(const truth_value left, const truth_value right)
{
    if ((left == TRUTH_VALUE_TRUE) || (right == TRUTH_VALUE_TRUE))
    {
        return TRUTH_VALUE_TRUE;
    }
    if ((left == TRUTH_VALUE_FALSE) && (right == TRUTH_VALUE_FALSE))
    {
        return TRUTH_VALUE_FALSE;
    }
    return TRUTH_VALUE_UNKNOWN;
}

/**
 *  @brief          1 行の命令列を評価します。
 *  @param[in]      context      評価する行と文字列カタログ項目。
 *  @param[in]      honors_jumps 短絡評価のジャンプに従う場合は true。
 *                               事前計算では false とし、結合の命令で 3 値の論理演算を行います。
 *
 *  命令列は適用の時点で検証済みです。スタックの深さの確認は、検証の漏れに備えた防御です。
 */
static truth_value evaluate_line(const evaluation_context *context, const bool honors_jumps)
{
    string_catalog_filter_record_header header;
    string_catalog_filter_instruction instruction;
    uint8_t stack[CPLAT_STRING_CATALOG_FILTER_LINE_WIDTH_MAX];
    uint32_t depth = 0U;
    uint32_t index = 0U;

    string_catalog_filter_read_record_header(context->record, &header);

    while (index < header.instruction_count)
    {
        string_catalog_filter_read_instruction(context->record, index, &instruction);
        index++;

        switch (instruction.opcode)
        {
        case STRING_CATALOG_FILTER_OPCODE_PREDICATE:
            stack[depth++] = (uint8_t)evaluate_predicate(context, &instruction);
            break;
        case STRING_CATALOG_FILTER_OPCODE_NOT:
            stack[depth - 1U] = (uint8_t)truth_not((truth_value)stack[depth - 1U]);
            break;
        case STRING_CATALOG_FILTER_OPCODE_AND:
            stack[depth - 2U] = (uint8_t)truth_and((truth_value)stack[depth - 2U], (truth_value)stack[depth - 1U]);
            depth--;
            break;
        case STRING_CATALOG_FILTER_OPCODE_OR:
            stack[depth - 2U] = (uint8_t)truth_or((truth_value)stack[depth - 2U], (truth_value)stack[depth - 1U]);
            depth--;
            break;
        case STRING_CATALOG_FILTER_OPCODE_JUMP_IF_FALSE:
            if (honors_jumps && (stack[depth - 1U] == (uint8_t)TRUTH_VALUE_FALSE))
            {
                index = instruction.operand;
            }
            break;
        case STRING_CATALOG_FILTER_OPCODE_JUMP_IF_TRUE:
            if (honors_jumps && (stack[depth - 1U] == (uint8_t)TRUTH_VALUE_TRUE))
            {
                index = instruction.operand;
            }
            break;
        default:
            return TRUTH_VALUE_FALSE;
        }
    }

    if (depth != 1U)
    {
        return TRUTH_VALUE_FALSE;
    }
    return (truth_value)stack[0];
}

/* ===== 面の管理 ===== */

static uint8_t *line_states_of(const cplat_string_catalog_filter_slot *slot, filter_plane *plane,
                               const uint32_t line_index)
{
    return plane->line_states + ((size_t)line_index * slot->entry_count);
}

static int8_t *argument_maps_of(const cplat_string_catalog_filter_slot *slot, filter_plane *plane,
                                const uint32_t line_index)
{
    return plane->argument_maps +
           ((size_t)line_index * slot->entry_count * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX);
}

static int64_t *identifier_values_of(filter_plane *plane, const uint32_t line_index)
{
    return plane->identifier_values + ((size_t)line_index * CPLAT_STRING_CATALOG_FILTER_IDENTIFIER_REFERENCE_MAX);
}

static void free_plane(filter_plane *plane)
{
    free(plane->image);
    free(plane->entry_states);
    free(plane->entry_dependent_lines);
    free(plane->line_states);
    free(plane->argument_maps);
    free(plane->identifier_values);
    free(plane->line_errors);
    memset(plane, 0, sizeof(*plane));
}

static bool allocate_plane(const cplat_string_catalog_filter_slot *slot, filter_plane *plane)
{
    const size_t line_entries = (size_t)slot->line_capacity * slot->entry_count;

    memset(plane, 0, sizeof(*plane));
    plane->image = (unsigned char *)malloc(slot->image_size);
    /* 項目数が 0 のカタログでも確保に失敗と区別できるよう、要素数は 1 以上とする */
    plane->entry_states = (uint8_t *)calloc(slot->entry_count + 1U, sizeof(*plane->entry_states));
    plane->entry_dependent_lines = (uint64_t *)calloc(slot->entry_count + 1U, sizeof(*plane->entry_dependent_lines));
    plane->line_states = (uint8_t *)calloc(line_entries + 1U, sizeof(*plane->line_states));
    plane->argument_maps = (int8_t *)calloc((line_entries * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX) + 1U,
                                            sizeof(*plane->argument_maps));
    plane->identifier_values =
        (int64_t *)calloc(((size_t)slot->line_capacity * CPLAT_STRING_CATALOG_FILTER_IDENTIFIER_REFERENCE_MAX),
                          sizeof(*plane->identifier_values));
    plane->line_errors = (cplat_string_catalog_filter_error *)calloc(slot->line_capacity, sizeof(*plane->line_errors));

    if ((plane->image == NULL) || (plane->entry_states == NULL) || (plane->entry_dependent_lines == NULL) ||
        (plane->line_states == NULL) || (plane->argument_maps == NULL) || (plane->identifier_values == NULL) ||
        (plane->line_errors == NULL))
    {
        free_plane(plane);
        return false;
    }
    return true;
}

/**
 *  @brief          名前解決テーブルがカタログと整合することを確認します。
 *  @param[in]      catalog        カタログ。
 *  @param[in]      key_names      名前解決テーブル。@p key_name_count が 0 の場合は NULL を指定できます。
 *  @param[in]      key_name_count @p key_names の要素数。
 *  @return         すべての名前が NULL でなく、名前が重複せず、文字列キーがカタログに存在する場合は true を返します。
 *
 *  同じ名前が複数あると、どちらの値へ解決するかが決まらないため拒否します。\n
 *  異なる名前が同じ文字列キーを指すことは許可します。
 */
static bool is_key_name_table_valid(const cplat_string_catalog *catalog,
                                    const cplat_string_catalog_filter_key_name *key_names, size_t key_name_count)
{
    for (size_t index = 0; index < key_name_count; index++)
    {
        if ((key_names[index].name == NULL) || (cplat_string_catalog_get_entry(catalog, key_names[index].key) == NULL))
        {
            return false;
        }
        for (size_t other = 0; other < index; other++)
        {
            if (strcmp(key_names[index].name, key_names[other].name) == 0)
            {
                return false;
            }
        }
    }
    return true;
}

static const cplat_string_catalog_filter_key_name *find_key_name(const cplat_string_catalog_filter_slot *slot,
                                                                 const char *name)
{
    for (size_t index = 0; index < slot->key_name_count; index++)
    {
        if (strcmp(slot->key_names[index].name, name) == 0)
        {
            return &slot->key_names[index];
        }
    }
    return NULL;
}

/**
 *  @brief          分類値と比較する定数 1 個を、分類値の名前の範囲で確かめ、識別子なら名前で解決します。
 *  @return         受け入れられる場合は CPLAT_STRING_CATALOG_FILTER_ERROR_NONE、それ以外は無効にする原因。
 */
static cplat_string_catalog_filter_error
resolve_category_constant(const cplat_string_catalog_filter_category_names *names,
                          const string_catalog_filter_constant *constant, int64_t *identifiers,
                          bool *is_category_identifier)
{
    switch (constant->header.kind)
    {
    case STRING_CATALOG_FILTER_CONSTANT_KIND_IDENTIFIER:
        for (size_t index = 0; index < names->count; index++)
        {
            if (strcmp(names->names[index], constant->text) == 0)
            {
                identifiers[constant->header.slot] = (int64_t)index;
                is_category_identifier[constant->header.slot] = true;
                return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;
            }
        }
        return CPLAT_STRING_CATALOG_FILTER_ERROR_UNRESOLVED_CATEGORY_NAME;

    case STRING_CATALOG_FILTER_CONSTANT_KIND_INTEGER:
    case STRING_CATALOG_FILTER_CONSTANT_KIND_CHARACTER:
        if (((constant->header.flags & STRING_CATALOG_FILTER_CONSTANT_FLAG_NEGATIVE) != 0U) ||
            (constant->magnitude >= (uint64_t)names->count))
        {
            return CPLAT_STRING_CATALOG_FILTER_ERROR_CATEGORY_OUT_OF_RANGE;
        }
        return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;

    case STRING_CATALOG_FILTER_CONSTANT_KIND_FLOAT:
        /* 分類値は整数のため、整数でない値は範囲外として扱う */
        if ((constant->real < 0.0) || (constant->real >= (double)names->count) ||
            (constant->real != floor(constant->real)))
        {
            return CPLAT_STRING_CATALOG_FILTER_ERROR_CATEGORY_OUT_OF_RANGE;
        }
        return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;

    default:
        /* 文字列と null は、コンパイルの時点で分類値との比較から除かれている */
        return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;
    }
}

/**
 *  @brief          分類値の比較に現れる定数を確かめます。分類値の名前が設定されている場合に限ります。
 *
 *  分類値の意味と範囲を決めるのは利用側です。名前が設定されていなければ、分類値の定数を確かめません。
 */
static cplat_string_catalog_filter_error resolve_category_predicates(const cplat_string_catalog_filter_slot *slot,
                                                                     const unsigned char *record,
                                                                     const unsigned char *constants,
                                                                     const string_catalog_filter_record_header *header,
                                                                     int64_t *identifiers, bool *is_category_identifier)
{
    string_catalog_filter_instruction instruction;
    string_catalog_filter_constant constant;

    if (slot->category_names == NULL)
    {
        return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;
    }

    for (uint32_t index = 0; index < header->instruction_count; index++)
    {
        uint32_t offset;

        string_catalog_filter_read_instruction(record, index, &instruction);
        if ((instruction.opcode != (uint8_t)STRING_CATALOG_FILTER_OPCODE_PREDICATE) ||
            (instruction.field != (uint8_t)STRING_CATALOG_FILTER_FIELD_CATEGORY))
        {
            continue;
        }

        offset = instruction.operand;
        for (uint16_t operand = 0; operand < instruction.operand_count; operand++)
        {
            cplat_string_catalog_filter_error error;

            (void)string_catalog_filter_read_constant(constants, header->constant_size, offset, &constant);
            offset = constant.next_offset;

            error = resolve_category_constant(slot->category_names, &constant, identifiers, is_category_identifier);
            if (error != CPLAT_STRING_CATALOG_FILTER_ERROR_NONE)
            {
                return error;
            }
        }
    }
    return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;
}

/**
 *  @brief          1 行の名前を解決し、項目ごとの判定結果を事前計算します。
 *  @return         解決できた場合は CPLAT_STRING_CATALOG_FILTER_ERROR_NONE、それ以外は無効にする原因。
 */
static cplat_string_catalog_filter_error resolve_line(const cplat_string_catalog_filter_slot *slot, filter_plane *plane,
                                                      const uint32_t line_index)
{
    const unsigned char *record =
        string_catalog_filter_record_address_const(plane->image, slot->record_size, line_index);
    const unsigned char *constants = string_catalog_filter_record_constants(record, slot->line_width);
    string_catalog_filter_record_header header;
    string_catalog_filter_constant constant;
    uint8_t *states = line_states_of(slot, plane, line_index);
    int8_t *maps = argument_maps_of(slot, plane, line_index);
    int64_t *identifiers = identifier_values_of(plane, line_index);
    bool is_category_identifier[CPLAT_STRING_CATALOG_FILTER_IDENTIFIER_REFERENCE_MAX] = {false};
    cplat_string_catalog_filter_error error;
    uint32_t offset = 0U;

    string_catalog_filter_read_record_header(record, &header);
    memset(states, 0, slot->entry_count * sizeof(*states));
    memset(maps, -1, slot->entry_count * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX * sizeof(*maps));
    memset(identifiers, 0, CPLAT_STRING_CATALOG_FILTER_IDENTIFIER_REFERENCE_MAX * sizeof(*identifiers));

    /* 分類値の比較を先に確かめる。分類値と比較する識別子は分類値の名前で解決し、文字列キーの名前では解決しない */
    error = resolve_category_predicates(slot, record, constants, &header, identifiers, is_category_identifier);
    if (error != CPLAT_STRING_CATALOG_FILTER_ERROR_NONE)
    {
        return error;
    }

    /* 定数をたどり、文字列キーの名前と引数名を解決する */
    while (offset < header.constant_size)
    {
        (void)string_catalog_filter_read_constant(constants, header.constant_size, offset, &constant);
        offset = constant.next_offset;

        if ((constant.header.kind == (uint8_t)STRING_CATALOG_FILTER_CONSTANT_KIND_IDENTIFIER) &&
            is_category_identifier[constant.header.slot])
        {
            /* 分類値の名前で解決済み */
        }
        else if (constant.header.kind == (uint8_t)STRING_CATALOG_FILTER_CONSTANT_KIND_IDENTIFIER)
        {
            const cplat_string_catalog_filter_key_name *key_name = find_key_name(slot, constant.text);

            if (key_name == NULL)
            {
                return CPLAT_STRING_CATALOG_FILTER_ERROR_UNRESOLVED_KEY_NAME;
            }
            identifiers[constant.header.slot] = key_name->key;
        }
        else if (constant.header.kind == (uint8_t)STRING_CATALOG_FILTER_CONSTANT_KIND_ARGUMENT_NAME)
        {
            bool is_found = false;

            for (size_t entry_index = 0; entry_index < slot->entry_count; entry_index++)
            {
                const cplat_string_catalog_entry *entry = &slot->catalog->entries[entry_index];

                for (int argument = 0; argument < entry->argument_count; argument++)
                {
                    if ((entry->arguments[argument].kind != CPLAT_STRING_CATALOG_ARGUMENT_KIND_UNUSED) &&
                        (entry->arguments[argument].name != NULL) &&
                        (strcmp(entry->arguments[argument].name, constant.text) == 0))
                    {
                        maps[(entry_index * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX) +
                             constant.header.slot] = (int8_t)argument;
                        is_found = true;
                        break;
                    }
                }
            }
            if (!is_found)
            {
                return CPLAT_STRING_CATALOG_FILTER_ERROR_UNRESOLVED_ARGUMENT_NAME;
            }
        }
        else
        {
            /* 比較対象の定数は、名前の解決を必要としない */
        }
    }

    for (size_t entry_index = 0; entry_index < slot->entry_count; entry_index++)
    {
        evaluation_context context;

        memset(&context, 0, sizeof(context));
        context.entry = &slot->catalog->entries[entry_index];
        context.record = record;
        context.constants = constants;
        context.constant_size = header.constant_size;
        context.argument_map = maps + (entry_index * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX);
        context.identifier_values = identifiers;
        context.values = NULL;
        states[entry_index] = (uint8_t)evaluate_line(&context, false);
    }

    return CPLAT_STRING_CATALOG_FILTER_ERROR_NONE;
}

/** 参照中の面から、内容が同一の行を探します。 */
static bool find_reusable_line(const cplat_string_catalog_filter_slot *slot, const filter_plane *current,
                               const unsigned char *record, uint32_t *line_index_out)
{
    string_catalog_filter_record_header header;

    string_catalog_filter_read_record_header(record, &header);

    for (uint32_t index = 0; index < current->line_count; index++)
    {
        const unsigned char *candidate =
            string_catalog_filter_record_address_const(current->image, slot->record_size, index);
        string_catalog_filter_record_header candidate_header;

        string_catalog_filter_read_record_header(candidate, &candidate_header);
        /* ハッシュ値の衝突に備え、一致した場合は内容を比較する */
        if ((candidate_header.line_hash == header.line_hash) && (memcmp(candidate, record, slot->record_size) == 0))
        {
            *line_index_out = index;
            return true;
        }
    }
    return false;
}

/** 複製済みのフィルター オブジェクトから、面の判定用の内容を構築します。 */
static void build_plane(const cplat_string_catalog_filter_slot *slot, filter_plane *target, filter_plane *current,
                        cplat_string_catalog_filter_diagnostic *diagnostics, const size_t diagnostic_capacity,
                        size_t *invalid_count)
{
    string_catalog_filter_image_header header;

    string_catalog_filter_read_image_header(target->image, &header);
    target->line_count = header.line_count;
    target->enabled_lines = 0U;

    for (uint32_t line_index = 0; line_index < header.line_count; line_index++)
    {
        const unsigned char *record =
            string_catalog_filter_record_address_const(target->image, slot->record_size, line_index);
        uint32_t reusable_index;

        if (find_reusable_line(slot, current, record, &reusable_index))
        {
            memcpy(line_states_of(slot, target, line_index), line_states_of(slot, current, reusable_index),
                   slot->entry_count * sizeof(*target->line_states));
            memcpy(argument_maps_of(slot, target, line_index), argument_maps_of(slot, current, reusable_index),
                   slot->entry_count * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX *
                       sizeof(*target->argument_maps));
            memcpy(identifier_values_of(target, line_index), identifier_values_of(current, reusable_index),
                   CPLAT_STRING_CATALOG_FILTER_IDENTIFIER_REFERENCE_MAX * sizeof(*target->identifier_values));
            target->line_errors[line_index] = current->line_errors[reusable_index];
        }
        else
        {
            target->line_errors[line_index] = resolve_line(slot, target, line_index);
        }

        if (target->line_errors[line_index] == CPLAT_STRING_CATALOG_FILTER_ERROR_NONE)
        {
            target->enabled_lines |= (uint64_t)1U << line_index;
        }
        else
        {
            if ((diagnostics != NULL) && (*invalid_count < diagnostic_capacity))
            {
                diagnostics[*invalid_count].line_index = line_index;
                diagnostics[*invalid_count].column = 0U;
                diagnostics[*invalid_count].error = target->line_errors[line_index];
            }
            (*invalid_count)++;
        }
    }

    /* 行ごとの結果を、論理和の規則で項目ごとに集約する */
    for (size_t entry_index = 0; entry_index < slot->entry_count; entry_index++)
    {
        uint64_t dependent_lines = 0U;
        bool is_always = false;

        for (uint32_t line_index = 0; line_index < target->line_count; line_index++)
        {
            uint8_t state;

            if ((target->enabled_lines & ((uint64_t)1U << line_index)) == 0U)
            {
                continue;
            }
            state = line_states_of(slot, target, line_index)[entry_index];
            if (state == (uint8_t)TRUTH_VALUE_TRUE)
            {
                is_always = true;
                break;
            }
            if (state == (uint8_t)TRUTH_VALUE_UNKNOWN)
            {
                dependent_lines |= (uint64_t)1U << line_index;
            }
        }

        if (is_always)
        {
            target->entry_states[entry_index] = (uint8_t)CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH;
            target->entry_dependent_lines[entry_index] = 0U;
        }
        else if (dependent_lines != 0U)
        {
            target->entry_states[entry_index] = (uint8_t)CPLAT_STRING_CATALOG_FILTER_STATE_ARGUMENT_DEPENDENT;
            target->entry_dependent_lines[entry_index] = dependent_lines;
        }
        else
        {
            target->entry_states[entry_index] = (uint8_t)CPLAT_STRING_CATALOG_FILTER_STATE_NEVER_MATCH;
            target->entry_dependent_lines[entry_index] = 0U;
        }
    }
}

/** 文字列キーに対応する項目のインデックスを求めます。 */
static bool find_entry_index(const cplat_string_catalog_filter_slot *slot, const int string_key,
                             size_t *entry_index_out)
{
    const cplat_string_catalog_entry *entry = cplat_string_catalog_get_entry(slot->catalog, string_key);

    if (entry == NULL)
    {
        return false;
    }
    *entry_index_out = (size_t)(entry - slot->catalog->entries);
    return true;
}

/* ===== 公開関数 ===== */

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_create(const cplat_string_catalog *catalog,
                                            const cplat_string_catalog_filter_key_name *key_names,
                                            const size_t key_name_count, const size_t line_capacity,
                                            const size_t line_width, cplat_string_catalog_filter_slot **slot_out)
{
    cplat_string_catalog_filter_slot *slot;
    int ret;

    if (slot_out == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *slot_out = NULL;

    if ((catalog == NULL) || (catalog->entry_count < 0) || ((catalog->entry_count > 0) && (catalog->entries == NULL)) ||
        ((key_names == NULL) && (key_name_count > 0U)) || (line_capacity == 0U) ||
        (line_capacity > CPLAT_STRING_CATALOG_FILTER_LINE_MAX) ||
        (line_width < CPLAT_STRING_CATALOG_FILTER_LINE_WIDTH_MIN) ||
        (line_width > CPLAT_STRING_CATALOG_FILTER_LINE_WIDTH_MAX))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    /* 判定と説明文はカタログの引数定義を直接参照するため、作成時に定義全体を検査する。
     * 項目数が 0 のカタログは、どの文字列キーにも一致しない何もしないカタログとして検査せずに受け付ける */
    if (catalog->entry_count > 0)
    {
        ret = cplat_string_catalog_verify(catalog, NULL, NULL);
        if (ret != CPLAT_OK)
        {
            return ret;
        }
    }
    if (!is_key_name_table_valid(catalog, key_names, key_name_count))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    slot = (cplat_string_catalog_filter_slot *)calloc(1U, sizeof(*slot));
    if (slot == NULL)
    {
        return CPLAT_ERR_OUT_OF_MEMORY;
    }

    slot->catalog = catalog;
    slot->key_names = key_names;
    slot->key_name_count = key_name_count;
    slot->entry_count = (size_t)catalog->entry_count;
    slot->line_capacity = (uint32_t)line_capacity;
    slot->line_width = (uint32_t)line_width;
    slot->record_size = (uint32_t)CPLAT_STRING_CATALOG_FILTER_RECORD_SIZE(line_width);
    slot->image_size = CPLAT_STRING_CATALOG_FILTER_IMAGE_SIZE(line_capacity, line_width);

    if (!allocate_plane(slot, &slot->planes[0]) || !allocate_plane(slot, &slot->planes[1]))
    {
        cplat_string_catalog_filter_slot_dispose(&slot);
        return CPLAT_ERR_OUT_OF_MEMORY;
    }

    /* ロックの作成に失敗した場合は、作成関数の結果コードをそのまま返す */
    ret = cplat_local_rwlock_create(&slot->plane_lock);
    if (ret == CPLAT_OK)
    {
        ret = cplat_local_lock_create(&slot->apply_lock);
    }
    if (ret != CPLAT_OK)
    {
        cplat_string_catalog_filter_slot_dispose(&slot);
        return ret;
    }

    /* 行を持たないフィルター オブジェクトを適用した状態から始める。全項目が「常に不一致」となる */
    (void)cplat_string_catalog_filter_compile(NULL, 0U, line_width, line_capacity, slot->planes[0].image,
                                              slot->image_size, NULL, 0U, NULL);
    slot->active_plane = 0;

    *slot_out = slot;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

void cplat_string_catalog_filter_slot_dispose(cplat_string_catalog_filter_slot **slot)
{
    if ((slot == NULL) || (*slot == NULL))
    {
        return;
    }

    if ((*slot)->plane_lock != NULL)
    {
        cplat_local_rwlock_dispose((*slot)->plane_lock);
    }
    if ((*slot)->apply_lock != NULL)
    {
        cplat_local_lock_dispose((*slot)->apply_lock);
    }
    free_plane(&(*slot)->planes[0]);
    free_plane(&(*slot)->planes[1]);
    free(*slot);
    *slot = NULL;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_apply(cplat_string_catalog_filter_slot *slot, const void *image,
                                           const size_t image_size, cplat_string_catalog_filter_diagnostic *diagnostics,
                                           const size_t diagnostic_capacity, size_t *invalid_count_out)
{
    string_catalog_filter_image_header header;
    filter_plane *target;
    filter_plane *current;
    size_t invalid_count = 0U;
    int target_plane;
    int ret;

    if ((slot == NULL) || (image == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    ret = cplat_local_lock_lock(slot->apply_lock, CPLAT_SYNC_WAIT_FOREVER);
    if (ret != CPLAT_OK)
    {
        return ret;
    }

    /* 面を書き換えるのは適用だけであり、適用は apply_lock で直列化している。
     * そのため active_plane と参照中の面は、ここではロックなしで読める。 */
    target_plane = 1 - slot->active_plane;
    target = &slot->planes[target_plane];
    current = &slot->planes[slot->active_plane];

    /* 引き渡された領域は、複製してから検証する。
     * 共有メモリの領域が検証後に書き換えられても、検証済みの内容だけを使用するためです。 */
    ret = CPLAT_ERR_CORRUPT_DESCRIPTOR;
    if (image_size >= slot->image_size)
    {
        memcpy(target->image, image, slot->image_size);
        if (cplat_string_catalog_filter_validate(target->image, slot->image_size) == CPLAT_OK)
        {
            string_catalog_filter_read_image_header(target->image, &header);
            if ((header.line_capacity == slot->line_capacity) && (header.line_width == slot->line_width))
            {
                ret = CPLAT_OK;
            }
        }
    }

    if (ret == CPLAT_OK)
    {
        build_plane(slot, target, current, diagnostics, diagnostic_capacity, &invalid_count);

        /* 判定の途中で面が変わらないよう、切り替えは排他モードで行う */
        ret = cplat_local_rwlock_lock_exclusive(slot->plane_lock, CPLAT_SYNC_WAIT_FOREVER);
        if (ret == CPLAT_OK)
        {
            slot->active_plane = target_plane;
            (void)cplat_local_rwlock_unlock_exclusive(slot->plane_lock);
        }
    }

    (void)cplat_local_lock_unlock(slot->apply_lock);

    if ((ret == CPLAT_OK) && (invalid_count_out != NULL))
    {
        *invalid_count_out = invalid_count;
    }
    return ret;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_snapshot(cplat_string_catalog_filter_slot *slot, void *image_out,
                                              const size_t image_size, uint64_t *enabled_lines_out)
{
    const filter_plane *plane;
    int ret;

    if ((slot == NULL) || (image_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (image_size < slot->image_size)
    {
        return CPLAT_ERR_BUFFER_TOO_SMALL;
    }

    ret = cplat_local_rwlock_lock_shared(slot->plane_lock, CPLAT_SYNC_WAIT_FOREVER);
    if (ret != CPLAT_OK)
    {
        return ret;
    }

    plane = &slot->planes[slot->active_plane];
    memcpy(image_out, plane->image, slot->image_size);
    if (enabled_lines_out != NULL)
    {
        *enabled_lines_out = plane->enabled_lines;
    }

    (void)cplat_local_rwlock_unlock_shared(slot->plane_lock);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_test(cplat_string_catalog_filter_slot *slot, const int string_key,
                                          cplat_string_catalog_filter_state *state_out)
{
    size_t entry_index;
    int ret;

    if ((slot == NULL) || (state_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    if (!find_entry_index(slot, string_key, &entry_index))
    {
        return CPLAT_ERR_NOT_FOUND;
    }

    ret = cplat_local_rwlock_lock_shared(slot->plane_lock, CPLAT_SYNC_WAIT_FOREVER);
    if (ret != CPLAT_OK)
    {
        return ret;
    }
    *state_out = (cplat_string_catalog_filter_state)slot->planes[slot->active_plane].entry_states[entry_index];
    (void)cplat_local_rwlock_unlock_shared(slot->plane_lock);
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_set_category_names(
    cplat_string_catalog_filter_slot *slot, const cplat_string_catalog_filter_category_names *category_names)
{
    if (slot == NULL)
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }

    if (category_names != NULL)
    {
        if ((category_names->names == NULL) || (category_names->count == 0U) ||
            (category_names->subject_japanese == NULL) || (category_names->subject_neutral == NULL))
        {
            return CPLAT_ERR_INVALID_ARGUMENT;
        }
        for (size_t index = 0; index < category_names->count; index++)
        {
            if (category_names->names[index] == NULL)
            {
                return CPLAT_ERR_INVALID_ARGUMENT;
            }
        }
    }

    slot->category_names = category_names;
    return CPLAT_OK;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_describe_line(cplat_string_catalog_filter_slot *slot, const size_t line_index,
                                                   char *dest, const size_t dest_size)
{
    string_catalog_filter_describe_source source;
    filter_plane *plane;
    const uint8_t *states;
    size_t candidate_count = 0U;
    int ret;

    if ((slot == NULL) || (dest == NULL) || (dest_size == 0U))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    dest[0] = '\0';

    ret = cplat_local_rwlock_lock_shared(slot->plane_lock, CPLAT_SYNC_WAIT_FOREVER);
    if (ret != CPLAT_OK)
    {
        return ret;
    }

    plane = &slot->planes[slot->active_plane];
    if (line_index >= plane->line_count)
    {
        ret = CPLAT_ERR_INVALID_ARGUMENT;
    }
    else if ((plane->enabled_lines & ((uint64_t)1U << line_index)) == 0U)
    {
        /* 名前を解決できずに無効とした行は、メタ情報と結び付けられない */
        ret = CPLAT_ERR_MALFORMED_DEFINITION;
    }
    else
    {
        memset(&source, 0, sizeof(source));
        source.catalog = slot->catalog;
        source.record =
            string_catalog_filter_record_address_const(plane->image, slot->record_size, (uint32_t)line_index);
        source.identifier_values = identifier_values_of(plane, (uint32_t)line_index);
        source.category_names = slot->category_names;
        source.line_width = slot->line_width;
        source.is_japanese = (cplat_string_catalog_get_language() == CPLAT_STRING_CATALOG_LANGUAGE_JAPANESE);

        /* 事前計算で「常に不一致」とならなかった項目が 1 つだけなら、その行は 1 つの項目に限定されている */
        states = line_states_of(slot, plane, (uint32_t)line_index);
        for (size_t entry_index = 0; entry_index < slot->entry_count; entry_index++)
        {
            if (states[entry_index] != (uint8_t)TRUTH_VALUE_FALSE)
            {
                source.single_entry = &slot->catalog->entries[entry_index];
                candidate_count++;
            }
        }
        if (candidate_count != 1U)
        {
            source.single_entry = NULL;
        }

        ret = string_catalog_filter_describe_record(&source, dest, dest_size);
    }

    (void)cplat_local_rwlock_unlock_shared(slot->plane_lock);
    return ret;
}

/** 参照中の面で、1 項目の一致を判定します。共有モードのロックの下で呼び出します。 */
static bool is_matched(const cplat_string_catalog_filter_slot *slot, filter_plane *plane, const size_t entry_index,
                       va_list args)
{
    const cplat_string_catalog_entry *entry = &slot->catalog->entries[entry_index];
    argument_value values[CPLAT_STRING_CATALOG_ARGUMENT_MAX];
    uint64_t lines;
    va_list copied_args;
    bool is_collected;

    switch (plane->entry_states[entry_index])
    {
    case CPLAT_STRING_CATALOG_FILTER_STATE_ALWAYS_MATCH:
        return true;
    case CPLAT_STRING_CATALOG_FILTER_STATE_ARGUMENT_DEPENDENT:
        break;
    default:
        return false;
    }

    /* 書式展開でも元の引数リストを使用するため、判定には複製を使用する */
    va_copy(copied_args, args);
    is_collected = collect_arguments(entry, copied_args, values);
    va_end(copied_args);
    if (!is_collected)
    {
        return false;
    }

    lines = plane->entry_dependent_lines[entry_index];
    for (uint32_t line_index = 0; lines != 0U; line_index++, lines >>= 1)
    {
        const unsigned char *record;
        evaluation_context context;

        if ((lines & 1U) == 0U)
        {
            continue;
        }

        record = string_catalog_filter_record_address_const(plane->image, slot->record_size, line_index);
        memset(&context, 0, sizeof(context));
        context.entry = entry;
        context.record = record;
        context.constants = string_catalog_filter_record_constants(record, slot->line_width);
        {
            string_catalog_filter_record_header header;

            string_catalog_filter_read_record_header(record, &header);
            context.constant_size = header.constant_size;
        }
        context.argument_map = argument_maps_of(slot, plane, line_index) +
                               (entry_index * CPLAT_STRING_CATALOG_FILTER_ARGUMENT_REFERENCE_MAX);
        context.identifier_values = identifier_values_of(plane, line_index);
        context.values = values;

        if (evaluate_line(&context, true) == TRUTH_VALUE_TRUE)
        {
            return true;
        }
    }
    return false;
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_vformat(cplat_string_catalog_filter_slot *slot, char *dest, const size_t dest_size,
                                             int *matched_out, const int string_key, va_list args)
{
    size_t entry_index;
    int ret;

    if ((slot == NULL) || (matched_out == NULL))
    {
        return CPLAT_ERR_INVALID_ARGUMENT;
    }
    *matched_out = 0;

    if (find_entry_index(slot, string_key, &entry_index))
    {
        /* 判定できない場合は不一致と区別するため、文字列を組み立てずに結果コードを返す */
        ret = cplat_local_rwlock_lock_shared(slot->plane_lock, CPLAT_SYNC_WAIT_FOREVER);
        if (ret != CPLAT_OK)
        {
            if ((dest != NULL) && (dest_size > 0U))
            {
                dest[0] = '\0';
            }
            return ret;
        }
        *matched_out = is_matched(slot, &slot->planes[slot->active_plane], entry_index, args);
        (void)cplat_local_rwlock_unlock_shared(slot->plane_lock);
    }

    /* 書式展開はロックの外で行う。判定の結果によらず文字列を組み立てる */
    return cplat_string_catalog_vformat(slot->catalog, dest, dest_size, string_key, args);
}

/* Doxygen コメントは、ヘッダーに記載 */

int cplat_string_catalog_filter_slot_format(cplat_string_catalog_filter_slot *slot, char *dest, const size_t dest_size,
                                            int *matched_out, const int string_key, ...)
{
    va_list args;
    int ret;

    va_start(args, string_key);
    ret = cplat_string_catalog_filter_slot_vformat(slot, dest, dest_size, matched_out, string_key, args);
    va_end(args);
    return ret;
}
